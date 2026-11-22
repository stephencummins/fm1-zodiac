/* TAURUS: the bull, earth. Steady, patient, fond of its comfort: it chews, half asleep, while things grow round it.
 * Slow to anger: one strike gets a snort and a stamp. Keep striking and it stops chewing, opens its eyes and lowers
 * its head. The bull is art/taurus.svg; the growth, the petals and the dust are drawn here. */
#include "scene.h"
#include "sign.h"
#include "pic_taurus.h"

enum { K_LINE, K_UMBER, K_OCHRE, K_GREEN };

#define NBIT 40
#define NPUFF 28
typedef struct { f32 x, y, vx, vy, age, life; } Bit;

static struct {
    Scene s;
    f32 temper;                  /* 0 content .. 1 it has had enough */
    f32 stamp, stamp_t;          /* the jolt of a hoof */
    f32 swing;                   /* how far the ring is swinging */
    f32 next_snort, chew, chewing;
    Bit bit[NBIT]; f32 spawn; int dust;
    Bit puff[NPUFF];
    int jet;
    f32 flare, steam, hy;        /* how wide the nostrils are; steam owed; how far the head has dropped */
} T;

void taurus_init(void)
{
    taurus_pic_load();
    sc_init(&T.s, 11u);
    T.temper = 0.0f; T.stamp = 0.0f; T.stamp_t = -1.0f; T.swing = 0.0f; T.next_snort = 8.0f; T.chew = 0.0f; T.chewing = 1.0f;
    T.spawn = 0.0f; T.dust = 0; T.flare = 0.0f; T.steam = 0.0f; T.hy = 0.0f;
    for (int i = 0; i < NBIT; i++) T.bit[i].life = 0.0f;
    for (int i = 0; i < NPUFF; i++) T.puff[i].life = 0.0f;
}

static void snort(f32 hard)
{
    int made = 0;
    for (int i = 0; i < NPUFF && made < 2; i++) {
        Bit *b = &T.puff[i];
        if (b->life > 0.0f) continue;
        f32 sg = made ? 1.0f : -1.0f;
        b->x = CX + sg * 13.5f; b->y = 169.0f; b->vx = sg * (18.0f + 62.0f * hard); b->vy = 22.0f + 70.0f * hard;
        b->age = 0.0f; b->life = 1.6f + 1.2f * hard;
        made++;
    }
}

void taurus_strike(void)
{
    T.temper = minf(1.0f, T.temper + 0.30f);
    T.stamp_t = 0.0f; T.swing = 0.07f; T.dust = 12;
    snort(1.0f); T.jet = 11;                  /* and steam shoots from the nostrils for a few frames more */
}

