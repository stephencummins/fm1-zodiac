/* CANCER: the crab, water. Guarded, moody, ruled by the Moon, happiest at home: it sidles about under a moon,
 * snipping its claws, and at a strike it pulls everything in behind them, then peeps out, eyes first.
 * The crab is art/cancer.svg; the moon, the tide, the ripples and the bubbles are drawn here. */
#include "scene.h"
#include "sign.h"
#include "art_cancer.h"

enum { K_LINE, K_CORAL, K_TEAL, K_BLUE };

#define NBUB 22
typedef struct { f32 x, y, age, life, r; } Bub;

static struct {
    Scene s;
    f32 hide, hide_t, next_start;
    f32 xoff, vel, yw;           /* where it has sidled to, how fast, and the water's level */
    f32 splash;                  /* 0 .. 1 a ring running out; < 0 none */
    Bub b[NBUB]; f32 spawn;
} C;

void cancer_init(void)
{
    sc_init(&C.s, 37u);
    C.hide = 0.0f; C.hide_t = -1.0f; C.next_start = 19.0f; C.xoff = 0.0f; C.vel = 0.0f; C.yw = 180.0f; C.splash = -1.0f; C.spawn = 0.0f;
    for (int i = 0; i < NBUB; i++) C.b[i].life = 0.0f;
}

void cancer_strike(void) { C.hide_t = 0.0f; C.splash = 0.0f; }

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

static void moon(void)
{
    f32 t = C.s.t, ph = 16.0f + 9.0f * sin_turns(t * 0.012f);
    g_mode(G_MAX); g_ink(K_LINE);
    g_glow(CX, 70.0f, 34.0f, 0.30f);
    g_disc(CX, 70.0f, 13.0f, 0.95f);
    g_mode(G_PAINT); g_disc(CX + ph, 67.0f, 13.5f, 0.0f);
    g_mode(G_MAX);
}

static f32 wave(f32 x, f32 depth) { return 2.2f * sin_turns(x * 0.021f + C.s.t * (depth > 0.0f ? -0.21f : 0.27f) + depth * 0.37f) + 1.0f * sin_turns(x * 0.05f - C.s.t * 0.4f); }
static f32 half_at(f32 y) { f32 h = y > 171.0f ? 74.5f * (214.0f - y) / 43.0f : y < 85.0f ? 74.5f * (y - 42.0f) / 43.0f : 74.5f; return maxf(0.0f, h - 2.5f); }

