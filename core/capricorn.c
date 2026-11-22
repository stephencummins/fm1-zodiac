/* CAPRICORN: the goat, earth. Patient, ambitious, sure-footed: the long-horned goat before its mountain.
 * At a note it dips its head and tosses it, the ears flick, and snow shakes loose.
 * The goat is art/pic/capricorn.jpg; the snow is drawn here. */
#include "scene.h"
#include "pic_capricorn.h"

enum { K_BONE, K_SLATE, K_PINE, K_BRONZE };

static struct { Scene s; f32 age; } C;
static uint8_t part[GW * GH];

void capricorn_init(void) { sc_init(&C.s, 127u); C.age = 9.0f; }
void capricorn_strike(void) { C.age = 0.0f; }

static void snow(f32 w)
{
    f32 h = C.s.p.heat, t = C.s.t;
    int n = (int)(5.0f + 26.0f * h + 30.0f * w);
    g_mode(G_MAX); g_ink(K_BONE);
    for (int i = 0; i < n; i++) {
        f32 u = t * (0.05f + 0.08f * hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 x = hash01((uint32_t)i * 29u + 5u) * 240.0f + 5.0f * sin_turns(t * 0.2f + (f32)i * 0.37f), y = -6.0f + 252.0f * u;
        g_disc(x, y, 1.1f, 0.35f + 0.4f * h + 0.3f * w);
    }
}

void capricorn_frame(f32 dt, const ZParams *want)
{
    sc_step(&C.s, dt, want);
    f32 t = C.s.t, h = C.s.p.heat;
    C.age += dt;
    f32 w = C.age < 1.6f ? 1.0f - C.age / 1.6f : 0.0f;
    f32 nod = w * sin_turns(C.age * 1.05f);                         /* down first, then up */
    sc_begin(&C.s);
    sc_mist(&C.s, K_SLATE);
    p_begin(capricorn_pic);
    f32 fl = w * sin_turns(t * 5.0f) + 0.15f * maxf(0.0f, sin_turns(t * 0.13f) - 0.9f) * 10.0f * sin_turns(t * 4.0f);
    p_warp(capricorn_pic, 76.0f, 114.0f, 17.0f, 13.0f, 0.0f, 0.03f * fl, 0.0f, 0.0f, 0);          /* ears */
    p_warp(capricorn_pic, 164.0f, 114.0f, 17.0f, 13.0f, 0.0f, -0.03f * fl, 0.0f, 0.0f, 0);
    p_warp(capricorn_pic, 120.0f, 196.0f, 9.0f, 13.0f, 0.0f, 0.02f * sin_turns(t * 0.3f) + 0.05f * w * sin_turns(t * 3.0f), 0.0f, 0.0f, 0);   /* beard */
    for (int i = 0; i < GW * GH; i++) part[i] = g_work[i];
    p_warp(part, 120.0f, 140.0f, 60.0f, 70.0f, 0.02f * w, 0.006f * sin_turns(t * 0.05f), 0.0f, 8.0f * nod + 0.8f * sin_turns(t * 0.09f), 0);
    g_blit(g_work, 1.0f + 0.006f * sin_turns(t * 0.05f), -400.0f, 0.0f, 0.0f, 0.0f, 0.92f + 0.12f * h + 0.15f * w);
    snow(w);
}

void capricorn_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xDCD6BCu, 0x5C7C90u, 0x4C8060u, 0x9C7848u};
    /* Climb slate blue, Echo bronze, Mist grey violet, Light snow */
    static const f32 tint[4][3] = {{30, 60, 100}, {110, 70, 20}, {70, 60, 110}, {120, 120, 110}};
    f32 w = C.age < 1.6f ? 1.0f - C.age / 1.6f : 0.0f;
    sc_palette_tint(&C.s, inks, 0.10f * w, tint, 0.4f * w, pal);
}