/* stems, leaves and flowers from behind the frame: more of them, further, the higher the first knob */
static void growth(void)
{
    f32 grow = T.s.p.heat, t = T.s.t;
    enum { NV = 14, N = 10 };
    for (int i = 0; i < NV; i++) {
        f32 th = ((f32)i + 0.5f * hash01((uint32_t)i * 31u)) / (f32)NV;
        f32 dx = cos_turns(th), dy = sin_turns(th);
        f32 L = (16.0f + 66.0f * grow * (0.55f + 0.45f * hash01((uint32_t)i * 17u + 5u))) * (1.0f + 0.25f * maxf(0.0f, -dy));
        f32 curl = (hash01((uint32_t)i * 7u + 2u) - 0.5f) * 1.5f;
        f32 sway = sin_turns(t * 0.06f + (f32)i * 0.21f) * 3.0f + T.stamp * 4.0f * sin_turns(t * 6.0f + (f32)i * 0.4f);
        f32 xy[(N + 1) * 2];
        for (int k = 0; k <= N; k++) {
            f32 u = (f32)k / (f32)N;
            f32 off = curl * L * 0.35f * u * u + sway * u + sin_turns(u * 0.8f + (f32)i * 0.3f) * 4.0f * u;
            xy[2 * k] = CX + dx * (66.0f + L * u) - dy * off; xy[2 * k + 1] = CY + dy * (66.0f + L * u) + dx * off;
        }
        g_mode(G_MAX); g_ink(K_GREEN); g_poly(xy, N + 1, 0, 1.5f, 0.80f);
        for (int k = 2; k < N; k += 2) {
            f32 u = (f32)k / (f32)N;
            if (u > 0.25f + grow) continue;                                   /* the far leaves come with growth */
            f32 tx = xy[2 * k + 2] - xy[2 * k - 2], ty = xy[2 * k + 3] - xy[2 * k - 1], len = sqrtf_(tx * tx + ty * ty);
            if (len < 0.01f) continue;
            tx /= len; ty /= len;
            f32 sd = ((k / 2 + i) & 1) ? 1.0f : -1.0f, nx = -ty * sd, ny = tx * sd;
            f32 sz = (10.0f - 4.0f * u) * (0.45f + 0.55f * grow) * (1.0f + 0.08f * sin_turns(t * 0.11f + (f32)(i + k) * 0.3f));
            f32 px = xy[2 * k], py = xy[2 * k + 1];
            f32 lf[8] = {px, py, px + tx * sz * 0.7f + nx * sz * 0.25f, py + ty * sz * 0.7f + ny * sz * 0.25f,
                         px + tx * sz * 0.55f + nx * sz, py + ty * sz * 0.55f + ny * sz,
                         px - tx * sz * 0.15f + nx * sz * 0.55f, py - ty * sz * 0.15f + ny * sz * 0.55f};
            g_ink(K_GREEN); g_fill(lf, 4, 0.50f);
            g_ink(K_LINE); g_line(px, py, lf[4], lf[5], 0.8f, 0.30f);
        }
        if (grow > 0.30f) {                                                   /* a flower at the tip */
            f32 r = 1.5f + 4.0f * (grow - 0.30f) * (0.6f + 0.4f * hash01((uint32_t)i * 3u + 8u));
            f32 fx = xy[2 * N], fy = xy[2 * N + 1];
            g_ink(K_OCHRE); g_disc(fx, fy, r, 0.95f);
            g_ink(K_LINE); g_ring(fx, fy, r + 1.3f, 0.8f, 0.55f);
            g_ink(K_UMBER); g_disc(fx, fy, r * 0.35f, 0.9f);
        }
    }
}

/* petals drift down; a stamp throws dust up from the ground */
static void bits(f32 dt)
{
    f32 grow = T.s.p.heat;
    T.spawn += dt * (0.6f + 6.0f * grow);
    for (int i = 0; i < NBIT; i++) {
        Bit *b = &T.bit[i];
        if (b->life <= 0.0f) {
            if (T.dust > 0) {
                T.dust--;
                b->x = 20.0f + sc_rnd(&T.s) * 200.0f; b->y = 238.0f; b->vx = (sc_rnd(&T.s) - 0.5f) * 30.0f; b->vy = -(40.0f + sc_rnd(&T.s) * 60.0f);
                b->age = 0.0f; b->life = 1.6f + sc_rnd(&T.s);
            } else if (T.spawn >= 1.0f) {
                T.spawn -= 1.0f;
                b->x = sc_rnd(&T.s) * 240.0f; b->y = -3.0f; b->vx = (sc_rnd(&T.s) - 0.5f) * 6.0f; b->vy = 8.0f + sc_rnd(&T.s) * 8.0f;
                b->age = 0.0f; b->life = 14.0f + sc_rnd(&T.s) * 10.0f;
            }
            continue;
        }
        b->age += dt;
        if (b->age >= b->life || b->y > 244.0f) { b->life = 0.0f; continue; }
        b->vy = sc_follow(b->vy, 11.0f, dt, 1.6f, 1.6f);
        b->x += (b->vx + sin_turns(b->age * 0.21f + (f32)i * 0.37f) * 8.0f) * dt; b->y += b->vy * dt;
        f32 u = b->age / b->life, a = u < 0.1f ? u * 10.0f : u > 0.8f ? (1.0f - u) * 5.0f : 1.0f;
        f32 tw = sin_turns(b->age * 0.5f + (f32)i * 0.11f), w = 1.2f + 1.3f * absf(tw);
        f32 d[8] = {b->x - w, b->y, b->x, b->y - 1.6f, b->x + w, b->y, b->x, b->y + 1.6f};
        g_mode(G_MAX); g_ink(i % 3 == 0 ? K_UMBER : K_OCHRE); g_fill(d, 4, a * 0.85f);
    }
    g_ink(K_LINE);
    for (int i = 0; i < NPUFF; i++) {
        Bit *b = &T.puff[i];
        if (b->life <= 0.0f) continue;
        b->age += dt;
        if (b->age >= b->life) { b->life = 0.0f; continue; }
        b->vx *= 1.0f - dt * 1.5f; b->vy = b->vy * (1.0f - dt * 1.5f) - 22.0f * dt;      /* out and down, then it rises */
        b->x += (b->vx + sin_turns(b->age * 0.35f + (f32)i * 0.3f) * 5.0f) * dt; b->y += b->vy * dt;
        f32 u = b->age / b->life;
        /* solid little clouds, over the picture: they swell, then thin and shrink away */
        g_mode(G_PAINT); g_disc(b->x, b->y, (2.5f + 9.0f * u) * (1.0f - u * u * u), u < 0.5f ? 1.0f : 1.0f - 0.9f * (u - 0.5f));
        g_disc(b->x + 3.0f * sin_turns((f32)i * 0.37f), b->y - 2.0f, (1.5f + 6.0f * u) * (1.0f - u * u), u < 0.5f ? 0.9f : 0.9f - 0.9f * (u - 0.5f));
    }
}

