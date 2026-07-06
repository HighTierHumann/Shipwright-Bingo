#include "randomizer_gossip_tracker.h"

#include <soh/OTRGlobals.h>
#include "soh/SohGui/SohGui.hpp"
#include <spdlog/spdlog.h>
#include <imgui.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include "soh/cvar_prefixes.h"

extern "C" {
#include <z64.h>
#include "variables.h"
extern PlayState* gPlayState;
}

using namespace UIWidgets;

#define COLOR_GRAY IM_COL32(155, 155, 155, 255)

namespace HintTracker {

bool doAreaScroll;

RandomizerCheckArea previousArea = RCAREA_INVALID;
RandomizerCheckArea currentArea = RCAREA_INVALID;

static std::vector<HintEntry> gHintEntries;
static std::unordered_map<RandomizerHint, size_t> gHintKeyToIndex;

static ImGuiTextFilter hintSearch;

Color_RGBA8 Color_Background = { 0, 0, 0, 255 };
static WidgetInfo backgroundColorWidget;
static WidgetInfo windowTypeWidget;

u8 Hint_GetIsHintDiscovered(u16 hintIndex) {
    u16 bitsPerIndex = sizeof(u32) * 8;
    u32 idx = hintIndex / bitsPerIndex;
    if (idx < RH_MAX) {
        u32 hintBit = 1 << (hintIndex - (idx * bitsPerIndex));
        return (gSaveContext.ship.stats.hintsDiscovered[idx] & hintBit) != 0;
    }
    return 0;
}

bool IsHintDiscovered(s16 index) {
    bool isDiscovered = Hint_GetIsHintDiscovered(index);
    return isDiscovered;
}

bool IsAreaScene(SceneID sceneNum) {
    switch (sceneNum) {
        case SCENE_HYRULE_FIELD:
        case SCENE_KAKARIKO_VILLAGE:
        case SCENE_GRAVEYARD:
        case SCENE_ZORAS_RIVER:
        case SCENE_KOKIRI_FOREST:
        case SCENE_SACRED_FOREST_MEADOW:
        case SCENE_LAKE_HYLIA:
        case SCENE_ZORAS_DOMAIN:
        case SCENE_ZORAS_FOUNTAIN:
        case SCENE_GERUDO_VALLEY:
        case SCENE_LOST_WOODS:
        case SCENE_DESERT_COLOSSUS:
        case SCENE_GERUDOS_FORTRESS:
        case SCENE_HAUNTED_WASTELAND:
        case SCENE_HYRULE_CASTLE:
        case SCENE_DEATH_MOUNTAIN_TRAIL:
        case SCENE_DEATH_MOUNTAIN_CRATER:
        case SCENE_GORON_CITY:
        case SCENE_LON_LON_RANCH:
        case SCENE_DEKU_TREE:
        case SCENE_DODONGOS_CAVERN:
        case SCENE_JABU_JABU:
        case SCENE_FOREST_TEMPLE:
        case SCENE_FIRE_TEMPLE:
        case SCENE_WATER_TEMPLE:
        case SCENE_SPIRIT_TEMPLE:
        case SCENE_SHADOW_TEMPLE:
        case SCENE_BOTTOM_OF_THE_WELL:
        case SCENE_ICE_CAVERN:
        case SCENE_GERUDO_TRAINING_GROUND:
        case SCENE_GANONS_TOWER:
        case SCENE_INSIDE_GANONS_CASTLE:
        case SCENE_BACK_ALLEY_DAY:
        case SCENE_BACK_ALLEY_NIGHT:
        case SCENE_MARKET_DAY:
        case SCENE_MARKET_NIGHT:
        case SCENE_MARKET_RUINS:
            return true;
        default:
            return false;
    }
}

struct HintAreaInfo {
    RandomizerCheckArea area;
    HintTypes type;
    RandomizerSettingKey settingKey;
};

std::unordered_map<RandomizerHint, HintAreaInfo> hintMetadata = {
    { RH_COLOSSUS_GOSSIP_STONE, { RCAREA_DESERT_COLOSSUS, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_DMC_GOSSIP_STONE, { RCAREA_DEATH_MOUNTAIN_CRATER, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_DMC_UPPER_GROTTO_GOSSIP_STONE, { RCAREA_DEATH_MOUNTAIN_CRATER, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_DMT_GOSSIP_STONE, { RCAREA_DEATH_MOUNTAIN_TRAIL, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_DMT_STORMS_GROTTO_GOSSIP_STONE, { RCAREA_DEATH_MOUNTAIN_TRAIL, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_DODONGOS_CAVERN_GOSSIP_STONE, { RCAREA_DODONGOS_CAVERN, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_ZF_FAIRY_GOSSIP_STONE, { RCAREA_ZORAS_FOUNTAIN, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_ZF_JABU_GOSSIP_STONE, { RCAREA_ZORAS_FOUNTAIN, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_GC_MAZE_GOSSIP_STONE, { RCAREA_GORON_CITY, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_GC_MEDIGORON_GOSSIP_STONE, { RCAREA_GORON_CITY, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_GV_GOSSIP_STONE, { RCAREA_GERUDO_VALLEY, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_GRAVEYARD_GOSSIP_STONE, { RCAREA_GRAVEYARD, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HC_MALON_GOSSIP_STONE, { RCAREA_HYRULE_CASTLE, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HC_ROCK_WALL_GOSSIP_STONE, { RCAREA_HYRULE_CASTLE, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HC_STORMS_GROTTO_GOSSIP_STONE, { RCAREA_HYRULE_CASTLE, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HF_COW_GROTTO_GOSSIP_STONE, { RCAREA_HYRULE_FIELD, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HF_NEAR_MARKET_GROTTO_GOSSIP_STONE, { RCAREA_HYRULE_FIELD, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HF_OPEN_GROTTO_GOSSIP_STONE, { RCAREA_HYRULE_FIELD, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_HF_SOUTHEAST_GROTTO_GOSSIP_STONE, { RCAREA_HYRULE_FIELD, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_KF_DEKU_TREE_LEFT_GOSSIP_STONE, { RCAREA_KOKIRI_FOREST, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_KF_DEKU_TREE_RIGHT_GOSSIP_STONE, { RCAREA_KOKIRI_FOREST, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_KF_GOSSIP_STONE, { RCAREA_KOKIRI_FOREST, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_KF_STORMS_GROTTO_GOSSIP_STONE, { RCAREA_KOKIRI_FOREST, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_KAK_OPEN_GROTTO_GOSSIP_STONE, { RCAREA_KAKARIKO_VILLAGE, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_LH_LAB_GOSSIP_STONE, { RCAREA_LAKE_HYLIA, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_LH_SOUTHEAST_GOSSIP_STONE, { RCAREA_LAKE_HYLIA, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_LH_SOUTHWEST_GOSSIP_STONE, { RCAREA_LAKE_HYLIA, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_LW_GOSSIP_STONE, { RCAREA_LOST_WOODS, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_LW_NEAR_SHORTCUTS_GROTTO_GOSSIP_STONE, { RCAREA_LOST_WOODS, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_SFM_MAZE_NEAR_LW_GOSSIP_STONE, { RCAREA_SACRED_FOREST_MEADOW, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_SFM_MAZE_CENTER_GOSSIP_STONE, { RCAREA_SACRED_FOREST_MEADOW, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_SFM_SARIA_GOSSIP_STONE, { RCAREA_SACRED_FOREST_MEADOW, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_TOT_LEFT_CENTER_GOSSIP_STONE, { RCAREA_MARKET, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_TOT_LEFTMOST_GOSSIP_STONE, { RCAREA_MARKET, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_TOT_RIGHT_CENTER_GOSSIP_STONE, { RCAREA_MARKET, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_TOT_RIGHTMOST_GOSSIP_STONE, { RCAREA_MARKET, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_ZD_GOSSIP_STONE, { RCAREA_ZORAS_DOMAIN, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_ZR_NEAR_DOMAIN_GOSSIP_STONE, { RCAREA_ZORAS_RIVER, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_ZR_NEAR_GROTTOS_GOSSIP_STONE, { RCAREA_ZORAS_RIVER, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_ZR_OPEN_GROTTO_GOSSIP_STONE, { RCAREA_ZORAS_RIVER, TYPE_GOSSIP_STONE, RSK_GOSSIP_STONE_HINTS } },
    { RH_FOREST_BOSS_KEY_HINT, { RCAREA_FOREST_TEMPLE, TYPE_BOSS_KEY, RSK_BOSS_KEY_HINT } },
    { RH_FIRE_BOSS_KEY_HINT, { RCAREA_FIRE_TEMPLE, TYPE_BOSS_KEY, RSK_BOSS_KEY_HINT } },
    { RH_WATER_BOSS_KEY_HINT, { RCAREA_WATER_TEMPLE, TYPE_BOSS_KEY, RSK_BOSS_KEY_HINT } },
    { RH_SPIRIT_BOSS_KEY_HINT, { RCAREA_SPIRIT_TEMPLE, TYPE_BOSS_KEY, RSK_BOSS_KEY_HINT } },
    { RH_SHADOW_BOSS_KEY_HINT, { RCAREA_SHADOW_TEMPLE, TYPE_BOSS_KEY, RSK_BOSS_KEY_HINT } },
    { RH_GANONS_BOSS_KEY_HINT, { RCAREA_GANONS_CASTLE, TYPE_BOSS_KEY, RSK_BOSS_KEY_HINT } },
    { RH_STORMS_HINT, { RCAREA_KAKARIKO_VILLAGE, TYPE_ITEM, RSK_STORMS_HINT } },
    { RH_EPONAS_HINT, { RCAREA_LON_LON_RANCH, TYPE_ITEM, RSK_EPONAS_HINT } },
    { RH_LULLABY_HINT, { RCAREA_HYRULE_CASTLE, TYPE_ITEM, RSK_LULLABY_HINT } },
    { RH_GREG_RUPEE, { RCAREA_MARKET, TYPE_ITEM, RSK_GREG_HINT } },
    { RH_DAMPES_DIARY, { RCAREA_GRAVEYARD, TYPE_ITEM, RSK_DAMPES_DIARY_HINT } },
    { RH_SCALE_HINT, { RCAREA_ZORAS_DOMAIN, TYPE_ITEM, RSK_SCALE_HINT } },
    { RH_SARIA_HINT, { RCAREA_SACRED_FOREST_MEADOW, TYPE_ITEM, RSK_SARIA_HINT } },
    { RH_MIDO_HINT, { RCAREA_KOKIRI_FOREST, TYPE_ITEM, RSK_MIDO_HINT } },
    { RH_STRENGTH_HINT, { RCAREA_DEATH_MOUNTAIN_TRAIL, TYPE_ITEM, RSK_STRENGTH_HINT } },
    { RH_BOOMERANG_HINT, { RCAREA_JABU_JABUS_BELLY, TYPE_ITEM, RSK_BOOMERANG_HINT } },
    { RH_HAMMER_HINT, { RCAREA_FIRE_TEMPLE, TYPE_ITEM, RSK_HAMMER_HINT } },
    { RH_MIRROR_HINT, { RCAREA_SPIRIT_TEMPLE, TYPE_ITEM, RSK_MIRROR_HINT } },
    { RH_HOVERS_HINT, { RCAREA_SHADOW_TEMPLE, TYPE_ITEM, RSK_HOVERS_HINT } },
    { RH_GANONDORF_HINT, { RCAREA_GANONS_CASTLE, TYPE_ITEM, RSK_GANONDORF_HINT } },
};

static constexpr std::array<RandomizerCheckArea, SPOILER_ENTRANCE_GROUP_COUNT> AreaFromEntranceGroup = {
    RCAREA_INVALID,          RCAREA_KOKIRI_FOREST, RCAREA_LOST_WOODS,           RCAREA_SACRED_FOREST_MEADOW,
    RCAREA_KAKARIKO_VILLAGE, RCAREA_GRAVEYARD,     RCAREA_DEATH_MOUNTAIN_TRAIL, RCAREA_DEATH_MOUNTAIN_CRATER,
    RCAREA_GORON_CITY,       RCAREA_ZORAS_RIVER,   RCAREA_ZORAS_DOMAIN,         RCAREA_ZORAS_FOUNTAIN,
    RCAREA_HYRULE_FIELD,     RCAREA_LON_LON_RANCH, RCAREA_LAKE_HYLIA,           RCAREA_GERUDO_VALLEY,
    RCAREA_GERUDO_FORTRESS,  RCAREA_WASTELAND,     RCAREA_DESERT_COLOSSUS,      RCAREA_MARKET,
    RCAREA_HYRULE_CASTLE,
};

RandomizerCheckArea GetCheckArea() {
    auto scene = SCENE_TESTROOM; // Default, invalid value
    if (gPlayState != nullptr) {
        scene = static_cast<SceneID>(gPlayState->sceneNum);
    }
    bool grottoScene = (scene == SCENE_GROTTOS || scene == SCENE_FAIRYS_FOUNTAIN);
    const EntranceData* ent = EntranceTracker::GetEntranceData(
        grottoScene ? ENTRANCE_GROTTO_EXIT_START + EntranceTracker::GetCurrentGrottoId() : gSaveContext.entranceIndex);
    RandomizerCheckArea area = RCAREA_INVALID;
    if (ent != nullptr && !IsAreaScene(scene) && ent->type != ENTRANCE_TYPE_DUNGEON) {
        if (ent->source == "Desert Colossus" || ent->destination == "Desert Colossus") {
            area = RCAREA_DESERT_COLOSSUS;
        } else {
            area = AreaFromEntranceGroup[ent->dstGroup];
        }
    }
    if (area == RCAREA_INVALID) {
        if (grottoScene && (EntranceTracker::GetCurrentGrottoId() == -1) &&
            (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_GROTTO_ENTRANCES) == RO_GENERIC_OFF)) {
            area = previousArea;
        } else {
            area = RandomizerCheckObjects::GetRCAreaBySceneID(scene);
        }
    }
    return area;
}

void InitHintTrackingData() {
    auto ctx = Rando::Context::GetInstance();
    CustomMessage cleanMsg;

    // Create a HintEntry for each hint whose gating setting is currently enabled.
    for (const auto& [hint, info] : hintMetadata) {
        if (!ctx->GetOption(info.settingKey)) {
            continue;
        }

        HintEntry e;
        e.hintKey = hint;
        e.name = Rando::StaticData::hintNames.at(hint).GetForCurrentLanguage(MF_CLEAN);
        e.areaName = RandomizerCheckObjects::GetRCAreaName(info.area);
        e.revealed = IsHintDiscovered(static_cast<s16>(hint));
        if (hint == RH_GANONDORF_HINT) {
            cleanMsg = OTRGlobals::Instance->gRandoContext->GetHint(hint)->GetHintMessage(MF_RAW, 2);
        } else {
            cleanMsg = OTRGlobals::Instance->gRandoContext->GetHint(hint)->GetHintMessage(MF_RAW);
        }
        std::string cleanTrackerMessage = cleanMsg.GetForCurrentLanguage(MF_ENCODE);
        e.hintText = cleanTrackerMessage;

        switch (info.type) {
            case TYPE_GOSSIP_STONE:
                e.hintType = "Gossip Stone";
                break;
            case TYPE_BOSS_KEY:
                e.hintType = "Boss Key";
                break;
            case TYPE_ITEM:
                e.hintType = "Item";
                break;
            default:
                e.hintType = "Unknown";
                break;
        }

        gHintKeyToIndex[hint] = gHintEntries.size();
        gHintEntries.push_back(std::move(e));
    }

    previousArea = currentArea;
    currentArea = GetCheckArea();

    if (CVarGetInteger(CVAR_TRACKER_HINTS("GroupBy"), 0) == 0 &&
        CVarGetInteger(CVAR_TRACKER_HINTS("AutoScroll"), 1) != 0) {
        doAreaScroll = true;
    } else {
        doAreaScroll = false;
    }
}

void RevealHint(RandomizerHint hintKey) {
    if (hintKey == RH_NONE) {
        SPDLOG_DEBUG("HintTracker: Received RH_NONE");
        return;
    }

    auto it = gHintKeyToIndex.find(hintKey);
    if (it != gHintKeyToIndex.end()) {
        size_t idx = it->second;
        if (!gHintEntries[idx].revealed) {
            gSaveContext.ship.stats.hintsDiscovered[hintKey / 32] |= (1 << (hintKey % 32));
            gHintEntries[idx].revealed = true;
        }
    } else {
        SPDLOG_DEBUG("hintKey not found in list!");
    }
}

const std::vector<HintEntry>& GetHintEntries() {
    return gHintEntries;
}

static const std::unordered_map<char, ImU32> hintColorCodeMap = {
    { 'w', IM_COL32_WHITE },
    { 'r', IM_COL32(255, 90, 90, 255) },
    { 'g', IM_COL32(90, 220, 90, 255) },
    { 'b', IM_COL32(110, 130, 255, 255) },
    { 'c', IM_COL32(120, 200, 255, 255) },
    { 'p', IM_COL32(230, 140, 230, 255) },
    { 'y', IM_COL32(240, 220, 80, 255) },
    { 'B', IM_COL32(120, 120, 120, 255) }, // black (lightened for legibility on dark backgrounds)
};

struct HintColorSegment {
    std::string text;
    ImU32 color;
};

// Splits `text` on "%<letter>" tokens (matching MF_RAW's output) into runs of text
// paired with the color that should be applied to them. Any text preceding the first
// token, or an unrecognized "%<letter>" sequence, uses `defaultColor`.
static std::vector<HintColorSegment> ParseHintColorSegments(const std::string& text, ImU32 defaultColor) {
    std::vector<HintColorSegment> segments;
    ImU32 currentColor = defaultColor;
    std::string current;

    for (size_t i = 0; i < text.size();) {
        if (text[i] == '%' && i + 1 < text.size() && hintColorCodeMap.contains(text[i + 1])) {
            if (!current.empty()) {
                segments.push_back({ current, currentColor });
                current.clear();
            }
            currentColor = hintColorCodeMap.at(text[i + 1]);
            i += 2;
            continue;
        }

        text.at(i) == '^' ? current += ' ' :
        current += text[i];
        i++;
    }

    if (!current.empty()) {
        segments.push_back({ current, currentColor });
    }

    return segments;
}

// Renders a series of colored text segments as a single word-wrapped paragraph.
// ImGui has no built-in way to mix colors within one wrapped Text call, so this
// manually measures and places each word, using zero-spacing SameLine() calls to
// keep same-line words glued together and wrapping to a new line when a word
// would overflow the available width.
//
// `fullLineWidth` is the width available to a line that starts at the left edge
// of the content region (captured by the caller before any text was drawn on the
// current line). The first line here may have less room than that if it continues
// from previously-drawn text via a preceding SameLine() call - that leftover space
// is measured internally via GetContentRegionAvail(). Every line after the first
// wrap uses the full `fullLineWidth` instead, since wrapped lines always restart
// at the left edge of the region rather than inheriting the first line's leftover
// space.
static void DrawHintColorSegments(const std::vector<HintColorSegment>& segments, float fullLineWidth) {
    float wrapWidth = ImGui::GetContentRegionAvail().x;
    float lineWidth = 0.0f;
    bool startOfLine = true;

    for (const auto& segment : segments) {
        size_t pos = 0;
        while (pos < segment.text.size()) {
            size_t nextSpace = segment.text.find(' ', pos);
            size_t wordEnd = (nextSpace == std::string::npos) ? segment.text.size() : nextSpace + 1;
            std::string word = segment.text.substr(pos, wordEnd - pos);
            pos = wordEnd;

            const float wordWidth = ImGui::CalcTextSize(word.c_str()).x;
            if (!startOfLine && (lineWidth + wordWidth) > wrapWidth) {
                startOfLine = true;
                lineWidth = 0.0f;
                wrapWidth = fullLineWidth;
            }
            if (!startOfLine) {
                ImGui::SameLine(0.0f, 0.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_Text, segment.color);
            ImGui::TextUnformatted(word.c_str());
            ImGui::PopStyleColor();

            lineWidth += wordWidth;
            startOfLine = false;
        }
    }
}

void HintTrackerSettingsWindow::DrawElement() {
    ImGui::TableNextColumn();
    SohGui::GetSohMenu()->MenuDrawItem(backgroundColorWidget, ImGui::GetContentRegionAvail().x, THEME_COLOR);

    SohGui::GetSohMenu()->MenuDrawItem(windowTypeWidget, ImGui::GetContentRegionAvail().x, THEME_COLOR);

    if (CVarGetInteger(CVAR_TRACKER_HINTS("WindowType"), TRACKER_WINDOW_WINDOW) == TRACKER_WINDOW_FLOATING) {
        CVarCheckbox("Enable Dragging", CVAR_TRACKER_HINTS("Draggable"), CheckboxOptions().Color(THEME_COLOR));
        CVarCheckbox("Only Enable While Paused", CVAR_TRACKER_HINTS("ShowOnlyPaused"),
                     CheckboxOptions().Color(THEME_COLOR));
        CVarCombobox("Display Mode", CVAR_TRACKER_HINTS("DisplayType"), showMode,
                     ComboboxOptions()
                         .LabelPosition(LabelPositions::Far)
                         .ComponentAlignment(ComponentAlignments::Right)
                         .Color(THEME_COLOR)
                         .DefaultIndex(0));
        if (CVarGetInteger(CVAR_TRACKER_HINTS("DisplayType"), TRACKER_DISPLAY_ALWAYS) == TRACKER_DISPLAY_COMBO_BUTTON) {
            CVarCombobox("Combo Button 1", CVAR_TRACKER_HINTS("ComboButton1"), buttonStrings,
                         ComboboxOptions()
                             .LabelPosition(LabelPositions::Far)
                             .ComponentAlignment(ComponentAlignments::Right)
                             .Color(THEME_COLOR)
                             .DefaultIndex(TRACKER_COMBO_BUTTON_L));
            CVarCombobox("Combo Button 2", CVAR_TRACKER_HINTS("ComboButton2"), buttonStrings,
                         ComboboxOptions()
                             .LabelPosition(LabelPositions::Far)
                             .ComponentAlignment(ComponentAlignments::Right)
                             .Color(THEME_COLOR)
                             .DefaultIndex(TRACKER_COMBO_BUTTON_L));
        }
    }

    if (ImGui::BeginTable("hintTrackerSubSettings", 1,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("column 1", ImGuiTableColumnFlags_WidthStretch, 150.0f);
        ImGui::TableNextColumn();

        ImGui::Text("List Items");
        CVarCheckbox("Auto scroll", CVAR_TRACKER_HINTS("AutoScroll"),
                     CheckboxOptions()
                         .Tooltip("Automatically scroll to the first available hint in the current scene")
                         .Color(THEME_COLOR));

        ImGui::Text("Group By");
        CVarRadioButton("Area", CVAR_TRACKER_HINTS("GroupBy"), 0,
                        RadioButtonsOptions().Color(THEME_COLOR));
        CVarRadioButton("Type", CVAR_TRACKER_HINTS("GroupBy"), 1,
                        RadioButtonsOptions().Color(THEME_COLOR));

        ImGui::EndTable();
    }
}

void HintTrackerWindow::Draw() {
    if (!IsVisible()) {
        return;
    }
    DrawElement();
    SyncVisibilityConsoleVariable();
}

void HintTrackerWindow::DrawElement() {
    Color_Background = CVarGetColor(CVAR_TRACKER_HINTS("BgColor.Value"), Color_Bg_Default);

    if (CVarGetInteger(CVAR_TRACKER_HINTS("WindowType"), TRACKER_WINDOW_WINDOW) == TRACKER_WINDOW_FLOATING) {
        if (CVarGetInteger(CVAR_TRACKER_HINTS("ShowOnlyPaused"), 0) &&
            (gPlayState == nullptr || gPlayState->pauseCtx.state == 0)) {
            return;
        }
    }

    if (Trackers::BeginFloatWindows(
            "Hint Tracker", mIsVisible, Color_Background,
            static_cast<TrackerWindowType>(CVarGetInteger(CVAR_TRACKER_HINTS("WindowType"), TRACKER_WINDOW_WINDOW)),
            CVarGetInteger(CVAR_TRACKER_HINTS("Draggable"), 1), ImGuiWindowFlags_NoScrollbar)) {

        if (!GameInteractor::IsSaveLoaded()) {
            ImGui::Text("Waiting for file load...");
            Trackers::EndFloatWindows();
            return;
        }

        UIWidgets::PushStyleCombobox(THEME_COLOR);
        if (hintSearch.Draw("##HintSearch", ImGui::GetContentRegionAvail().x - 42)) {}
        ImGui::SameLine();
        if (UIWidgets::Button(ICON_FA_ERASER, UIWidgets::ButtonOptions()
                                                  .Size(UIWidgets::Sizes::Inline)
                                                  .Color(THEME_COLOR)
                                                  .Padding(ImVec2(10.f, 6.f)))) {
            hintSearch.Clear();
            doAreaScroll = true;
        }
        if (hintSearch.InputBuf[0] == '\0') {
            ImGui::SameLine(20.0f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "Search...");
        }
        UIWidgets::PopStyleCombobox();

        ImGui::Separator();

        ImGui::BeginChild("HintList", ImVec2(0, -8));

        const int8_t groupByArea = CVarGetInteger(CVAR_TRACKER_HINTS("GroupBy"), 0);

        // Group entries by area
        std::map<std::string, std::vector<const HintEntry*>> grouped;
        if (groupByArea == 0) {
            for (const auto& s : gHintEntries) {
                std::string areaKey = s.areaName.empty() ? "Unknown Area" : s.areaName;
                grouped[areaKey].push_back(&s);
            }
        } else {
            for (const auto& s : gHintEntries) {
                std::string typeKey = s.hintType.empty() ? "Unknown Type" : s.hintType;
                grouped[typeKey].push_back(&s);
            }
        }

        for (auto& [groupName, vec] : grouped) {
            // Sort: revealed first, then by name
            std::sort(vec.begin(), vec.end(), [](const HintEntry* a, const HintEntry* b) {
                if (a->revealed != b->revealed) {
                    return a->revealed > b->revealed;
                }
                return a->name < b->name;
            });

            // Filter entries for search
            std::vector<const HintEntry*> displayEntries;

            for (const auto* sPtr : vec) {
                const HintEntry& s = *sPtr;

                std::string searchStr = s.name + " " + s.areaName;
                if (s.revealed) {
                    searchStr += " " + s.hintText;
                }

                // Check if entry matches search or is undiscovered
                if (hintSearch.PassFilter(searchStr.c_str())) {
                    displayEntries.push_back(sPtr);
                }
            }

            // Also match group name for search
            if (!hintSearch.PassFilter(groupName.c_str()) && displayEntries.empty()) {
                continue;
            }

            bool isOpen = ImGui::TreeNodeEx(groupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            if (groupByArea == 0) {
                std::string currentAreaName = RandomizerCheckObjects::GetRCAreaName(currentArea);
                if (doAreaScroll && groupName == currentAreaName) {
                    ImGui::SetScrollHereY(0.0f);
                    doAreaScroll = false;
                }
            }

            if (isOpen) {
                ImGui::Separator();

                // Display revealed entries first, then undiscovered
                for (const auto* sPtr : displayEntries) {
                    const HintEntry& s = *sPtr;

                    if (!s.revealed) {
                        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_GRAY);
                        ImGui::TextWrapped("%s\u00A0-> \"???\"", s.name.c_str());
                        ImGui::PopStyleColor();
                        continue;
                    }

                    // Capture the full line width now, while the cursor is still at the
                    // left edge of the content region (before the name/arrow prefix is
                    // drawn). This is what every wrapped line after the first will use.
                    const float fullLineWidth = ImGui::GetContentRegionAvail().x;

                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
                    ImGui::TextWrapped("%s\u00A0-> ", s.name.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0.0f, 0.0f);

                    DrawHintColorSegments(ParseHintColorSegments("\"" + s.hintText + "\"", IM_COL32_WHITE),
                                          fullLineWidth);
                }

                ImGui::Spacing();
                ImGui::TreePop();
            }
        }

        ImGui::EndChild();
    }
    Trackers::EndFloatWindows();
}

void ClearHintTrackingData() {
    gHintEntries.clear();
    gHintKeyToIndex.clear();
}

void HintTrackerWindow::InitElement() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) { InitHintTrackingData(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t fileNum) { ClearHintTrackingData(); });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnTransitionEnd>([](uint32_t) {
        previousArea = currentArea;
        currentArea = GetCheckArea();

        if (CVarGetInteger(CVAR_TRACKER_HINTS("GroupBy"), 0) == 0 &&
            CVarGetInteger(CVAR_TRACKER_HINTS("AutoScroll"), 1) != 0) {
            doAreaScroll = true;
        }
    });
}

void RegisterHintTrackerWidgets() {
    backgroundColorWidget = { .name = "Background Color##HintTracker", .type = WidgetType::WIDGET_CVAR_COLOR_PICKER };
    backgroundColorWidget.CVar(CVAR_TRACKER_HINTS("BgColor"))
        .Options(
            ColorPickerOptions().Color(THEME_COLOR).DefaultValue(Color_Bg_Default).UseAlpha().ShowReset().ShowRandom());
    SohGui::GetSohMenu()->AddSearchWidget({ backgroundColorWidget, "Randomizer", "Hint Tracker", "General Settings" });

    windowTypeWidget = { .name = "Window Type##HintTracker", .type = WidgetType::WIDGET_CVAR_COMBOBOX };
    windowTypeWidget.CVar(CVAR_TRACKER_HINTS("WindowType"))
        .Options(ComboboxOptions()
                     .DefaultIndex(TRACKER_WINDOW_WINDOW)
                     .ComponentAlignment(ComponentAlignments::Right)
                     .LabelPosition(LabelPositions::Far)
                     .Color(THEME_COLOR)
                     .ComboMap(windowType));
    SohGui::GetSohMenu()->AddSearchWidget({ windowTypeWidget, "Randomizer", "Hint Tracker", "General Settings" });
}

static RegisterShipInitFunc gInitFunc([]() { RegisterHintTrackerWidgets(); }, { "IS_RANDO" });

} // namespace HintTracker