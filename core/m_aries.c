/* ARIES, the piece. Soft low horns in fourths, a little jazz: a slow swing, a bass that mostly waits, a muted
 * trumpet that says little. Heavy and dark. When the ram bleats the horns blare: two short calls and a long one.
 * Everything is chosen as it goes, so it does not come round the same way twice.
 * Heat: busier and brighter, more blares. Echo: the calls repeat. Mist: a bigger, darker room. Light: opens the horns. */
#include "music.h"

#define STEPS 12                                  /* steps a beat: swing puts the off-beat on step 8 */
#define BPM 54.0f
static const int8_t dorian[7] = {0, 2, 3, 5, 7, 9, 10};

typedef MHorn Horn;

static struct {
    f32 secs; int beats;
    f32 sr, dt, clock; uint32_t rng;
    int step, bar, key, key_bars, blare_in;
    /* the horns */
    Horn pad[4]; int pad_deg, pad_ahead, pad_rest; f32 pad_lp[2];
    /* the bass */
    f32 b_ph, b_mph, b_hz, b_env, b_pluck, b_gate; int b_idx;
    /* the trumpet */
    Horn tp; int tp_wait, tp_left, tp_idx, tp_off; f32 tp_low, tp_band, tp_vel;
    /* the cymbal */
    f32 r_env, r_lp, r_lp2;
    /* the blare */
    Horn bl[3]; int bl_pos; f32 bl_env;
    f32 heat, light, busy;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static int note_of(int idx, int base)
{
    int o = idx >= 0 ? idx / 7 : -((6 - idx) / 7), d = idx - 7 * o;
    return base + M.key + 12 * o + dorian[d];
}

#define horn_on m_horn_on

static void pad_on(void)
{
    static const int8_t next[5] = {0, 1, 3, 4, -1};          /* which step of the scale the stack of fourths stands on */
    int d; do d = next[(int)(rnd() * 5.0f)]; while (d == M.pad_deg);
    if (rnd() < 0.35f) d = 0;
    M.pad_deg = d;
    for (int v = 0; v < 4; v++) horn_on(&M.pad[v], (f32)note_of(d + 3 * v, 48) + (rnd() - 0.5f) * 0.10f, 0.992f);
}
static void pad_off(void) { for (int v = 0; v < 4; v++) M.pad[v].gate = 0.0f; }

static void bass_on(int idx, f32 vel)
{
    M.b_idx = idx; M.b_hz = m_hz((f32)note_of(idx, 36)); M.b_gate = vel; M.b_pluck = 1.0f;
}

static void blast(int which)                                 /* 0, 1: short calls; 2: the long one */
{
    static const int8_t n[3] = {48, 55, 60};
    for (int v = 0; v < 3; v++) horn_on(&M.bl[v], (f32)(n[v] + M.key) + (which == 2 && v == 2 ? 3.0f : 0.0f) + (rnd() - 0.5f) * 0.12f, 0.90f);
    m_event();
}
static void blare_start(void) { if (M.bl_pos < 0) { M.bl_pos = 1; blast(0); } }
static void blare_step(void)
{
    int p = M.bl_pos++;
    if (p == 5 || p == 12) for (int v = 0; v < 3; v++) M.bl[v].gate = 0.0f;
    if (p == 8) blast(1);
    if (p == 14) blast(2);
    if (p >= 40) { for (int v = 0; v < 3; v++) M.bl[v].gate = 0.0f; M.bl_pos = -1; }
}

static int to_grid(int steps)                                /* the next step that falls on a beat or its swung half */
{
    int at = M.step + steps;
    while (at % STEPS != 0 && at % STEPS != 8) at++;
    return at - M.step;
}

static void trumpet_step(void)
{
    if (M.tp_off > 0 && --M.tp_off == 0) M.tp.gate = 0.0f;
    if (M.tp_wait > 0) { M.tp_wait--; return; }
    if (M.bl_pos >= 0) { M.tp_wait = to_grid(24); return; }  /* it keeps out of the way of the blare */
    if (M.tp_left == 0) {                                    /* a new thought */
        M.tp_left = 2 + (int)(rnd() * (2.5f + 3.5f * M.busy));
        M.tp_idx += (int)(rnd() * 5.0f) - 2;
    }
    static const int8_t move[10] = {-2, -1, -1, -1, 1, 1, 2, 2, -3, 4};
    M.tp_idx += move[(int)(rnd() * 10.0f)];
    if (M.tp_idx < 0) M.tp_idx += 3;
    if (M.tp_idx > 11) M.tp_idx -= 4;
    int last = --M.tp_left == 0, dur;
    if (last) {
        int d = ((M.tp_idx % 7) + 7) % 7; if (d == 1 || d == 5) M.tp_idx--;      /* come to rest on a settled note */
        dur = 20 + (int)(rnd() * 28.0f);
    } else {
        f32 r = rnd();
        dur = r < 0.40f ? 4 : r < 0.75f ? 8 : r < 0.92f ? 12 : 20;
    }
    int nxt = to_grid(dur);
    M.tp_vel = 0.55f + 0.45f * rnd();
    horn_on(&M.tp, (f32)note_of(M.tp_idx, 60), 0.975f);
    M.tp_off = last ? dur : nxt > 2 ? nxt - 1 : nxt;
    M.tp_wait = (last ? to_grid(dur + (int)((14.0f + rnd() * 70.0f) * (1.5f - M.busy))) : nxt) - 1;
}

static void step(void)
{
    if (M.step == 0 && (M.bar & 7) == 0 && M.bar && rnd() < 0.3f) M.beats = 7 - M.beats;      /* four, or a slow jazz waltz */
    int s = M.step, odd = M.bar & 1, end = M.beats * STEPS;
    if (s == 0) {
        if ((M.bar & 7) == 0) {                              /* every eight bars the ground may shift */
            if (M.key != 0) { M.key = 0; M.key_bars = 0; }
            else if (M.key_bars >= 16 && rnd() < 0.35f) M.key = 3;
        }
        M.key_bars++;
        M.busy = sat(0.15f + 0.6f * M.heat + 0.5f * (noise1((f32)M.bar / 12.0f, 77u) - 0.5f));
        if (--M.blare_in <= 0) { blare_start(); M.blare_in = 5 + (int)(rnd() * 9.0f * (1.4f - M.heat)); }
    }
    /* the horns: a chord for most of two bars, a breath, the next; sometimes it comes in early, sometimes they sit out */
    if (!odd && s == 0) {
        if (M.pad_rest > 0) M.pad_rest--;
        else if (!M.pad_ahead) pad_on();
        M.pad_ahead = 0;
    }
    if (odd && s == 30) pad_off();
    if (odd && s == end - 4 && M.pad_rest == 0) {
        if (rnd() < 0.12f) M.pad_rest = 1;
        else if (rnd() < 0.4f) { pad_on(); M.pad_ahead = 1; }
    }
    /* the bass */
    if (s == 0) bass_on(0, 1.0f);
    else if (s == 24) { f32 r = rnd(); bass_on(r < 0.55f ? 4 : r < 0.8f ? 0 : r < 0.9f ? 7 : 2, 0.8f); }
    else if ((s == 12 || s == 36) && rnd() < 0.10f + 0.6f * M.busy) bass_on(M.b_idx + (rnd() < 0.5f ? 1 : -1) * (1 + (rnd() < 0.3f)), 0.6f);
    else if (s == end - 4 && rnd() < 0.3f) bass_on(rnd() < 0.5f ? -1 : 1, 0.5f);
    if (s % STEPS == 10) M.b_gate *= 0.0f;                   /* let go before the next beat */
    /* the cymbal: ding, ding-a, ding, ding-a */
    if (s % STEPS == 0) M.r_env = 0.6f + 0.4f * rnd();
    else if (s % 24 == 20) M.r_env = 0.35f + 0.25f * rnd();
    trumpet_step();
    if (M.bl_pos >= 0) blare_step();
    if (++M.step >= end) { M.step = 0; M.bar++; }
}

static void aries_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.sr = sr; M.dt = 1.0f / sr; M.rng = 0xA51E5u; M.clock = 1.0f;
    M.beats = 4; M.blare_in = 4; M.bl_pos = -1; M.tp_wait = 3 * STEPS * 4; M.tp_idx = 4; M.heat = 0.45f; M.light = 0.7f;
    for (int v = 0; v < 4; v++) { M.pad[v].pan = 0.2f + 0.2f * (f32)v; M.pad[v].vib = 0.13f * (f32)v; }
    for (int v = 0; v < 3; v++) { M.bl[v].pan = 0.3f + 0.2f * (f32)v; M.bl[v].vib = 0.3f * (f32)v; }
    M.pad_deg = 9;
    m_fx_init(sr);
}

