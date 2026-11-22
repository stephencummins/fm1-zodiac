/* PISCES: the fishes, water. Gentle, dreaming, two currents in one sea: a pair of fish joined by a ribbon.
 * At a note both fish dart upward with a flick of their tails and a cloud of bubbles rises.
 * The fish are art/pic/pisces.jpg; the bubbles are drawn here. */
#include "scene.h"
#include "pic_pisces.h"

enum { K_PEARL, K_GREEN, K_INDIGO, K_CORAL };

static struct { Scene s; f32 dart, age; } P;

void pisces_init(void) { sc_init(&P.s, 139u); P.dart = 0.0f; P.age = 9.0f; }
void pisces_strike(void) { P.dart = 1.0f; P.age = 0.0f; }

static void bubbles(void)
{
    f32 h = P.s.p.heat, t = P.s.t;
    int n = (int)(5.0f + 22.0f * h);
    g_mode(G_MAX); g_ink(K_PEARL);
    for (int i = 0; i < n; i++) {                                   /* always a few rising */
        f32 u = t * (0.04f + 0.07f * hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 x = hash01((uint32_t)i * 29u + 5u) * 240.0f + 4.0f * sin_turns(t * 0.3f + (f32)i * 0.41f), y = 246.0f - 252.0f * u;
        g_ring(x, y, 1.5f + 2.0f * hash01((uint32_t)i * 17u), 1.0f, 0.3f + 0.5f * h);
    }
    if (P.age < 2.6f)                                               /* and a cloud from between the fish at a note */
        for (int i = 0; i < 14; i++) {
            f32 r1 = hash01((uint32_t)i * 37u + 2u), r2 = hash01((uint32_t)i * 41u + 6u), a = P.age * (0.6f + 0.6f * r1);
            f32 x = 120.0f + (r2 - 0.5f) * 34.0f + 6.0f * sin_turns(a * 0.9f + r1), y = 150.0f - 95.0f * a;
            g_ring(x, y, 1.5f + 2.5f * r1, 1.0f, sat(2.6f - P.age));
        }
}

void pisces_frame(f32 dt, const ZParams *want)
{
    sc_step(&P.s, dt, want);
    f32 t = P.s.t, h = P.s.p.heat;
    P.dart = maxf(0.0f, P.dart - dt * 0.75f); P.age += dt;
    f32 w = P.dart * P.dart * (3.0f - 2.0f * P.dart), fl = w * sin_turns(t * 3.2f);
    f32 idle = 0.006f * sin_turns(t * 0.11f);
    sc_begin(&P.s);
    sc_mist(&P.s, K_INDIGO);
    p_begin(pisces_pic);
    /* tails first, then the bodies over them */
    p_warp(pisces_pic, 84.0f, 176.0f, 34.0f, 20.0f, 0.0f, 0.02f * sin_turns(t * 0.21f) + 0.07f * fl, 0.0f, 0.0f, 0);
    p_warp(pisces_pic, 156.0f, 176.0f, 34.0f, 20.0f, 0.0f, -0.02f * sin_turns(t * 0.21f) - 0.07f * fl, 0.0f, 0.0f, 0);
    p_warp(pisces_pic, 84.0f, 104.0f, 44.0f, 62.0f, 0.03f * w, idle + 0.018f * fl, 0.0f, -7.0f * w, 0);
    p_warp(pisces_pic, 156.0f, 104.0f, 44.0f, 62.0f, 0.03f * w, -idle - 0.018f * fl, 0.0f, -7.0f * w, 0);
    g_blit(g_work, 1.0f + 0.008f * sin_turns(t * 0.05f), 0.0f, 0.5f + 2.4f * h, 0.018f, t * (0.08f + 0.2f * h), 0.95f + 0.15f * w);
    bubbles();
}

void pisces_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xE8ECD8u, 0x80D0B0u, 0x5870A0u, 0xE87078u};
    /* Dream indigo, Echo coral, Mist sea green, Light pearl */
    static const f32 tint[4][3] = {{30, 30, 120}, {130, 50, 60}, {20, 100, 90}, {120, 120, 110}};
    sc_palette_tint(&P.s, inks, 0.12f * P.dart, tint, 0.5f * P.dart, pal);
}
