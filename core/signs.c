/* the signs there are so far, in the year's order */
#include "zodiac.h"

const ZSign z_signs[] = {
    {"ARIES",  "fire",  "Heat",   aries_init,  aries_frame,  aries_strike,  aries_palette},
    {"TAURUS", "earth", "Growth", taurus_init, taurus_frame, taurus_strike, taurus_palette},
    {"GEMINI", "air",   "Wind",   gemini_init, gemini_frame, gemini_strike, gemini_palette},
    {"CANCER", "water", "Tide",   cancer_init, cancer_frame, cancer_strike, cancer_palette},
    {"LEO",    "fire",  "Blaze",  leo_init,    leo_frame,    leo_strike,    leo_palette},
    {"VIRGO",  "earth", "Harvest", virgo_init, virgo_frame,  virgo_strike,  virgo_palette},
    {"LIBRA",  "air",   "Breeze", libra_init,  libra_frame,  libra_strike,  libra_palette},
    {"SCORPIO", "water", "Depth", scorpio_init, scorpio_frame, scorpio_strike, scorpio_palette},
    {"SAGITTARIUS", "fire", "Flight", sagittarius_init, sagittarius_frame, sagittarius_strike, sagittarius_palette},
    {"CAPRICORN", "earth", "Climb", capricorn_init, capricorn_frame, capricorn_strike, capricorn_palette},
    {"AQUARIUS", "air", "Flow", aquarius_init, aquarius_frame, aquarius_strike, aquarius_palette},
    {"PISCES", "water", "Dream", pisces_init, pisces_frame, pisces_strike, pisces_palette},
};
const int z_nsigns = sizeof z_signs / sizeof z_signs[0];