void taurus_frame(f32 dt, const ZParams *want)
{
    sc_step(&T.s, dt, want);
    T.temper = maxf(0.0f, T.temper - dt * 0.075f);
    T.swing *= 1.0f - minf(1.0f, dt * 0.7f);
    if (T.stamp_t >= 0.0f) { T.stamp_t += dt; if (T.stamp_t > 0.6f) T.stamp_t = -1.0f; }
    T.stamp = sc_follow(T.stamp, T.stamp_t >= 0.0f && T.stamp_t < 0.10f ? 1.0f : 0.0f, dt, 40.0f, 6.0f);
    /* Echo is its breath: the more of it, the wider the nostrils flare on each breath out and the more steam */
    f32 out = maxf(0.0f, sin_turns(T.s.t * (0.16f + 0.16f * T.s.p.echo)));
    T.flare = sc_follow(T.flare, sat(T.s.p.echo * (0.25f + 0.75f * out) + (T.stamp_t >= 0.0f ? 1.0f : 0.0f) + 0.3f * T.temper), dt, 14.0f, 4.0f);
    T.steam += dt * T.s.p.echo * out * 22.0f;
    if (T.jet > 0) { T.jet--; snort(0.75f + 0.25f * sc_rnd(&T.s)); }
    if (T.steam >= 1.0f) { T.steam -= 1.0f; snort(0.25f + 0.6f * T.s.p.echo); }
    { extern int z_led; if (!z_led) T.next_snort -= dt; }      /* with its piece playing, the music says when */
    if (T.next_snort < 0.0f) { T.next_snort = 9.0f + sc_rnd(&T.s) * 9.0f; snort(0.3f); }
    /* it chews in spells, and not at all when it is cross */
    f32 spell = noise1(T.s.t * 0.09f, 61u) > 0.33f && T.temper < 0.45f ? 1.0f : 0.0f;
    T.chewing = sc_follow(T.chewing, spell, dt, 3.0f, 3.0f);
    T.chew += dt * 0.85f;

    sc_begin(&T.s);
    growth();
    sc_mist(&T.s, K_GREEN);
    /* the picture hardly moves: a slow breath, a jolt when it stamps, and the nostrils swelling as it blows */
    p_begin(taurus_pic);
    p_warp(taurus_pic, 107.5f, 164.0f, 10.0f, 9.0f, 1.1f * T.flare, 0.0f, 0.0f, 0.0f, 0);
    p_warp(taurus_pic, 131.5f, 164.0f, 10.0f, 9.0f, 1.1f * T.flare, 0.0f, 0.0f, 0.0f, 0);
    g_blit(g_work, 1.06f + 0.008f * sin_turns(T.s.t * 0.085f) + 0.035f * T.stamp, -400.0f, 0.0f, 0.0f, 0.0f, 0.95f + 0.2f * T.stamp);
    bits(dt);
}

void taurus_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xECDFB4u, 0xA4522Cu, 0xE4A93Cu, 0x5C9E3Cu};
    /* Growth green, Echo warm earth, Mist moss, Light ochre */
    static const f32 tint[4][3] = {{24, 110, 30}, {130, 50, 20}, {14, 80, 90}, {150, 112, 16}};
    sc_palette_tint(&T.s, inks, 0.15f * T.stamp, tint, T.stamp, pal);
}
