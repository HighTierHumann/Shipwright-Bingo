#pragma once

#include <array>
#include <string>
#include "randomizerTypes.h"

namespace Rando {
/**
 * @brief Holds the data and runtime shuffled state for a single "Silver Rupee area" -
 * a room (or set of rooms sharing one collection) that contains 5 Silver Rupees which can
 * optionally be shuffled into the item pool as freestanding items instead of appearing as
 * their vanilla EnGSwitch actors.
 */
class SilverRupeeAreaInfo {
  public:
    SilverRupeeAreaInfo(std::string name_, RandomizerHintTextKey hintKey_, RandomizerSettingKey settingKey_,
                        RandomizerGet rupeeItem_, RandomizerGet bagItem_);
    SilverRupeeAreaInfo();
    ~SilverRupeeAreaInfo();

    const std::string& GetName() const;
    RandomizerHintTextKey GetHintKey() const;
    RandomizerSettingKey GetSettingKey() const;
    RandomizerGet GetRupeeItem() const;
    RandomizerGet GetBagItem() const;
    /// @brief Marks this area as shuffled. When asBag is true, the area's 5 Silver Rupees are
    /// shuffled into the pool as a single Silver Rupee Bag item (instead of 5 individual items).
    void SetShuffled(bool asBag = false);
    void ClearShuffled();
    bool IsShuffled() const;
    bool IsBag() const;

  private:
    std::string name;
    RandomizerHintTextKey hintKey;
    RandomizerSettingKey settingKey;
    RandomizerGet rupeeItem;
    RandomizerGet bagItem;
    bool shuffled = false;
    bool bag = false;
};

typedef enum {
    SRA_SPIRIT_TEMPLE_BOULDERS,
    SRA_SPIRIT_TEMPLE_CHILD,
    SRA_SPIRIT_TEMPLE_SUN_BLOCK,
    SRA_SHADOW_TEMPLE_SCYTHE,
    SRA_SHADOW_TEMPLE_BEAMOS,
    SRA_SHADOW_TEMPLE_REDEAD,
    SRA_GANONS_CASTLE_FOREST_TRIAL,
    SRA_GANONS_CASTLE_FIRE_TRIAL,
    SRA_GANONS_CASTLE_SPIRIT_TRIAL,
    SRA_GANONS_CASTLE_LIGHT_TRIAL,
    SRA_ICE_CAVERN_SCYTHE,
    SRA_ICE_CAVERN_PUSH_BLOCK,
    SRA_GERUDO_TRAINING_GROUND_BOULDERS,
    SRA_GERUDO_TRAINING_GROUND_LAVA,
    SRA_GERUDO_TRAINING_GROUND_TOILET,
    SRA_BOTTOM_OF_THE_WELL_BASEMENT,
    SRA_MAX
} SilverRupeeAreaKey;

class SilverRupeeAreas {
  public:
    SilverRupeeAreas();
    ~SilverRupeeAreas();
    SilverRupeeAreaInfo* GetArea(SilverRupeeAreaKey key);
    /// @brief Finds the area whose Location(s) share the given hint text key, or nullptr if
    /// the hint key does not belong to any Silver Rupee area.
    SilverRupeeAreaInfo* GetAreaForHintKey(RandomizerHintTextKey hintKey);
    void ClearAllShuffled();
    std::array<SilverRupeeAreaInfo*, SRA_MAX> GetAreaList();
    size_t GetAreaListSize() const;
    void ParseJson(const nlohmann::json& spoilerFileJson);

  private:
    std::array<SilverRupeeAreaInfo, SRA_MAX> mAreas;
};
} // namespace Rando
