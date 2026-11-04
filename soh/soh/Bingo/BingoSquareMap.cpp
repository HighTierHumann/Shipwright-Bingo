#include "BingoSquareMap.h"

#include <unordered_map>

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
}

namespace Bingo {

namespace {

// To track a new square, map the string name of the square to the BINGO_INF flag here.
const std::unordered_map<std::string, BingoInf> squares = {
    { "Obtain the treasure thrown by Princess Zelda", BINGO_INF_TREASURE_ZELDA },
    { "Prelude of Light Check", BINGO_INF_PRELUDE_CHECK },
    { "Nocturne of Shadow Check", BINGO_INF_NOCTURNE_CHECK },

    { "Forest & Spirit Medallion and Goron Ruby", BINGO_INF_FOREST_SPIRIT_MEDAL_RUBY },
    { "Shadow & Fire Medallion and Kokiri Emerald", BINGO_INF_SHADOW_FIRE_MEDAL_EMERALD },
    { "Light & Water Medallion and Zora Sapphire", BINGO_INF_LIGHT_WATER_MEDAL_SAPPHIRE },
    { "2 Spiritual Stones", BINGO_INF_2_STONES },
    { "5 Medallions", BINGO_INF_5_MEDALLIONS },

    { "4 Compasses", BINGO_INF_4_COMPASSES },
    { "6 Compasses", BINGO_INF_6_COMPASSES },
    { "4 Maps", BINGO_INF_4_MAPS },
    { "6 Maps", BINGO_INF_6_MAPS },

    { "Map and Compass in Deku Tree", BINGO_INF_DEKU_MAP_COMPASS },
    { "Map and Compass in Dodongo's Cavern", BINGO_INF_DC_MAP_COMPASS },
    { "Map and Compass in Jabu-Jabu's Belly", BINGO_INF_JABU_MAP_COMPASS },
    { "Map and Compass in Forest Temple", BINGO_INF_FOREST_MAP_COMPASS },
    { "Map and Compass in Fire Temple", BINGO_INF_FIRE_MAP_COMPASS },
    { "Map and Compass in Water Temple", BINGO_INF_WATER_MAP_COMPASS },
    { "Map and Compass in Shadow Temple", BINGO_INF_SHADOW_MAP_COMPASS },
    { "Map and Compass in Spirit Temple", BINGO_INF_SPIRIT_MAP_COMPASS },
    { "Map and Compass in Ice Cavern", BINGO_INF_ICE_MAP_COMPASS },
    { "Map and Compass in Bottom of the Well", BINGO_INF_BOTW_MAP_COMPASS },

    { "3 Boss Keys", BINGO_INF_3_BOSS_KEYS },
    { "5 Boss Keys", BINGO_INF_5_BOSS_KEYS },
    { "Forest Temple Boss Key", BINGO_INF_FOREST_BOSS_KEY },
    { "Fire Temple Boss Key", BINGO_INF_FIRE_BOSS_KEY },
    { "Water Temple Boss Key", BINGO_INF_WATER_BOSS_KEY },
    { "Spirit Temple Boss Key", BINGO_INF_SPIRIT_BOSS_KEY },
    { "Shadow Temple Boss Key", BINGO_INF_SHADOW_BOSS_KEY },
    { "Ganon Boss Key", BINGO_INF_GANON_BOSS_KEY },

    { "Enter every dungeon", BINGO_INF_ENTER_EVERY_DUNGEON },
    { "Defeat 3 Bosses", BINGO_INF_DEFEAT_3_BOSSES },
    { "Defeat 3 Temple Bosses", BINGO_INF_DEFEAT_3_TEMPLE_BOSSES },
    { "Defeat Gohma", BINGO_INF_DEFEAT_GOHMA },
    { "Defeat King Dodongo", BINGO_INF_DEFEAT_KING_DODONGO },
    { "Defeat Barinade", BINGO_INF_DEFEAT_BARINADE },
    { "Defeat Phantom Ganon", BINGO_INF_DEFEAT_PHANTOM_GANON },
    { "Defeat Volvagia", BINGO_INF_DEFEAT_VOLVAGIA },
    { "Defeat Morpha", BINGO_INF_DEFEAT_MORPHA },
    { "Defeat Twinrova", BINGO_INF_DEFEAT_TWINROVA },
    { "Defeat Bongo Bongo", BINGO_INF_DEFEAT_BONGO_BONGO },

    { "Activate all 3 Rusty Hammer Switches in Fire Temple", BINGO_INF_FIRE_RUSTY_HAMMER_SWITCHES },
    { "Activate both Rusty Hammer Switches in Spirit Temple", BINGO_INF_SPIRIT_RUSTY_HAMMER_SWITCHES },
    { "Activate 5 different Eye Switches", BINGO_INF_5_DIFFERENT_EYE_SWITCHES },
    { "Activate an Eye Switch in 4 different dungeons", BINGO_INF_4_EYE_SWITCHES_IN_DIFFERENT_DUNGEONS },
    { "Defeat the Hostile Eye Switch in Shadow Temple", BINGO_INF_DEFEAT_THE_SHADOW_EYE_SWITCH },
    { "Unlock all 5 Locked Doors in Water Temple", BINGO_INF_UNLOCK_ALL_5_LOCKED_WATER_DOORS },
    { "Unlock all 5 Locked Doors in Spirit Temple", BINGO_INF_UNLOCK_ALL_5_LOCKED_SPIRIT_DOORS },
    { "Unlock Forest Temple's Boss Door", BINGO_INF_UNLOCK_FOREST_BOSS_DOOR },
    { "Unlock 4 Locked Boss Doors", BINGO_INF_UNLOCK_4_BOSS_DOORS },
    { "Scare off all 5 Deku Scrubs in Deku Tree", BINGO_INF_SCARE_OFF_ALL_DEKU_SCRUBS_IN_TREE },
    { "Speak with all 4 Business Scrubs in Dodongo's Cavern", BINGO_INF_SPEAK_WITH_4_DC_BUSINESS_SCRUBS },
    { "Feed a Regular Dodongo a Bomb or a Bombflower", BINGO_INF_FEED_A_DODONGO_A_BOMB_OR_FLOWER },
    { "Throw Princess Ruto Into Water and Drown Her", BINGO_INF_DROWN_RUTO },
    { "Twist 1 Hallway in Forest Temple", BINGO_INF_TWIST_1_FOREST_HALLWAY },
    { "Open 5 Gorons cages in Fire Temple", BINGO_INF_FREE_5_GORONS },
    { "Open all 9 Gorons cages in Fire Temple", BINGO_INF_FREE_ALL_9_GORONS },
    { "Set the water to the middle level in Water Temple", BINGO_INF_WATER_TEMPLE_MIDDLE_LEVEL },
    { "Play Zelda's Lullaby for all 3 Triforces in Spirit Temple", BINGO_INF_PLAYED_FOR_ALL_3_TRIFORCES_IN_SPIRIT },
    { "Give the Statue in Spirit Temple a Nipple Piercing with your Hookshot", BINGO_INF_SPIRIT_STATUE_NIPPLE_PIERCING },
    { "Lower the water in Bottom of the Well", BINGO_INF_LOWER_THE_WATER_IN_BOTTOM_OF_THE_WELL },
    { "Burn the wooden spiked walls in Shadow Temple", BINGO_INF_BURN_THE_WOOD_WALLS_IN_SHADOW },
    { "Break a Door Mimic in 2 different dungeons", BINGO_INF_BREAK_2_DOOR_MIMICS },
    { "Explode the 4 Giant Skull Pots in Shadow Temple", BINGO_INF_EXPLODE_4_SKULL_POTS },
    { "Collect both sets of Silver Rupee items in Ice Cavern", BINGO_INF_COLLECT_ICE_CAVERN_SILVER_ITEMS },
    { "Collect all 3 sets of Silver Rupee items in Shadow Temple", BINGO_INF_COLLECT_SHADOW_TEMPLE_SILVER_ITEMS },
    { "Collect all 4 sets of Silver Rupee items in Ganon's Castle", BINGO_INF_COLLECT_GANONS_CASTLE_SILVER_ITEMS },

    { "Toilet Room Chest", BINGO_INF_TOILET_ROOM_CHEST },
    { "Open the Golden Gauntlets Chest in Ganon's Castle", BINGO_INF_GOLD_GAUNTLETS_CHEST },
    { "Open the Chests on both of the Desert Colossus Hands", BINGO_INF_DESERT_COLOSSUS_HAND_CHESTS },
    { "Open the Ice Arrow Chest in Gerudo Training Grounds", BINGO_INF_ICE_ARROW_CHEST },
    { "Open the Ganon Boss Key Chest", BINGO_INF_GANON_BOSS_KEY_CHEST },
    { "Open both Timer Challenge Chests in Fire Temple", BINGO_INF_FIRE_TEMPLE_TIMER_CHALLENGE_CHESTS },
    { "Open all 6 Chests in Deku Tree", BINGO_INF_ALL_DEKU_TREE_CHESTS },
    { "Open all 3 Chests in Jabu-Jabu's Belly", BINGO_INF_ALL_JABU_JABU_CHESTS },

    { "Break both pots at the end of Forest Trial", BINGO_INF_BREAK_FOREST_TRIAL_POTS },
    { "Break both pots at the end of Fire Trial", BINGO_INF_BREAK_FIRE_TRIAL_POTS },
    { "Break both pots at the end of Water Trial", BINGO_INF_BREAK_WATER_TRIAL_POTS },
    { "Break both pots at the end of Spirit Trial", BINGO_INF_BREAK_SPIRIT_TRIAL_POTS },
    { "Break both pots at the end of Shadow Trial", BINGO_INF_BREAK_SHADOW_TRIAL_POTS },
    { "Break both pots at the end of Light Trial", BINGO_INF_BREAK_LIGHT_TRIAL_POTS },
    { "Disable the Barrier in Ganon's Castle", BINGO_INF_DISABLE_THE_BARRIER },

    { "Be frozen by an Ice Keese", BINGO_INF_FROZEN_BY_ICE_KEESE },
    { "Be ignited by Fire Keese in 3 different dungeons", BINGO_INF_IGNITED_BY_3_FIRE_KEESE },
    { "Be voided by Wallmasters in 3 different dungeons", BINGO_INF_VOIDED_BY_3_DIFFERENT_DUNGEON_WALLMASTERS },
    { "Be damaged by a flying pot in 3 different dungeons", BINGO_INF_DAMAGED_BY_3_DIFFERENT_DUNGEON_POTS },

    { "Defeat 4 Different Iron Knuckles", BINGO_INF_DEFEAT_4_DIFFERENT_IRON_KNUCKLES },
    { "Defeat 4 Different Dinolfos", BINGO_INF_DEFEAT_4_DIFFERENT_DINOLFOS },
    { "Defeat all 4 Lizalfos in Spirit Temple", BINGO_INF_DEFEAT_ALL_SPIRIT_LIZALFOS },
    { "Defeat all 4 Lizalfos in Dodongo's Cavern", BINGO_INF_DEFEAT_ALL_DODONGOS_LIZALFOS },
    { "Defeat all 6 Octoroks in Jabu-Jabu", BINGO_INF_DEFEAT_ALL_JABU_OCTOROKS },
    { "Defeat all 5 Stalfos in Forest Temple", BINGO_INF_DEFEAT_ALL_FOREST_STALFOS },
    { "Defeat all 7 Blue Bubbles in Forest Temple", BINGO_INF_DEFEAT_ALL_FOREST_BLUE_BUBBLES },
    { "Defeat a White Bubble", BINGO_INF_DEFEAT_A_WHITE_BUBBLE },
    { "Defeat a Red Bubble in 2 different world locations", BINGO_INF_DEFEAT_2_DIFFERENT_RED_BUBBLES },
    { "Defeat 3 Different Green Bubbles", BINGO_INF_DEFEAT_3_DIFFERENT_GREEN_BUBBLES },
    { "Defeat a Like-Like in 3 different dungeons", BINGO_INF_DEFEAT_3_DIFFERENT_DUNGEON_LIKE_LIKES },
    { "Defeat all Wolfos and Like-Likes in Gerudo Training Ground", BINGO_INF_DEFEAT_ALL_WOLFOS_AND_LIKE_LIKES_IN_GTG },
    { "Defeat the White Wolfos in Ice Cavern", BINGO_INF_DEFEAT_ICE_CAVERN_WOLFOS },
    { "Defeat all 3 White Wolfos", BINGO_INF_DEFEAT_ALL_3_WHITE_WOLFOS },
    { "Defeat Big Octo", BINGO_INF_DEFEAT_BIG_OCTO },
    { "Defeat Amy (Green Poe)", BINGO_INF_DEFEAT_AMY },
    { "Defeat Joelle and Beth (Red and Blue Poe)", BINGO_INF_DEFEAT_JOELLE_AND_BETH },
    { "Defeat Meg (Purple Poe)", BINGO_INF_DEFEAT_MEG },
    { "Defeat both Flare Dancers", BINGO_INF_DEFEAT_BOTH_FLARE_DANCERS },
    { "Defeat Dark Link", BINGO_INF_DEFEAT_DARK_LINK },
    { "Defeat the Dead Hand in Bottom of the Well", BINGO_INF_DEFEAT_DEAD_HAND_BOTTOM_OF_THE_WELL },
    { "Defeat the Dead Hand in Shadow Temple", BINGO_INF_DEFEAT_DEAD_HAND_SHADOW_TEMPLE },
};

} // namespace

bool IsComplete(const std::string& name) {
    auto it = squares.find(name);
    if (it == squares.end()) {
        return false;
    }
    return Flags_GetBingoInf(it->second) != 0;
}

} // namespace Bingo
