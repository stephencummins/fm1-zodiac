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
    f32 roar;                    /* 0 at rest .. 1 the mouth wide */
    f32 roar_t;                  /* seconds since it was struck; < 0 none */
    f32 next_roar;
    int burst;                   /* embers owed from the mouth */
} A;

static f32 rnd(void) { return hash01(A.seed++); }

void aries_init(void)
{
    A.t = 0.0f; A.seed = 7u; A.spawn = 0.0f; A.blink = 0.0f; A.next_blink = 3.0f;
    A.roar = 0.0f; A.roar_t = -1.0f; A.next_roar = 5.0f; A.burst = 0;
    A.p.heat = 0.5f; A.p.echo = 0.0f; A.p.mist = 0.2f; A.p.light = 0.7f;
    for (int i = 0; i < NEMBER; i++) A.em[i].life = 0.0f;
}

void aries_strike(void) { A.roar_t = 0.0f; A.burst = 22; }

/* one tongue of flame: rooted at (bx, by), reaching h along (dx, dy), half as wide as b at the root */
static void tongue(int i, f32 bx, f32 by, f32 dx, f32 dy, f32 h, f32 b, int ink, f32 a, f32 edge)
{
    enum { N = 9 };
    f32 xy[(2 * N + 1) * 2];
    f32 lean = (noise1(A.t * 0.7f + (f32)i * 3.1f, 11u) - 0.5f) * 0.42f * h;
    for (int k = 0; k <= N; k++) {
        f32 u = (f32)k / (f32)N, v = 1.0f - u;
        f32 off = lean * u * u + sin_turns(u * 1.2f - A.t * 0.9f + (f32)i * 0.37f) * 5.0f * u;
        f32 w = b * v * sqrtf_(v) * (1.0f + 0.30f * sin_turns(u * 1.6f - A.t * 1.1f + (f32)i * 0.21f)) + 0.2f * v;
        f32 cx = bx + dx * h * u - dy * off, cy = by + dy * h * u + dx * off;
        xy[2 * k] = cx + dy * w; xy[2 * k + 1] = cy - dx * w;
        if (k < N) { xy[2 * (2 * N - k)] = cx - dy * w; xy[2 * (2 * N - k) + 1] = cy + dx * w; }
    }
    g_ink(ink); g_mode(G_MAX);
    g_fill(xy, 2 * N + 1, a);
    if (edge > 0.0f) { g_ink(INK_GOLD); g_poly(xy, 2 * N + 1, 0, 1.0f, edge); }
}

#define CX 120.0f
#define CY 128.0f
#define HEXR 86.0f

/* a sunburst of flame from behind a six-sided frame */
static void fire(void)
{
    f32 heat = A.p.heat, push = 1.0f + 0.9f * A.roar;
    enum { NT = 22 };
    for (int i = 0; i < NT; i++) {
        f32 th = ((f32)i + 0.5f * hash01((uint32_t)i * 31u)) / (f32)NT + 0.004f * sin_turns(A.t * 0.07f + (f32)i * 0.3f);
        f32 dx = cos_turns(th), dy = sin_turns(th), up = maxf(0.0f, -dy);
        f32 flick = 0.60f + 0.80f * noise1(A.t * 1.9f + (f32)i * 5.7f, 3u);
        f32 h = (26.0f + 62.0f * heat * (0.6f + 0.4f * hash01((uint32_t)i * 17u + 5u))) * flick * push * (1.0f + 0.45f * up);
        f32 b = 10.0f + 5.0f * heat;
        tongue(i, CX + dx * 66.0f, CY + dy * 66.0f, dx, dy, h + 16.0f, b, INK_RED, 0.44f + 0.26f * heat, 0.22f + 0.30f * heat);
        tongue(i + 40, CX + dx * 66.0f, CY + dy * 66.0f, dx, dy, (h + 16.0f) * 0.66f, b * 0.55f, INK_AMBER, 0.50f + 0.35f * heat, 0.0f);
    }
    f32 hx[12], in[12];
    f32 beat = 1.0f + 0.012f * sin_turns(A.t * 0.11f) + 0.05f * A.roar;
    for (int k = 0; k < 6; k++) {
        f32 th = (f32)k / 6.0f + 0.25f;
        hx[2 * k] = CX + cos_turns(th) * HEXR * beat; hx[2 * k + 1] = CY + sin_turns(th) * HEXR * beat;
        in[2 * k] = CX + cos_turns(th) * (HEXR - 6.0f) * beat; in[2 * k + 1] = CY + sin_turns(th) * (HEXR - 6.0f) * beat;
    }
    g_mode(G_GROUND); g_fill(hx, 6, 1.0f);                    /* the frame hides the flames' roots */
    g_mode(G_MAX);
    g_glow(CX, CY + 10.0f, 70.0f, 0.22f + 0.30f * heat + 0.3f * A.roar);
    g_ink(INK_AMBER); g_fill(hx, 6, 0.03f + 0.05f * heat + 0.12f * A.roar);
    g_ink(INK_GOLD);
    for (int k = 0; k < 6; k++) g_line(CX, CY, in[2 * k], in[2 * k + 1], 0.8f, 0.10f + 0.25f * A.roar);
    g_poly(hx, 6, 1, 1.6f, 0.95f);
    g_poly(in, 6, 1, 0.8f, 0.45f);
}

