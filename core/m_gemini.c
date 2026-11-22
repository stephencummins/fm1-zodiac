/* GEMINI, the piece. A light dance in six for two harpsichord hands, the twins: one plays the tune, the other
 * answers it a third higher, turn and turn about. A key sets both running up the scale together.
 * Wind: faster, more ornaments. Echo, Mist: as everywhere. Light: brighter strings. */
#include "music.h"

#define STEPS 2                                   /* steps to a quaver; six quavers a bar */
#define NV 12
static const int8_t major[7] = {0, 2, 4, 5, 7, 9, 11};
static const int8_t prog[4][4] = {{0, 4, 5, 3}, {0, 3, 4, 0}, {0, 5, 1, 4}, {0, 0, 3, 4}};
static const uint8_t rhythm[5] = {0x3f, 0x3d, 0x2d, 0x2f, 0x39};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, deg, pr, rh, run, vi, last_v[2], last_idx[2], key, reg;
    int8_t motif[6];
    MPluck v[NV];
    f32 wind;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }

static int play(int idx, int base, f32 vel, f32 pan, f32 secs)
{
    int k = M.vi; M.vi = (M.vi + 1) % NV;
    m_pluck_on(&M.v[k], (f32)(base + M.key + m_scale(major, idx)), vel, m_k(secs, M.dt));
    M.v[k].pan = pan;
    return k;
}
static void twin(int who, int idx, f32 vel)
{
    M.v[M.last_v[who]].gate = 0.0f;                          /* the key comes up as the next goes down */
    M.last_v[who] = play(idx + M.reg, 74, vel * m_slow.v[who], who ? 0.72f : 0.28f, 0.7f);
    M.last_idx[who] = idx;
}

static void new_motif(void)
{
    static const int8_t move[6] = {-1, 1, 1, -1, 2, -2};
    int at = 2 * (int)(rnd() * 3.0f);
    for (int i = 0; i < 6; i++) {
        if (i) at += move[(int)(rnd() * 6.0f)];
        if (i == 3 && (at & 1)) at++;
        if (at < 0) at += 2; if (at > 8) at -= 3;
        M.motif[i] = (int8_t)at;
    }
    M.rh = rhythm[(int)(rnd() * 5.0f)];
}

static void step(void)
{
    int s = M.step, pos = s / STEPS, half = s % STEPS, b = M.bar & 3;
    if (s == 0) {
        if (b == 0) { if ((M.bar & 7) == 0) { M.key = m_slow.key; M.reg = m_slow.reg; new_motif(); if (M.bar && rnd() < 0.35f) M.beats = 5 - M.beats; }      /* two threes, or three twos */
                      M.pr = (int)(rnd() * 4.0f); }
        M.deg = prog[M.pr][b]; if (M.deg > 3) M.deg -= 7;
    }
    if (M.run > 0) {                                         /* both twins run up together */
        int k = 8 - M.run;
        twin(0, M.deg + k, 0.8f); twin(1, M.deg + k + 2, 0.8f);
        M.run--;
    } else if (!half) {
        int lead = b & 1, on = (M.rh >> pos) & 1, off = M.motif[pos];
        if (b == 3 && pos >= 3) { on = pos == 3; off = 0; }  /* the phrase comes home */
        if (on && (pos % M.beats == 0 || rnd() > -0.5f * m_slow.fill)) twin(lead, M.deg + off + (lead ? 2 : 0), pos % M.beats == 0 ? 0.9f : 0.65f);
        if (pos % M.beats == 0) twin(!lead, M.deg + (pos ? 4 : 2) - 7 + (lead ? 0 : 2), 0.45f);
        if (pos == 0) play(M.deg, 50, 0.6f * m_slow.v[2], 0.5f, 1.2f);
        if (pos == (M.beats == 2 ? 4 : 3)) play(M.deg > 1 ? M.deg - 3 : M.deg + 4, 50, 0.4f * m_slow.v[2], 0.5f, 1.0f);
    } else if (rnd() < 0.08f + 0.35f * M.wind + 0.12f * m_slow.fill && pos != 5) {
        int lead = b & 1;
        twin(lead, M.last_idx[lead] + 1, 0.4f);              /* a quick note between */
    }
    if (++M.step >= 6 * STEPS) { M.step = 0; M.bar++; }
}

static void gemini_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = m_seeded(0x6E3141u); M.clock = 1.0f; M.beats = 3; M.last_v[1] = 1; M.vi = 2;
    m_fx_init(sr);
}

static void gemini_m_strike(void) { if (M.run == 0) { M.run = 8; m_event(); } }

static void gemini_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.wind = sat(p->heat);
    f32 nominal = (84.0f + 30.0f * M.wind) / 60.0f * 3.0f * (f32)STEPS, rate = nominal * m_drift(M.secs += (f32)n * dt, 13u, 0.14f);
    m_fx.room = 0.4f + 0.5f * mist; m_fx.damp = 0.3f + 0.4f * mist - 0.15f * light; m_fx.wet = 0.12f + 0.5f * mist;
    m_fx.echo_secs = 3.0f * (f32)STEPS / (96.0f / 60.0f * 3.0f * (f32)STEPS); (void)nominal; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 rel = m_k(0.05f, dt), cut = sat((2200.0f + 6000.0f * light) * 6.2832f * dt);
    for (int i = 0; i < n; i++) {
        M.clock += rate * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        f32 l = 0.0f, r = 0.0f;
        for (int k = 0; k < NV; k++) { f32 x = m_pluck(&M.v[k], dt, rel, cut); l += x * (1.0f - M.v[k].pan); r += x * M.v[k].pan; }
        l *= 0.5f; r *= 0.5f;
        m_fx_tick(&l, &r, (l + r) * 0.8f, (l + r) * 0.6f);
        lr[2 * i] = m_clip(l * 0.8f); lr[2 * i + 1] = m_clip(r * 0.8f);
    }
}

const ZMusic m_gemini = {gemini_m_init, gemini_m_strike, gemini_m_render};
