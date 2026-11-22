/* ARIES: the ram, fire. The ram is art/aries.svg; the fire, the embers and the mist are drawn here. */
#include "zodiac.h"
#include "sign.h"
#include "mathx.h"
#include "art_aries.h"

enum { INK_GOLD, INK_RED, INK_AMBER, INK_BLUE };

#define NEMBER 56
typedef struct { f32 x, y, vx, vy, age, life; } Ember;

static struct {
    f32 t;
    ZParams p;                   /* the hands' settings, followed smoothly */
    Ember em[NEMBER];
    f32 spawn;
    uint32_t seed;
    f32 blink;                   /* 0 open .. 1 shut */
    f32 next_blink;
} A;

static f32 rnd(void) { return hash01(A.seed++); }

void aries_init(void)
{
    A.t = 0.0f; A.seed = 7u; A.spawn = 0.0f; A.blink = 0.0f; A.next_blink = 3.0f;
    A.p.heat = 0.5f; A.p.echo = 0.0f; A.p.mist = 0.2f; A.p.light = 0.7f;
    for (int i = 0; i < NEMBER; i++) A.em[i].life = 0.0f;
}

/* one tongue of flame: rooted at (bx, by), h high, half as wide as b at the root */
static void tongue(int i, f32 bx, f32 by, f32 h, f32 b, int ink, f32 a, f32 edge)
{
    enum { N = 9 };
    f32 xy[(2 * N + 1) * 2];
    f32 lean = (noise1(A.t * 0.55f + (f32)i * 3.1f, 11u) - 0.5f) * 26.0f + (bx - 120.0f) * 0.10f;
    for (int k = 0; k <= N; k++) {
        f32 u = (f32)k / (f32)N, v = 1.0f - u;
        f32 cx = bx + lean * u * u + sin_turns(u * 1.2f - A.t * 0.7f + (f32)i * 0.37f) * 5.0f * u;
        f32 w = b * v * sqrtf_(v) * (1.0f + 0.30f * sin_turns(u * 1.6f - A.t * 0.9f + (f32)i * 0.21f)) + 0.2f * v;
        f32 y = by - h * u;
        xy[2 * k] = cx - w; xy[2 * k + 1] = y;
        if (k < N) { xy[2 * (2 * N - k)] = cx + w; xy[2 * (2 * N - k) + 1] = y; }
    }
    g_ink(ink); g_mode(G_MAX);
    g_fill(xy, 2 * N + 1, a);
    if (edge > 0.0f) { g_ink(INK_GOLD); g_poly(xy, 2 * N + 1, 0, 1.0f, edge); }
}

static void fire(void)
{
    f32 heat = A.p.heat;
    g_ink(INK_RED);
    g_glow(120.0f, 128.0f, 84.0f + 30.0f * heat, 0.16f + 0.30f * heat);
    for (int i = 0; i < 13; i++) {
        f32 u = (f32)i / 12.0f, c = 1.0f - absf(u - 0.5f) * 2.0f;          /* 0 at the sides, 1 in the middle */
        f32 bx = 14.0f + 212.0f * u + (hash01((uint32_t)i * 31u) - 0.5f) * 8.0f;
        f32 flick = 0.72f + 0.56f * noise1(A.t * 1.6f + (f32)i * 5.7f, 3u);
        f32 h = (46.0f + 150.0f * c * (0.6f + 0.4f * hash01((uint32_t)i * 17u + 5u))) * (0.30f + 0.80f * heat) * flick;
        f32 b = 13.0f + 9.0f * c;
        tongue(i, bx, 244.0f, h, b, INK_RED, 0.42f + 0.22f * heat, 0.20f + 0.25f * heat);
        tongue(i + 40, bx + 2.0f, 244.0f, h * 0.58f, b * 0.55f, INK_AMBER, 0.50f + 0.30f * heat, 0.0f);
    }
}

static void embers(f32 dt)
{
    f32 heat = A.p.heat;
    A.spawn += dt * (2.0f + 26.0f * heat * heat);
    for (int i = 0; i < NEMBER; i++) {
        Ember *e = &A.em[i];
        if (e->life <= 0.0f) {
            if (A.spawn < 1.0f) continue;
            A.spawn -= 1.0f;
            e->x = 20.0f + rnd() * 200.0f; e->y = 236.0f - rnd() * 60.0f;
            e->vx = (rnd() - 0.5f) * 10.0f; e->vy = -(14.0f + rnd() * 30.0f) * (0.6f + 0.8f * heat);
            e->age = 0.0f; e->life = 2.5f + rnd() * 4.0f;
            continue;
        }
        e->age += dt;
        if (e->age >= e->life || e->y < -4.0f) { e->life = 0.0f; continue; }
        e->x += (e->vx + sin_turns(e->age * 0.35f + (f32)i * 0.19f) * 9.0f) * dt;
        e->y += e->vy * dt;
        f32 u = e->age / e->life, a = (u < 0.12f ? u / 0.12f : 1.0f - (u - 0.12f) / 0.88f);
        a *= 0.55f + 0.45f * noise1(e->age * 5.0f, (uint32_t)i);            /* a glimmer */
        g_mode(G_MAX); g_ink(i & 3 ? INK_AMBER : INK_GOLD);
        if (i % 5 == 0) g_disc(e->x, e->y, 1.1f, a); else g_dot(e->x, e->y, a);
    }
}

