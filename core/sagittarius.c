/* SAGITTARIUS: the archer, fire. Free, honest, aimed at the far horizon: a drawn bow, one arrow pointing up.
 * At a note the arrow is drawn back and let fly, and a new one comes up onto the string.
 * The bow is art/pic/sagittarius.jpg; the sparks are drawn here. */
#include "scene.h"
#include "pic_sagittarius.h"

enum { K_IVORY, K_FLAME, K_RED, K_INDIGO };

static struct { Scene s; f32 age, flash; } A;

void sagittarius_init(void) { sagittarius_pic_load(); sc_init(&A.s, 113u); A.age = 9.0f; A.flash = 0.0f; }
void sagittarius_strike(void) { if (A.age > 0.8f) A.age = 0.0f; }      /* not while one is still on its way */

static void sparks(void)
{
    f32 h = A.s.p.heat, t = A.s.t;
    int n = (int)(6.0f + 34.0f * h);
    g_mode(G_MAX);
    for (int i = 0; i < n; i++) {                                   /* sparks flying outward, like the rays */
        f32 u = t * (0.10f + 0.30f * h) * (0.5f + hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 a = hash01((uint32_t)i * 29u + 5u), c = cos_turns(a), s = sin_turns(a), r = 96.0f + 80.0f * u;
        g_ink(i % 3 == 0 ? K_IVORY : i % 3 == 1 ? K_FLAME : K_RED);
        g_line(CX + c * r, 120.0f + s * r, CX + c * (r + 5.0f + 8.0f * h), 120.0f + s * (r + 5.0f + 8.0f * h), 1.1f, (1.0f - u) * (0.4f + 0.6f * h));
    }
}

void sagittarius_frame(f32 dt, const ZParams *want)
{
    sc_step(&A.s, dt, want);
    f32 t = A.s.t, h = A.s.p.heat, a = A.age;
    A.age += dt; A.flash = maxf(0.0f, A.flash - dt * 1.6f);
    if (a < 0.30f && A.age >= 0.30f) A.flash = 1.0f;                /* the moment it leaves the string */
    /* where the arrow is: 0 at rest; drawn back a little; gone up and away; a new one rising from below */
    int dy = 0, gone = 0;
    if (a < 0.30f) { f32 u = a / 0.30f; dy = (int)(7.0f * u * u * (3.0f - 2.0f * u)); }
    else if (a < 0.72f) { f32 u = (a - 0.30f) / 0.42f; dy = (int)(7.0f - 270.0f * u * u); }
    else if (a < 1.25f) gone = 1;
    else if (a < 2.0f) { f32 u = 1.0f - (a - 1.25f) / 0.75f; dy = (int)(150.0f * u * u * u); }
    else dy = (int)(0.5f + 0.8f * sin_turns(t * 0.11f));            /* held: the faintest tremble */
    sc_begin(&A.s);
    sparks();
    sc_mist(&A.s, K_RED);
    p_begin(sagittarius_pic);
    if (dy != 0 || gone) {
        p_fill(103, 58, 35, 35, 0x00 | 63);                        /* the bare field behind the arrow, above the bow */
        p_fill(103, 106, 35, 94, 0x00 | 63);                        /* and below it */
        p_fill(115, 93, 10, 13, 0);                                 /* the shaft out of the grip... */
        p_fill(115, 96, 10, 7, 0xC0 | 63);                          /* ...and the grip made whole; it stays with the bow */
        if (!gone)                                                  /* the arrow alone, without the field round it */
            for (int y = 58; y < 200; y++) {
                int Y = y + dy; if (Y < 0 || Y >= 200) continue;
                for (int x = (y >= 93 && y < 106) ? 116 : 103, xe = (y >= 93 && y < 106) ? 124 : 138; x < xe; x++) { uint8_t v = sagittarius_pic[y * GW + x]; if (v >> 6 != K_IVORY && v) g_work[Y * GW + x] = v; }
            }
    }
    /* the limbs of the bow spring as it looses */
    f32 tw = A.flash * sin_turns(a * 5.0f);
    p_warp(sagittarius_pic, 62.0f, 108.0f, 40.0f, 24.0f, 0.0f, 0.012f * tw, 0.0f, -2.0f * tw, 0);
    p_warp(sagittarius_pic, 178.0f, 108.0f, 40.0f, 24.0f, 0.0f, -0.012f * tw, 0.0f, -2.0f * tw, 0);
    /* the star it aims at */
    p_warp(sagittarius_pic, 120.0f, 45.0f, 13.0f, 13.0f, 0.08f * sin_turns(t * 0.3f) + 0.35f * A.flash, 0.0f, 0.0f, 0.0f, 0);
    g_blit(g_work, 1.0f + 0.006f * sin_turns(t * 0.07f), 0.0f, 0.2f + 0.9f * h, 0.03f, t * 0.5f, 0.95f + 0.2f * A.flash);
    if (dy != 0 || gone) {                                          /* the string, to where the nock is now */
        f32 ny = a < 0.30f ? 172.0f + (f32)dy : 172.0f + 3.0f * A.flash * sin_turns(a * 6.0f);
        g_mode(G_PAINT); g_ink(K_INDIGO);
        g_line(100.0f, 157.0f, 120.0f, ny, 4.5f, 1.0f); g_line(140.0f, 157.0f, 120.0f, ny, 4.5f, 1.0f);
    }
    if (A.flash > 0.0f) { g_mode(G_MAX); g_ink(K_FLAME); g_ring(120.0f, 45.0f, 6.0f + 60.0f * (1.0f - A.flash), 1.4f, A.flash); }
}

void sagittarius_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xF0E0C0u, 0xF06828u, 0xC82850u, 0x2C2468u};
    /* Flight ember orange, Echo magenta, Mist indigo, Light gold */
    static const f32 tint[4][3] = {{130, 50, 8}, {120, 16, 70}, {40, 30, 120}, {140, 110, 40}};
    sc_palette_tint(&A.s, inks, 0.12f * A.flash, tint, 0.5f * A.flash, pal);
}
