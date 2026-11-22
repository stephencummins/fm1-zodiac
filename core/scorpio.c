/* SCORPIO: the scorpion, water. Intense, loyal, deep: a still jewel of a creature over dark water.
 * At a note its claws snip, its legs ripple, the tail's tip flicks and a drop falls from it into the deep.
 * The scorpion is art/pic/scorpio.jpg; the falling water and the rings are drawn here. */
#include "scene.h"
#include "pic_scorpio.h"

enum { K_BONE, K_CRIMSON, K_VIOLET, K_TEAL };

static struct { Scene s; f32 snap, age; } S;

void scorpio_init(void) { scorpio_pic_load(); sc_init(&S.s, 97u); S.snap = 0.0f; S.age = 9.0f; }
void scorpio_strike(void) { S.snap = 1.0f; S.age = 0.0f; }

static void deep(void)
{
    f32 h = S.s.p.heat, t = S.s.t;
    g_mode(G_MAX);
    for (int k = 0; k < 5; k++) {                                   /* slow rings out from under the frame */
        f32 u = t * (0.03f + 0.07f * h) + (f32)k / 5.0f; u -= (f32)(int)u;
        g_ink(k & 1 ? K_VIOLET : K_TEAL); g_ring(CX, 120.0f, 96.0f + 80.0f * u, 1.3f, (1.0f - u) * (0.25f + 0.6f * h));
    }
    int n = (int)(4.0f + 30.0f * h);
    for (int i = 0; i < n; i++) {                                   /* long drops falling */
        f32 u = t * (0.10f + 0.12f * hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 x = hash01((uint32_t)i * 29u + 5u) * 240.0f, y = -20.0f + 280.0f * u * u;
        g_ink(i % 3 == 0 ? K_CRIMSON : i % 3 == 1 ? K_VIOLET : K_TEAL);
        g_line(x, y - 9.0f, x, y, 1.0f, 0.5f); g_disc(x, y + 1.0f, 1.6f, 0.8f);
    }
}

void scorpio_frame(f32 dt, const ZParams *want)
{
    sc_step(&S.s, dt, want);
    f32 t = S.s.t, h = S.s.p.heat;
    S.snap = maxf(0.0f, S.snap - dt * 0.8f); S.age += dt;
    f32 w = S.snap * S.snap * (3.0f - 2.0f * S.snap), sn = w * maxf(0.0f, sin_turns(t * 3.4f));
    sc_begin(&S.s);
    deep();
    sc_mist(&S.s, K_VIOLET);
    p_begin(scorpio_pic);
    /* legs first, then the claws over them; the red tail is held still under the claws, the claws under the tip */
    p_warp(scorpio_pic, 88.0f, 118.0f, 24.0f, 36.0f, 0.0f, 0.012f * w * sin_turns(t * 5.5f), 1.6f * w * sin_turns(t * 5.5f + 0.25f), 0.0f, 3);
    p_warp(scorpio_pic, 150.0f, 118.0f, 22.0f, 36.0f, 0.0f, -0.012f * w * sin_turns(t * 5.5f + 0.3f), -1.6f * w * sin_turns(t * 5.5f + 0.55f), 0.0f, 3);
    p_warp(scorpio_pic, 86.0f, 60.0f, 21.0f, 21.0f, 0.10f * sn, 0.004f * sin_turns(t * 0.17f) + 0.035f * sn, 0.0f, 0.0f, 2 | 8);
    p_warp(scorpio_pic, 152.0f, 58.0f, 19.0f, 19.0f, 0.10f * sn, -0.004f * sin_turns(t * 0.15f) - 0.035f * sn, 0.0f, 0.0f, 2 | 8);
    p_warp(scorpio_pic, 121.0f, 54.0f, 15.0f, 15.0f, 0.12f * w, 0.004f * sin_turns(t * 0.12f) + 0.03f * w * sin_turns(t * 6.0f), 0.0f, 3.0f * w * maxf(0.0f, sin_turns(S.age * 1.2f)), 4 | 8);
    g_blit(g_work, 1.0f + 0.008f * sin_turns(t * 0.06f), 0.0f, 0.3f + 2.2f * h, 0.022f, t * (0.10f + 0.2f * h), 0.95f + 0.15f * w);
    if (S.age < 2.2f) {                                             /* the drop under it falls, and a ring opens where it lands */
        f32 y = 198.0f + 150.0f * S.age * S.age;
        g_mode(G_PAINT); g_ink(K_CRIMSON);
        if (y < 232.0f) { g_disc(120.0f, y, 2.6f, 1.0f); g_line(120.0f, y - 6.0f, 120.0f, y, 1.4f, 1.0f); }
        else { f32 u = (S.age - 0.48f) / 1.72f; g_mode(G_MAX); g_ink(K_BONE); g_ring(120.0f, 120.0f, 20.0f + 150.0f * u, 1.5f, 1.0f - u); }
    }
}

void scorpio_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xECE4D4u, 0xC82840u, 0x7038B0u, 0x148478u};
    /* Depth midnight blue, Echo crimson, Mist violet, Light sea green */
    static const f32 tint[4][3] = {{8, 24, 120}, {120, 10, 40}, {70, 30, 130}, {40, 120, 110}};
    sc_palette_tint(&S.s, inks, 0.12f * S.snap, tint, 0.5f * S.snap, pal);
}
