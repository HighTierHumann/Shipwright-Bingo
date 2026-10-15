#include "BingoSquareMap.h"

#include <unordered_map>
#include <vector>

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
}

namespace Bingo {

namespace {

constexpr int kDungeonScenes[] = {
    SCENE_DEKU_TREE,     SCENE_DODONGOS_CAVERN,    SCENE_JABU_JABU,    SCENE_FOREST_TEMPLE,  SCENE_FIRE_TEMPLE,
    SCENE_WATER_TEMPLE,  SCENE_SPIRIT_TEMPLE,      SCENE_SHADOW_TEMPLE, SCENE_BOTTOM_OF_THE_WELL, SCENE_ICE_CAVERN,
};

constexpr int kBossKeyScenes[] = {
    SCENE_FOREST_TEMPLE, SCENE_FIRE_TEMPLE, SCENE_WATER_TEMPLE, SCENE_SPIRIT_TEMPLE, SCENE_SHADOW_TEMPLE,
};

constexpr int kMedallions[] = {
    QUEST_MEDALLION_FOREST, QUEST_MEDALLION_FIRE,   QUEST_MEDALLION_WATER,
    QUEST_MEDALLION_SPIRIT, QUEST_MEDALLION_SHADOW, QUEST_MEDALLION_LIGHT,
};

constexpr int kStones[] = { QUEST_KOKIRI_EMERALD, QUEST_GORON_RUBY, QUEST_ZORA_SAPPHIRE };

bool HasDungeonItem(int item, int scene) {
    return CHECK_DUNGEON_ITEM(item, scene) != 0;
}

bool HasQuestItem(int item) {
    return CHECK_QUEST_ITEM(item) != 0;
}

template <size_t N> int CountDungeonItem(int item, const int (&scenes)[N]) {
    int count = 0;
    for (int scene : scenes) {
        count += HasDungeonItem(item, scene) ? 1 : 0;
    }
    return count;
}

template <size_t N> int CountQuestItems(const int (&items)[N]) {
    int count = 0;
    for (int item : items) {
        count += HasQuestItem(item) ? 1 : 0;
    }
    return count;
}

bool HasMapAndCompass(int scene) {
    return HasDungeonItem(DUNGEON_MAP, scene) && HasDungeonItem(DUNGEON_COMPASS, scene);
}

// To track a new square: add its BS_ enum to BingoSquares.h, then add a row here with the exact website name and the
// condition (inventory, quest items, event flags, RAND_INF flags...) that means the player completed it.
const std::vector<SquareDef>& Squares() {
    static const std::vector<SquareDef> squares = {
        { BS_TREASURE_ZELDA, "Obtain the treasure thrown by Princess Zelda", nullptr },
        { BS_PRELUDE_CHECK, "Prelude of Light Check",
          []() { return Flags_GetEventChkInf(EVENTCHKINF_BONGO_BONGO_ESCAPED_FROM_WELL) != 0; } },
        { BS_NOCTURNE_CHECK, "Nocturne of Shadow Check", nullptr },

        { BS_FOREST_SPIRIT_MEDAL_RUBY, "Forest & Spirit Medallion and Goron Ruby",
          []() {
              return HasQuestItem(QUEST_MEDALLION_FOREST) && HasQuestItem(QUEST_MEDALLION_SPIRIT) &&
                     HasQuestItem(QUEST_GORON_RUBY);
          } },
        { BS_SHADOW_FIRE_MEDAL_EMERALD, "Shadow & Fire Medallion and Kokiri Emerald",
          []() {
              return HasQuestItem(QUEST_MEDALLION_SHADOW) && HasQuestItem(QUEST_MEDALLION_FIRE) &&
                     HasQuestItem(QUEST_KOKIRI_EMERALD);
          } },
        { BS_LIGHT_WATER_MEDAL_SAPPHIRE, "Light & Water Medallion and Zora Sapphire",
          []() {
              return HasQuestItem(QUEST_MEDALLION_LIGHT) && HasQuestItem(QUEST_MEDALLION_WATER) &&
                     HasQuestItem(QUEST_ZORA_SAPPHIRE);
          } },
        { BS_2_STONES, "2 Spiritual Stones", []() { return CountQuestItems(kStones) >= 2; } },
        { BS_5_MEDALS, "5 Medallions", []() { return CountQuestItems(kMedallions) >= 5; } },

        { BS_4_COMPASSES, "4 Compasses", []() { return CountDungeonItem(DUNGEON_COMPASS, kDungeonScenes) >= 4; } },
        { BS_6_COMPASSES, "6 Compasses", []() { return CountDungeonItem(DUNGEON_COMPASS, kDungeonScenes) >= 6; } },
        { BS_4_MAPS, "4 Maps", []() { return CountDungeonItem(DUNGEON_MAP, kDungeonScenes) >= 4; } },
        { BS_6_MAPS, "6 Maps", []() { return CountDungeonItem(DUNGEON_MAP, kDungeonScenes) >= 6; } },

        { BS_DEKU_MAP_COMPASS, "Map and Compass in Deku Tree", []() { return HasMapAndCompass(SCENE_DEKU_TREE); } },
        { BS_DC_MAP_COMPASS, "Map and Compass in Dodongo's Cavern", []() { return HasMapAndCompass(SCENE_DODONGOS_CAVERN); } },
        { BS_JABU_MAP_COMPASS, "Map and Compass in Jabu-Jabu's Belly", []() { return HasMapAndCompass(SCENE_JABU_JABU); } },
        { BS_FOREST_MAP_COMPASS, "Map and Compass in Forest Temple", []() { return HasMapAndCompass(SCENE_FOREST_TEMPLE); } },
        { BS_FIRE_MAP_COMPASS, "Map and Compass in Fire Temple", []() { return HasMapAndCompass(SCENE_FIRE_TEMPLE); } },
        { BS_WATER_MAP_COMPASS, "Map and Compass in Water Temple", []() { return HasMapAndCompass(SCENE_WATER_TEMPLE); } },
        { BS_SHADOW_MAP_COMPASS, "Map and Compass in Shadow Temple", []() { return HasMapAndCompass(SCENE_SHADOW_TEMPLE); } },
        { BS_SPIRIT_MAP_COMPASS, "Map and Compass in Spirit Temple", []() { return HasMapAndCompass(SCENE_SPIRIT_TEMPLE); } },
        { BS_ICE_MAP_COMPASS, "Map and Compass in Ice Cavern", []() { return HasMapAndCompass(SCENE_ICE_CAVERN); } },
        { BS_BOTW_MAP_COMPASS, "Map and Compass in Bottom of the Well", []() { return HasMapAndCompass(SCENE_BOTTOM_OF_THE_WELL); } },

        // Counts the five dungeon boss keys; Ganon's Boss Key has its own square.
        { BS_3_BOSS_KEYS, "3 Boss Keys", []() { return CountDungeonItem(DUNGEON_KEY_BOSS, kBossKeyScenes) >= 3; } },
        { BS_5_BOSS_KEYS, "5 Boss Keys", []() { return CountDungeonItem(DUNGEON_KEY_BOSS, kBossKeyScenes) >= 5; } },
        { BS_FOREST_BOSS_KEY, "Forest Temple Boss Key", []() { return HasDungeonItem(DUNGEON_KEY_BOSS, SCENE_FOREST_TEMPLE); } },
    };
    return squares;
}

} // namespace

const SquareDef* FindSquareByName(const std::string& name) {
    static const std::unordered_map<std::string, const SquareDef*> byName = []() {
        std::unordered_map<std::string, const SquareDef*> map;
        for (const SquareDef& def : Squares()) {
            map.emplace(def.name, &def);
        }
        return map;
    }();

    auto it = byName.find(name);
    return it == byName.end() ? nullptr : it->second;
}

} // namespace Bingo