/* the sea inside the frame, up to the tide's level */
static void water(int front)
{
    enum { N = 16 };
    f32 yw = C.yw, h = half_at(yw), xy[(N + 4) * 2];
    int n = 0;
    if (h <= 1.0f) return;
    for (int k = 0; k < N; k++) { f32 x = CX - h + 2.0f * h * (f32)k / (f32)(N - 1); xy[2 * n] = x; xy[2 * n + 1] = yw + wave(x, 0.0f); n++; }
    g_mode(G_MAX);
    if (front) { g_ink(K_LINE); g_poly(xy, N, 0, 1.2f, 0.75f); return; }
    if (yw < 171.0f) { xy[2 * n] = CX + 72.0f; xy[2 * n + 1] = 171.0f; n++; }
    xy[2 * n] = CX; xy[2 * n + 1] = 211.0f; n++;
    if (yw < 171.0f) { xy[2 * n] = CX - 72.0f; xy[2 * n + 1] = 171.0f; n++; }
    g_ink(K_BLUE); g_fill(xy, n, 0.34f);
    g_ink(K_TEAL);
    int d = 1;
    for (f32 y = yw + 11.0f; y < 208.0f; y += 11.0f, d++) {
        f32 hh = half_at(y), ln[N * 2];
        if (hh < 6.0f) break;
        for (int k = 0; k < N; k++) { f32 x = CX - hh + 2.0f * hh * (f32)k / (f32)(N - 1); ln[2 * k] = x; ln[2 * k + 1] = y + wave(x, (f32)d); }
        g_poly(ln, N, 0, 1.0f, 0.50f - 0.05f * (f32)d);
    }
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

static void arm_xf(int right, f32 *xy, int n)
{
    f32 sg = right ? -1.0f : 1.0f, t = C.s.t, h = sat(C.hide * 1.5f);
    f32 idle = 0.010f * sin_turns(t * 0.09f + (f32)right * 0.35f) + 0.020f * maxf(0.0f, noise1(t * 0.25f + (f32)right * 5.0f, 83u) - 0.6f);
    sc_rot(xy, n, right ? 154.0f : 86.0f, 127.0f, sg * (0.115f * h - idle));
    sc_shift(xy, n, sg * 3.0f * h, 30.0f * h);
}

static void crab_move(int g, f32 *xy, int n, f32 *gain, void *ctx)
{
    (void)ctx;
    static const f32 base[4][2] = {{80, 134}, {78, 142}, {80, 150}, {86, 157}};
    f32 t = C.s.t, hide = C.hide;

    if (g <= CANCER_G_LEGR3) {                                               /* legs: they tap in a wave, faster as it walks */
        int right = g >= CANCER_G_LEGR0, k = right ? g - CANCER_G_LEGR0 : g - CANCER_G_LEGL0;
        f32 bx = right ? 240.0f - base[k][0] : base[k][0], by = base[k][1], sg = right ? -1.0f : 1.0f;
        f32 walk = sat(absf(C.vel) * 0.5f);
        f32 tap = sin_turns(t * (0.35f + 1.9f * walk) + (f32)k * 0.25f + (f32)right * 0.5f);
        sc_rot(xy, n, bx, by, sg * ((0.006f + 0.022f * walk) * tap + 0.03f * hide));
        for (int j = 0; j < n; j++) { xy[2 * j] = bx + (xy[2 * j] - bx) * (1.0f - 0.45f * hide); xy[2 * j + 1] = by + (xy[2 * j + 1] - by) * (1.0f - 0.30f * hide); }
    } else switch (g) {
    case CANCER_G_EYEL: case CANCER_G_EYER: {
        int right = g == CANCER_G_EYER;
        f32 peep = sat(hide * 2.2f - 1.2f);                                    /* the eyes come back out first */
        sc_squash(xy, n, 121.0f, 1.0f - 0.80f * peep);
        sc_rot(xy, n, right ? 130.0f : 110.0f, 121.0f, 0.045f * (noise1(t * 0.4f + (f32)right * 3.0f, 87u) - 0.5f) - 0.03f * C.vel * 0.3f);
        break; }
    case CANCER_G_FINGERL: case CANCER_G_FINGERR: {
        int right = g == CANCER_G_FINGERR;
        f32 snip = sat((noise1(t * 0.9f + (f32)right * 11.0f, 89u) - 0.30f) * 3.0f) * (1.0f - sat(hide * 2.0f));
        sc_rot(xy, n, right ? 162.5f : 77.5f, 85.0f, (right ? -1.0f : 1.0f) * 0.075f * snip);
        arm_xf(right, xy, n);
        break; }
    case CANCER_G_ARML: arm_xf(0, xy, n); break;
    case CANCER_G_ARMR: arm_xf(1, xy, n); break;
    case CANCER_G_BODY: *gain = 0.85f + 0.2f * sin_turns(t * 0.1f) - 0.25f * hide; break;
    }

    /* the whole crab: sidles, bobs, shrinks down when it hides; and what is under water wavers */
    f32 sc = 1.0f - 0.08f * hide, bob = 1.2f * sin_turns(t * 0.16f) + 5.0f * hide;
    for (int k = 0; k < n; k++) {
        f32 x = CX + (xy[2 * k] - CX) * sc + C.xoff, y = 146.0f + (xy[2 * k + 1] - 146.0f) * sc + bob;
        if (y > C.yw) x += minf(1.0f, (y - C.yw) * 0.2f) * 1.8f * sin_turns(y * 0.045f - t * 0.55f);
        xy[2 * k] = x; xy[2 * k + 1] = y;
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
    sc_frame_back(1.0f, K_BLUE, 0.20f);
    moon();
    water(0);
    sc_frame_line(1.0f, 0.05f);
    sc_mist(&C.s, K_BLUE);
    sign_draw(&cancer_art, crab_move, 0);
    water(1);
    bubbles(dt);
}

void cancer_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xDCE8F4u, 0xE4586Cu, 0x34B4A8u, 0x2C54C8u};
    /* Tide deep blue, Echo sea green, Mist violet, Light moon silver */
    static const f32 tint[4][3] = {{10, 40, 150}, {8, 110, 110}, {60, 40, 130}, {110, 120, 140}};
    f32 sp = C.splash >= 0.0f ? 1.0f - C.splash : 0.0f;
    sc_palette_tint(&C.s, inks, 0.2f * sp, tint, sp, pal);
}
