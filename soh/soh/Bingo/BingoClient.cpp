#include "BingoClient.h"
#include "BingoOAuth.h"

#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <mutex>

namespace Bingo {

void EnsureCurlInitialized() {
    static std::once_flag curlInit;
    std::call_once(curlInit, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

namespace {

constexpr auto kPollInterval = std::chrono::milliseconds(250); // should be ~4 pings per second like the website?
constexpr const char* kNotLinkedMessage = "Link your ShipBingo account first";
constexpr const char* kLinkLostMessage = "ShipBingo account is not linked. Link it again on the Login page.";

std::string UrlEscape(const std::string& value) {
    char* escaped = curl_easy_escape(nullptr, value.c_str(), static_cast<int>(value.size()));
    std::string result = escaped ? escaped : "";
    curl_free(escaped);
    return result;
}

ApiResponse HttpGet(const std::string& path, std::atomic<bool>& cancel) {
    return OAuthClient::Instance().AuthorizedRequest("GET", "/" + path, "", &cancel);
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

std::string ToString(const nlohmann::json& value) {
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_number() || value.is_boolean()) {
        return value.dump();
    }
    return "";
}

std::string StringField(const nlohmann::json& object, const char* key) {
    auto it = object.find(key);
    return it == object.end() ? "" : ToString(*it);
}

bool BoolField(const nlohmann::json& object, const char* key) {
    auto it = object.find(key);
    return it != object.end() && it->is_boolean() && it->get<bool>();
}

// Error responses are usually a bare JSON string such as "Failed to Find User".
std::string DescribeError(const ApiResponse& response) {
    switch (response.result) {
        case AuthResult::Network:
            return response.error;
        case AuthResult::Unlinked:
            return kLinkLostMessage;
        case AuthResult::Outdated:
            return "Update the game to keep using ShipBingo";
        default:
            break;
    }
    if (response.body.is_string()) {
        return response.body.get<std::string>();
    }
    return "HTTP " + std::to_string(response.status);
}

bool IsAuthFailure(const ApiResponse& response) {
    return response.result == AuthResult::Unlinked || response.result == AuthResult::Forbidden ||
           response.result == AuthResult::Outdated;
}

} // namespace

Client& Client::Instance() {
    static Client instance;
    return instance;
}

Client::~Client() {
    Disconnect();
    roomsStopRequested = true;
    std::lock_guard<std::mutex> roomsWorkerLock(roomsWorkerMutex);
    if (roomsWorker.joinable()) {
        roomsWorker.join();
    }
    weekliesStopRequested = true;
    std::lock_guard<std::mutex> weekliesWorkerLock(weekliesWorkerMutex);
    if (weekliesWorker.joinable()) {
        weekliesWorker.join();
    }
}

void Client::Connect(const std::string& gameIdInput) {
    Disconnect();

    // Accept a full game URL such as https://shipbingo.zarashane.dev/play/1 as well as a bare ID.
    std::string gameId = gameIdInput;
    while (!gameId.empty() && (std::isspace(static_cast<unsigned char>(gameId.back())) || gameId.back() == '/')) {
        gameId.pop_back();
    }
    gameId.erase(0, gameId.find_last_of('/') == std::string::npos ? 0 : gameId.find_last_of('/') + 1);
    gameId.erase(0, gameId.find_first_not_of(" \t\r\n"));

    const bool validId = !gameId.empty() && std::all_of(gameId.begin(), gameId.end(), [](unsigned char c) {
                             return std::isalnum(c) != 0;
                         });
    if (!validId) {
        SetStatus(ConnectionState::Error, "Enter a valid Game ID");
        return;
    }
    if (!IsLoggedIn()) {
        SetStatus(ConnectionState::Error, kNotLinkedMessage);
        return;
    }

    stopRequested = false;
    SetStatus(ConnectionState::Connecting, "Connecting...");
    worker = std::thread(&Client::Run, this, gameId);
}

void Client::Disconnect() {
    stopRequested = true;
    wake.notify_all();
    if (worker.joinable()) {
        worker.join();
    }

    std::lock_guard<std::mutex> lock(mutex);
    snapshot = Snapshot{};
    pendingToggles.clear();
}

void Client::RenewCard(const std::string& gameIdInput, const std::string& seedID) {
    if (!IsLoggedIn()) {
        return;
    }
    HttpGet("api/Game/RefreshBoard/" + UrlEscape(gameIdInput) + "/" + UrlEscape(seedID), stopRequested);
}

void Client::UpdateUserColor(const std::string& gameIdInput, const std::string& colorToUse) {
    if (!IsLoggedIn()) {
        return;
    }
    HttpGet("api/Game/UserColorSelected/" + UrlEscape(gameIdInput) + "/" + "!" + colorToUse, stopRequested);
}

void Client::CreateGame(const std::string& gameDescription) {
    if (!IsLoggedIn()) {
        SetStatus(ConnectionState::Error, kNotLinkedMessage);
        return;
    }

    std::atomic<bool> cancel = false;
    ApiResponse response = HttpGet("api/Game/StartGame/" + UrlEscape(gameDescription), cancel);
    if (IsAuthFailure(response)) {
        SetStatus(ConnectionState::Error, DescribeError(response));
        return;
    }
    if (response.status != 200) {
        const std::string status = "Could not create game: " + DescribeError(response);
        SPDLOG_WARN("[Bingo] {}", status);
        SetStatus(ConnectionState::Error, status);
        return;
    }

    const auto& gameId = response.body;
    if (!gameId.is_number_integer() && !gameId.is_number_unsigned()) {
        const std::string status = "The server returned an invalid Game ID.";
        SPDLOG_WARN("[Bingo] {}", status);
        SetStatus(ConnectionState::Error, status);
        return;
    }

    Connect(gameId.dump());
}

void Client::ReadRoomsList() {
    std::lock_guard<std::mutex> workerLock(roomsWorkerMutex);
    const bool linked = IsLoggedIn();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (roomsSnapshot.loading) {
            return;
        }
        if (!linked) {
            roomsSnapshot.status = "Link your ShipBingo account to load rooms.";
            return;
        }
        roomsSnapshot.hasRequested = true;
        roomsSnapshot.loading = true;
        roomsSnapshot.status = "Loading current rooms...";
    }

    if (roomsWorker.joinable()) {
        roomsWorker.join();
    }
    roomsWorker = std::thread(&Client::RunRoomsRequest, this);
}

RoomsSnapshot Client::GetRoomsSnapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    return roomsSnapshot;
}

void Client::RunRoomsRequest() {
    ApiResponse response = HttpGet("api/Game/GetGames/", roomsStopRequested);
    if (roomsStopRequested) {
        std::lock_guard<std::mutex> lock(mutex);
        roomsSnapshot.loading = false;
        return;
    }

    std::vector<GameRoom> rooms;
    std::string status;
    bool success = false;

    if (response.status == 200) {
        const auto& parsed = response.body;
        if (parsed.is_array()) {
            for (const auto& roomJson : parsed) {
                if (!roomJson.is_object()) {
                    continue;
                }

                GameRoom room;
                room.host = StringField(roomJson, "host");
                room.gameCode = StringField(roomJson, "gameCode");
                room.description = StringField(roomJson, "description");
                room.isRacer = BoolField(roomJson, "isRacer");
                room.isStarted = BoolField(roomJson, "isStarted");
                rooms.push_back(std::move(room));
            }
            success = true;
            status = rooms.empty() ? "No current rooms." : std::to_string(rooms.size()) + " current room(s).";
        } else {
            status = "The server returned an invalid room list.";
        }
    } else if (IsAuthFailure(response)) {
        status = DescribeError(response);
    } else {
        status = "Could not load rooms: " + DescribeError(response);
    }

    if (!success) {
        SPDLOG_WARN("[Bingo] Failed to load current rooms: {}", status);
    }

    std::lock_guard<std::mutex> lock(mutex);
    if (success) {
        roomsSnapshot.rooms = std::move(rooms);
    }
    roomsSnapshot.status = std::move(status);
    roomsSnapshot.loading = false;
}

void Client::ReadWeekliesList() {
    std::lock_guard<std::mutex> workerLock(weekliesWorkerMutex);
    const bool linked = IsLoggedIn();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (weekliesSnapshot.loading) {
            return;
        }
        if (!linked) {
            weekliesSnapshot.status = "Link your ShipBingo account to load weeklies.";
            return;
        }
        weekliesSnapshot.hasRequested = true;
        weekliesSnapshot.loading = true;
        weekliesSnapshot.status = "Loading current weeklies...";
    }

