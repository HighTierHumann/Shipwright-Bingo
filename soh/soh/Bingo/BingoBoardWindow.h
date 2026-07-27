#pragma once

#include <imgui.h>
#include <ship/window/gui/GuiWindow.h>

namespace SohGui {

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

} // namespace SohGui
