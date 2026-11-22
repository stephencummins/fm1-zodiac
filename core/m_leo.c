/* LEO, the piece. A march, majestic: low brass in chords, kettledrums on one and three, a side drum, a horn tune
 * in dotted steps. On the roar three trumpets sound a fanfare up the chord and hold the top.
 * Blaze: more drum, brighter brass. Echo, Mist: as everywhere. Light: opens the brass. */
#include "music.h"

#define STEPS 4
#define BPM 93.0f
static const int8_t major[7] = {0, 2, 4, 5, 7, 9, 11};
static const int8_t prog[4][4] = {{0, 0, 3, 4}, {0, 5, 3, 4}, {0, 3, 0, 4}, {0, 4, 5, 3}};
static const int8_t ct[8] = {-7, -5, -3, 0, 2, 4, 7, 9};         /* the notes of a chord, upward */
static const uint8_t pat[4][16] = {                          /* the tune's rhythms: a number is a note that long, in steps */
    {6, 0, 0, 0, 0, 0, 2, 0, 8, 0, 0, 0, 0, 0, 0, 0}, {4, 0, 0, 0, 4, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2, 0},
    {12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0}, {3, 0, 0, 1, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0}};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, deg, low, pr, pt, mel_idx, mel_off, mel_rest, fan;
    MHorn lo[3], mel, tp[3];
    MDrum timp;
    f32 sn_env, sn_lp, thump, blaze;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static f32 note(int idx, int base) { return (f32)(base + m_scale(major, idx)); }

static void blast(int q)
{
    for (int v = 0; v < 3; v++) m_horn_on(&M.tp[v], note(M.low + ct[q + 1 - v], 70) + (rnd() - 0.5f) * 0.08f, 0.94f);
}
static void fanfare_step(void)
{
    int f = M.fan++;
    if (f == 0 || f == 3) blast(1);                      /* from the fifth below, up the chord, to end on the fifth above */
    else if (f == 4) blast(2);
    else if (f == 6) blast(3);
    else if (f == 8) { blast(4); m_drum_on(&M.timp, m_hz(note(M.low, 34)), 1.0f, 1.12f); M.thump = 1.0f; }
    if (f == 2 || f == 22) for (int v = 0; v < 3; v++) M.tp[v].gate = 0.0f;
    if (f >= 26) M.fan = -1;
}

static void step(void)
{
    static const uint8_t snare[16] = {5, 0, 0, 0, 5, 0, 0, 0, 5, 0, 3, 3, 5, 0, 0, 0};
    int s = M.step, end = M.beats * STEPS;
    if (s == 0 && (M.bar & 7) == 0 && M.bar && M.fan < 0 && rnd() < (M.beats == 4 ? 0.2f : 0.7f)) M.beats = 7 - M.beats;      /* a march, or a slow three */
    if (s == 0) {
        if (M.fan < 0) {                                     /* the chord waits while the trumpets hold */
            int b = M.bar & 3;
            if (b == 0) M.pr = (int)(rnd() * 4.0f);
            M.deg = prog[M.pr][b]; M.low = M.deg > 3 ? M.deg - 7 : M.deg;
        }
        for (int v = 0; v < 3; v++) m_horn_on(&M.lo[v], note(M.low + 2 * v, 46), 0.99f);
        M.pt = (int)(rnd() * 4.0f); M.mel_rest = (M.bar & 3) == 3 && rnd() < 0.5f;
    }
    if (s == end - 2) for (int v = 0; v < 3; v++) M.lo[v].gate = 0.0f;
    if (s == 0) { m_drum_on(&M.timp, m_hz(note(M.low, 34)), 1.0f, 1.12f); M.thump = 1.0f; }
    else if (s == 8) { m_drum_on(&M.timp, m_hz(note(M.low + 4, 34) - (M.low + 4 > 2 ? 12.0f : 0.0f)), 0.7f, 1.12f); M.thump = 0.6f; }
    else if (s == end - 2 && rnd() < 0.25f + 0.5f * M.blaze) { m_drum_on(&M.timp, m_hz(note(M.low, 34)), 0.45f, 1.1f); M.thump = 0.3f; }
    if (snare[s]) M.sn_env = (f32)snare[s] * 0.2f * (0.8f + 0.2f * rnd());
    else if (s == end - 1 && (M.bar & 1)) M.sn_env = 0.5f;
    if (M.mel_off > 0 && --M.mel_off == 0) M.mel.gate = 0.0f;
    int len = pat[M.pt][s];
    if (len && !M.mel_rest) {
        int t = M.mel_idx;
        if (s == 0 || s == 8) {                              /* strong beats land on the chord */
            t += (int)(rnd() * 5.0f) - 2;
            for (int d = 0; d < 4; d++) {
                int a = (((t - d - M.deg) % 7) + 7) % 7, b = (((t + d - M.deg) % 7) + 7) % 7;
                if (a == 0 || a == 2 || a == 4) { t -= d; break; }
                if (b == 0 || b == 2 || b == 4) { t += d; break; }
            }
        } else t += rnd() < 0.5f ? 1 : -1;
        if (t < 0) t += 4; if (t > 9) t -= 4;
        m_horn_on(&M.mel, note(t, 58), 0.985f);
        M.mel_idx = t; M.mel_off = len > 1 ? len - 1 : 1;
    }
    if (M.fan >= 0) fanfare_step();
    if (++M.step >= end) { M.step = 0; M.bar++; }
}

