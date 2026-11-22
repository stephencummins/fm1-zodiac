/* TAURUS, the piece. Cellos on long notes over a low bass that plods, one hoof a beat. A huff when a key is pressed.
 * Growth: the top cello sings more and the tone opens. Echo, Mist: as everywhere. Light: brighter strings. */
#include "music.h"

#define STEPS 4
#define BPM 46.0f
static const int8_t mixo[7] = {0, 2, 4, 5, 7, 9, 10};
static const int8_t prog[4][4] = {{0, 3, 0, 4}, {0, 6, 3, 0}, {0, 5, 3, 4}, {0, 3, 6, 0}};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, deg, low, pr, key, reg;
    MBow lo[3], mel; int mel_idx, mel_wait, mel_off;
    MDrum hoof;
    f32 h_env, h_t, h_low, h_band; int h_on;
    f32 growth;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static f32 note(int idx, int base) { return (f32)(base + M.key + m_scale(mixo, idx)); }

static void step(void)
{
    int s = M.step;
    if (s == 0 && (M.bar & 7) == 0 && M.bar && rnd() < 0.3f) M.beats = 7 - M.beats;      /* the time may change: three, or four */
    if (s == 0 && (M.bar & 7) == 0) { M.key = m_slow.key; M.reg = m_slow.reg; }
    if (s == 0 && !(M.bar & 1)) {                            /* a chord lasts two bars */
        int unit = (M.bar >> 1) & 3;
        if (unit == 0) M.pr = (int)(rnd() * 4.0f);
        M.deg = prog[M.pr][unit]; M.low = M.deg > 3 ? M.deg - 7 : M.deg;
        m_bow_on(&M.lo[0], note(M.low, 43), 0); m_bow_on(&M.lo[1], note(M.low + 4, 43) + 0.06f, 0); m_bow_on(&M.lo[2], note(M.low, 31), 0);
    }
    if (s == M.beats * STEPS - 2 && (M.bar & 1)) for (int v = 0; v < 3; v++) M.lo[v].gate = 0.0f;       /* the bows turn */
    if (s % STEPS == 0) {                                    /* the plod */
        int b = s / STEPS;
        f32 n = b == 2 ? note(M.low < 0 ? M.low + 4 : M.low - 3, 31) : note(M.low, 31);
        m_drum_on(&M.hoof, m_hz(n), b == 0 ? 1.0f : b == 2 ? 0.8f : 0.5f, 1.3f);
    }
    if (M.mel_off > 0 && --M.mel_off == 0) M.mel.gate = 0.0f;
    if (M.mel_wait > 0) M.mel_wait--;
    if (M.mel_wait == 0 && s % STEPS == 0) {
        int t = M.mel_idx + (int)(rnd() * 5.0f) - 2;
        if (rnd() > 0.25f) for (int d = 0; d < 4; d++) {     /* mostly a note of the chord */
            int a = (((t - d - M.deg) % 7) + 7) % 7, b = (((t + d - M.deg) % 7) + 7) % 7;
            if (a == 0 || a == 2 || a == 4) { t -= d; break; }
            if (b == 0 || b == 2 || b == 4) { t += d; break; }
        }
        if (t < M.reg) t += 4; if (t > 11 + M.reg) t -= 4;
        f32 r = rnd();
        int dur = r < 0.3f ? 2 : r < 0.6f ? 4 : r < 0.8f ? 3 : r < 0.93f ? 6 : 8;
        int rest = rnd() < 0.40f - 0.3f * M.growth - 0.2f * m_slow.fill ? 2 + 2 * (int)(rnd() * 3.0f) : 0;
        m_bow_on(&M.mel, note(t, 55), 1);
        M.mel_idx = t; M.mel_off = rest ? dur * STEPS - 1 : 0; M.mel_wait = (dur + rest) * STEPS;
    }
    if (++M.step >= M.beats * STEPS) { M.step = 0; M.bar++; }
}

static void taurus_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = m_seeded(0x7A0205u); M.clock = 1.0f; M.beats = 4; M.mel_idx = 4; M.mel_wait = 2 * 4 * STEPS + 1;
    M.lo[0].pan = 0.3f; M.lo[1].pan = 0.7f; M.lo[2].pan = 0.5f; M.lo[1].vib = 0.4f; M.mel.vib = 0.7f;
    m_fx_init(sr);
}

static void taurus_m_strike(void) { M.h_on = 1; M.h_t = 0.0f; m_event(); }

static void taurus_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.growth = sat(p->heat);
    m_fx.room = 0.6f + 0.35f * mist; m_fx.damp = 0.5f + 0.3f * mist - 0.2f * light; m_fx.wet = 0.18f + 0.5f * mist;
    m_fx.echo_secs = 60.0f / BPM * 0.5f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 up = m_k(0.22f, dt), dn = m_k(0.35f, dt), glide = m_k(0.09f, dt);
    f32 cut = sat((450.0f + 1300.0f * light + 500.0f * M.growth) * 6.2832f * dt), bcut = sat(260.0f * 6.2832f * dt);
    f32 hdec = m_k(0.5f, dt), hbend = m_k(0.03f, dt), hup = m_k(0.025f, dt), hdn = m_k(0.20f, dt);
    f32 bpm = BPM * m_drift(M.secs += (f32)n * dt, 12u, 0.14f);
    f32 v0 = m_slow.v[0], v1 = m_slow.v[1], v2 = m_slow.v[2];
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        f32 a = m_bow(&M.lo[0], dt, up, dn, glide, 0.004f, cut), b = m_bow(&M.lo[1], dt, up, dn, glide, 0.004f, cut); a *= v0; b *= v0;
        f32 c = m_bow(&M.lo[2], dt, up, dn, glide, 0.002f, bcut) * 1.5f * (0.4f + 0.6f * v2);
        f32 m = m_bow(&M.mel, dt, up, dn, glide, 0.007f, cut) * (0.8f + 0.4f * M.growth) * v1;
        f32 hoof = m_drum(&M.hoof, dt, hdec, hbend);
        f32 huff = 0.0f;
        if (M.h_on) {
            M.h_t += dt;
            f32 aim = M.h_t < 0.10f ? 1.0f : 0.0f;
            M.h_env += (aim - M.h_env) * (aim > M.h_env ? hup : hdn);
            if (M.h_t > 1.3f) { M.h_on = 0; M.h_env = 0.0f; }
            f32 f = sat((950.0f - 500.0f * sat(M.h_t * 2.5f)) * 6.2832f * dt), nz = m_noise(&M.rng);
            M.h_low += f * M.h_band;
            f32 hp = nz - M.h_low - 0.9f * M.h_band;
            M.h_band += f * hp;
            huff = (M.h_band * 0.8f + hp * 0.12f) * M.h_env * 0.55f;
        }
        f32 l = a * 0.7f + b * 0.3f + c + m * 0.45f + hoof * 1.6f + huff * 2.6f;
        f32 r = a * 0.3f + b * 0.7f + c + m * 0.75f + hoof * 1.6f + huff * 3.2f;
        m_fx_tick(&l, &r, (a + b) * 0.5f + m * 0.9f + huff * 0.8f + hoof * 0.1f, m * 0.7f + huff * 0.5f);
        lr[2 * i] = m_clip(l * 0.24f); lr[2 * i + 1] = m_clip(r * 0.24f);
    }
}

const ZMusic m_taurus = {taurus_m_init, taurus_m_strike, taurus_m_render};
