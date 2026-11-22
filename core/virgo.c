/* VIRGO: the maiden, earth. Careful, modest, patient: she holds the harvest and everything is in its place.
 * At a note the wheat rustles, the sheaf in her hands lifts and grain flies up from it.
 * The maiden is art/pic/virgo.jpg; the falling grain is drawn here. */
#include "scene.h"
#include "pic_virgo.h"

enum { K_CREAM, K_GOLD, K_SAGE, K_INDIGO };

static struct { Scene s; f32 rustle, age; } V;

void virgo_init(void) { virgo_pic_load(); sc_init(&V.s, 71u); V.rustle = 0.0f; V.age = 9.0f; }
void virgo_strike(void) { V.rustle = 1.0f; V.age = 0.0f; }

static void grain(void)
{
    f32 h = V.s.p.heat, t = V.s.t;
    int n = (int)(8.0f + 60.0f * h);
    g_mode(G_MAX);
    for (int i = 0; i < n; i++) {                                   /* grain and chaff drifting down */
        f32 u = t * (0.035f + 0.05f * hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 x = hash01((uint32_t)i * 29u + 5u) * 240.0f + sin_turns(t * 0.13f + (f32)i * 0.3f) * 9.0f, y = -4.0f + 250.0f * u;
        f32 tw = absf(sin_turns(t * 0.4f + (f32)i * 0.17f));
        g_ink(i % 4 == 0 ? K_SAGE : K_GOLD);
        g_line(x - 1.5f * tw, y - 1.0f, x + 1.5f * tw, y + 1.0f, 1.3f, 0.45f + 0.5f * h);
    }
}

static void burst(void)                                             /* grain thrown up from the sheaf */
{
    if (V.age > 1.8f) return;
    f32 a = V.age, fade = 1.0f - a / 1.8f;
    g_mode(G_PAINT);
    for (int i = 0; i < 22; i++) {
        f32 vx = (i & 2 ? 1.0f : -1.0f) * (34.0f + 70.0f * hash01((uint32_t)i * 17u + 2u)), vy = -(60.0f + 80.0f * hash01((uint32_t)i * 5u + 9u));
        f32 x = 120.0f + vx * a, y = 150.0f + vy * a + 90.0f * a * a;
        g_ink(i & 1 ? K_GOLD : K_CREAM); g_disc(x, y, 1.6f, fade);
    }
}

void virgo_frame(f32 dt, const ZParams *want)
{
    sc_step(&V.s, dt, want);
    f32 t = V.s.t;
    V.rustle = maxf(0.0f, V.rustle - dt * 0.7f); V.age += dt;
    f32 w = V.rustle * V.rustle * (3.0f - 2.0f * V.rustle), br = 0.004f + 0.006f * V.s.p.heat;
    sc_begin(&V.s);
    grain();
    sc_mist(&V.s, K_SAGE);
    p_begin(virgo_pic);
    for (int i = 0; i < 7; i++) {                                   /* the crown of ears and the leaves at her sides */
        static const f32 at[7][4] = {{120, 40, 15, 17}, {92, 50, 15, 15}, {148, 50, 15, 15}, {68, 78, 17, 14}, {172, 78, 17, 14},
                                     {58, 132, 20, 30}, {182, 132, 20, 30}};
        f32 ph = t * 5.0f + 0.19f * (f32)i;
        p_warp(virgo_pic, at[i][0], at[i][1], at[i][2], at[i][3], 0.06f * w * sin_turns(ph * 0.5f),
               br * sin_turns(t * 0.11f + 0.2f * (f32)i) + 0.03f * w * sin_turns(ph), 1.5f * w * sin_turns(ph + 0.25f), 0.0f, 8);
    }
    /* the sheaf lifts and sways in her hands */
    p_warp(virgo_pic, 120.0f, 166.0f, 15.0f, 24.0f, 0.10f * w, 0.02f * w * sin_turns(t * 3.6f), 0.0f, -4.0f * w, 0);
    g_blit(g_work, 1.0f + 0.006f * sin_turns(t * 0.07f) + 0.012f * w, -400.0f, 0.0f, 0.0f, 0.0f, 0.95f + 0.15f * w);
    burst();
}

void virgo_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xF5ECCEu, 0xD8A838u, 0x7C9C6Cu, 0x2C2C78u};
    /* Harvest warm gold, Echo deep green, Mist dusk blue, Light straw */
    static const f32 tint[4][3] = {{130, 84, 10}, {20, 90, 40}, {40, 50, 120}, {140, 124, 60}};
    sc_palette_tint(&V.s, inks, 0.12f * V.rustle, tint, 0.6f * V.rustle, pal);
}
