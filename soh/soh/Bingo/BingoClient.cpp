#include "BingoClient.h"

#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <mutex>

namespace Bingo {

namespace {

constexpr const char* kBaseUrl = "https://shipbingo.zarashane.dev/";
constexpr auto kPollInterval = std::chrono::milliseconds(250); // should be ~4 pings per second like the website?
constexpr const char* kDefaultCookieName = "sb.auth";

struct HttpResponse {
    long status = 0; // 0 means the request itself failed
    std::string body;
    std::string error;
};

size_t WriteBody(char* data, size_t size, size_t count, void* user) {
    static_cast<std::string*>(user)->append(data, size * count);
    return size * count;
}

int AbortIfCancelled(void* user, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return static_cast<std::atomic<bool>*>(user)->load() ? 1 : 0;
}

std::string UrlEscape(const std::string& value) {
    char* escaped = curl_easy_escape(nullptr, value.c_str(), static_cast<int>(value.size()));
    std::string result = escaped ? escaped : "";
    curl_free(escaped);
    return result;
}

HttpResponse HttpGet(const std::string& path, const std::string& cookie, std::atomic<bool>& cancel) {
    static std::once_flag curlInit;
    std::call_once(curlInit, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });

    HttpResponse response;
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        response.error = "Failed to initialize HTTP client";
        return response;
    }

    const std::string url = kBaseUrl + path;
    curl_slist* headers = curl_slist_append(nullptr, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Shipwright-Bingo");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); // a redirect means we were sent to the login page
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, AbortIfCancelled);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &cancel);

    CURLcode result = curl_easy_perform(curl);
    if (result == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status);
    } else {
        response.error = curl_easy_strerror(result);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

// Accepts either the raw value of the session cookie or a full "name=value; name2=value2" cookie string.
std::string NormalizeCookie(std::string cookie) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    cookie.erase(cookie.begin(), std::find_if_not(cookie.begin(), cookie.end(), isSpace));
    cookie.erase(std::find_if_not(cookie.rbegin(), cookie.rend(), isSpace).base(), cookie.end());

    if (Lower(cookie.substr(0, 7)) == "cookie:") {
        cookie.erase(0, 7);
        cookie.erase(cookie.begin(), std::find_if_not(cookie.begin(), cookie.end(), isSpace));
    }
    if (!cookie.empty() && cookie.find('=') == std::string::npos) {
        cookie = std::string(kDefaultCookieName) + "=" + cookie;
    }
    return cookie;
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
std::string DescribeError(const HttpResponse& response) {
    if (response.status == 0) {
        return response.error;
    }
    auto parsed = nlohmann::json::parse(response.body, nullptr, false);
    if (parsed.is_string()) {
        return parsed.get<std::string>();
    }
    return "HTTP " + std::to_string(response.status);
}

bool IsAuthFailure(const HttpResponse& response) {
    return response.status == 401 || response.status == 403 || (response.status >= 300 && response.status < 400);
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

void Client::Login(const std::string& sessionCookie) {
    Logout();

    const std::string cookie = NormalizeCookie(sessionCookie);
    if (cookie.empty()) {
        SetStatus(ConnectionState::Error, "Paste your session cookie first");
        return;
    }

    stopRequested = false;
    SetStatus(ConnectionState::Connecting, "Logging in...");
    worker = std::thread(&Client::Authenticate, this, cookie);
}

void Client::Logout() {
    stopRequested = true;
    wake.notify_all();
    if (worker.joinable()) {
        worker.join();
    }

    std::lock_guard<std::mutex> lock(mutex);
    snapshot = Snapshot{};
    pendingToggles.clear();
    myIdentifiers.clear();
}

void Client::Connect(const std::string& gameIdInput, const std::string& sessionCookie) {
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
    const std::string cookie = NormalizeCookie(sessionCookie);
    if (cookie.empty()) {
        SetStatus(ConnectionState::Error, "Paste your session cookie first");
        return;
    }

    stopRequested = false;
    SetStatus(ConnectionState::Connecting, "Connecting...");
    worker = std::thread(&Client::Run, this, gameId, cookie);
}

void Client::Disconnect() {
    stopRequested = true;
    wake.notify_all();
    if (worker.joinable()) {
        worker.join();
    }

    std::lock_guard<std::mutex> lock(mutex);
    snapshot = Snapshot{ConnectionState::Authenticated};
    pendingToggles.clear();
}

void Client::RenewCard(const std::string& gameIdInput, const std::string& sessionCookie, const std::string& seedID) {
    HttpResponse join = HttpGet("api/Game/RefreshBoard/" + UrlEscape(gameIdInput) + "/" + UrlEscape(seedID),
                                            NormalizeCookie(sessionCookie), stopRequested);
}

void Client::UpdateUserColor(const std::string& gameIdInput, const std::string& sessionCookie, const std::string& colorToUse) {
    HttpResponse join = HttpGet("api/Game/UserColorSelected/" + UrlEscape(gameIdInput) + "/" + "!" + colorToUse,
                                NormalizeCookie(sessionCookie), stopRequested);
}

void Client::CreateGame(const std::string& sessionCookie, const std::string& gameDescription) {
    const std::string cookie = NormalizeCookie(sessionCookie);
    if (cookie.empty()) {
        SetStatus(ConnectionState::Error, "Paste your session cookie first");
        return;
    }

    std::atomic<bool> cancel = false;
    HttpResponse response = HttpGet("api/Game/StartGame/" + UrlEscape(gameDescription), cookie, cancel);
    if (IsAuthFailure(response)) {
        SetStatus(ConnectionState::Error, "Session expired. Paste a new session cookie.");
        return;
    }
    if (response.status != 200) {
        const std::string status = "Could not create game: " + DescribeError(response);
        SPDLOG_WARN("[Bingo] {}", status);
        SetStatus(ConnectionState::Error, status);
        return;
    }

    const auto gameId = nlohmann::json::parse(response.body, nullptr, false);
    if (!gameId.is_number_integer() && !gameId.is_number_unsigned()) {
        const std::string status = "The server returned an invalid Game ID.";
        SPDLOG_WARN("[Bingo] {}", status);
        SetStatus(ConnectionState::Error, status);
        return;
    }

    Connect(gameId.dump(), cookie);
}

void Client::ReadRoomsList(const std::string& sessionCookie) {
    std::lock_guard<std::mutex> workerLock(roomsWorkerMutex);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (roomsSnapshot.loading) {
            return;
        }
        roomsSnapshot.hasRequested = true;
        roomsSnapshot.loading = true;
        roomsSnapshot.status = "Loading current rooms...";
    }

    if (roomsWorker.joinable()) {
        roomsWorker.join();
    }
    roomsWorker = std::thread(&Client::RunRoomsRequest, this, NormalizeCookie(sessionCookie));
}

RoomsSnapshot Client::GetRoomsSnapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    return roomsSnapshot;
}

