// seedgen: runs the game's randomizer generation without the game.
// Exit codes: 0 = wrote the spoiler, 1 = fill failed (retry with another seed), 2 = bad input or setup.

#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ship/Context.h>

#include "soh/OTRGlobals.h"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/Lang/Lang.h"
#include "soh/Enhancements/randomizer/3drando/menu.hpp"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/randomizer_check_objects.h"
#include "soh/Enhancements/randomizer/settings.h"
#include "soh/Enhancements/randomizer/static_data.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

// en_US.json, embedded in the binary.
__asm__(".section .rodata\n"
        ".global seedgenLangStart\n"
        "seedgenLangStart:\n"
        ".incbin \"" SEEDGEN_LANG_FILE "\"\n"
        ".global seedgenLangEnd\n"
        "seedgenLangEnd:\n"
        ".byte 0\n"
        ".text\n");
extern "C" const char seedgenLangStart[];
extern "C" const char seedgenLangEnd[];

namespace {
std::map<std::string, int32_t> sIntCVars;
std::map<std::string, std::string> sStringCVars;
std::string sWorkDir = ".";
json sLang;
bool sHasMasterQuest = false;

void LoadCVars(const std::string& prefix, const json& node) {
    for (const auto& item : node.items()) {
        const std::string name = prefix.empty() ? item.key() : prefix + "." + item.key();
        const auto& value = item.value();
        if (value.is_object()) {
            LoadCVars(name, value);
        } else if (value.is_string()) {
            sStringCVars[name] = value.get<std::string>();
        } else if (value.is_boolean()) {
            sIntCVars[name] = value.get<bool>();
        } else if (value.is_number_integer()) {
            sIntCVars[name] = value.get<int32_t>();
        }
    }
}

json ReadJsonFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("can't read " + path);
    }
    return json::parse(file);
}

std::string RemoveLineBreaks(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
    return text;
}

// Test mode: recreate the settings a spoiler log was generated with. See README.md.
void LoadSettingsFromSpoiler(const json& spoiler) {
    auto* settings = Rando::Settings::GetInstance().get();
    for (const auto& option : settings->GetAllOptions()) {
        if (option.GetName().empty() || option.GetCVarName().empty() ||
            !spoiler["settings"].contains(option.GetName())) {
            continue;
        }
        const auto wanted = spoiler["settings"][option.GetName()].get<std::string>();
        bool found = false;
        for (size_t i = 0; i < option.GetOptionCount() && !found; i++) {
            if (option.GetOptionText(i) == wanted) {
                sIntCVars[option.GetCVarName()] = static_cast<int32_t>(i);
                found = true;
            }
        }
        if (!found) {
            throw std::runtime_error("spoiler option has an unknown value: " + option.GetName() + " = " + wanted);
        }
    }

    std::set<std::string> excludedNames;
    for (const auto& name : spoiler.value("excludedLocations", json::array())) {
        excludedNames.insert(name.get<std::string>());
    }
    std::string excluded;
    for (const auto& area : settings->GetExcludeLocationsOptions()) {
        for (const auto* option : area) {
            if (excludedNames.contains(RemoveLineBreaks(option->GetName()))) {
                excluded += std::to_string(static_cast<int>(option->GetKey())) + ",";
            }
        }
    }
    sStringCVars[CVAR_RANDOMIZER_SETTING("ExcludedLocations")] = excluded;

    std::set<std::string> trickNames;
    for (const auto& name : spoiler.value("enabledTricks", json::array())) {
        trickNames.insert(name.get<std::string>());
    }
    std::string tricks;
    for (auto* option : settings->GetOptionGroup(RSG_TRICKS).GetOptions()) {
        if (trickNames.contains(RemoveLineBreaks(option->GetName()))) {
            tricks += static_cast<Rando::TrickSetting*>(option)->GetNameTag() + ",";
        }
    }
    sStringCVars[CVAR_RANDOMIZER_SETTING("EnabledTricks")] = tricks;
}

