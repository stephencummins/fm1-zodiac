/* CANCER: the crab, water. Guarded, moody, ruled by the Moon, happiest at home: it sidles about under a moon,
 * snipping its claws, and at a strike it pulls everything in behind them, then peeps out, eyes first.
 * The crab is art/cancer.svg; the moon, the tide, the ripples and the bubbles are drawn here. */
#include "scene.h"
#include "sign.h"
#include "pic_cancer.h"

enum { K_LINE, K_CORAL, K_TEAL, K_BLUE };

#define NBUB 22
typedef struct { f32 x, y, age, life, r; } Bub;

static struct {
    Scene s;
    f32 hide, hide_t, next_start;
    f32 xoff, vel, yw;           /* where it has sidled to, how fast, and the water's level */
    f32 wig;                     /* 1 just after a note, falling: how hard the claws and legs wiggle */
    f32 splash;                  /* 0 .. 1 a ring running out; < 0 none */
    Bub b[NBUB]; f32 spawn;
} C;

void cancer_init(void)
{
    sc_init(&C.s, 37u);
    C.hide = 0.0f; C.hide_t = -1.0f; C.next_start = 19.0f; C.xoff = 0.0f; C.vel = 0.0f; C.yw = 180.0f; C.splash = -1.0f; C.spawn = 0.0f; C.wig = 0.0f;
    for (int i = 0; i < NBUB; i++) C.b[i].life = 0.0f;
}

void cancer_strike(void) { C.wig = 1.0f; C.splash = 0.0f; }

static f32 hex_r(f32 th)                 /* the frame's radius in direction th, as a part of its corner radius */
{
    f32 a = (th - 0.25f) * 6.0f; a -= (f32)(int)a; if (a < 0.0f) a += 1.0f;
    return 0.8660254f / cos_turns((a - 0.5f) / 6.0f);
}

static void ring(f32 r, f32 wob, f32 a)
{
    enum { N = 48 };
    f32 xy[N * 2];
    for (int k = 0; k < N; k++) {
        f32 th = (f32)k / (f32)N, rr = r * hex_r(th) + wob * sin_turns(th * 7.0f + C.s.t * 0.4f + r * 0.02f);
        xy[2 * k] = CX + cos_turns(th) * rr; xy[2 * k + 1] = CY + sin_turns(th) * rr;
    }
    g_poly(xy, N, 1, 1.3f, a);
}

static void ripples(void)
{
    f32 tide = C.s.p.heat, t = C.s.t;
    enum { NR = 6 };
    g_mode(G_MAX);
    for (int k = 0; k < NR; k++) {
        f32 u = t * (0.05f + 0.10f * tide) + (f32)k / (f32)NR; u -= (f32)(int)u;
        g_ink(k & 1 ? K_TEAL : K_BLUE);
        ring(HEXR + 4.0f + 62.0f * u, 1.0f + 5.0f * u, (1.0f - u) * (0.35f + 0.60f * tide));
    }
    if (C.splash >= 0.0f) { g_ink(K_LINE); ring(HEXR + 2.0f + 70.0f * C.splash, 6.0f * C.splash, 1.0f - C.splash); }
}

static void bubbles(f32 dt)
{
    if (C.yw < 150.0f) C.spawn += dt * (1.2f + 3.0f * C.s.p.heat + 10.0f * C.hide);
    g_mode(G_MAX);
    for (int i = 0; i < NBUB; i++) {
        Bub *b = &C.b[i];
        if (b->life <= 0.0f) {
            if (C.spawn < 1.0f) continue;
            C.spawn -= 1.0f;
            b->x = CX + C.xoff + (sc_rnd(&C.s) - 0.5f) * 16.0f; b->y = 160.0f; b->age = 0.0f; b->life = 5.0f; b->r = 1.0f + sc_rnd(&C.s) * 1.8f;
            continue;
        }
        b->age += dt;
        b->y -= (16.0f + 8.0f * b->r) * dt; b->x += sin_turns(b->age * 0.8f + (f32)i * 0.3f) * 10.0f * dt;
        if (b->y < C.yw + 2.0f || b->age > b->life) { b->life = 0.0f; continue; }
        g_ink(i & 1 ? K_LINE : K_TEAL); g_ring(b->x, b->y, b->r, 0.8f, 0.8f);
    }
}

