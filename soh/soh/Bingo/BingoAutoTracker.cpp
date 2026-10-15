#include "BingoClient.h"
#include "BingoSquareMap.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace Bingo {

namespace {

constexpr int kFramesBetweenChecks = 20; // the game runs at 20 updates per second
constexpr auto kRetryDelay = std::chrono::seconds(15);

using Clock = std::chrono::steady_clock;

std::unordered_map<std::string, Clock::time_point> sAttempts;
std::vector<std::string> sBoardIds;
int sFrameCounter = 0;

void OnGameFrameUpdate() {
    if (++sFrameCounter < kFramesBetweenChecks) {
        return;
    }
    sFrameCounter = 0;

    if (!GameInteractor::IsSaveLoaded()) {
        return;
    }

    Snapshot snapshot = Client::Instance().GetSnapshot();
    if (snapshot.state != ConnectionState::Connected || !snapshot.hasStarted) {
        return;
    }

    std::vector<std::string> ids;
    for (const BoardSquare& square : snapshot.squares) {
        ids.push_back(square.id);
    }
    if (ids != sBoardIds) {
        sBoardIds = std::move(ids);
        sAttempts.clear();
    }

    const auto now = Clock::now();
    for (const BoardSquare& square : snapshot.squares) {
        const SquareDef* def = FindSquareByName(square.name);
        if (def == nullptr || def->isComplete == nullptr || !def->isComplete()) {
            continue;
        }

        const bool markedByUs =
            !snapshot.myColor.empty() &&
            std::find(square.taken.begin(), square.taken.end(), snapshot.myColor) != square.taken.end();
        if (markedByUs) {
            continue;
        }

        // The server endpoint toggles, so never re-send unless we can confirm from our color that it did not stick.
        auto attempt = sAttempts.find(square.id);
        if (attempt != sAttempts.end() && (snapshot.myColor.empty() || now - attempt->second < kRetryDelay)) {
            continue;
        }

        sAttempts[square.id] = now;
        Client::Instance().RequestToggle(square.id);
    }
}

void RegisterBingoAutoTracker() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(OnGameFrameUpdate);
}

} // namespace

static RegisterShipInitFunc sInitFunc(RegisterBingoAutoTracker);

} // namespace Bingo
