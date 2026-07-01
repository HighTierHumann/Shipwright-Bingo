#include "SohMenu.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/OTRGlobals.h"
#include "soh/SohGui/SohGui.hpp"

extern "C" {
#include "variables.h"
}

namespace SohGui {

extern std::shared_ptr<SohMenu> mSohMenu;
using namespace UIWidgets;

static const std::map<int32_t, const char*> skipGetItemAnimationOptions = {
    { SGIA_DISABLED, "Disabled" },
    { SGIA_JUNK, "Junk Items" },
    { SGIA_ALL, "All Items" },
};

static bool locationsDirty = true;
static bool tricksDirty = true;
static int32_t prevMQDungeonSetting;
static char seedString[MAX_SEED_STRING_SIZE];
static std::set<RandomizerCheck> excludedLocations;
static std::set<RandomizerTrick> enabledTricks;
static std::set<RandomizerTrick> enabledGlitches;

void SaveEnabledTricks() {
    std::string enabledTrickString = "";
    for (auto enabledTrickIt : enabledTricks) {
        enabledTrickString += Rando::Settings::GetInstance()->GetTrickSetting(enabledTrickIt).GetNameTag();
        enabledTrickString += ",";
    }
    if (enabledTricks.size() == 0) {
        CVarClear(CVAR_RANDOMIZER_SETTING("EnabledTricks"));
    } else {
        CVarSetString(CVAR_RANDOMIZER_SETTING("EnabledTricks"), enabledTrickString.c_str());
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    tricksDirty = false;
    return;
}

void DrawLocationsMenu(WidgetInfo& info) {
    auto ctx = OTRGlobals::Instance->gRandoContext;
    int32_t currMQDungeonSetting = CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeons"), 0) |
                                   CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeonCount"), 0) << 8;
    static ImVec2 cellPadding(8.0f, 8.0f);
    bool generating = CVarGetInteger(CVAR_GENERAL("RandoGenerating"), 0);
    bool disableEditingRandoSettings = generating || CVarGetInteger(CVAR_GENERAL("OnFileSelectNameEntry"), 0);
    ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0) || disableEditingRandoSettings);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cellPadding);
    if (locationsDirty || currMQDungeonSetting != prevMQDungeonSetting || GameInteractor::IsSaveLoaded()) {
        locationsDirty = false;
        prevMQDungeonSetting = currMQDungeonSetting;
        UpdateMenuLocations();
    } else {
        RandomizerCheckObjects::UpdateImGuiVisibility();
    }

    int includedCount = 0;
    static ImGuiTextFilter locationSearch;
    for (auto& [rcArea, locations] : RandomizerCheckObjects::GetAllRCObjectsByArea()) {
        for (RandomizerCheck rc : locations) {
            if (ctx->GetItemLocation(rc)->IsVisible() && !excludedLocations.count(rc) &&
                locationSearch.PassFilter(Rando::StaticData::GetLocation(rc)->GetName().c_str())) {
                includedCount++;
            }
        }
    }

    if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonReward"), 0) == RO_DUNGEON_REWARDS_END_OF_DUNGEON ||
        CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonReward"), 0) == RO_DUNGEON_REWARDS_VANILLA) {
        ImGui::Text("Total Checks: %d", includedCount + 9); // Link's Pocket, 5 Medallions, 3 Stones
    } else {
        ImGui::Text("Total Checks: %d", includedCount);
    }

    if (ImGui::BeginTable("tableRandoLocations", 2, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Included", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Excluded", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::TableHeadersRow();
        ImGui::PopItemFlag();
        ImGui::TableNextRow();

        // COLUMN 1 - INCLUDED LOCATIONS
        ImGui::TableNextColumn();
        // window->DC.CurrLineTextBaseOffset = 0.0f;

        static ImGuiTextFilter locationSearch;
        UIWidgets::PushStyleInput(THEME_COLOR);
        locationSearch.Draw();
        UIWidgets::PopStyleInput();

        ImGui::BeginChild("ChildIncludedLocations", ImVec2(0, -8));
        for (auto& [rcArea, locations] : RandomizerCheckObjects::GetAllRCObjectsByArea()) {
            bool hasItems = false;
            for (RandomizerCheck rc : locations) {
                if (ctx->GetItemLocation(rc)->IsVisible() && !excludedLocations.count(rc) &&
                    locationSearch.PassFilter(Rando::StaticData::GetLocation(rc)->GetName().c_str())) {

                    hasItems = true;
                    break;
                }
            }

            if (hasItems) {
                int areaIncludedCount = 0;
                for (RandomizerCheck rc : locations) {
                    if (ctx->GetItemLocation(rc)->IsVisible() && !excludedLocations.count(rc) &&
                        locationSearch.PassFilter(Rando::StaticData::GetLocation(rc)->GetName().c_str())) {
                        areaIncludedCount++;
                    }
                }
                std::string areaLabel =
                    RandomizerCheckObjects::GetRCAreaName(rcArea) + " (" + std::to_string(areaIncludedCount) + ")";
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode(areaLabel.c_str())) {
                    for (auto& location : locations) {
                        if (ctx->GetItemLocation(location)->IsVisible() && !excludedLocations.count(location) &&
                            locationSearch.PassFilter(Rando::StaticData::GetLocation(location)->GetName().c_str())) {
                            UIWidgets::PushStyleButton(THEME_COLOR, ImVec2(7.f, 5.f));
                            if (ImGui::ArrowButton(std::to_string(location).c_str(), ImGuiDir_Right)) {
                                excludedLocations.insert(location);
                                // todo: this efficiently when we build out cvar array support
                                std::string excludedLocationString = "";
                                for (auto excludedLocationIt : excludedLocations) {
                                    if (!excludedLocationString.empty()) {
                                        excludedLocationString += ",";
                                    }
                                    excludedLocationString += std::to_string(excludedLocationIt);
                                }
                                CVarSetString(CVAR_RANDOMIZER_SETTING("ExcludedLocations"),
                                              excludedLocationString.c_str());
                                Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
                                locationsDirty = true;
                            }
                            UIWidgets::PopStyleButton();
                            ImGui::SameLine();
                            ImGui::Text("%s", Rando::StaticData::GetLocation(location)->GetShortName().c_str());
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();

        // COLUMN 2 - EXCLUDED LOCATIONS
        ImGui::TableNextColumn();
        // window->DC.CurrLineTextBaseOffset = 0.0f;

        ImGui::BeginChild("ChildExcludedLocations", ImVec2(0, -8));
        for (auto& [rcArea, locations] : RandomizerCheckObjects::GetAllRCObjectsByArea()) {
            bool hasItems = false;
            for (RandomizerCheck rc : locations) {
                if (ctx->GetItemLocation(rc)->IsVisible() && excludedLocations.count(rc)) {
                    hasItems = true;
                    break;
                }
            }

            if (hasItems) {
                int areaExcludedCount = 0;
                for (RandomizerCheck rc : locations) {
                    if (ctx->GetItemLocation(rc)->IsVisible() && excludedLocations.count(rc)) {
                        areaExcludedCount++;
                    }
                }
                std::string areaLabel =
                    RandomizerCheckObjects::GetRCAreaName(rcArea) + " (" + std::to_string(areaExcludedCount) + ")";
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if (ImGui::TreeNode(areaLabel.c_str())) {
                    for (auto& location : locations) {
                        auto elfound = excludedLocations.find(location);
                        if (ctx->GetItemLocation(location)->IsVisible() && elfound != excludedLocations.end()) {
                            UIWidgets::PushStyleButton(THEME_COLOR, ImVec2(7.f, 5.f));
                            if (ImGui::ArrowButton(std::to_string(location).c_str(), ImGuiDir_Left)) {
                                excludedLocations.erase(elfound);
                                // todo: this efficiently when we build out cvar array support
                                std::string excludedLocationString = "";
                                for (auto excludedLocationIt : excludedLocations) {
                                    if (!excludedLocationString.empty()) {
                                        excludedLocationString += ",";
                                    }
                                    excludedLocationString += std::to_string(excludedLocationIt);
                                }
                                if (excludedLocationString == "") {
                                    CVarClear(CVAR_RANDOMIZER_SETTING("ExcludedLocations"));
                                } else {
                                    CVarSetString(CVAR_RANDOMIZER_SETTING("ExcludedLocations"),
                                                  excludedLocationString.c_str());
                                }
                                Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
                                locationsDirty = true;
                            }
                            UIWidgets::PopStyleButton();
                            ImGui::SameLine();
                            ImGui::Text("%s", Rando::StaticData::GetLocation(location)->GetShortName().c_str());
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
    ImGui::PopStyleVar(1);
    // ImGui::EndTabItem();
    ImGui::EndDisabled();
}

void UpdateMenuLocations() {
    RandomizerCheckObjects::UpdateImGuiVisibility();
    // todo: this efficiently when we build out cvar array support
    std::stringstream excludedLocationStringStream(CVarGetString(CVAR_RANDOMIZER_SETTING("ExcludedLocations"), ""));
    std::string excludedLocationString;
    excludedLocations.clear();
    while (getline(excludedLocationStringStream, excludedLocationString, ',')) {
        if (!excludedLocationString.empty()) {
            excludedLocations.insert((RandomizerCheck)std::stoi(excludedLocationString));
        }
    }
}

void UpdateMenuTricks() {
    // RandomizerTricks::UpdateImGuiVisibility();
    //  todo: this efficiently when we build out cvar array support
    std::stringstream enabledTrickStringStream(CVarGetString(CVAR_RANDOMIZER_SETTING("EnabledTricks"), ""));
    std::string enabledTrickString;
    enabledTricks.clear();
    while (getline(enabledTrickStringStream, enabledTrickString, ',')) {
        if (Rando::StaticData::trickToEnum.contains(enabledTrickString)) {
            enabledTricks.insert(Rando::StaticData::trickToEnum[enabledTrickString]);
        }
    }
    std::stringstream enabledGlitchStringStream(CVarGetString(CVAR_RANDOMIZER_SETTING("EnabledGlitches"), ""));
    std::string enabledGlitchString;
    enabledGlitches.clear();
    while (getline(enabledGlitchStringStream, enabledGlitchString, ',')) {
        if (!enabledGlitchString.empty()) {
            enabledGlitches.insert((RandomizerTrick)std::stoi(enabledGlitchString));
        }
    }
}

void DrawTricksMenu(WidgetInfo& info) {
    auto ctx = OTRGlobals::Instance->gRandoContext;
    auto randoSettings = Rando::Settings::GetInstance();
    static ImVec2 cellPadding(8.0f, 8.0f);
    bool generating = CVarGetInteger(CVAR_GENERAL("RandoGenerating"), 0);
    bool disableEditingRandoSettings = generating || CVarGetInteger(CVAR_GENERAL("OnFileSelectNameEntry"), 0);
    if (tricksDirty) {
        tricksDirty = false;
        UpdateMenuTricks();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cellPadding);
    ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0) || disableEditingRandoSettings);

    // Tricks
    static std::map<RandomizerArea, bool> areaTreeDisabled{
        { RA_NONE, true },
        { RA_KOKIRI_FOREST, true },
        { RA_THE_LOST_WOODS, true },
        { RA_SACRED_FOREST_MEADOW, true },
        { RA_HYRULE_FIELD, true },
        { RA_LAKE_HYLIA, true },
        { RA_GERUDO_VALLEY, true },
        { RA_GERUDO_FORTRESS, true },
        { RA_HAUNTED_WASTELAND, true },
        { RA_DESERT_COLOSSUS, true },
        { RA_THE_MARKET, true },
        { RA_HYRULE_CASTLE, true },
        { RA_KAKARIKO_VILLAGE, true },
        { RA_THE_GRAVEYARD, true },
        { RA_DEATH_MOUNTAIN_TRAIL, true },
        { RA_GORON_CITY, true },
        { RA_DEATH_MOUNTAIN_CRATER, true },
        { RA_ZORAS_RIVER, true },
        { RA_ZORAS_DOMAIN, true },
        { RA_ZORAS_FOUNTAIN, true },
        { RA_LON_LON_RANCH, true },
        { RA_DEKU_TREE, true },
        { RA_DODONGOS_CAVERN, true },
        { RA_JABU_JABUS_BELLY, true },
        { RA_FOREST_TEMPLE, true },
        { RA_FIRE_TEMPLE, true },
        { RA_WATER_TEMPLE, true },
        { RA_SPIRIT_TEMPLE, true },
        { RA_SHADOW_TEMPLE, true },
        { RA_BOTTOM_OF_THE_WELL, true },
        { RA_ICE_CAVERN, true },
        { RA_GERUDO_TRAINING_GROUND, true },
        { RA_GANONS_CASTLE, true },
    };
    static std::map<RandomizerArea, bool> areaTreeEnabled{
        { RA_NONE, true },
        { RA_KOKIRI_FOREST, true },
        { RA_THE_LOST_WOODS, true },
        { RA_SACRED_FOREST_MEADOW, true },
        { RA_HYRULE_FIELD, true },
        { RA_LAKE_HYLIA, true },
        { RA_GERUDO_VALLEY, true },
        { RA_GERUDO_FORTRESS, true },
        { RA_HAUNTED_WASTELAND, true },
        { RA_DESERT_COLOSSUS, true },
        { RA_THE_MARKET, true },
        { RA_HYRULE_CASTLE, true },
        { RA_KAKARIKO_VILLAGE, true },
        { RA_THE_GRAVEYARD, true },
        { RA_DEATH_MOUNTAIN_TRAIL, true },
        { RA_GORON_CITY, true },
        { RA_DEATH_MOUNTAIN_CRATER, true },
        { RA_ZORAS_RIVER, true },
        { RA_ZORAS_DOMAIN, true },
        { RA_ZORAS_FOUNTAIN, true },
        { RA_LON_LON_RANCH, true },
        { RA_DEKU_TREE, true },
        { RA_DODONGOS_CAVERN, true },
        { RA_JABU_JABUS_BELLY, true },
        { RA_FOREST_TEMPLE, true },
        { RA_FIRE_TEMPLE, true },
        { RA_WATER_TEMPLE, true },
        { RA_SPIRIT_TEMPLE, true },
        { RA_SHADOW_TEMPLE, true },
        { RA_BOTTOM_OF_THE_WELL, true },
        { RA_ICE_CAVERN, true },
        { RA_GERUDO_TRAINING_GROUND, true },
        { RA_GANONS_CASTLE, true },
    };

    static std::map<Rando::Tricks::Tag, bool> showTag{
        { Rando::Tricks::Tag::NOVICE, true },   { Rando::Tricks::Tag::INTERMEDIATE, true },
        { Rando::Tricks::Tag::ADVANCED, true }, { Rando::Tricks::Tag::EXPERT, true },
        { Rando::Tricks::Tag::EXTREME, true },  { Rando::Tricks::Tag::EXPERIMENTAL, true },
        { Rando::Tricks::Tag::GLITCH, false },
    };

    static ImGuiTextFilter trickSearch;
    UIWidgets::PushStyleInput(THEME_COLOR);
    trickSearch.Draw("Filter (inc,-exc)", 490.0f);
    UIWidgets::PopStyleInput();
    if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LogicRules"), RO_LOGIC_GLITCHLESS) != RO_LOGIC_NO_LOGIC) {
        ImGui::SameLine();
        if (UIWidgets::Button("Disable All", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(250.f, 0.f)))) {
            for (int i = 0; i < RT_MAX; i++) {
                auto etfound = enabledTricks.find(static_cast<RandomizerTrick>(i));
                if (etfound != enabledTricks.end()) {
                    enabledTricks.erase(etfound);
                }
            }
            SaveEnabledTricks();
        }
        ImGui::SameLine();
        if (UIWidgets::Button("Enable All", UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(250.f, 0.f)))) {
            for (int i = 0; i < RT_MAX; i++) {
                if (!enabledTricks.count(static_cast<RandomizerTrick>(i))) {
                    enabledTricks.insert(static_cast<RandomizerTrick>(i));
                }
            }
            SaveEnabledTricks();
        }
    }
    if (ImGui::BeginTable("trickTags", static_cast<int>(showTag.size()),
                          ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders)) {
        for (auto [rtTag, isShown] : showTag) {
            ImGui::TableNextColumn();
            if (isShown) {
                ImGui::PushStyleColor(ImGuiCol_Text, Rando::Tricks::GetTextColor(rtTag));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 1.0f });
            }
            ImGui::PushStyleColor(ImGuiCol_Header, Rando::Tricks::GetTagColor(rtTag));
            ImGui::Selectable(Rando::Tricks::GetTagName(rtTag).c_str(), &showTag[rtTag]);
            ImGui::PopStyleColor(2);
        }
        ImGui::EndTable();
    }

    if (ImGui::BeginTable("tableRandoTricks", 2, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("Disabled Tricks", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Enabled Tricks", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::TableHeadersRow();
        ImGui::PopItemFlag();
        ImGui::TableNextRow();

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LogicRules"), RO_LOGIC_GLITCHLESS) != RO_LOGIC_NO_LOGIC) {
            // COLUMN 1 - DISABLED TRICKS
            ImGui::TableNextColumn();
            // window->DC.CurrLineTextBaseOffset = 0.0f;

            if (UIWidgets::Button("Collapse All##disabled",
                                  UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.f, 0.f)))) {
                for (int i = 0; i < RA_MAX; i++) {
                    areaTreeDisabled[static_cast<RandomizerArea>(i)] = false;
                }
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Open All##disabled",
                                  UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.f, 0.f)))) {
                for (int i = 0; i < RA_MAX; i++) {
                    areaTreeDisabled[static_cast<RandomizerArea>(i)] = true;
                }
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Enable Visible",
                                  UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.f, 0.f)))) {
                for (int i = 0; i < RT_MAX; i++) {
                    auto option = randoSettings->GetTrickSetting(static_cast<RandomizerTrick>(i));
                    if (!enabledTricks.count(static_cast<RandomizerTrick>(i)) &&
                        trickSearch.PassFilter(option.GetName().c_str()) && areaTreeDisabled[option.GetArea()] &&
                        Rando::Tricks::CheckTags(showTag, option.GetTags())) {
                        enabledTricks.insert(static_cast<RandomizerTrick>(i));
                    }
                }
                SaveEnabledTricks();
            }

            ImGui::BeginChild("ChildTricksDisabled", ImVec2(0, -8), false, ImGuiWindowFlags_HorizontalScrollbar);

            for (auto [area, trickIds] : randoSettings->mTricksByArea) {
                bool hasTricks = false;
                for (auto rt : trickIds) {
                    auto option = randoSettings->GetTrickSetting(rt);
                    if (!option.IsHidden() && trickSearch.PassFilter(option.GetName().c_str()) &&
                        !enabledTricks.count(rt) && Rando::Tricks::CheckTags(showTag, option.GetTags())) {
                        hasTricks = true;
                        break;
                    }
                }
                if (hasTricks) {
                    ImGui::TreeNodeSetOpen(ImGui::GetID((Rando::Tricks::GetAreaName(area) + "##disabled").c_str()),
                                           areaTreeDisabled[area]);
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                    if (ImGui::TreeNode((Rando::Tricks::GetAreaName(area) + "##disabled").c_str())) {
                        for (auto rt : trickIds) {
                            auto option = randoSettings->GetTrickSetting(rt);
                            if (!option.IsHidden() && trickSearch.PassFilter(option.GetName().c_str()) &&
                                !enabledTricks.count(rt) && Rando::Tricks::CheckTags(showTag, option.GetTags())) {
                                ImGui::TreeNodeSetOpen(
                                    ImGui::GetID((Rando::Tricks::GetAreaName(option.GetArea()) + "##disabled").c_str()),
                                    areaTreeDisabled[option.GetArea()]);
                                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                                UIWidgets::PushStyleButton(THEME_COLOR, ImVec2(7.f, 5.f));
                                if (ImGui::ArrowButton(std::to_string(rt).c_str(), ImGuiDir_Right)) {
                                    enabledTricks.insert(rt);
                                    SaveEnabledTricks();
                                }
                                UIWidgets::PopStyleButton();
                                Rando::Tricks::DrawTagChips(option.GetTags(), option.GetName());
                                ImGui::SameLine();
                                ImGui::Text("%s", option.GetName().c_str());
                                UIWidgets::Tooltip(option.GetDescription().c_str());
                            }
                        }
                        areaTreeDisabled[area] = true;
                        ImGui::TreePop();
                    } else {
                        areaTreeDisabled[area] = false;
                    }
                }
            }
            ImGui::EndChild();

            // COLUMN 2 - ENABLED TRICKS
            ImGui::TableNextColumn();
            // window->DC.CurrLineTextBaseOffset = 0.0f;

            if (UIWidgets::Button("Collapse All##enabled",
                                  UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.f, 0.f)))) {
                for (int i = 0; i < RA_MAX; i++) {
                    areaTreeEnabled[static_cast<RandomizerArea>(i)] = false;
                }
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Open All##enabled",
                                  UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.f, 0.f)))) {
                for (int i = 0; i < RA_MAX; i++) {
                    areaTreeEnabled[static_cast<RandomizerArea>(i)] = true;
                }
            }
            ImGui::SameLine();
            if (UIWidgets::Button("Disable Visible",
                                  UIWidgets::ButtonOptions().Color(THEME_COLOR).Size(ImVec2(0.f, 0.f)))) {
                for (int i = 0; i < RT_MAX; i++) {
                    auto option = randoSettings->GetTrickSetting(static_cast<RandomizerTrick>(i));
                    if (enabledTricks.count(static_cast<RandomizerTrick>(i)) &&
                        trickSearch.PassFilter(option.GetName().c_str()) && areaTreeEnabled[option.GetArea()] &&
                        Rando::Tricks::CheckTags(showTag, option.GetTags())) {
                        enabledTricks.erase(static_cast<RandomizerTrick>(i));
                    }
                }
                SaveEnabledTricks();
            }

            ImGui::BeginChild("ChildTricksEnabled", ImVec2(0, -8), false, ImGuiWindowFlags_HorizontalScrollbar);

            for (auto [area, trickIds] : randoSettings->mTricksByArea) {
                bool hasTricks = false;
                for (auto rt : trickIds) {
                    auto option = randoSettings->GetTrickSetting(rt);
                    if (!option.IsHidden() && trickSearch.PassFilter(option.GetName().c_str()) &&
                        enabledTricks.count(rt) && Rando::Tricks::CheckTags(showTag, option.GetTags())) {
                        hasTricks = true;
                        break;
                    }
                }
                if (hasTricks) {
                    ImGui::TreeNodeSetOpen(ImGui::GetID((Rando::Tricks::GetAreaName(area) + "##enabled").c_str()),
                                           areaTreeEnabled[area]);
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                    if (ImGui::TreeNode((Rando::Tricks::GetAreaName(area) + "##enabled").c_str())) {
                        for (auto rt : trickIds) {
                            auto option = randoSettings->GetTrickSetting(rt);
                            if (!option.IsHidden() && trickSearch.PassFilter(option.GetName().c_str()) &&
                                enabledTricks.count(rt) && Rando::Tricks::CheckTags(showTag, option.GetTags())) {
                                ImGui::TreeNodeSetOpen(
                                    ImGui::GetID((Rando::Tricks::GetAreaName(option.GetArea()) + "##enabled").c_str()),
                                    areaTreeEnabled[option.GetArea()]);
                                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                                UIWidgets::PushStyleButton(THEME_COLOR, ImVec2(7.f, 5.f));
                                if (ImGui::ArrowButton(std::to_string(rt).c_str(), ImGuiDir_Left)) {
                                    enabledTricks.erase(rt);
                                    SaveEnabledTricks();
                                }
                                UIWidgets::PopStyleButton();
                                Rando::Tricks::DrawTagChips(option.GetTags(), option.GetName());
                                ImGui::SameLine();
                                ImGui::Text("%s", option.GetName().c_str());
                                UIWidgets::Tooltip(option.GetDescription().c_str());
                            }
                        }
                        areaTreeEnabled[area] = true;
                        ImGui::TreePop();
                    } else {
                        areaTreeEnabled[area] = false;
                    }
                }
            }

            ImGui::EndChild();
        } else {
            ImGui::TableNextColumn();
            ImGui::BeginChild("ChildTricksDisabled", ImVec2(0, -8));
            ImGui::Text("Requires Logic Turned On.");
            ImGui::EndChild();
            ImGui::TableNextColumn();
            ImGui::BeginChild("ChildTricksEnabled", ImVec2(0, -8));
            ImGui::Text("Requires Logic Turned On.");
            ImGui::EndChild();
        }
        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    ImGui::PopStyleVar(1);
}

