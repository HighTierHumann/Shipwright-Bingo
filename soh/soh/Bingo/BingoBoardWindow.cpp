#include "BingoBoardWindow.h"
#include "BingoClient.h"
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
#include <vector>
#include <unordered_map>

using namespace UIWidgets;

namespace BingoTracker {

Color_RGBA8 Color_Background = { 0, 0, 0, 255 };
static WidgetInfo backgroundColorWidget;
static WidgetInfo windowTypeWidget;

static bool presetLoaded = false;
static ImVec2 presetPos;
static ImVec2 presetSize;

static std::unordered_map<std::string, float> sSquareHoldTimes;
static std::unordered_map<std::string, bool> sSquareLongHoldTriggered;

// Parses "#rrggbb" into an ImGui color, returning false for anything else.
static bool ParseHexColor(const std::string& hex, ImU32& out, int alpha) {
    if (hex.size() != 7 || hex[0] != '#') {
        return false;
    }
    char* end = nullptr;
    unsigned long rgb = strtoul(hex.c_str() + 1, &end, 16);
    if (end != hex.c_str() + 7) {
        return false;
    }
    out = IM_COL32((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, alpha);
    return true;
}

static std::unordered_map<std::string, int> sSquareIcons;

static void DrawCellContents(ImDrawList* drawList, const Bingo::BoardSquare& square, ImVec2 cellTL, float cellSize) {
    std::vector<ImU32> colors;
    for (const std::string& hex : square.taken) {
        ImU32 color;
        if (ParseHexColor(hex, color, 200)) {
            colors.push_back(color);
        }
    }
    for (size_t i = 0; i < colors.size(); ++i) {
        float left = cellTL.x + cellSize * i / colors.size();
        float right = cellTL.x + cellSize * (i + 1) / colors.size();
        drawList->AddRectFilled(ImVec2(left, cellTL.y), ImVec2(right, cellTL.y + cellSize), colors[i]);
    }

    const ImVec2 cellBR(cellTL.x + cellSize, cellTL.y + cellSize);
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();
    drawList->PushClipRect(cellTL, ImVec2(cellTL.x + cellSize, cellTL.y + cellSize), true);
    if (!square.helpText.empty()) {
        drawList->AddText(font, fontSize * 2, ImVec2(cellTL.x + (cellSize - ImGui::CalcTextSize("*").x * 2.5f), cellTL.y + 1.0f), IM_COL32(0, 255, 0, 255), "*", nullptr);
    }

    const float textPadding = 4.0f;
    const float wrapWidth = cellSize - textPadding * 2.0f;
    if (wrapWidth > 0.0f && !square.name.empty()) {
        const char* text = square.name.c_str();
        ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false, wrapWidth);
        ImVec2 textPos(cellTL.x + (cellSize - std::min(textSize.x, wrapWidth)) * 0.5f,
                       cellTL.y + std::max(textPadding, (cellSize - textSize.y) * 0.5f));
        drawList->AddText(font, fontSize, ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), text,
                          nullptr, wrapWidth);
        drawList->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), text, nullptr, wrapWidth);
    }

    int& iconToUse = sSquareIcons[square.id];
    const bool isHovered = ImGui::IsMouseHoveringRect(cellTL, cellBR);

    if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        sSquareHoldTimes[square.id] += ImGui::GetIO().DeltaTime;

        if (sSquareHoldTimes[square.id] >= 0.6f && !sSquareLongHoldTriggered[square.id]) {
            iconToUse = 0;
            sSquareLongHoldTriggered[square.id] = true;
        }
    }

    if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        if (!sSquareLongHoldTriggered[square.id]) {
            iconToUse = (iconToUse + 1) % 10;
        }
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        sSquareHoldTimes[square.id] = 0.0f;
        sSquareLongHoldTriggered[square.id] = false;
    }

    if (iconToUse != 0) {
        const float padding = 4.0f;
        const float iconSize = std::min(24.0f, cellSize * 0.25f);
        const ImVec2 imgMin(cellTL.x + padding, cellTL.y + padding);
        const ImVec2 imgMax(imgMin.x + iconSize, imgMin.y + iconSize);
        std::string itemName;

        switch (iconToUse) {
            case 1:
                itemName = "ITEM_BOW";
                break;
            case 2:
                itemName = "ITEM_HOOKSHOT";
                break;
            case 3:
                itemName = "ITEM_BOMB";
                break;
            case 4:
                itemName = "ITEM_ARROW_ICE";
                break;
            case 5:
                itemName = "ITEM_HAMMER";
                break;
            case 6:
                itemName = "ITEM_SWORD_MASTER";
                break;
            case 7:
                itemName = "QUEST_KOKIRI_EMERALD";
                break;
            case 8:
                itemName = "QUEST_GORON_RUBY";
                break;
            case 9:
                itemName = "QUEST_ZORA_SAPPHIRE";
                break;
            default:
                break;
        }
        drawList->AddImage(std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui())
                     ->GetTextureByName(itemName), imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1));
    }
    
    drawList->PopClipRect();

    if (isHovered && !square.helpText.empty()) {
        ImGui::SetTooltip("%s", square.helpText.c_str());
    }
}