static void embers(f32 dt)
{
    f32 heat = A.p.heat;
    A.spawn += dt * (2.0f + 26.0f * heat * heat);
    for (int i = 0; i < NEMBER; i++) {
        Ember *e = &A.em[i];
        if (e->life <= 0.0f) {
            if (A.burst > 0 && A.roar > 0.5f) {                               /* breathed out */
                A.burst--;
                f32 th = 0.25f + (rnd() - 0.5f) * 0.5f, sp = 40.0f + rnd() * 70.0f;
                e->x = CX + (rnd() - 0.5f) * 16.0f; e->y = 186.0f;
                e->vx = cos_turns(th) * sp; e->vy = sin_turns(th) * sp * 0.6f - 20.0f;
                e->age = 0.0f; e->life = 1.2f + rnd() * 1.6f;
                continue;
            }
            if (A.spawn < 1.0f) continue;
            A.spawn -= 1.0f;
            e->x = 10.0f + rnd() * 220.0f; e->y = 240.0f - rnd() * 90.0f;
            e->vx = (rnd() - 0.5f) * 10.0f; e->vy = -(14.0f + rnd() * 30.0f) * (0.6f + 0.8f * heat);
            e->age = 0.0f; e->life = 2.5f + rnd() * 4.0f;
            continue;
        }
        e->age += dt;
        if (e->age >= e->life || e->y < -4.0f) { e->life = 0.0f; continue; }
        e->vy -= 26.0f * dt * (e->vy > -30.0f ? 1.0f : 0.0f);                 /* hot air lifts them */
        e->x += (e->vx + sin_turns(e->age * 0.35f + (f32)i * 0.19f) * 9.0f) * dt;
        e->y += e->vy * dt;
        f32 u = e->age / e->life, a = (u < 0.12f ? u / 0.12f : 1.0f - (u - 0.12f) / 0.88f);
        a *= 0.55f + 0.45f * noise1(e->age * 5.0f, (uint32_t)i);
        g_mode(G_MAX); g_ink(i & 3 ? INK_AMBER : INK_GOLD);
        if (i % 4 == 0) g_disc(e->x, e->y, 1.2f, a); else g_dot(e->x, e->y, a);
    }
}

static void mist(void)
{
    f32 m = A.p.mist;
    if (m < 0.01f) return;
    g_mode(G_MAX); g_ink(INK_BLUE);
    for (int i = 0; i < 6; i++) {
        f32 sp = 3.0f + 4.0f * hash01((uint32_t)i * 7u + 1u);
        f32 x = hash01((uint32_t)i * 13u) * 340.0f + A.t * sp;
        x = x - 340.0f * (f32)(int)(x / 340.0f) - 50.0f;
        f32 y = 70.0f + 150.0f * hash01((uint32_t)i * 29u + 3u) + sin_turns(A.t * 0.03f + (f32)i * 0.3f) * 12.0f;
        g_glow(x, y, 46.0f + 34.0f * hash01((uint32_t)i * 5u + 9u), m * (0.30f + 0.25f * noise1(A.t * 0.2f + (f32)i, 77u)));
    }
    int n = (int)(m * 260.0f);
    for (int i = 0; i < n; i++) {
        f32 x = hash01((uint32_t)i * 3u + 100u) * 240.0f + A.t * (1.5f + 3.0f * hash01((uint32_t)i + 500u));
        x -= 240.0f * (f32)(int)(x / 240.0f);
        f32 y = hash01((uint32_t)i * 3u + 101u) * 240.0f + sin_turns(A.t * 0.05f + (f32)i * 0.13f) * 4.0f;
        g_dot(x, y, m * (0.35f + 0.5f * noise1(A.t * 0.4f + (f32)i * 0.7f, 9u)));
    }
}

