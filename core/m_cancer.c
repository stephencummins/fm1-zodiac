/* CANCER, the piece. A violin, sweet and a little tragic, on long sliding notes over a falling Spanish ground.
 * When the crab clicks its claws the castanets start and for four bars it is a paso doble: bass and strummed
 * chords in two, the violin stepping out; then it sinks back. The claws click with the castanets.
 * Tide: the violin plays more and stronger. Echo, Mist: as everywhere. Light: brighter. */
#include "music.h"

#define STEPS 12
#define BPM 112.0f
#define NG 8
static const int8_t ch[4][3] = {{9, 0, 4}, {7, 11, 2}, {5, 9, 0}, {4, 8, 11}};       /* A minor, G, F, E */
static const int8_t roots[4] = {45, 43, 41, 40};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, ci, paso, roll, gi;
    MBow vn, pad[2]; int vn_note, vn_wait, vn_off;
    MPluck g[NG];
    f32 c_env[2], c_ph[2], pz, tide;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static int snap(int t)
{
    for (int d = 0; d < 7; d++) for (int sg = -1; sg <= 1; sg += 2) {
        int n = t + sg * d, pc = ((n % 12) + 12) % 12;
        if (pc == ch[M.ci][0] || pc == ch[M.ci][1] || pc == ch[M.ci][2]) return n;
    }
    return t;
}
static void pluck(int midi, f32 vel, f32 pan, f32 secs)
{
    int k = M.gi; M.gi = (M.gi + 1) % NG;
    m_pluck_on(&M.g[k], (f32)midi, vel, m_k(secs, M.dt)); M.g[k].pan = pan;
}
static void clack(int which, f32 vel) { M.c_env[which] = vel; M.c_ph[which] = 0.0f; }

static void step(void)
{
    int s = M.step, bs = s % STEPS, paso = M.paso > 0;
    if (s == 0 && (M.bar & 15) == 0 && M.bar && !paso && rnd() < 0.4f) M.beats = 5 - M.beats;      /* in two, or a slow waltz */
    if (s == 0 && (M.bar & 3) == 0) {
        M.ci = (M.bar >> 2) & 3;
        m_bow_on(&M.pad[0], (f32)(roots[M.ci] + 12), 1); m_bow_on(&M.pad[1], (f32)(roots[M.ci] + 19), 1);
    }
    if (M.roll > 0) { clack(M.roll & 1, M.roll == 1 ? 1.0f : 0.5f); M.roll--; }
    if (paso) {
        if (bs == 0) { clack(0, 1.0f); if (s == 0) m_event(); }
        else if (bs == 6) clack(1, 0.55f);
        else if (bs == 9) clack(0, 0.5f);
        if ((M.bar & 1) && s >= 18 && !(s & 1)) clack(1, 0.45f);
        if (bs == 0) pluck(roots[M.ci] + (s ? 7 : 0), 1.0f, 0.5f, 0.5f);
        if (bs == 6) for (int k = 0; k < 3; k++) pluck(57 + ((ch[M.ci][k] + 3) % 12), 0.5f, 0.35f + 0.15f * (f32)k, 0.22f);
    } else {
        if (s == 0) pluck(roots[M.ci], 0.6f, 0.5f, 1.0f);
        if (s == 12 || s == 24) pluck(57 + ((ch[M.ci][(M.bar & 1) + 1] + 3) % 12), 0.3f, 0.6f, 0.8f);
    }
    if (M.vn_off > 0 && --M.vn_off == 0) M.vn.gate = 0.0f;
    if (M.vn_wait > 0) M.vn_wait--;
    if (M.vn_wait == 0 && s % 6 == 0) {
        int n = snap(M.vn_note + (int)(rnd() * 9.0f) - 4);
        if (n > 88) n -= 12; if (n < 67) n += 12;
        f32 r = rnd();
        int dur = paso ? (r < 0.3f ? 6 : r < 0.75f ? 12 : 24) : (r < 0.2f ? 24 : r < 0.5f ? 48 : r < 0.7f ? 36 : r < 0.9f ? 72 : 96);
        int rest = !paso && rnd() < 0.35f - 0.3f * M.tide ? 24 : 0;
        m_bow_on(&M.vn, (f32)n, 1);
        M.vn_note = n; M.vn_off = rest ? dur - 2 : 0; M.vn_wait = dur + rest;
    }
    if (++M.step >= M.beats * STEPS) { M.step = 0; M.bar++; if (M.paso > 0) M.paso--; }
}

