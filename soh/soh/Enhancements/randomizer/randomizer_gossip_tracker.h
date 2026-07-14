#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "soh/SohGui/UIWidgets.hpp"
#include "3drando/hints.hpp"
#include "randomizerTypes.h"

#include <nlohmann/json_fwd.hpp>
#include <libultraship/color.h>
#include <libultraship/libultra.h>
#include <ship/window/gui/GuiWindow.h>
#include "randomizerTypes.h"

#define HINT_TRACKER_ITEM(id) { id, #id, #id "_Faded" }

typedef enum {
    TYPE_GOSSIP_STONE,
    TYPE_BOSS_KEY,
    TYPE_ITEM,
    TYPE_ALTAR
} HintTypes;

struct HintEntry {
    RandomizerHint hintKey;
    std::string name;
    std::string hintText;
    bool revealed = false;
    std::string areaName;
    std::string hintType;
    std::string iconTexture;
    std::string iconTextureFaded;
};

struct HintTrackerItem {
    uint32_t id;
    std::string name;
    std::string nameFaded;
};

namespace HintTracker {
class HintTrackerSettingsWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  protected:
    void InitElement() override {};
    void DrawElement() override;
    void UpdateElement() override {};
};

void InitHintTrackingData();
void RevealHint(RandomizerHint hintKey);

// Returns the list of all tracked gossip stones (useful for UI).
const std::vector<HintEntry>& GetHintEntries();

class HintTrackerWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override;
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override {};
};
} // namespace HintTracker