    if (weekliesWorker.joinable()) {
        weekliesWorker.join();
    }
    weekliesWorker = std::thread(&Client::RunWeekliesRequest, this);
}

WeekliesSnapshot Client::GetWeekliesSnapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    return weekliesSnapshot;
}

void Client::RunWeekliesRequest() {
    ApiResponse response = HttpGet("api/WeeklySeed/ListWeeklies/", weekliesStopRequested);
    if (weekliesStopRequested) {
        std::lock_guard<std::mutex> lock(mutex);
        weekliesSnapshot.loading = false;
        return;
    }

    std::vector<Weeklies> weeklies;
    std::string status;
    bool success = false;

    if (response.status == 200) {
        const auto& parsed = response.body;
        if (parsed.is_array()) {
            for (const auto& weeklyJson : parsed) {
                if (!weeklyJson.is_object()) {
                    continue;
                }

                Weeklies weekly;
                weekly.name = StringField(weeklyJson, "name");
                weekly.isMystery = StringField(weeklyJson, "isMystery");
                weekly.runStatus = StringField(weeklyJson, "runStatus");
                weeklies.push_back(std::move(weekly));
            }
            success = true;
            status = weeklies.empty() ? "No current weeklies." : std::to_string(weeklies.size()) + " current weeklies.";
        } else {
            status = "The server returned an invalid weeklies list.";
        }
    } else if (IsAuthFailure(response)) {
        status = DescribeError(response);
    } else {
        status = "Could not load weeklies: " + DescribeError(response);
    }

    if (!success) {
        SPDLOG_WARN("[Bingo] Failed to load current weeklies: {}", status);
    }

    std::lock_guard<std::mutex> lock(mutex);
    if (success) {
        weekliesSnapshot.weeklies = std::move(weeklies);
    }
    weekliesSnapshot.status = std::move(status);
    weekliesSnapshot.loading = false;
}

