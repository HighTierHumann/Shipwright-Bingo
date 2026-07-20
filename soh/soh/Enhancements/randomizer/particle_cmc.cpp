#include "particle_cmc.h"
#include "soh/Enhancements/cosmetics/CosmeticsEditor.h"
#include "soh/cvar_prefixes.h"

// Color of the circle for the particles
static Color_RGBA8 mainColors[7] = {
    { 255, 255, 255, 255 }, // Junk
    { 170, 50, 0, 255 },    // Lesser
    { 250, 0, 0, 255 },     // Health
    { 255, 255, 0, 255 },   // Boss Key
    { 85, 220, 255, 255 }, // Small Key
    { 0, 0, 0, 255 },       // Skulltula Token
    { 250, 185, 40, 255 },  // Major
};

// Secondary color of the circle for the particles
static Color_RGBA8 secColors[7] = {
    { 255, 255, 255, 255 }, // Junk
    { 250, 75, 0, 255 },    // Lesser
    { 0, 0, 255, 255 },     // Health
    { 0, 200, 255, 255 },   // Boss Key
    { 45, 100, 170, 255 }, // Small Key
    { 255, 250, 190, 255 }, // Skulltula Token
    { 255, 220, 135, 255 }, // Major
};

// Color of the faded flares stretching off the particles
static Color_RGBA8 flareColors[7] = {
    { 135, 135, 135, 255 }, // Junk
    { 255, 160, 100, 255 }, // Lesser
    { 255, 125, 125, 255 }, // Health
    { 0, 200, 255, 255 },   // Boss Key
    { 45, 50, 170, 255 }, // Small Key
    { 255, 255, 255, 255 }, // Skulltula Token
    { 250, 220, 180, 255 }, // Major
};

static bool ColorEquals(const Color_RGBA8& a, const Color_RGBA8& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}


Color_RGBA8 Randomizer_GetParticleCMCColor(GetItemCategory colorIndex, ParticleCMCColorType colorType) {
    Color_RGBA8 primary;
    switch (colorIndex) {
        case ITEM_CATEGORY_JUNK:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.Junk.Value"), Color_RGBA8{ 255, 255, 255, 255 });
            break;
        case ITEM_CATEGORY_LESSER:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.Lesser.Value"), Color_RGBA8{ 170, 50, 0, 255 });
            break;
        case ITEM_CATEGORY_SILVER_RUPEE:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.SilverRupee.Value"), Color_RGBA8{ 155, 0, 255, 255 });
            break;
        case ITEM_CATEGORY_HEALTH:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.Health.Value"), Color_RGBA8{ 250, 0, 0, 255 });
            break;
        case ITEM_CATEGORY_BOSS_KEY:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.BossKey.Value"), Color_RGBA8{ 255, 255, 0, 255 });
            break;
        case ITEM_CATEGORY_SMALL_KEY:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.SmallKey.Value"), Color_RGBA8{ 85, 220, 255, 255 });
            break;
        case ITEM_CATEGORY_SKULLTULA_TOKEN:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.Skulltula.Value"), Color_RGBA8{ 0, 0, 0, 255 });
            break;
        case ITEM_CATEGORY_MAJOR:
            primary = CVarGetColor(CVAR_COSMETIC("CMC.Major.Value"), Color_RGBA8{ 250, 185, 40, 255 });
            break;
        default:
            primary = { 255, 255, 255, 255 };
            break;
    }

    switch (colorType) {
        case COLOR_PRIMARY:
            return primary;
        case COLOR_SECONDARY: {
            if (ColorEquals(primary, mainColors[colorIndex])) {
                return secColors[colorIndex];
            } else {
                // Takes the value of 'primary' and darkens it by 30% to use as the secondary color
                Color_RGBA8 secondary = { static_cast<uint8_t>(primary.r * 0.7f),
                                          static_cast<uint8_t>(primary.g * 0.7f),
                                          static_cast<uint8_t>(primary.b * 0.7f), primary.a };
                return secondary;
            }
        }
        case COLOR_FLARE: {
            if (ColorEquals(primary, mainColors[colorIndex])) {
                return flareColors[colorIndex];
            } else {
                Color_RGBA8 flare = { static_cast<uint8_t>(primary.r * 0.6f),
                                      static_cast<uint8_t>(primary.g * 0.6f),
                                      static_cast<uint8_t>(primary.b * 0.6f), primary.a };
                return flare;
            }
        }
        default:
            return mainColors[colorIndex];
    }
}
