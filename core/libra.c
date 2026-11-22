/* LIBRA: the scales, air. Fair, graceful, always weighing: sun in one pan, moon in the other, level.
 * At a note the scales tip one way, then the other, and come slowly back to balance.
 * The scales are art/pic/libra.jpg; the wind is drawn here. */
#include "scene.h"
#include "pic_libra.h"

enum { K_CREAM, K_ROSE, K_TEAL, K_LAV };

static struct { Scene s; f32 age, side, ting; } B;

void libra_init(void) { libra_pic_load(); sc_init(&B.s, 83u); B.age = 99.0f; B.side = 1.0f; B.ting = 0.0f; }
void libra_strike(void) { B.age = 0.0f; B.side = -B.side; B.ting = 1.0f; }      /* each note tips it the other way first */

static void wind(void)
{
    f32 h = B.s.p.heat, t = B.s.t;
    int n = (int)(4.0f + 30.0f * h);
    g_mode(G_MAX);
    for (int i = 0; i < n; i++) {
        f32 u = t * (0.06f + 0.16f * h) * (0.6f + 0.8f * hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 x = -40.0f + 320.0f * u, y = hash01((uint32_t)i * 29u + 5u) * 240.0f, len = 12.0f + 26.0f * h;
        f32 xy[10];
        for (int k = 0; k < 5; k++) { f32 q = (f32)k / 4.0f; xy[2 * k] = x - len * q; xy[2 * k + 1] = y + 3.0f * sin_turns(q * 0.7f + t * 0.3f + (f32)i * 0.21f); }
        g_ink(i % 3 == 0 ? K_TEAL : i % 3 == 1 ? K_LAV : K_CREAM);
        g_poly(xy, 5, 0, 1.2f, (0.3f + 0.5f * h) * 4.0f * u * (1.0f - u));
    }
}

void libra_frame(f32 dt, const ZParams *want)
{
    sc_step(&B.s, dt, want);
    f32 t = B.s.t, h = B.s.p.heat;
    B.age += dt; B.ting = maxf(0.0f, B.ting - dt * 1.4f);
    /* how far the beam leans: a swing that dies away after a note, and a little from the wind */
    f32 damp = B.age < 12.0f ? 1.0f / (1.0f + B.age * B.age * 0.9f) : 0.0f;
    f32 lean = B.side * 0.115f * damp * sin_turns(B.age * 0.85f) + (0.006f + 0.022f * h) * (noise1(t * 0.23f, 17u) - 0.5f) * 2.0f;
    f32 drop = 62.0f * lean;
    sc_begin(&B.s);
    wind();
    sc_mist(&B.s, K_LAV);
    p_begin(libra_pic);
    p_warp(libra_pic, 120.0f, 86.0f, 84.0f, 11.0f, 0.0f, lean / 6.2832f, 0.0f, 0.0f, 0);
    p_warp(libra_pic, 65.0f, 136.0f, 31.0f, 50.0f, 0.0f, 0.0f, 0.0f, -drop, 0);
    p_warp(libra_pic, 175.0f, 136.0f, 31.0f, 50.0f, 0.0f, 0.0f, 0.0f, drop, 0);
    /* the feather stirs */
    p_warp(libra_pic, 120.0f, 52.0f, 13.0f, 26.0f, 0.0f, (0.006f + 0.016f * h) * sin_turns(t * 0.21f) + 0.02f * B.ting * sin_turns(t * 3.0f), 0.0f, 0.0f, 0);
    g_blit(g_work, 1.0f + 0.006f * sin_turns(t * 0.06f), -400.0f, 0.0f, 0.0f, 0.0f, 0.95f + 0.15f * B.ting);
    if (B.ting > 0.0f) { g_mode(G_MAX); g_ink(K_CREAM); g_ring(120.0f, 86.0f, 4.0f + 40.0f * (1.0f - B.ting), 1.2f, B.ting); }
}

void libra_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xF4ECCCu, 0xE0607Cu, 0x30C0B8u, 0x9C78D0u};
    /* Breeze sky turquoise, Echo rose, Mist violet, Light dawn */
    static const f32 tint[4][3] = {{10, 100, 120}, {130, 30, 70}, {70, 44, 130}, {140, 110, 90}};
    sc_palette_tint(&B.s, inks, 0.12f * B.ting, tint, 0.5f * B.ting, pal);
}