void Client::RunRoomsRequest(std::string cookie) {
    HttpResponse response = HttpGet("api/Game/GetGames/", cookie, roomsStopRequested);
    if (roomsStopRequested) {
        std::lock_guard<std::mutex> lock(mutex);
        roomsSnapshot.loading = false;
        return;
    }

    std::vector<GameRoom> rooms;
    std::string status;
    bool success = false;

    if (response.status == 200) {
        auto parsed = nlohmann::json::parse(response.body, nullptr, false);
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
        status = "Session expired. Paste a new session cookie.";
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

void Client::ReadWeekliesList(const std::string& sessionCookie) {
    std::lock_guard<std::mutex> workerLock(weekliesWorkerMutex);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (weekliesSnapshot.loading) {
            return;
        }
        weekliesSnapshot.hasRequested = true;
        weekliesSnapshot.loading = true;
        weekliesSnapshot.status = "Loading current weeklies...";
    }

    if (weekliesWorker.joinable()) {
        weekliesWorker.join();
    }
    weekliesWorker = std::thread(&Client::RunWeekliesRequest, this, NormalizeCookie(sessionCookie));
}

WeekliesSnapshot Client::GetWeekliesSnapshot() {
    std::lock_guard<std::mutex> lock(mutex);
    return weekliesSnapshot;
}

void Client::RunWeekliesRequest(std::string cookie) {
    HttpResponse response = HttpGet("api/WeeklySeed/ListWeeklies/", cookie, weekliesStopRequested);
    if (weekliesStopRequested) {
        std::lock_guard<std::mutex> lock(mutex);
        weekliesSnapshot.loading = false;
        return;
    }

    std::vector<Weeklies> weeklies;
    std::string status;
    bool success = false;

    if (response.status == 200) {
        auto parsed = nlohmann::json::parse(response.body, nullptr, false);
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
        status = "Session expired. Paste a new session cookie.";
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
    std::lock_guard<std::mutex> lock(mutex);
    return snapshot.state == ConnectionState::Authenticated;
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

void Client::Authenticate(std::string cookie) {
    SetStatus(ConnectionState::Authenticating, "Authenticating...");

    HttpResponse auth = HttpGet("auth/IsAuthenticated", cookie, stopRequested);
    if (stopRequested) {
        return;
    }
    if (auth.status == 0) {
        SetStatus(ConnectionState::Error, "Could not reach shipbingo: " + auth.error);
        return;
    }
    auto authJson = nlohmann::json::parse(auth.body, nullptr, false);
    if (auth.status != 200 || !authJson.is_boolean() || !authJson.get<bool>()) {
        SetStatus(ConnectionState::Error,
                  "Session cookie was rejected. It may be wrong or expired (sessions last about 24h).");
        return;
    }

    HttpResponse me = HttpGet("auth/me", cookie, stopRequested);
    if (me.status == 200) {
        auto meJson = nlohmann::json::parse(me.body, nullptr, false);
        if (meJson.is_object()) {
            std::lock_guard<std::mutex> lock(mutex);
            for (const char* key : { "id", "userId", "discordId", "name", "username", "userName", "displayName" }) {
                std::string value = StringField(meJson, key);
                if (!value.empty()) {
                    myIdentifiers.push_back(Lower(value));
                }
            }
        }
    }

    SetStatus(ConnectionState::Authenticated, "Authenticated");
}

void Client::Run(std::string gameId, std::string cookie) {
    const std::string escapedGameId = UrlEscape(gameId);

    HttpResponse auth = HttpGet("auth/IsAuthenticated", cookie, stopRequested);
    if (stopRequested) {
        return;
    }
    if (auth.status == 0) {
        SetStatus(ConnectionState::Error, "Could not reach shipbingo: " + auth.error);
        return;
    }
    auto authJson = nlohmann::json::parse(auth.body, nullptr, false);
    if (auth.status != 200 || !authJson.is_boolean() || !authJson.get<bool>()) {
        SetStatus(ConnectionState::Error,
                  "Session cookie was rejected. It may be wrong or expired (sessions last about 24h).");
        return;
    }

    HttpResponse me = HttpGet("auth/me", cookie, stopRequested);
    if (me.status == 200) {
        auto meJson = nlohmann::json::parse(me.body, nullptr, false);
        if (meJson.is_object()) {
            std::lock_guard<std::mutex> lock(mutex);
            for (const char* key : { "id", "userId", "discordId", "name", "username", "userName", "displayName" }) {
                std::string value = StringField(meJson, key);
                if (!value.empty()) {
                    myIdentifiers.push_back(Lower(value));
                }
            }
        }
    }

    SetStatus(ConnectionState::Connecting, "Joining game " + gameId + "...");
    HttpResponse join = HttpGet("api/Game/JoinGame/" + escapedGameId + "/false", cookie, stopRequested);
    if (stopRequested) {
        return;
    }
    if (IsAuthFailure(join)) {
        SetStatus(ConnectionState::Error, "Session expired. Paste a new session cookie.");
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
            HttpResponse toggle =
                HttpGet("api/Game/ToggleCell/" + escapedGameId + "/" + UrlEscape(squareId), cookie, stopRequested);
            if (toggle.status != 200 && !stopRequested) {
                SPDLOG_WARN("[Bingo] Failed to mark square {}: {}", squareId, DescribeError(toggle));
            }
        }

        // The server decides which sections to include per account, so the id is only a hint; -1 asks for everything.
        HttpResponse stateResponse = HttpGet("api/Game/GetGameState/" + escapedGameId + "/-1", cookie, stopRequested);
        if (stopRequested) {
            return;
        }
        if (IsAuthFailure(stateResponse)) {
            SetStatus(ConnectionState::Error, "Session expired. Paste a new session cookie.");
            return;
        }
        if (stateResponse.status == 200) {
            auto state = nlohmann::json::parse(stateResponse.body, nullptr, false);
            if (state.is_object()) {
                ApplyGameState(state);
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