static void mist(void)
{
    f32 m = A.p.mist;
    if (m < 0.01f) return;
    g_mode(G_MAX); g_ink(INK_BLUE);
    for (int i = 0; i < 6; i++) {                                           /* slow banks of it */
        f32 sp = 3.0f + 4.0f * hash01((uint32_t)i * 7u + 1u);
        f32 x = hash01((uint32_t)i * 13u) * 340.0f + A.t * sp;
        x = x - 340.0f * (f32)(int)(x / 340.0f) - 50.0f;
        f32 y = 70.0f + 150.0f * hash01((uint32_t)i * 29u + 3u) + sin_turns(A.t * 0.03f + (f32)i * 0.3f) * 12.0f;
        g_glow(x, y, 46.0f + 34.0f * hash01((uint32_t)i * 5u + 9u), m * (0.30f + 0.25f * noise1(A.t * 0.2f + (f32)i, 77u)));
    }
    int n = (int)(m * 260.0f);                                              /* and grains, as a pen would stipple it */
    for (int i = 0; i < n; i++) {
        f32 x = hash01((uint32_t)i * 3u + 100u) * 240.0f + A.t * (1.5f + 3.0f * hash01((uint32_t)i + 500u));
        x -= 240.0f * (f32)(int)(x / 240.0f);
        f32 y = hash01((uint32_t)i * 3u + 101u) * 240.0f + sin_turns(A.t * 0.05f + (f32)i * 0.13f) * 4.0f;
        g_dot(x, y, m * (0.35f + 0.5f * noise1(A.t * 0.4f + (f32)i * 0.7f, 9u)));
    }
}

/* how the ram's parts move */
static void rot(f32 *xy, int n, f32 cx, f32 cy, f32 turns)
{
    f32 c = cos_turns(turns), s = sin_turns(turns);
    for (int k = 0; k < n; k++) {
        f32 x = xy[2 * k] - cx, y = xy[2 * k + 1] - cy;
        xy[2 * k] = cx + x * c - y * s; xy[2 * k + 1] = cy + x * s + y * c;
    }
}

static void ram_move(int g, f32 *xy, int n, f32 *gain, void *ctx)
{
    (void)ctx;
    f32 heat = A.p.heat, breath = sin_turns(A.t * 0.11f);
    if (g == ARIES_G_HORNL) rot(xy, n, 58.0f, 112.0f, 0.004f * breath);
    else if (g == ARIES_G_HORNR) rot(xy, n, 182.0f, 112.0f, -0.004f * breath);
    else if (g == ARIES_G_EARL || g == ARIES_G_EARR) {
        f32 fl = noise1(A.t * 0.35f + (g == ARIES_G_EARL ? 0.0f : 9.0f), 21u);
        fl = fl > 0.72f ? (fl - 0.72f) * 0.11f : 0.0f;                       /* now and then, a flick */
        if (g == ARIES_G_EARL) rot(xy, n, 97.0f, 133.0f, fl + 0.003f * breath);
        else rot(xy, n, 143.0f, 133.0f, -fl - 0.003f * breath);
    } else if (g == ARIES_G_EYEL || g == ARIES_G_EYER) {
        for (int k = 0; k < n; k++) xy[2 * k + 1] = 125.0f + (xy[2 * k + 1] - 125.0f) * (1.0f - 0.92f * A.blink);
        *gain = 0.55f + 0.55f * heat;
    } else if (g == ARIES_G_JEWEL) *gain = 0.6f + 0.4f * sin_turns(A.t * 0.2f) * heat + 0.3f * heat;
    else if (g == ARIES_G_NECK) *gain = 0.8f + 0.5f * heat * noise1(A.t * 1.3f + xy[0] * 0.05f, 5u);   /* firelight on the fleece */
    for (int k = 0; k < n; k++) {                                            /* breathing, and the air's shimmer in the heat */
        f32 y = xy[2 * k + 1];
        xy[2 * k + 1] = y - 0.9f * breath * (240.0f - y) * (1.0f / 240.0f) * 1.6f;
        xy[2 * k] += heat * heat * 0.9f * sin_turns(y * 0.021f - A.t * 0.8f);
    }
}

void aries_frame(f32 dt, const ZParams *want)
{
    f32 k = 1.0f - 1.0f / (1.0f + dt * 5.0f);                               /* follow the hands in about a fifth of a second */
    A.p.heat += (sat(want->heat) - A.p.heat) * k;
    A.p.echo += (sat(want->echo) - A.p.echo) * k;
    A.p.mist += (sat(want->mist) - A.p.mist) * k;
    A.p.light += (sat(want->light) - A.p.light) * k;
    A.t += dt;

    A.next_blink -= dt;
    if (A.next_blink < 0.0f) A.next_blink = 4.0f + rnd() * 7.0f;
    f32 shut = A.next_blink < 0.16f ? 1.0f : 0.0f;
    A.blink += (shut - A.blink) * (1.0f - 1.0f / (1.0f + dt * 30.0f));

    if (A.p.echo > 0.02f) g_fade((int)(256.0f * (0.62f + 0.36f * A.p.echo))); else g_clear();
    fire();
    mist();
    sign_draw(&aries_art, ram_move, 0);
    embers(dt);
}

void aries_palette(uint16_t pal[256])
{
    /* LIGHT: low is embers in the dark, the gold gone to copper; high is full gold */
    f32 l = A.p.light, w = sat(l * 1.4f);
    uint32_t gold = ((uint32_t)(0xC8 + (int)(0x20 * w)) << 16) | ((uint32_t)(0x78 + (int)(0x61 * w)) << 8) | (uint32_t)(0x40 + (int)(0x68 * w));
    uint32_t inks[G_INKS] = {gold, 0xC0202Au, 0xF58A1Cu, 0x3048B8u};
    g_palette(inks, 0.30f + 0.85f * l, pal);
}