void LoadFromPreset(const nlohmann::json& info) {
    presetLoaded = true;
    presetPos = { info.at("pos").at("x"), info.at("pos").at("y") };
    presetSize = { info.at("size").at("width"), info.at("size").at("height") };
}

void DrawBingoBoardContents() {
    constexpr int GRID_N = 5; // 5x5 grid
    constexpr float windowPadding = 6.0f;
    gSaveContext.inventory.dungeonItems[0];

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x <= 0.0f || avail.y <= 0.0f) {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool canvasVisible =
        ImGui::BeginChild("Bingo Board Squares", avail, false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (canvasVisible) {
        ImGui::SetWindowFontScale(CVarGetFloat(CVAR_BINGO_BOARD("FontSize"), 1.0f));

        ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
        float maxSide = std::min(canvasAvail.x, canvasAvail.y);
        float gridTotalSpacing = GRID_N - 1;
        float gridInnerSize = maxSide - (windowPadding * 2.0f);

        if (gridInnerSize > 0.0f) {
            float cellSize = (gridInnerSize - gridTotalSpacing) / static_cast<float>(GRID_N);
            if (cellSize > 0.0f) {
                float gridWidth = cellSize * GRID_N + gridTotalSpacing;
                float gridHeight = gridWidth;
                const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                ImVec2 gridTopLeft =
                    ImVec2(canvasPos.x + (canvasAvail.x - gridWidth) * 0.5f,
                           canvasPos.y + (canvasAvail.y - gridHeight) * 0.5f);

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImU32 outlineCol = IM_COL32(80, 80, 80, 255);
                const float outlineThickness = 1.0f;
                const Bingo::Snapshot snapshot = Bingo::Client::Instance().GetSnapshot();

                for (int r = 0; r < GRID_N; ++r) {
                    for (int c = 0; c < GRID_N; ++c) {
                        ImVec2 cellTL = ImVec2(gridTopLeft.x + c * cellSize,
                                               gridTopLeft.y + r * cellSize);
                        ImVec2 cellBR = ImVec2(cellTL.x + cellSize, cellTL.y + cellSize);

                        drawList->AddRectFilled(cellTL, cellBR, IM_COL32(20, 20, 20, 255), 0.0f);
                        size_t index = static_cast<size_t>(r * GRID_N + c);
                        if (index < snapshot.squares.size()) {
                            DrawCellContents(drawList, snapshot.squares[index], cellTL, cellSize);
                        }
                        drawList->AddRect(cellTL, cellBR, outlineCol, 0.0f, 0, outlineThickness);
                    }
                }

                if (snapshot.squares.empty()) {
                    const char* message = snapshot.state == Bingo::ConnectionState::Disconnected
                                              ? "Not connected. Enter a Game ID and session cookie under Bingo > Login."
                                              : snapshot.status.c_str();
                    float wrapWidth = gridWidth * 0.8f;
                    ImVec2 textSize = ImGui::CalcTextSize(message, nullptr, false, wrapWidth);
                    drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                                      ImVec2(gridTopLeft.x + (gridWidth - std::min(textSize.x, wrapWidth)) * 0.5f,
                                             gridTopLeft.y + (gridHeight - textSize.y) * 0.5f),
                                      IM_COL32(255, 255, 255, 255), message, nullptr, wrapWidth);
                }

                ImGui::Dummy(ImVec2(gridWidth, gridHeight));
            }
        }
    }

    ImGui::EndChild();
}

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

    Color_Background = CVarGetColor(CVAR_BINGO_BOARD("BgColor.Value"), Color_Bg_Default);
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
            DrawBingoBoardContents();
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
    if (presetLoaded) {
        ImGui::SetNextWindowSize(presetSize);
        ImGui::SetNextWindowPos(presetPos);
        presetLoaded = false;
    } else {
        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
    }
    if (Trackers::BeginFloatWindows(
            "Bingo Board", mIsVisible, Color_Background,
            static_cast<TrackerWindowType>(CVarGetInteger(CVAR_BINGO_BOARD("WindowType"), TRACKER_WINDOW_WINDOW)),
            CVarGetInteger(CVAR_BINGO_BOARD("Draggable"), 1), ImGuiWindowFlags_NoScrollbar)) {

        DrawBingoBoardContents();

        PushStyleCombobox(THEME_COLOR);
        PopStyleCombobox();

        ImGui::Separator();

        ImGui::SetWindowFontScale(CVarGetFloat(CVAR_TRACKER_HINTS("FontSize"), 1.0f));
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

} // namespace BingoTracker