static void rot(f32 *xy, int n, f32 cx, f32 cy, f32 turns)
{
    f32 c = cos_turns(turns), s = sin_turns(turns);
    for (int k = 0; k < n; k++) {
        f32 x = xy[2 * k] - cx, y = xy[2 * k + 1] - cy;
        xy[2 * k] = cx + x * c - y * s; xy[2 * k + 1] = cy + x * s + y * c;
    }
}
static void squash(f32 *xy, int n, f32 y0, f32 by) { for (int k = 0; k < n; k++) xy[2 * k + 1] = y0 + (xy[2 * k + 1] - y0) * by; }
static void drop(f32 *xy, int n, f32 d) { for (int k = 0; k < n; k++) xy[2 * k + 1] += d; }

/* how the ram's parts move */
static void ram_move(int g, f32 *xy, int n, f32 *gain, void *ctx)
{
    (void)ctx;
    f32 heat = A.p.heat, roar = A.roar, breath = sin_turns(A.t * 0.13f);
    f32 pant = heat * 0.10f * (0.5f + 0.5f * sin_turns(A.t * (0.35f + 0.5f * heat)));
    f32 open = sat(roar + pant), d = 21.0f * open;                          /* how far the jaw hangs */
    f32 glare = sat(0.35f + 0.5f * heat + roar);

    switch (g) {
    case ARIES_G_MAW:     for (int k = 0; k < n; k++) if (xy[2 * k + 1] > 182.0f) xy[2 * k + 1] += d;
                          *gain = 0.7f + 0.5f * roar; break;
    case ARIES_G_TONGUE:  drop(xy, n, d * 0.95f + 2.0f * roar * sin_turns(A.t * 5.0f)); break;
    case ARIES_G_TEETHLO: squash(xy, n, 178.0f, 0.25f + 0.75f * sat(open * 3.0f)); drop(xy, n, d); break;
    case ARIES_G_JAW:     drop(xy, n, d); break;
    case ARIES_G_TEETHUP: squash(xy, n, 175.0f, sat(open * 3.0f)); break;
    case ARIES_G_HORNL: case ARIES_G_HORNR: {
        f32 hx = g == ARIES_G_HORNL ? 58.0f : 182.0f, sg = g == ARIES_G_HORNL ? 1.0f : -1.0f;
        rot(xy, n, hx, 112.0f, sg * (0.004f * breath - 0.012f * roar));
        if (n == 6) {                                                        /* a cell: light runs down the spiral */
            f32 rx = xy[0] - hx, ry = xy[1] - 112.0f, r = sqrtf_(rx * rx + ry * ry);
            f32 w = sin_turns(r * 0.022f + A.t * (0.25f + 0.5f * heat));
            *gain = 0.62f + 0.42f * w * w + 0.6f * roar + 0.25f * heat;
        }
        break; }
    case ARIES_G_EARL: case ARIES_G_EARR: {
        f32 fl = noise1(A.t * 0.35f + (g == ARIES_G_EARL ? 0.0f : 9.0f), 21u);
        fl = fl > 0.68f ? (fl - 0.68f) * 0.12f : 0.0f;
        if (g == ARIES_G_EARL) rot(xy, n, 96.0f, 112.0f, fl + 0.003f * breath - 0.03f * roar);
        else rot(xy, n, 144.0f, 112.0f, -fl - 0.003f * breath + 0.03f * roar);
        break; }
    case ARIES_G_EYEL: case ARIES_G_EYER:
        squash(xy, n, 126.0f, (1.0f - 0.92f * A.blink) * (0.82f + 0.38f * roar - 0.12f * heat));
        *gain = 0.50f + 0.60f * glare + 0.12f * sin_turns(A.t * 0.9f) * heat; break;
    case ARIES_G_BROWL:   rot(xy, n, 116.0f, 126.0f, 0.012f * heat + 0.03f * roar); drop(xy, n, 0.8f * heat + 1.2f * roar); break;
    case ARIES_G_BROWR:   rot(xy, n, 124.0f, 126.0f, -0.012f * heat - 0.03f * roar); drop(xy, n, 0.8f * heat + 1.2f * roar); break;
    case ARIES_G_HEAD:    *gain = 0.85f + 0.25f * heat * noise1(A.t * 1.4f + xy[0] * 0.07f, 5u) + 0.35f * roar; break;
    }

    /* the whole head: sways, breathes, and comes at you when it roars, shaking */
    f32 sc = 1.0f + 0.016f * breath + 0.10f * roar;
    f32 sway = 0.006f * sin_turns(A.t * 0.045f) + 0.004f * (noise1(A.t * 0.5f, 31u) - 0.5f);
    f32 shx = roar * 1.6f * (noise1(A.t * 23.0f, 41u) - 0.5f), shy = roar * 1.6f * (noise1(A.t * 27.0f, 43u) - 0.5f);
    f32 c = cos_turns(sway), s = sin_turns(sway);
    for (int k = 0; k < n; k++) {
        f32 x = (xy[2 * k] - CX) * sc, y = (xy[2 * k + 1] - 150.0f) * sc;
        f32 xr = x * c - y * s, yr = x * s + y * c;
        xy[2 * k] = CX + xr + shx + heat * heat * 0.8f * sin_turns(yr * 0.021f - A.t * 0.8f);
        xy[2 * k + 1] = 150.0f + yr + shy - 4.0f * roar;
    }
}

