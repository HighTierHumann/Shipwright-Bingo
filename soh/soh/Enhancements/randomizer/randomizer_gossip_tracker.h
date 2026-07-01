#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <libultraship/libultraship.h>
#include "3drando/text.hpp"
#include "3drando/hints.hpp"
#include "randomizerTypes.h"

    namespace HintTracker {

    class HintTrackerSettingsWindow final : public Ship::GuiWindow {
      public:
        using GuiWindow::GuiWindow;

      protected:
        void InitElement() override {};
        void DrawElement() override;
        void UpdateElement() override {};
    };

    typedef enum {
        TYPE_GOSSIP_STONE,
        TYPE_BOSS_KEY,
        TYPE_ITEM,
    } HintTypes;

    // Minimal struct for an entry shown in the tracker
    struct HintEntry {
        RandomizerHint hintKey;
        std::string name;     // human-friendly stone name (from StaticData::hintNames)
        std::string hintText; // revealed hint text (empty if not revealed)
        bool revealed = false;
        std::string areaName;
        std::string hintType;
    };

    // Initialize the tracker state (called on game load).
    void InitHintTrackingData();

    // Called when a gossip stone hint is shown in game. hintKey may be RH_NONE for special cases.
    void RevealHint(RandomizerHint hintKey, const std::string& formattedMessage);

    // Returns the list of all tracked gossip stones (useful for UI).
    const std::vector<HintEntry>& GetHintEntries();

    // Tracker window class (registered by SohGui in SetupGuiElements).
    class HintTrackerWindow final : public Ship::GuiWindow {
      public:
        using GuiWindow::GuiWindow;
        void Draw() override;
        void InitElement() override;
        void DrawElement() override;
        void UpdateElement() override {};
    };

} // namespace HintTracker