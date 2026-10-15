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

// Talks to shipbingo.zarashane.dev on a worker thread. All public methods are safe to call from the game thread.
class Client {
  public:
    static Client& Instance();

    void Connect(const std::string& gameId, const std::string& sessionCookie);
    void Disconnect();
    bool IsActive();
    Snapshot GetSnapshot();

    // Asks the server to toggle a square for the logged in user. Callers must make sure the square is not already
    // marked with the user's color, since the server endpoint is a toggle.
    void RequestToggle(const std::string& squareId);

  private:
    Client() = default;
    ~Client();

    void Run(std::string gameId, std::string cookie);
    void SetStatus(ConnectionState state, const std::string& status);
    void ApplyGameState(const nlohmann::json& state);

    std::mutex mutex;
    std::condition_variable wake;
    std::thread worker;
    std::atomic<bool> stopRequested = false;
    Snapshot snapshot;
    std::deque<std::string> pendingToggles;
    std::vector<std::string> myIdentifiers; // values from /auth/me used to find our entry in the user list
};

} // namespace Bingo