void aries_frame(f32 dt, const ZParams *want)
{
    f32 k = 1.0f - 1.0f / (1.0f + dt * 5.0f);
    A.p.heat += (sat(want->heat) - A.p.heat) * k;
    A.p.echo += (sat(want->echo) - A.p.echo) * k;
    A.p.mist += (sat(want->mist) - A.p.mist) * k;
    A.p.light += (sat(want->light) - A.p.light) * k;
    A.t += dt;

    A.next_blink -= dt;
    if (A.next_blink < 0.0f) A.next_blink = 3.0f + rnd() * 6.0f;
    f32 shut = A.next_blink < 0.16f ? 1.0f : 0.0f;
    A.blink += (shut - A.blink) * (1.0f - 1.0f / (1.0f + dt * 30.0f));

    /* it roars by itself now and then (sooner when hot), and whenever it is struck */
    A.next_roar -= dt;
    if (A.next_roar < 0.0f) { A.next_roar = (7.0f + rnd() * 9.0f) * (1.3f - 0.9f * A.p.heat); aries_strike(); }
    f32 aim = 0.0f;
    if (A.roar_t >= 0.0f) {
        A.roar_t += dt;
        aim = A.roar_t < 1.1f ? 1.0f : 0.0f;
        if (A.roar_t > 2.6f) A.roar_t = -1.0f;
    }
    A.roar += (aim - A.roar) * (1.0f - 1.0f / (1.0f + dt * (aim > A.roar ? 16.0f : 3.2f)));

    if (A.p.echo > 0.02f) g_fade((int)(256.0f * (0.62f + 0.36f * A.p.echo))); else g_clear();
    fire();
    mist();
    sign_draw(&aries_art, ram_move, 0);
    embers(dt);
}

void aries_palette(uint16_t pal[256])
{
    f32 l = A.p.light, w = sat(l * 1.4f);
    uint32_t gold = ((uint32_t)(0xC8 + (int)(0x20 * w)) << 16) | ((uint32_t)(0x78 + (int)(0x61 * w)) << 8) | (uint32_t)(0x40 + (int)(0x68 * w));
    uint32_t inks[G_INKS] = {gold, 0xC0202Au, 0xF58A1Cu, 0x3048B8u};
    /* the ground behind everything takes its colour from the four settings, so a turn of any knob shows at once:
     * Heat red, Echo violet, Mist blue, Light yellow; a roar flashes it */
    static const f32 tint[4][3] = {{150, 14, 8}, {96, 22, 132}, {22, 70, 150}, {150, 118, 10}};
    f32 wt[4] = {A.p.heat * A.p.heat, A.p.echo, A.p.mist, l * l * 0.8f}, c[3];
    for (int k = 0; k < 3; k++) {
        f32 v = 0.0f;
        for (int j = 0; j < 4; j++) v += tint[j][k] * wt[j];
        c[k] = minf(170.0f, v * (0.58f + 0.45f * A.roar));
    }
    g_palette_on(inks, 0.30f + 0.85f * l + 0.25f * A.roar, ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | (uint32_t)c[2], pal);
}
