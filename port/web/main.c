/* The core in a browser: one call per frame, the picture handed back as RGBA. */
#include <emscripten.h>
#include "../../core/zodiac.h"
#include "../../core/music.h"

static uint8_t rgba[GW * GH * 4];

static const ZSign *Z;
static const ZMusic *Mu;            /* the sign's piece, and whether the page has its sound on */
static int which_sign; static float rate; static ZParams P = {0.45f, 0.0f, 0.15f, 0.7f};
static float abuf[2 * 4096];

static void start_music(void) { Mu = z_music(which_sign); z_led = rate > 0.0f && Mu; if (z_led) z_music_start(Mu, rate); }

EMSCRIPTEN_KEEPALIVE void z_init(int which) { which_sign = which < 0 || which >= z_nsigns ? 0 : which; Z = &z_signs[which_sign]; g_clear(); Z->init(); start_music(); }

/* sound on at this sample rate, or off at 0 */
EMSCRIPTEN_KEEPALIVE void z_sound(float sr) { rate = sr; start_music(); }

/* the next n stereo samples; with the sound on, the music leads and the picture follows it */
EMSCRIPTEN_KEEPALIVE float *z_audio(int n)
{
    if (n > 4096) n = 4096;
    if (z_led) z_music_render(Mu, abuf, n, &P);
    else for (int i = 0; i < 2 * n; i++) abuf[i] = 0.0f;
    return abuf;
}

EMSCRIPTEN_KEEPALIVE void z_strike(void) { if (z_led) Mu->strike(); else Z->strike(); }

EMSCRIPTEN_KEEPALIVE uint8_t *z_frame(float dt, float heat, float echo, float mist, float light)
{
    static uint16_t pal[256];
    ZParams p = {heat, echo, mist, light};
    P = p;
    for (int e = z_music_event(); e > 0; e--) Z->strike();
    Z->frame(dt, &p);
    Z->palette(pal);
    for (int i = 0; i < GW * GH; i++) {
        uint16_t c = pal[g_fb[i]];
        rgba[4 * i] = (uint8_t)(((c >> 11) & 31) * 255 / 31);
        rgba[4 * i + 1] = (uint8_t)(((c >> 5) & 63) * 255 / 63);
        rgba[4 * i + 2] = (uint8_t)((c & 31) * 255 / 31);
        rgba[4 * i + 3] = 255;
    }
    return rgba;
}