typedef enum {
    SKWIMBLE_NONE,
    SKWIMBLE_LIGHTLY_SALTED,
    SKWIMBLE_WITH_PEPPER,
    SKWIMBLE_SIDE_OF_FRIES,
    SKWIMBLE_OMEGA_DESTRUCTION_DEMONCORE,
    SKWIMBLE_CHEESY,
    SKWIMBLE_HEAVILY_CODE_BROWNED,
} SkwimbleSeasonings;

static const std::map<int32_t, const char*> skwimbleSeasoning = {
    { SKWIMBLE_NONE, "Unseasoned" }, { SKWIMBLE_LIGHTLY_SALTED, "Lightly Salted" }, { SKWIMBLE_WITH_PEPPER, "With Pepper" },
    { SKWIMBLE_SIDE_OF_FRIES, "A side of fries" },  { SKWIMBLE_OMEGA_DESTRUCTION_DEMONCORE, "Omega Destruction Demon Core" }, { SKWIMBLE_CHEESY, "Cheesy" },
    { SKWIMBLE_HEAVILY_CODE_BROWNED, "Heavily Code Browned" },
};

void SohMenu::AddMenuRandomizer() {
    // Add Randomizer Menu
    AddMenuEntry("Randomizer", CVAR_SETTING("Menu.RandomizerSidebarSection"));

    // Seed Settings
    WidgetPath path = { "Randomizer", "General", SECTION_COLUMN_1 };
    AddSidebarEntry("Randomizer", path.sidebarName, 2);
    AddWidget(path,
              "Be sure to explore the Presets and Enhancements Menus for various Speedups and Quality of life changes!",
              WIDGET_TEXT)
        .Options(TextOptions().Color(UIWidgets::Colors::Gray));
    AddWidget(path, "Seed Entry", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Manual seed entry", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"))
        .Options(CheckboxOptions().DefaultValue(true));
    AddWidget(path, "Seed", WIDGET_CUSTOM).CustomFunction([](WidgetInfo& info) {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"), 0)) {
            UIWidgets::PushStyleInput(THEME_COLOR);
            ImGui::InputText("##RandomizerSeed", seedString, MAX_SEED_STRING_SIZE,
                             ImGuiInputTextFlags_CallbackCharFilter, UIWidgets::TextFilters::FilterAlphaNum);
            UIWidgets::Tooltip("Characters from a-z, A-Z, and 0-9 are supported.\n"
                               "Character limit is 1023, after which the seed will be truncated.");
            ImGui::SameLine();
            if (UIWidgets::Button(
                    ICON_FA_RANDOM,
                    UIWidgets::ButtonOptions()
                        .Size(UIWidgets::Sizes::Inline)
                        .Color(THEME_COLOR)
                        .Padding(ImVec2(10.f, 6.f))
                        .Tooltip("Creates a new random seed value to be used when generating a randomizer"))) {
                SohUtils::CopyStringToCharArray(seedString, std::to_string(rand() & 0xFFFFFFFF), MAX_SEED_STRING_SIZE);
            }
            ImGui::SameLine();
            if (UIWidgets::Button(ICON_FA_ERASER, UIWidgets::ButtonOptions()
                                                      .Size(UIWidgets::Sizes::Inline)
                                                      .Color(THEME_COLOR)
                                                      .Padding(ImVec2(10.f, 6.f)))) {
                memset(seedString, 0, MAX_SEED_STRING_SIZE);
            }
            if (strnlen(seedString, MAX_SEED_STRING_SIZE) == 0) {
                ImGui::SameLine(17.0f);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "Leave blank for random seed");
            }
            UIWidgets::PopStyleInput();
        }
    });
    AddWidget(path, "Generate Randomizer", WIDGET_BUTTON)
        .Callback([](WidgetInfo& info) {
            OTRGlobals::Instance->gRandoContext->SetSpoilerLoaded(false);
            GenerateRandomizer(CVarGetInteger(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"), 0) ? seedString : "");
        })
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled = (gSaveContext.gameMode != GAMEMODE_FILE_SELECT) || GameInteractor::IsSaveLoaded();
        })
        .Options(ButtonOptions()
                     .Size(ImVec2(250.f, 0.f))
                     .DisabledTooltip("Must be on File Select to generate a randomizer seed."));
    AddWidget(path, "Randomize All Settings", WIDGET_BUTTON)
        .Callback([](WidgetInfo& info) { Rando::Settings::GetInstance()->RandomizeAllSettings(); })
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled = CVarGetInteger(CVAR_GENERAL("RandoGenerating"), 0) ||
                                     CVarGetInteger(CVAR_GENERAL("OnFileSelectNameEntry"), 0);
        })
        .Options(ButtonOptions()
                     .Size(ImVec2(250.f, 0.f))
                     .Tooltip("Randomizes all randomizer settings to random valid values (excludes tricks)."))
        .SameLine(true);
    AddWidget(path, "Spoiler File", WIDGET_CUSTOM).CustomFunction([](WidgetInfo& info) {
        JoinRandoGenerationThread();
        if (!CVarGetInteger(CVAR_RANDOMIZER_SETTING("DontGenerateSpoiler"), 0)) {
            std::string spoilerfilepath = CVarGetString(CVAR_GENERAL("SpoilerLog"), "");
            ImGui::Text("Spoiler File: %s", spoilerfilepath.c_str());
        }
    });

    // Enhancements
    AddWidget(path, "Enhancements", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "These enhancements are only useful in the Randomizer mode but do not affect the randomizer logic.",
              WIDGET_TEXT)
        .Options(TextOptions().Color(UIWidgets::Colors::Gray));
    AddWidget(path, "Rando-Relevant Navi Hints", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("RandoRelevantNavi"))
        .Options(CheckboxOptions()
                     .Tooltip("Replace Navi's overworld quest hints with rando-related gameplay hints.")
                     .DefaultValue(true));
    AddWidget(path, "Random Rupee Names", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"))
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("When obtaining Rupees, randomize what the Rupee is called in the textbox.")
                     .DefaultValue(true));
    AddWidget(path, "Use Custom Key Models", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("CustomKeyModels"))
        .Options(
            CheckboxOptions()
                .Tooltip("Use Custom graphics for Dungeon Keys, Big and Small, so that they can be easily told apart.")
                .DefaultValue(true));
    AddWidget(path, "Map & Compass Colors Match Dungeon", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("ColoredMapsAndCompasses"))
        .Options(
            CheckboxOptions()
                .Tooltip("Matches the color of maps & compasses to the dungeon they belong to. "
                         "This helps identify maps & compasses from afar and adds a little bit of flair.\n\nThis only "
                         "applies to seeds with maps & compasses shuffled to \"Any Dungeon\", \"Overworld\", or "
                         "\"Anywhere\".")
                .DefaultValue(true));
    AddWidget(path, "Jabber Nut Colors Match Kind", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("GenericJabberNutModel"))
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("With Shuffle Speak, jabber nut model & color will be generic.")
                     .DefaultValue(true));
    AddWidget(path, "Quest Item Fanfares", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("QuestItemFanfares"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Play unique fanfares when obtaining quest items (medallions/stones/songs). Note that these "
            "fanfares can be longer than usual."));
    AddWidget(path, "Mysterious Shuffled Items", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("MysteriousShuffle"))
        .Options(CheckboxOptions().Tooltip(
            "Displays a \"Mystery Item\" model in place of any freestanding/GS/shop items that were shuffled, "
            "and replaces item names for them and scrubs and merchants, regardless of hint settings, "
            "so you never know what you're getting."));
    AddWidget(path, "Simpler Boss Soul Models", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("SimplerBossSoulModels"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "When shuffling boss souls, they'll appear as a simpler model instead of showing the boss' models."
            "This might make boss souls more distinguishable from a distance, and can help with performance."));
    AddWidget(path, "Skip Get Item Animations", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimation"))
        .Options(ComboboxOptions().ComboMap(skipGetItemAnimationOptions).DefaultIndex(SGIA_JUNK));
    AddWidget(path, "Item Scale: %.2f", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimationScale"))
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled =
                !CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimation"), SGIA_JUNK);
            info.options->disabledTooltip =
                "This slider only applies when using the \"Skip Get Item Animations\" option.";
        })
        .Options(FloatSliderOptions().Min(5.0f).Max(15.0f).Format("%.2f").DefaultValue(10.0f).Tooltip(
            "The size of the item when it is picked up."));
    WidgetPath column2 = { "Randomizer", "General", SECTION_COLUMN_2 };
    AddWidget(column2, "Sign Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(column2, "Signs Hint Entrances", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("EntrancesOnSigns"))
        .Options(CheckboxOptions().Tooltip("If enabled, signs near loading zones will tell you where they lead to."));
    AddWidget(column2, "Inverted Sign Entrance Hints", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"))
        .Options(CheckboxOptions().Tooltip("If enabled, signs near dungeons will tell you the location of the dungeon "
                                           "they're associated with instead. (e.g. the DMT sign outside of Dodongo's "
                                           "Cavern will instead reveal the location of Dodongo's Cavern) \n\n'Signs Hint Entrances' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool entrancesOnSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("EntrancesOnSigns"), 0);
            info.options->disabled = !entrancesOnSigns;
            if (!entrancesOnSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0);
            }
        });
    AddWidget(column2, "Extra Dungeon Entrance Signs", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"))
        .Options(CheckboxOptions().Tooltip("Adds signs outside of every dungeon, and changes the Rusty Switch sign in Lake Hylia to hint what dungeon is in Water Temple. \n\n'Signs Hint Entrances' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool entrancesOnSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("EntrancesOnSigns"), 0);
            info.options->disabled = !entrancesOnSigns;
            if (!entrancesOnSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0); // A little sloppy forcing these to 0, but it prevents crashes when reading the Lake Hylia sign in particular
            }
        });

    AddWidget(column2, "Deku Tree Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("DekuTreeSign"))
        .Options(CheckboxOptions().Tooltip(
            "Adds a sign outside of Deku Tree"
            "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("DekuTreeSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Deku Tree Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedDekuTreeSign"))
        .Options(CheckboxOptions().Tooltip("Inverts the sign outside of Deku Tree"
                                           "\n\n'Inverted Sign Entrance Hints' AND 'Deku Tree Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("DekuTreeSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedDekuTreeSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Dodongo's Cavern Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedDodongosSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Dodongo's Cavern"
            "\n\n'Inverted Sign Entrance Hints' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0);
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedDodongosSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Jabu Jabu Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedJabuJabuSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Jabu Jabu"
            "\n\n'Inverted Sign Entrance Hints' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0);
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedJabuJabuSign"), 0);
            }
        });

    AddWidget(column2, "Forest Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("ForestTempleSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Forest Temple"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ForestTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Forest Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedForestTempleSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Forest Temple"
            "\n\n'Inverted Sign Entrance Hints' AND 'Forest Temple Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ForestTempleSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedForestTempleSign"), 0);
            }
        });

    AddWidget(column2, "Fire Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("FireTempleSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Fire Temple"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("FireTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Fire Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedFireTempleSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Fire Temple"
            "\n\n'Inverted Sign Entrance Hints' AND 'Fire Temple Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("FireTempleSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedFireTempleSign"), 0);
            }
        });

    AddWidget(column2, "Ice Cavern Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("IceCavernSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Ice Cavern"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("IceCavernSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Ice Cavern Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedIceCavernSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Ice Cavern"
            "\n\n'Inverted Sign Entrance Hints' AND 'Ice Cavern Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("IceCavernSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedIceCavernSign"), 0);
            }
        });

    AddWidget(column2, "Water Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("WaterTempleSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Water Temple"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("WaterTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Water Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedWaterTempleSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Water Temple"
            "\n\n'Inverted Sign Entrance Hints' AND 'Water Temple Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("WaterTempleSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedWaterTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted GTG Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedGTGSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of GTG"
            "\n\n'Inverted Sign Entrance Hints' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0);
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedGTGSign"), 0);
            }
        });

    AddWidget(column2, "Spirit Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("SpiritTempleSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Spirit Temple"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("SpiritTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Spirit Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedSpiritTempleSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Spirit Temple"
            "\n\n'Inverted Sign Entrance Hints' AND 'Spirit Temple Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("SpiritTempleSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedSpiritTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Bottom of the Well Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedBotWSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Bottom of the Well"
            "\n\n'Inverted Sign Entrance Hints' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0);
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedBotWSign"), 0);
            }
        });

    AddWidget(column2, "Shadow Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("ShadowTempleSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Shadow Temple"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ShadowTempleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Shadow Temple Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedShadowTempleSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Shadow Temple"
            "\n\n'Inverted Sign Entrance Hints' AND 'Shadow Temple Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ShadowTempleSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedShadowTempleSign"), 0);
            }
        });

    AddWidget(column2, "Ganon's Castle Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("GanonsCastleSign"))
        .Options(CheckboxOptions().Tooltip("Adds a sign outside of Ganon's Castle"
                                           "\n\n'Extra Dungeon Entrance Signs' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool extraSigns = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("ExtraDungeonSigns"), 0);
            info.isHidden = !extraSigns;
            if (!extraSigns) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("GanonsCastleSign"), 0);
            }
        });

    AddWidget(column2, "Inverted Ganon's Castle Sign", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("InvertedGanonsCastleSign"))
        .Options(CheckboxOptions().Tooltip(
            "Inverts the sign outside of Shadow Temple"
            "\n\n'Inverted Sign Entrance Hints' AND 'Ganons Castle Sign' MUST be Enabled first."))
        .PreFunc([](WidgetInfo& info) {
            bool shouldEnable = false;
            if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("GanonsCastleSign"), 0) &&
                CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertEntranceSigns"), 0)) {
                shouldEnable = true;
            }
            info.isHidden = !shouldEnable;
            if (!shouldEnable) {
                CVarSetInteger(CVAR_RANDOMIZER_ENHANCEMENT("InvertedGanonsCastleSign"), 0);
            }
        });

    auto randoSettings = Rando::Settings::GetInstance();
    randoSettings->CreateOptions();
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_LOGIC_ACCESS).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_DUNGEONS).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_SHUFFLES).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_HINTS_TRAPS).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_STARTING_ITEMS).AddWidgets(path);
    path.sidebarName = "Locations";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);
    AddWidget(path, "Excluded Locations", WIDGET_CUSTOM).CustomFunction(DrawLocationsMenu);
    path.sidebarName = "Tricks/Glitches";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);
    AddWidget(path, "Tricks/Glitches", WIDGET_CUSTOM).CustomFunction(DrawTricksMenu);

    // Plandomizer
    path.sidebarName = "Plandomizer";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);
    AddWidget(path, "Popout Plandomizer Window", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("PlandomizerEditor"))
        .RaceDisable(false)
        .WindowName("Plandomizer Editor")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Randomizer Settings Window."));

    // Item Tracker
    path.sidebarName = "Item Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Item Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Item Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ItemTracker"))
        .RaceDisable(false)
        .WindowName("Item Tracker")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Item Tracker.").EmbedWindow(false));

    AddWidget(path, "Item Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Item Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ItemTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Item Tracker Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Item Tracker Settings Window."));

    // Entrance Tracker
    path.sidebarName = "Entrance Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Entrance Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Entrance Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("EntranceTracker"))
        .RaceDisable(false)
        .WindowName("Entrance Tracker")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Entrance Tracker.").EmbedWindow(false));

    AddWidget(path, "Entrance Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Entrance Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("EntranceTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Entrance Tracker Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Entrance Tracker Settings Window."));

    // Check Tracker
    path.sidebarName = "Check Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Check Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Check Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("CheckTracker"))
        .RaceDisable(false)
        .WindowName("Check Tracker")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Check Tracker.").EmbedWindow(false));

    AddWidget(path, "Check Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Check Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("CheckTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Check Tracker Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Check Tracker Settings Window."));

    // Hint Tracker
    path.sidebarName = "Hint Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Hint Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Hint Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("HintTracker"))
        .RaceDisable(false)
        .WindowName("Hint Tracker")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Toggles the Hint Tracker.").EmbedWindow(false));
    AddWidget(path, "Hint Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Hint Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("HintTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Hint Tracker Settings")
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Enables the separate Hint Tracker Settings Window."));
}

} // namespace SohGui