bool Client::IsLoggedIn() {
    return OAuthClient::Instance().IsLinked();
}

bool Client::IsActive() {
    std::lock_guard<std::mutex> lock(mutex);
    return snapshot.state == ConnectionState::Connected;
}

Snapshot Client::GetSnapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    return snapshot;
}

void Client::RequestToggle(const std::string& squareId) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (snapshot.state != ConnectionState::Connected ||
            std::find(pendingToggles.begin(), pendingToggles.end(), squareId) != pendingToggles.end()) {
            return;
        }
        pendingToggles.push_back(squareId);
    }
    wake.notify_all();
}

void Client::SetStatus(ConnectionState state, const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex);
    snapshot.state = state;
    snapshot.status = status;
}

void Client::Run(std::string gameId) {
    const std::string escapedGameId = UrlEscape(gameId);

    ApiResponse me = HttpGet("api/app/v1/me", stopRequested);
    if (stopRequested) {
        return;
    }
    if (me.result != AuthResult::Ok) {
        SetStatus(ConnectionState::Error, IsAuthFailure(me) ? DescribeError(me)
                                                           : "Could not reach ShipBingo: " + DescribeError(me));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        myIdentifiers.clear();
        for (const char* key : { "userId", "username" }) {
            std::string value = StringField(me.body, key);
            if (!value.empty()) {
                myIdentifiers.push_back(Lower(value));
            }
        }
    }

    SetStatus(ConnectionState::Connecting, "Joining game " + gameId + "...");
    ApiResponse join = HttpGet("api/Game/JoinGame/" + escapedGameId + "/false", stopRequested);
    if (stopRequested) {
        return;
    }
    if (IsAuthFailure(join)) {
        SetStatus(ConnectionState::Error, DescribeError(join));
        return;
    }
    if (join.status != 200) {
        SetStatus(ConnectionState::Error, "Could not join game: " + DescribeError(join));
        return;
    }

    while (!stopRequested) {
        std::deque<std::string> toggles;
        {
            std::lock_guard<std::mutex> lock(mutex);
            toggles.swap(pendingToggles);
        }
        for (const std::string& squareId : toggles) {
            ApiResponse toggle =
                HttpGet("api/Game/ToggleCell/" + escapedGameId + "/" + UrlEscape(squareId), stopRequested);
            if (toggle.status != 200 && !stopRequested) {
                SPDLOG_WARN("[Bingo] Failed to mark square {}: {}", squareId, DescribeError(toggle));
            }
        }

        // The server decides which sections to include per account, so the id is only a hint; -1 asks for everything.
        ApiResponse stateResponse = HttpGet("api/Game/GetGameState/" + escapedGameId + "/-1", stopRequested);
        if (stopRequested) {
            return;
        }
        if (IsAuthFailure(stateResponse)) {
            SetStatus(ConnectionState::Error, DescribeError(stateResponse));
            return;
        }
        if (stateResponse.status == 200) {
            if (stateResponse.body.is_object()) {
                ApplyGameState(stateResponse.body);
            }
        } else {
            SetStatus(ConnectionState::Connecting, "Retrying: " + DescribeError(stateResponse));
        }

        std::unique_lock<std::mutex> lock(mutex);
        wake.wait_for(lock, kPollInterval, [this]() { return stopRequested.load() || !pendingToggles.empty(); });
    }
}