// A location file that's in the game but not registered here would silently shrink the pool.
void CheckLocationTable() {
    // Checks in the enum that the game itself never gives a location.
    static const std::set<RandomizerCheck> sNoLocation = {
        RC_HIDEOUT_JAIL_GUARD_1_TORCH,
        RC_HIDEOUT_JAIL_GUARD_2_TORCHES,
        RC_HIDEOUT_JAIL_GUARD_3_TORCHES,
        RC_HIDEOUT_JAIL_GUARD_4_TORCHES,
        RC_HIDEOUT_GERUDO_MEMBERSHIP_CARD,
        RC_PIERRE,
        RC_DELIVER_RUTOS_LETTER,
        RC_POWER_TRIFORCE_COMPLETED,
        RC_TREASURE_GAME_HEART_HINT,
        RC_OCARINA_HINT,
        RC_SONG_OF_TIME_HINT,
        RC_FISHING_SIGN_HINT,
    };
    int missing = 0;
    auto& table = Rando::StaticData::GetLocationTable();
    for (size_t i = 1; i < table.size(); i++) {
        const auto check = static_cast<RandomizerCheck>(i);
        if (table[i].GetRandomizerCheck() != check && !sNoLocation.contains(check)) {
            missing++;
            std::cerr << "seedgen: location table has no entry for check " << i << "\n";
        }
    }
    if (missing != 0) {
        throw std::runtime_error("location table is incomplete, a location registrar is missing from seedgen");
    }
}

// Same order as OTRGlobals::Initialize, then menu setup, then the end of InitOTR.
void InitRandomizer() {
    OTRGlobals::Instance = new OTRGlobals();
    auto* globals = OTRGlobals::Instance;
    globals->gRandoContext->InitStaticData();
    globals->gRandoContext = Rando::Context::CreateInstance();
    Rando::Settings::GetInstance()->AssignContext(globals->gRandoContext);
    Rando::StaticData::InitItemTable();
    globals->gRandomizer = std::make_shared<Randomizer>();
    Rando::Settings::GetInstance()->CreateOptions();

    // The game runs these through ShipInit::InitAll(). The rest of that list is gameplay hooks.
    using StaticData = Rando::StaticData;
    StaticData::RegisterSongLocations();
    StaticData::RegisterBeehiveLocations();
    StaticData::RegisterCowLocations();
    StaticData::RegisterFishLocations();
    StaticData::RegisterFairyLocations();
    StaticData::RegisterPotLocations();
    StaticData::RegisterFreestandingLocations();
    StaticData::RegisterGrassLocations();
    StaticData::RegisterCrateLocations();
    StaticData::RegisterRockLocations();
    StaticData::RegisterTreeLocations();
    StaticData::RegisterSignLocations();
    StaticData::RegisterWonderItemLocations();
    StaticData::RegisterBeggarLocations();
    StaticData::RegisterIcicleLocations();
    StaticData::RegisterRedIceLocations();
    StaticData::RegisterSilverRupeeLocations();
    CheckLocationTable();

    StaticData::InitHashMaps();
    globals->gRandoContext->AddExcludedOptions();
}

// Same steps as GenerateRandomizerImgui in randomizer.cpp.
bool Generate(const std::string& seed) {
    auto ctx = Rando::Context::GetInstance();
    Rando::Settings::GetInstance()->SetAllToContext();

    std::set<RandomizerCheck> excludedLocations;
    std::stringstream excludedStream(CVarGetString(CVAR_RANDOMIZER_SETTING("ExcludedLocations"), ""));
    std::string excluded;
    while (std::getline(excludedStream, excluded, ',')) {
        excludedLocations.insert(static_cast<RandomizerCheck>(std::stoi(excluded)));
    }

    std::set<RandomizerTrick> enabledTricks;
    std::stringstream trickStream(CVarGetString(CVAR_RANDOMIZER_SETTING("EnabledTricks"), ""));
    std::string trick;
    while (std::getline(trickStream, trick, ',')) {
        if (Rando::StaticData::trickToEnum.contains(trick)) {
            enabledTricks.insert(Rando::StaticData::trickToEnum[trick]);
        }
    }

    RandomizerCheckObjects::UpdateImGuiVisibility();
    for (auto& location : Rando::StaticData::GetLocationTable()) {
        auto found = excludedLocations.find(location.GetRandomizerCheck());
        if (!ctx->GetItemLocation(location.GetRandomizerCheck())->IsVisible() && found != excludedLocations.end()) {
            excludedLocations.erase(found);
        }
    }

    return GenerateRandomizer(excludedLocations, enabledTricks, seed);
}

int Usage() {
    std::cerr << "usage: seedgen --preset PRESET.json --seed SEED --out OUT.json [--mq] [--verbose]\n"
                 "       seedgen --from-spoiler SPOILER.json --out OUT.json [--seed SEED] [--mq] [--verbose]\n";
    return 2;
}
} // namespace

