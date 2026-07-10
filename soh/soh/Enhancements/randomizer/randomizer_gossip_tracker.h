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

typedef enum {
    TYPE_GOSSIP_STONE,
    TYPE_BOSS_KEY,
    TYPE_ITEM,
} HintTypes;

struct HintEntry {
    RandomizerHint hintKey;
    std::string name;
    std::string hintText;
    bool revealed = false;
    std::string areaName;
    std::string hintType;
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