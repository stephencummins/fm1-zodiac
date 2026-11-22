/* AQUARIUS, the piece. Airy and glassy, like looking into a tank of slow fish: small bells falling in ripples
 * through each chord, a glass voice above them stepping slowly down, soft strings below. A key pours the water:
 * the bells run upward.
 * Flow: the ripples fall faster and further. Echo, Mist: as everywhere. Light: brighter bells. */
#include "music.h"

#define STEPS 4
#define BPM 72.0f
#define NB 12
static const int8_t chords[4][4] = {{2, 5, 9, 4}, {10, 2, 5, 9}, {7, 10, 2, 4}, {9, 2, 4, 7}};    /* D minor's country */
static const int8_t roots[4] = {38, 34, 31, 33};
static const int8_t minor[7] = {0, 2, 3, 5, 7, 8, 10};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, ci, bi, at, pour, g_idx, g_wait, key, reg;
    int8_t tones[12]; int nt;
    MBell b[NB]; MHorn glass; MBow st[2];
    f32 flow;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static void ring(int midi, f32 vel)
{
    int k = M.bi; M.bi = (M.bi + 1) % NB;
    m_bell_on(&M.b[k], (f32)midi, vel, m_k(0.45f, M.dt)); M.b[k].pan = 0.2f + 0.6f * rnd();
}

static void step(void)
{
    int s = M.step;
    if (s == 0 && (M.bar & 7) == 0 && M.bar && rnd() < 0.35f) M.beats = 7 - M.beats;      /* the time may change: three, or four */
    if (s == 0) {
        if ((M.bar & 7) == 0) { M.key = m_slow.key; M.reg = m_slow.reg; }
        M.ci = M.bar & 3; M.nt = 0;
        for (int m = 62; m < 94 && M.nt < 12; m++) for (int k = 0; k < 4; k++) if ((m - M.key + 120) % 12 == chords[M.ci][k]) M.tones[M.nt++] = (int8_t)m;
        m_bow_on(&M.st[0], (f32)(roots[M.ci] + 12 + M.key), 1); m_bow_on(&M.st[1], (f32)(roots[M.ci] + 19 + M.key), 1);
    }
    if (M.pour > 0) { ring(M.tones[(12 - M.pour) % M.nt] + (M.pour <= 4 ? 12 : 0), 0.7f); M.pour--; M.at = M.nt - 1; }
    else if (!(s & 1) || M.flow + 0.3f * m_slow.fill > 0.55f) {                   /* the ripple: down through the chord, then back to the top */
        int span = 5 + (int)(5.0f * M.flow);
        if (s % 8 == 0 || M.at < 0 || M.at < M.nt - 1 - span) M.at = M.nt - 1 - (int)(rnd() * 3.0f);
        if (M.at >= M.nt) M.at = M.nt - 1;
        if (!(s & 3) || m_slow.fill > -0.4f) ring(M.tones[M.at], (s % 8 == 0 ? 0.6f : 0.4f) * m_slow.v[1]);      /* thin: every other bell */
        M.at--;
    }
    if (M.g_wait > 0) M.g_wait--;
    if (M.g_wait == 0 && s % STEPS == 0) {                   /* the glass voice: slowly down the scale, then up again */
        M.g_idx -= 1 + (rnd() < 0.25f);
        if (M.g_idx < 3 || rnd() < 0.12f) M.g_idx = 9 + M.reg + (int)(rnd() * 3.0f);
        m_horn_on(&M.glass, (f32)(74 + M.key + m_scale(minor, M.g_idx)), 1.0f);
        M.g_wait = (2 + (int)(rnd() * 3.0f)) * STEPS;
    }
    if (++M.step >= M.beats * STEPS) { M.step = 0; M.bar++; }
}

static void aquarius_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = m_seeded(0xA90A31u); M.clock = 1.0f; M.beats = 4; M.g_idx = 11; M.g_wait = 2 * STEPS + 1; M.at = -1;
    M.st[0].pan = 0.35f; M.st[1].pan = 0.65f;
    m_fx_init(sr);
}

static void aquarius_m_strike(void) { if (M.pour == 0 && M.nt) { M.pour = 12; m_event(); } }

static void aquarius_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.flow = sat(p->heat);
    m_fx.room = 0.75f + 0.22f * mist; m_fx.damp = 0.25f + 0.4f * mist - 0.15f * light; m_fx.wet = 0.3f + 0.45f * mist;
    m_fx.echo_secs = 60.0f / BPM * 0.75f; m_fx.echo_fb = 0.35f + 0.4f * echo; m_fx.echo_wet = 0.2f + 0.8f * echo;
    f32 gup = m_k(0.25f, dt), gdn = m_k(1.2f, dt), bend = m_k(0.08f, dt), sup = m_k(0.8f, dt), sdn = m_k(1.0f, dt), sglide = m_k(0.2f, dt);
    f32 scut = sat(700.0f * 6.2832f * dt), depth = 0.08f + 0.22f * light;
    f32 bpm = BPM * m_drift(M.secs += (f32)n * dt, 19u, 0.14f);
    f32 v0 = m_slow.v[0], v2 = m_slow.v[2];
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        f32 l = 0.0f, r = 0.0f;
        for (int k = 0; k < NB; k++) { f32 x = m_bell(&M.b[k], dt, 4.0f, depth) * 0.22f; l += x * (1.0f - M.b[k].pan); r += x * M.b[k].pan; }
        f32 g = m_horn(&M.glass, dt, gup, gdn, bend, 0.05f, 0.25f, 0.005f) * 0.16f * v2;
        f32 a = m_bow(&M.st[0], dt, sup, sdn, sglide, 0.003f, scut) * 0.3f, b = m_bow(&M.st[1], dt, sup, sdn, sglide, 0.003f, scut) * 0.3f; a *= v0; b *= v0;
        f32 send = (l + r) * 1.2f + g * 2.5f + (a + b) * 0.8f, es = (l + r) * 0.8f + g;
        l += g + a * 0.7f + b * 0.3f; r += g + a * 0.3f + b * 0.7f;
        m_fx_tick(&l, &r, send, es);
        lr[2 * i] = m_clip(l * 0.9f); lr[2 * i + 1] = m_clip(r * 0.9f);
    }
}

const ZMusic m_aquarius = {aquarius_m_init, aquarius_m_strike, aquarius_m_render};
