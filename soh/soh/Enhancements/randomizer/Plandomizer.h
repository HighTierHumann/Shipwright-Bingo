#pragma once
#ifndef PLANDOMIZER_H
#define PLANDOMIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif // PLANDOMIZER_H

#include <ship/window/gui/GuiWindow.h>
#include "soh/Enhancements/randomizer/item.h"

#ifdef __cplusplus
class PlandomizerWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override{};
};

typedef struct {
    std::string checkName;
    Rando::Item checkRewardItem;
    int32_t shopPrice;
    Rando::Item iceTrapModel;
    std::string iceTrapName;
} SpoilerCheckObject;

typedef struct {
    std::string hintName;
    std::string hintType;
    std::string hintText;
} SpoilerHintObject;

typedef struct {
    uint8_t entranceTypeRaw;
    int16_t entranceIndex;                    // "index"       - this entrance itself (immutable)
    int16_t entranceDestinationIndex;         // "destination" - paired reverse entrance, -1 if one-way
    int16_t entranceOverrideIndex;            // "override"    - entrance currently placed in this slot; -1 = "None"
    int16_t entranceOverrideDestinationIndex; // "overrideDestination" - reverse pairing of the override

    // True if this row was explicitly present in the loaded spoiler log's "entrances" array.
    // Rows synthesized to also list non-shuffled entrances only get written back out on save
    // if the user actually edits their destination away from the vanilla default.
    bool wasInSpoilerLog;

    // Display strings derived from the indices above via entrance.cpp's enums/lookups.
    std::string entranceType;
    std::string entranceName;
    std::string entranceDestination;
    std::string entranceOverride;
    std::string entranceOverrideDestination;
    
    int32_t entranceAreaOrder;
} SpoilerEntranceObject;

typedef enum PlandoTabs {
    TAB_HINTS,
    TAB_LOCATIONS,
    TAB_ENTRANCES,
} PlandoTabs;

typedef enum PlandoHints {
    HINT_SINGLE,
    HINT_ALL,
} PlandoHints;

#endif