static void cancer_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0xC4AB01u; M.clock = 1.0f; M.beats = 2; M.vn_note = 76; M.vn_wait = 4 * STEPS + 1;
    M.pad[0].pan = 0.35f; M.pad[1].pan = 0.65f;
    m_fx_init(sr);
}

static void cancer_m_strike(void)
{
    M.paso = 4; M.roll = 5; M.beats = 2; if (M.vn_wait > 6) M.vn_wait = 6;
    m_event();
}

static void cancer_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.tide = sat(p->heat);
    m_fx.room = 0.6f + 0.35f * mist; m_fx.damp = 0.4f + 0.35f * mist - 0.15f * light; m_fx.wet = 0.2f + 0.5f * mist;
    m_fx.echo_secs = 60.0f / BPM * 1.5f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 up = m_k(0.07f, dt), dn = m_k(0.22f, dt), glide = m_k(0.06f, dt), pup = m_k(0.5f, dt), pdn = m_k(0.6f, dt);
    f32 cut = sat((1700.0f + 2600.0f * light) * 6.2832f * dt), pcut = sat(650.0f * 6.2832f * dt);
    f32 rel = m_k(0.05f, dt), gcut = sat((1600.0f + 2500.0f * light) * 6.2832f * dt), cdec = m_k(0.016f, dt), kz = m_k(0.3f, dt);
    f32 bpm = BPM * m_drift(M.secs += (f32)n * dt, 14u, 0.12f);
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        M.pz += ((M.paso > 0 ? 1.0f : 0.0f) - M.pz) * kz;
        f32 v = m_bow(&M.vn, dt, up, dn, glide, 0.009f, cut) * (0.55f + 0.25f * M.tide + 0.2f * M.pz);
        f32 a = m_bow(&M.pad[0], dt, pup, pdn, glide, 0.003f, pcut), b = m_bow(&M.pad[1], dt, pup, pdn, glide, 0.003f, pcut);
        f32 gl = 0.0f, gr = 0.0f;
        for (int k = 0; k < NG; k++) { f32 x = m_pluck(&M.g[k], dt, rel, gcut); gl += x * (1.0f - M.g[k].pan); gr += x * M.g[k].pan; }
        f32 c0, c1;
        M.c_env[0] -= M.c_env[0] * cdec; M.c_env[1] -= M.c_env[1] * cdec;
        M.c_ph[0] += 1450.0f * dt; if (M.c_ph[0] >= 1.0f) M.c_ph[0] -= 1.0f;
        M.c_ph[1] += 1900.0f * dt; if (M.c_ph[1] >= 1.0f) M.c_ph[1] -= 1.0f;
        c0 = (sin_turns(M.c_ph[0]) * 0.6f + m_noise(&M.rng) * 0.5f) * M.c_env[0] * 0.5f;
        c1 = (sin_turns(M.c_ph[1]) * 0.6f + m_noise(&M.rng) * 0.5f) * M.c_env[1] * 0.5f;
        f32 l = v * 0.9f + a * 0.5f + b * 0.2f + gl * 0.45f + c0 * 0.9f + c1 * 0.3f;
        f32 r = v * 0.7f + a * 0.2f + b * 0.5f + gr * 0.45f + c0 * 0.3f + c1 * 0.9f;
        m_fx_tick(&l, &r, v * 1.3f + (a + b) * 0.4f + (gl + gr) * 0.3f + (c0 + c1) * 0.5f, v * 0.8f);
        lr[2 * i] = m_clip(l * 0.62f); lr[2 * i + 1] = m_clip(r * 0.62f);
    }
}

const ZMusic m_cancer = {cancer_m_init, cancer_m_strike, cancer_m_render};