void Client::ApplyGameState(const nlohmann::json& state) {
    std::lock_guard<std::mutex> lock(mutex);
    snapshot.state = ConnectionState::Connected;
    snapshot.hasStarted = state.value("hasStarted", false);

    auto board = state.find("boardSquares");
    if (board != state.end() && board->is_array() && !board->empty()) {
        snapshot.boardSeed = StringField(state, "boardSeed");
        snapshot.squares.clear();
        for (const auto& square : *board) {
            BoardSquare entry;
            entry.id = StringField(square, "id");
            entry.name = StringField(square, "name");
            entry.helpText = StringField(square, "helpText");
            if (square.contains("taken") && square["taken"].is_array()) {
                for (const auto& color : square["taken"]) {
                    entry.taken.push_back(Lower(ToString(color)));
                }
            }
            snapshot.squares.push_back(std::move(entry));
        }
    }

    // The server only fills in sections it considers changed, so an empty section means "no change".
    if (snapshot.squares.empty()) {
        snapshot.status = "Connected, but the server has not sent board data yet.\n\n"
                          "If the site is open on this account in a browser, it may be receiving the updates instead; "
                          "close that tab and reconnect in ship.";
    } else {
        snapshot.status = "Connected";
    }

    auto users = state.find("users");
    if (users != state.end() && users->is_array() && !users->empty()) {
        snapshot.users.clear();
        for (const auto& user : *users) {
            snapshot.users.push_back(
                { StringField(user, "id"), StringField(user, "name"), Lower(StringField(user, "color")) });
        }
    }

    snapshot.myColor.clear();
    for (const BoardUser& user : snapshot.users) {
        auto matches = [&](const std::string& value) {
            return !value.empty() &&
                   std::find(myIdentifiers.begin(), myIdentifiers.end(), Lower(value)) != myIdentifiers.end();
        };
        if (matches(user.id) || matches(user.name)) {
            snapshot.myColor = user.color;
            break;
        }
    }
}

} // namespace Bingo