// CVars, backed by the preset.
extern "C" int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    auto found = sIntCVars.find(name);
    return found == sIntCVars.end() ? defaultValue : found->second;
}
extern "C" void CVarSetInteger(const char* name, int32_t value) {
    sIntCVars[name] = value;
}
extern "C" const char* CVarGetString(const char* name, const char* defaultValue) {
    auto found = sStringCVars.find(name);
    return found == sStringCVars.end() ? defaultValue : found->second.c_str();
}
extern "C" void CVarSetString(const char* name, const char* value) {
    sStringCVars[name] = value;
}

// Same lookup as the game's version, without loading from an o2r archive.
std::string Lang::Translate(const char* path) {
    const json* node = &sLang;
    std::stringstream segments(path);
    std::string segment;
    while (std::getline(segments, segment, '.')) {
        if (!node->contains(segment)) {
            return path;
        }
        node = &(*node)[segment];
    }
    if (node->is_string()) {
        return node->get<std::string>();
    }
    if (node->is_array()) {
        std::string text;
        for (const auto& part : *node) {
            if (!part.is_string()) {
                return path;
            }
            text += part.get<std::string>();
        }
        return text;
    }
    return path;
}

// spoiler_log.cpp writes under the "app directory", which is the temp folder here.
std::string Ship::Context::GetPathRelativeToAppDirectory(const std::string& path, const std::string&) {
    return sWorkDir + "/" + path;
}

OTRGlobals* OTRGlobals::Instance = nullptr;
OTRGlobals::OTRGlobals() {
    hasOriginal = true;
    hasMasterQuest = sHasMasterQuest;
}
OTRGlobals::~OTRGlobals() {
}
bool OTRGlobals::HasMasterQuest() {
    return hasMasterQuest;
}
bool OTRGlobals::HasOriginal() {
    return hasOriginal;
}

Randomizer::Randomizer() {
    Rando::StaticData::InitItemTable();
    Rando::StaticData::InitLocationTable();
}
Randomizer::~Randomizer() {
}
u8 Randomizer::GetRandoSettingValue(RandomizerSettingKey key) {
    return Rando::Context::GetInstance()->GetOption(key).Get();
}

int main(int argc, char** argv) {
    std::string presetPath, spoilerPath, seed, outPath;
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--mq") {
            sHasMasterQuest = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (i + 1 == argc) {
            return Usage();
        } else if (arg == "--preset") {
            presetPath = argv[++i];
        } else if (arg == "--from-spoiler") {
            spoilerPath = argv[++i];
        } else if (arg == "--seed") {
            seed = argv[++i];
        } else if (arg == "--out") {
            outPath = argv[++i];
        } else {
            return Usage();
        }
    }
    if (presetPath.empty() == spoilerPath.empty() || outPath.empty() || (spoilerPath.empty() && seed.empty())) {
        return Usage();
    }
    // GenerateRandomizer treats this prefix as a request to stress test and writes no spoiler.
    if (seed.rfind("seed_testing_count", 0) == 0) {
        std::cerr << "seedgen: invalid seed\n";
        return 2;
    }

    spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::err);

    // Written beside the output and renamed at the end, so readers never see a partial file.
    sWorkDir = outPath + ".tmp." + std::to_string(getpid());
    int result = 2;
    try {
        sLang = json::parse(seedgenLangStart, seedgenLangEnd);
        const json input = ReadJsonFile(presetPath.empty() ? spoilerPath : presetPath);
        fs::create_directories(sWorkDir);

        InitRandomizer();
        if (!presetPath.empty()) {
            LoadCVars("", input.at("blocks").at("rando"));
        } else {
            LoadSettingsFromSpoiler(input);
            if (seed.empty()) {
                seed = input.at("seed").get<std::string>();
            }
        }
        Rando::Settings::GetInstance()->UpdateAllOptions();

        if (Generate(seed)) {
            fs::rename(sWorkDir + "/" + CVarGetString(CVAR_GENERAL("SpoilerLog"), ""), outPath);
            result = 0;
        } else {
            std::cerr << "seedgen: generation failed for this seed\n";
            result = 1;
        }
    } catch (const std::exception& e) { std::cerr << "seedgen: " << e.what() << "\n"; }

    std::error_code ignored;
    fs::remove_all(sWorkDir, ignored);
    return result;
}