void cancer_frame(f32 dt, const ZParams *want)
{
    sc_step(&C.s, dt, want);
    f32 t = C.s.t;
    C.next_start -= dt;
    if (C.next_start < 0.0f) { C.next_start = 16.0f + sc_rnd(&C.s) * 20.0f; C.hide_t = 0.0f; }     /* it is easily startled */
    if (C.hide_t >= 0.0f) { C.hide_t += dt; if (C.hide_t > 1.5f) C.hide_t = -1.0f; }
    C.hide = sc_follow(C.hide, C.hide_t >= 0.0f ? 1.0f : 0.0f, dt, 22.0f, 0.9f);
    if (C.splash >= 0.0f) { C.splash += dt * 1.1f; if (C.splash >= 1.0f) C.splash = -1.0f; }
    f32 to = 14.0f * (noise1(t * 0.11f, 91u) - 0.5f) * (1.0f - C.hide);
    C.vel = (to - C.xoff) * 1.2f; C.xoff += C.vel * dt;
    C.yw = 212.0f - 106.0f * C.s.p.heat + 3.0f * sin_turns(t * 0.05f);

    sc_begin(&C.s);
    ripples();
    sc_mist(&C.s, K_BLUE);
    /* a note sets the claws and the legs wiggling; at rest they only stir. The frame's pale line stays put */
    C.wig = maxf(0.0f, C.wig - dt * 0.8f);
    f32 w = C.wig * C.wig * (3.0f - 2.0f * C.wig), idle = 0.006f;
    f32 cl = idle * sin_turns(t * 0.31f) + 0.035f * w * sin_turns(t * 4.6f), cr = idle * sin_turns(t * 0.27f + 0.4f) + 0.035f * w * sin_turns(t * 4.6f + 0.5f);
    p_begin(cancer_pic);
    p_warp(cancer_pic, 72.0f, 66.0f, 34.0f, 34.0f, 0.10f * w * sin_turns(t * 2.3f), cl, 0.0f, 0.0f, 1);
    p_warp(cancer_pic, 168.0f, 66.0f, 34.0f, 34.0f, 0.10f * w * sin_turns(t * 2.3f + 0.5f), -cr, 0.0f, 0.0f, 1);
    for (int i = 0; i < 4; i++) {
        static const f32 at[4][4] = {{66, 172, 30, 24}, {174, 172, 30, 24}, {43, 128, 15, 22}, {197, 128, 15, 22}};
        f32 ph = t * 6.2f + 0.27f * (f32)i;
        p_warp(cancer_pic, at[i][0], at[i][1], at[i][2], at[i][3], 0.0f, (i & 1 ? -1.0f : 1.0f) * 0.022f * w * sin_turns(ph),
               2.2f * w * sin_turns(ph + 0.25f), 1.4f * w * sin_turns(ph * 2.0f), 1);
    }
    /* the picture breathes with the tide and wavers as if seen through water; startled, it shrinks back */
    f32 tide = C.s.p.heat;
    g_blit(g_work, 1.0f + 0.012f * sin_turns(t * 0.07f) - 0.07f * C.hide, 60.0f,
           0.5f + 3.0f * tide, 0.022f, t * (0.12f + 0.25f * tide), 0.95f);
    bubbles(dt);
}

void cancer_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xDCE0E8u, 0xEC5C4Cu, 0x189C98u, 0x245C90u};
    /* Tide deep blue, Echo sea green, Mist violet, Light moon silver */
    static const f32 tint[4][3] = {{10, 40, 150}, {8, 110, 110}, {60, 40, 130}, {110, 120, 140}};
    f32 sp = C.splash >= 0.0f ? 1.0f - C.splash : 0.0f;
    sc_palette_tint(&C.s, inks, 0.2f * sp, tint, sp, pal);
}
