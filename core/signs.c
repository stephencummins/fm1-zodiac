/* the signs there are so far, in the year's order */
#include "zodiac.h"

const ZSign z_signs[] = {
    {"ARIES",  "fire",  "Heat",   aries_init,  aries_frame,  aries_strike,  aries_palette},
    {"TAURUS", "earth", "Growth", taurus_init, taurus_frame, taurus_strike, taurus_palette},
    {"GEMINI", "air",   "Wind",   gemini_init, gemini_frame, gemini_strike, gemini_palette},
    {"CANCER", "water", "Tide",   cancer_init, cancer_frame, cancer_strike, cancer_palette},
};
const int z_nsigns = sizeof z_signs / sizeof z_signs[0];
