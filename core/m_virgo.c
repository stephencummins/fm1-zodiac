/* VIRGO, the piece. A jig on a hurdy-gurdy: two drone strings, a chanter playing a tune in two halves, each
 * played twice, and the buzzing bridge marking the beat. A key gives the wheel a hard turn.
 * Harvest: quicker, more buzz and more grace notes. Echo, Mist: as everywhere. Light: brighter. */
#include "music.h"

#define STEPS 2                                   /* steps to a quaver; six quavers a bar */
static const int8_t major[7] = {0, 2, 4, 5, 7, 9, 11};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, crank, cur;
    int8_t tune[2][12];
    MBow ch, dr[2];
    f32 tr, wheel, harvest;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static void set(int idx) { M.ch.aim = m_hz((f32)(67 + m_scale(major, idx))); }

static void gen(int part)
{
    static const int8_t move[6] = {-1, 1, 1, -1, 2, -2};
    int at = part ? 4 + 3 * (int)(rnd() * 2.0f) : 2 * (int)(rnd() * 3.0f);
    for (int i = 0; i < 12; i++) {
        if (i % 3 == 1 && rnd() < 0.35f) { M.tune[part][i] = -1; continue; }
        if (i) at += move[(int)(rnd() * 6.0f)];
        if (i % 3 == 0 && (at & 1) && at != 7 && at != 9) at += rnd() < 0.5f ? 1 : -1;
        if (at < 0) at += 2; if (at > 9) at -= 3;
        M.tune[part][i] = (int8_t)at;
    }
    M.tune[part][9] = part ? 0 : 4; M.tune[part][10] = M.tune[part][11] = -1;
}

static void step(void)
{
    int s = M.step, pos = s / STEPS, half = s % STEPS, b = M.bar & 7, part = b >> 2;
    if (s == 0 && (M.bar & 15) == 0) { gen(0); gen(1); if (M.bar && rnd() < 0.4f) M.beats = 5 - M.beats; }       /* a jig, or a slip jig */
    if (!half) {
        int n = M.tune[part][((b & 1) * 6 + pos) % 12], home = 3 * (M.beats - 1);
        if ((b & 3) == 3 && pos >= home) n = pos == home ? (part ? 0 : 7) : -1;          /* second time, home */
        if (n >= 0) { M.cur = n; M.ch.env *= 0.6f; }
        set(M.cur);
        if (pos % 3 == 0) M.tr = 1.0f;
        else if (pos % 3 == 2 && rnd() < 0.2f + 0.6f * M.harvest) M.tr = 0.6f;
    } else if (rnd() < 0.08f + 0.3f * M.harvest) set(M.cur + 1);                   /* a grace note */
    if (M.crank > 0) { M.tr = 1.0f; M.crank--; }
    if (++M.step >= 3 * M.beats * STEPS) { M.step = 0; M.bar++; }
}

static void virgo_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0x91460u; M.clock = 1.0f; M.beats = 2;
    m_bow_on(&M.dr[0], 43.0f, 0); m_bow_on(&M.dr[1], 50.05f, 0); m_bow_on(&M.ch, 67.0f, 0);
    m_fx_init(sr);
}

static void virgo_m_strike(void) { M.crank = 6; m_event(); }

static void virgo_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.harvest = sat(p->heat);
    f32 rate = (104.0f + 20.0f * M.harvest) * m_drift(M.secs += (f32)n * dt, 16u, 0.12f) / 60.0f * 3.0f * (f32)STEPS;
    m_fx.room = 0.45f + 0.5f * mist; m_fx.damp = 0.4f + 0.35f * mist - 0.15f * light; m_fx.wet = 0.12f + 0.5f * mist;
    m_fx.echo_secs = 60.0f / 114.0f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 up = m_k(0.012f, dt), dn = m_k(0.1f, dt), glide = m_k(0.004f, dt), dglide = m_k(0.1f, dt);
    f32 cut = sat((2600.0f + 3500.0f * light) * 6.2832f * dt), dcut = sat((600.0f + 700.0f * light) * 6.2832f * dt);
    f32 trdec = m_k(0.06f, dt);
    for (int i = 0; i < n; i++) {
        M.clock += rate * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        M.wheel += 3.1f * dt; if (M.wheel >= 1.0f) M.wheel -= 1.0f;
        f32 turn = 1.0f + 0.07f * sin_turns(M.wheel);
        f32 c = m_bow(&M.ch, dt, up, dn, glide, 0.002f, cut) * turn;
        f32 a = m_bow(&M.dr[0], dt, up, dn, dglide, 0.0f, dcut), b = m_bow(&M.dr[1], dt, up, dn, dglide, 0.0f, dcut);
        M.tr -= M.tr * trdec;
        f32 buzz = m_clip((a + b) * 9.0f) * M.tr * (0.10f + 0.12f * M.harvest);
        f32 d = (a + b) * 0.5f * turn;
        f32 l = c * 0.34f + d * 0.6f + a * 0.2f + buzz, r = c * 0.42f + d * 0.6f + b * 0.2f + buzz;
        m_fx_tick(&l, &r, c * 0.6f + d * 0.4f + buzz * 0.5f, c * 0.5f);
        lr[2 * i] = m_clip(l * 0.566f); lr[2 * i + 1] = m_clip(r * 0.566f);
    }
}

const ZMusic m_virgo = {virgo_m_init, virgo_m_strike, virgo_m_render};
