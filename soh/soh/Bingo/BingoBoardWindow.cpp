#include "BingoBoardWindow.h"
#include "bingoEnums.h"
#include "soh/Enhancements/randomizer/rng.h"
#include "soh/cvar_prefixes.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include <ship/Context.h>
#include <fast/Fast3dGui.h>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <algorithm>
#include <unordered_map>

Color_RGBA8 Color_Background = { 0, 0, 0, 255 };
static WidgetInfo backgroundColorWidget;
static WidgetInfo windowTypeWidget;

using namespace UIWidgets;

std::unordered_map<uint32_t, std::string> bingoSquares = {
    { BS_TREASURE_ZELDA, "Obtain the Treasure thrown by Princess Zelda" },
    { BS_PRELUDE_CHECK, "Prelude of Light Check" },
    { BS_NOCTURNE_CHECK, "Nocturne of Shadow Check" },
    { BS_FOREST_SPIRIT_MEDAL_RUBY, "Forest & Spirit Medallion and Goron Ruby" },
    { BS_SHADOW_FIRE_MEDAL_EMERALD, "Shadow & Fire Medallion and Kokiri Emerald" },
    { BS_LIGHT_WATER_MEDAL_SAPPHIRE, "Light & Water Medallion and Zora Sapphire" },
    { BS_2_STONES, "2 Spiritual Stones" },
    { BS_5_MEDALS, "5 Medallions" },
    { BS_4_COMPASSES, "4 Compasses" },
    { BS_6_COMPASSES, "6 Compasses" },
    { BS_4_MAPS, "4 Maps" },
    { BS_6_MAPS, "6 Maps" },
    { BS_DEKU_MAP_COMPASS, "Map & Compass in Deku Tree" },
    { BS_DC_MAP_COMPASS, "Map & Compass in Dodongo's Cavern" },
    { BS_JABU_MAP_COMPASS, "Map & Compass in Jabu-Jabu's Belly" },
    { BS_FOREST_MAP_COMPASS, "Map & Compass in Forest Temple" },
    { BS_FIRE_MAP_COMPASS, "Map & Compass in Fire Temple" },
    { BS_WATER_MAP_COMPASS, "Map & Compass in Water Temple" },
    { BS_SHADOW_MAP_COMPASS, "Map & Compass in Shadow Temple" },
    { BS_SPIRIT_MAP_COMPASS, "Map & Compass in Spirit Temple" },
    { BS_ICE_MAP_COMPASS, "Map & Compass in Ice Cavern" },
    { BS_BOTW_MAP_COMPASS, "Map & Compass in Bottom of the Well" },
    { BS_3_BOSS_KEYS, "3 Boss Keys" },
    { BS_5_BOSS_KEYS, "5 Boss Keys" },
    { BS_FOREST_BOSS_KEY, "Forest Temple Boss Key" },
};

namespace SohGui {

void BingoBoardSettingsWindow::InitElement() {
    // Nothing to initialize for now
}

void BingoBoardSettingsWindow::UpdateElement() {
    // No dynamic state needed yet
}

void BingoBoardSettingsWindow::DrawElement() {
    ImGui::TableNextColumn();
    SohGui::GetSohMenu()->MenuDrawItem(backgroundColorWidget, static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), THEME_COLOR);

    SohGui::GetSohMenu()->MenuDrawItem(windowTypeWidget, static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), THEME_COLOR);

    if (CVarGetInteger(CVAR_BINGO_BOARD("WindowType"), TRACKER_WINDOW_WINDOW) == TRACKER_WINDOW_FLOATING) {
        CVarCheckbox("Enable Dragging", CVAR_BINGO_BOARD("Draggable"), CheckboxOptions().Color(THEME_COLOR));
    }

    if (ImGui::BeginTable("bingoBoardSubSettings", 1,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("column 1", ImGuiTableColumnFlags_WidthStretch, 150.0f);
        ImGui::TableNextColumn();

        CVarSliderFloat("Font Size", CVAR_BINGO_BOARD("FontSize"),
                     FloatSliderOptions()
                         .Tooltip("Sets the font size used in the hint tracker.")
                         .Format("%.1f")
                         .Step(0.1f)
                         .Min(0.3f)
                         .Max(2.0f)
                         .Color(THEME_COLOR)
                         .DefaultValue(1.0f));

        ImGui::EndTable();
    }
    ImGui::Separator();

    // Embed the live bingo board here if the board window is not popped out.
    // When the user pops out the board, it becomes visible as its own window
    // and we should not render it inside the settings UI.
    if (auto gui = Ship::Context::GetRawInstance()->GetWindow()->GetGui()) {
        auto boardWindow = gui->GetGuiWindow("Bingo Board");
        if (boardWindow != nullptr && !boardWindow->IsVisible()) {
            boardWindow->DrawElement();
        }
    }
}

void BingoBoardWindow::Draw() {
    if (!IsVisible()) {
        return;
    }
    DrawElement();
    SyncVisibilityConsoleVariable();
}