static void leo_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0x1E0AA5u; M.clock = 1.0f; M.beats = 4; M.fan = -1; M.mel_idx = 4;
    for (int v = 0; v < 3; v++) { M.lo[v].pan = 0.3f + 0.2f * (f32)v; M.tp[v].pan = 0.7f - 0.2f * (f32)v; M.tp[v].vib = 0.3f * (f32)v; }
    m_fx_init(sr);
}

static void leo_m_strike(void) { if (M.fan < 0) { M.fan = 0; m_event(); } }

static void leo_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.blaze = sat(p->heat);
    m_fx.room = 0.7f + 0.28f * mist; m_fx.damp = 0.35f + 0.35f * mist - 0.15f * light; m_fx.wet = 0.24f + 0.5f * mist;
    m_fx.echo_secs = 60.0f / BPM * 0.75f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 lup = m_k(0.07f, dt), ldn = m_k(0.18f, dt), bend = m_k(0.03f, dt), mup = m_k(0.04f, dt), mdn = m_k(0.10f, dt);
    f32 tup = m_k(0.012f, dt), tdn = m_k(0.07f, dt), tdec = m_k(0.45f, dt), tbend = m_k(0.04f, dt);
    f32 sdec = m_k(0.045f, dt), thdec = m_k(0.02f, dt);
    f32 lbase = 0.4f + 0.5f * light, lblown = 1.0f + 1.0f * light + 0.8f * M.blaze, snare = 0.05f + 0.07f * M.blaze;
    f32 bpm = BPM * m_drift(M.secs += (f32)n * dt, 15u, 0.10f);
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        f32 ll = 0.0f, lrr = 0.0f, tl = 0.0f, tr = 0.0f, te = 0.0f;
        for (int v = 0; v < 3; v++) {
            f32 x = m_horn(&M.lo[v], dt, lup, ldn, bend, lbase, lblown, 0.002f);
            ll += x * (1.0f - M.lo[v].pan); lrr += x * M.lo[v].pan;
            x = m_horn(&M.tp[v], dt, tup, tdn, bend, 1.6f, 3.0f + 1.0f * light, 0.004f);
            tl += x * (1.0f - M.tp[v].pan); tr += x * M.tp[v].pan; if (M.tp[v].env > te) te = M.tp[v].env;
        }
        f32 duck = 1.0f - 0.5f * te;
        f32 ks = 0.4f + 0.6f * sat(330.0f / (M.mel.hz + 1.0f));          /* the higher the note, the plainer the tone: no rasp at the top */
        f32 m = m_horn(&M.mel, dt, mup, mdn, bend, (0.9f + 0.5f * light) * ks, 1.8f * ks, 0.004f) * duck;
        f32 nz = m_noise(&M.rng);
        M.thump -= M.thump * thdec;
        f32 timp = m_drum(&M.timp, dt, tdec, tbend) + nz * M.thump * 0.12f;
        M.sn_env -= M.sn_env * sdec; M.sn_lp += (nz - M.sn_lp) * 0.35f;
        f32 sn = (nz - M.sn_lp) * M.sn_env * snare;
        f32 l = ll * 0.2f * duck + m * 0.26f + tl * 0.2f + timp * 0.5f + sn * 0.8f;
        f32 r = lrr * 0.2f * duck + m * 0.26f + tr * 0.2f + timp * 0.5f + sn * 1.0f;
        m_fx_tick(&l, &r, (ll + lrr) * 0.14f + m * 0.4f + (tl + tr) * 0.3f + timp * 0.25f + sn * 0.6f, (tl + tr) * 0.25f);
        lr[2 * i] = m_clip(l * 0.62f); lr[2 * i + 1] = m_clip(r * 0.62f);
    }
}

const ZMusic m_leo = {leo_m_init, leo_m_strike, leo_m_render};
