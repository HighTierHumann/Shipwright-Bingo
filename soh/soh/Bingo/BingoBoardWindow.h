#pragma once

#include <nlohmann/json.hpp>
#include <imgui.h>
#include <ship/window/gui/GuiWindow.h>

namespace BingoTracker {
  class BingoBoardSettingsWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};

class BingoBoardWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override;
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};

void LoadFromPreset(const nlohmann::json& info);
} // namespace BingoTracker
