#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

namespace Bingo {

// Initializes libcurl exactly once; call before the first request from any thread.
void EnsureCurlInitialized();

struct BoardSquare {
    std::string id;
    std::string name;
    std::string helpText;
    std::vector<std::string> taken; // hex colors of every player that marked this square
};

struct BoardUser {
    std::string id;
    std::string name;
    std::string color;
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Error,
};

struct Snapshot {
    ConnectionState state = ConnectionState::Disconnected;
    std::string status = "Not connected";
    bool hasStarted = false;
    std::string boardSeed;
    std::vector<BoardSquare> squares;
    std::vector<BoardUser> users;
    std::string myColor; // empty if our user could not be located in the user list
};

struct GameRoom {
    std::string description;
    std::string gameCode;
    std::string host;
    bool isRacer = false;
    bool isStarted = false;
};

struct Weeklies {
    std::string name;
    std::string isMystery;
    std::string runStatus;
};

struct RoomsSnapshot {
    bool hasRequested = false;
    bool loading = false;
    std::string status = "Rooms have not been loaded.";
    std::vector<GameRoom> rooms;
};

struct WeekliesSnapshot {
    bool hasRequested = false;
    bool loading = false;
    std::string status = "Weeklies have not been loaded.";
    std::vector<Weeklies> weeklies;
};

// Talks to shipbingo.zarashane.dev on worker threads, authenticated through the linked ShipBingo account (OAuthClient).
// All public methods are safe to call from the game thread.
class Client {
  public:
    static Client& Instance();

    void Connect(const std::string& gameId);
    void Disconnect();
    void RenewCard(const std::string& gameIdInput, const std::string& seedID);
    void UpdateUserColor(const std::string& gameIdInput, const std::string& colorToUse);
    void CreateGame(const std::string& gameDescription);
    void ReadRoomsList();
    void ReadWeekliesList();
    RoomsSnapshot GetRoomsSnapshot();
    WeekliesSnapshot GetWeekliesSnapshot();
    // True while the ShipBingo account is linked, which is required for every call below.
    bool IsLoggedIn();
    bool IsActive();
    Snapshot GetSnapshot();

    // Toggles a square for the logged in user. Callers must confirm its current mark state differs from the desired
    // state, since the server endpoint is a toggle.
    void RequestToggle(const std::string& squareId);

  private:
    Client() = default;
    ~Client();

    void Run(std::string gameId);
    void RunRoomsRequest();
    void RunWeekliesRequest();
    void SetStatus(ConnectionState state, const std::string& status);
    void ApplyGameState(const nlohmann::json& state);

    std::mutex mutex;
    std::mutex roomsWorkerMutex;
    std::mutex weekliesWorkerMutex;
    std::condition_variable wake;
    std::thread worker;
    std::thread roomsWorker;
    std::thread weekliesWorker;
    std::atomic<bool> stopRequested = false;
    std::atomic<bool> roomsStopRequested = false;
    std::atomic<bool> weekliesStopRequested = false;
    Snapshot snapshot;
    RoomsSnapshot roomsSnapshot;
    WeekliesSnapshot weekliesSnapshot;
    std::deque<std::string> pendingToggles;
    std::vector<std::string> myIdentifiers; // values from /api/app/v1/me used to find our entry in the user list
};

} // namespace Bingo