void BingoBoardWindow::InitElement() {
    // Nothing to initialize for now
}

void BingoBoardWindow::UpdateElement() {
    // No dynamic state needed yet
}

void BingoBoardWindow::DrawElement() {
    constexpr int GRID_N = 5; // 5x5 grid
    const float windowPadding = 6.0f;

    ImGui::SetWindowFontScale(CVarGetFloat(CVAR_BINGO_BOARD("FontSize"), 1.0f));

    // Determine the maximum square area that fits in the available content region
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float maxSide = std::min(avail.x, avail.y);

    float gridTotalSpacing = GRID_N - 1;
    float gridInnerSize = maxSide - (windowPadding * 2.0f);
    if (gridInnerSize <= 0.0f) {
        return;
    }

    float cellSize = (gridInnerSize - gridTotalSpacing) / static_cast<float>(GRID_N);
    if (cellSize <= 0.0f) {
        return;
    }

    float gridWidth = cellSize * GRID_N + gridTotalSpacing;
    float gridHeight = gridWidth; // square

    // Use a child region so the drawing doesn't overlap other widgets and is clipped
    ImGui::BeginChild("BingoBoardCanvas", ImVec2(gridWidth + windowPadding, gridHeight + windowPadding), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    // Center the grid inside the child area
    ImVec2 gridTopLeft = ImVec2(canvasPos.x + windowPadding + (avail.x - gridWidth) * 0.5f,
                                canvasPos.y + windowPadding + (avail.y - gridHeight) * 0.5f);

    ImU32 outlineCol = IM_COL32(150, 150, 150, 255); // White color for outlines
    const float outlineThickness = 3.0f;

    for (int r = 0; r < GRID_N; ++r) {
        for (int c = 0; c < GRID_N; ++c) {
            ImVec2 cellTL = ImVec2(gridTopLeft.x + c * cellSize,
                                   gridTopLeft.y + r * cellSize);
            ImVec2 cellBR = ImVec2(cellTL.x + cellSize, cellTL.y + cellSize);

            // Draw outline for each square
            drawList->AddRectFilled(cellTL, cellBR, IM_COL32(50, 50, 50, 255), 0.0f);
            drawList->AddRect(cellTL, cellBR, outlineCol, 0.0f, 0, outlineThickness);
            drawList->AddText(ImVec2(cellTL.x + 5.0f, cellTL.y + 5.0f), IM_COL32(255, 255, 255, 255),
                              "This is a test");
        }
    }

    // Reserve the same space in the layout so other widgets don't overlap
    ImGui::Dummy(ImVec2(gridWidth + windowPadding * 2.0f, gridHeight + windowPadding * 2.0f));

    if (Trackers::BeginFloatWindows(
            "BingoBoardCanvas", mIsVisible, Color_Background,
            static_cast<TrackerWindowType>(CVarGetInteger(CVAR_BINGO_BOARD("WindowType"), TRACKER_WINDOW_WINDOW)),
            CVarGetInteger(CVAR_BINGO_BOARD("Draggable"), 1), ImGuiWindowFlags_NoScrollbar)) {

        PushStyleCombobox(THEME_COLOR);
        PopStyleCombobox();

        ImGui::Separator();

        ImGui::SetWindowFontScale(CVarGetFloat(CVAR_TRACKER_HINTS("FontSize"), 1.0f));

        ImGui::EndChild();
    }
    Trackers::EndFloatWindows();
}

void RegisterBingoBoardWidgets() {
    backgroundColorWidget = { .name = "Background Color##BingoBoard", .type = WidgetType::WIDGET_CVAR_COLOR_PICKER };
    backgroundColorWidget.CVar(CVAR_BINGO_BOARD("BgColor"))
        .Options(
            ColorPickerOptions().Color(THEME_COLOR).DefaultValue(Color_Bg_Default).UseAlpha().ShowReset().ShowRandom());
    SohGui::GetSohMenu()->AddSearchWidget({ backgroundColorWidget, "Bingo", "Bingo Board", "General Settings" });

    windowTypeWidget = { .name = "Window Type##BingoBoard", .type = WidgetType::WIDGET_CVAR_COMBOBOX };
    windowTypeWidget.CVar(CVAR_BINGO_BOARD("WindowType"))
        .Options(ComboboxOptions()
                     .DefaultIndex(TRACKER_WINDOW_WINDOW)
                     .ComponentAlignment(ComponentAlignments::Right)
                     .LabelPosition(LabelPositions::Far)
                     .Color(THEME_COLOR)
                     .ComboMap(windowType));
    SohGui::GetSohMenu()->AddSearchWidget({ windowTypeWidget, "Bingo", "Bingo Board", "General Settings" });
}

static RegisterShipInitFunc gInitFunc([]() { RegisterBingoBoardWidgets(); }, { "IS_RANDO" });

} // namespace SohGui