static void aries_m_strike(void) { blare_start(); }

#define horn(h, up, down, bend, base, blown, vib) m_horn(h, M.dt, up, down, bend, base, blown, vib)

static void aries_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt;
    M.heat = sat(p->heat); M.light = sat(p->light);
    f32 heat = M.heat, light = M.light, mist = sat(p->mist), echo = sat(p->echo);
    f32 beat = 60.0f / BPM, bpm = BPM * m_drift(M.secs += (f32)n * dt, 11u, 0.12f);
    m_fx.room = 0.55f + 0.4f * mist; m_fx.damp = 0.45f + 0.35f * mist - 0.2f * light; m_fx.wet = 0.16f + 0.5f * mist;
    m_fx.echo_secs = beat * 0.6667f; m_fx.echo_fb = 0.30f + 0.45f * echo; m_fx.echo_wet = 0.25f + 0.75f * echo;
    f32 k_pad_up = m_k(0.30f, dt), k_pad_dn = m_k(0.55f, dt), k_bend = m_k(0.035f, dt);
    f32 k_tp_up = m_k(0.018f, dt), k_tp_dn = m_k(0.07f, dt), k_bl_up = m_k(0.035f, dt), k_bl_dn = m_k(0.16f, dt);
    f32 k_padlp = sat((500.0f + 1900.0f * light + 500.0f * heat - 250.0f * mist) * 6.2832f * dt);
    f32 pad_base = 0.25f + 0.5f * light, pad_blown = 0.5f + 1.2f * light + 0.8f * heat;
    f32 k_b = m_k(0.9f, dt), k_bp = m_k(0.07f, dt), k_r = m_k(0.11f + 0.08f * heat, dt);
    f32 tf = sat((1350.0f + 700.0f * light) * 6.2832f * dt), tq = 0.38f;       /* the mute: a narrow, nasal band */
    f32 ride = 0.035f + 0.07f * heat;
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        /* the blare first: the soft horns give way to it */
        f32 bl = 0.0f, bll = 0.0f, blr = 0.0f, be = 0.0f;
        for (int v = 0; v < 3; v++) {
            f32 x = horn(&M.bl[v], k_bl_up, k_bl_dn, k_bend, 1.4f, 2.6f + 1.2f * heat, 0.004f);
            bl += x; bll += x * (1.0f - M.bl[v].pan); blr += x * M.bl[v].pan; if (M.bl[v].env > be) be = M.bl[v].env;
        }
        M.bl_env = be;
        f32 pl = 0.0f, pr = 0.0f;
        for (int v = 0; v < 4; v++) {
            f32 x = horn(&M.pad[v], k_pad_up, k_pad_dn, k_bend, pad_base, pad_blown, 0.003f);
            x += m_noise(&M.rng) * 0.035f * M.pad[v].env;                       /* breath */
            pl += x * (1.0f - M.pad[v].pan); pr += x * M.pad[v].pan;
        }
        M.pad_lp[0] += (pl - M.pad_lp[0]) * k_padlp; M.pad_lp[1] += (pr - M.pad_lp[1]) * k_padlp;
        f32 duck = 1.0f - 0.55f * be;
        pl = M.pad_lp[0] * 0.30f * duck; pr = M.pad_lp[1] * 0.30f * duck;
        /* the bass: plucked, round */
        M.b_env += (M.b_gate - M.b_env) * (M.b_gate > M.b_env ? 0.02f : k_b * (M.b_gate > 0.0f ? 1.0f : 6.0f));
        if (M.b_gate > 0.0f) M.b_gate *= 1.0f - k_b;
        M.b_pluck -= M.b_pluck * k_bp;
        M.b_ph += M.b_hz * dt; if (M.b_ph >= 1.0f) M.b_ph -= 1.0f;
        M.b_mph += M.b_hz * 2.0f * dt; if (M.b_mph >= 1.0f) M.b_mph -= 1.0f;
        f32 bass = sin_turns(M.b_ph + (0.10f + 0.35f * M.b_pluck) * sin_turns(M.b_mph)) * M.b_env * 0.36f;
        /* the trumpet through its mute */
        f32 t = horn(&M.tp, k_tp_up, k_tp_dn, k_bend, 1.2f, 2.2f, 0.006f) + m_noise(&M.rng) * 0.05f * M.tp.env;
        M.tp_low += tf * M.tp_band;
        f32 hp = t - M.tp_low - tq * M.tp_band;
        M.tp_band += tf * hp;
        f32 tp = (M.tp_band * 0.9f + hp * 0.25f) * M.tp_vel * 0.20f;
        /* the cymbal */
        M.r_env -= M.r_env * k_r;
        f32 nz = m_noise(&M.rng);
        M.r_lp += (nz - M.r_lp) * 0.55f; M.r_lp2 += ((nz - M.r_lp) - M.r_lp2) * 0.5f;
        f32 cym = ((nz - M.r_lp) - M.r_lp2) * M.r_env * M.r_env * ride;
        f32 l = pl + bass + tp * 0.8f + cym * 0.9f + bll * 0.20f, r = pr + bass + tp * 1.2f + cym * 1.1f + blr * 0.20f;
        m_fx_tick(&l, &r, (pl + pr) * 0.6f + tp * 1.6f + bl * 0.16f + cym * 0.5f + bass * 0.12f, tp * 0.9f + bl * 0.14f);
        lr[2 * i] = m_clip(l * 0.8f); lr[2 * i + 1] = m_clip(r * 0.8f);
    }
}

const ZMusic m_aries = {aries_m_init, aries_m_strike, aries_m_render};
