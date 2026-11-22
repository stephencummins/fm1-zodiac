/* The core in a browser: one call per frame, the picture handed back as RGBA. */
#include <emscripten.h>
#include "../../core/zodiac.h"

static uint8_t rgba[GW * GH * 4];

EMSCRIPTEN_KEEPALIVE void z_init(void) { aries_init(); }

EMSCRIPTEN_KEEPALIVE void z_strike(void) { aries_strike(); }

EMSCRIPTEN_KEEPALIVE uint8_t *z_frame(float dt, float heat, float echo, float mist, float light)
{
    static uint16_t pal[256];
    ZParams p = {heat, echo, mist, light};
    aries_frame(dt, &p);
    aries_palette(pal);
    for (int i = 0; i < GW * GH; i++) {
        uint16_t c = pal[g_fb[i]];
        rgba[4 * i] = (uint8_t)(((c >> 11) & 31) * 255 / 31);
        rgba[4 * i + 1] = (uint8_t)(((c >> 5) & 63) * 255 / 63);
        rgba[4 * i + 2] = (uint8_t)((c & 31) * 255 / 31);
        rgba[4 * i + 3] = 255;
    }
    return rgba;
}
