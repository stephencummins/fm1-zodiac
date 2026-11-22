/* SCORPIO, the piece. Deep, warm notes: a low ground, a slow tune in the tenor with a second voice a sixth
 * under it now and then. Slow, but sweet. A key: one very deep note and a small bell far above it.
 * Depth: lower and fuller, more of the second voice. Echo, Mist: as everywhere. Light: opens the tone. */
#include "music.h"

#define STEPS 2
#define BPM 44.0f
static const int8_t major[7] = {0, 2, 4, 5, 7, 9, 11};
static const int8_t prog[3][4] = {{0, 3, 5, 4}, {0, 5, 3, 0}, {0, 2, 3, 4}};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, deg, low, pr, idx, wait, off;
    MHorn pad[2], mel, sixth;
    MDrum deep; MBell bell;
    f32 depth;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static f32 note(int idx, int base) { return (f32)(base + m_scale(major, idx)); }

static void step(void)
{
    int s = M.step;
    if (s == 0 && (M.bar & 7) == 0 && M.bar && rnd() < 0.3f) M.beats = 7 - M.beats;      /* the time may change: three, or four */
    if (s == 0 && !(M.bar & 1)) {
        int u = (M.bar >> 1) & 3;
        if (u == 0) M.pr = (int)(rnd() * 3.0f);
        M.deg = prog[M.pr][u]; M.low = M.deg > 2 ? M.deg - 7 : M.deg;
        m_horn_on(&M.pad[0], note(M.low, 37), 1.0f); m_horn_on(&M.pad[1], note(M.low + 9, 37), 1.0f);
    }
    if (s == M.beats * STEPS - 1 && (M.bar & 1)) M.pad[0].gate = M.pad[1].gate = 0.0f;
    if (M.off > 0 && --M.off == 0) M.mel.gate = M.sixth.gate = 0.0f;
    if (M.wait > 0) M.wait--;
    if (M.wait == 0 && !(s & 1)) {
        int t = M.idx + (rnd() < 0.7f ? (rnd() < 0.5f ? 1 : -1) : (int)(rnd() * 5.0f) - 2);
        if (s == 0) for (int d = 0; d < 3; d++) {
            int a = (((t - d - M.deg) % 7) + 7) % 7, b = (((t + d - M.deg) % 7) + 7) % 7;
            if (a == 0 || a == 2 || a == 4) { t -= d; break; }
            if (b == 0 || b == 2 || b == 4) { t += d; break; }
        }
        if (t < 0) t += 3; if (t > 9) t -= 3;
        f32 r = rnd();
        int dur = (r < 0.35f ? 2 : r < 0.7f ? 3 : r < 0.9f ? 4 : 6) * STEPS, rest = rnd() < 0.3f ? 2 * STEPS : 0;
        m_horn_on(&M.mel, note(t, 49), 0.995f);
        if (rnd() < 0.25f + 0.5f * M.depth) m_horn_on(&M.sixth, note(t - 5, 49), 0.995f); else M.sixth.gate = 0.0f;
        M.idx = t; M.off = dur - 1; M.wait = dur + rest;
    }
    if (++M.step >= M.beats * STEPS) { M.step = 0; M.bar++; }
}

static void scorpio_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0x5C0B10u; M.clock = 1.0f; M.beats = 4; M.idx = 4; M.wait = 4 * STEPS + 1;
    m_fx_init(sr);
}

static void scorpio_m_strike(void)
{
    m_drum_on(&M.deep, m_hz(note(M.low, 25)), 1.0f, 2.0f);
    m_bell_on(&M.bell, note(M.deg + 2, 73), 0.6f, m_k(2.8f, M.dt));
    m_event();
}

static void scorpio_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.depth = sat(p->heat);
    m_fx.room = 0.7f + 0.28f * mist; m_fx.damp = 0.6f + 0.3f * mist - 0.2f * light; m_fx.wet = 0.25f + 0.5f * mist;
    m_fx.echo_secs = 60.0f / BPM * 0.5f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = 0.1f + 0.9f * echo;
    f32 pup = m_k(0.5f, dt), pdn = m_k(0.9f, dt), up = m_k(0.12f, dt), dn = m_k(0.5f, dt), bend = m_k(0.05f, dt);
    f32 ddec = m_k(1.6f, dt), dbend = m_k(0.06f, dt);
    f32 base = 0.15f + 0.4f * light, blown = 0.4f + 0.8f * light;
    f32 bpm = BPM * m_drift(M.secs += (f32)n * dt, 17u, 0.14f);
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        f32 a = m_horn(&M.pad[0], dt, pup, pdn, bend, base, blown, 0.0f) * (0.5f + 0.3f * M.depth);
        f32 b = m_horn(&M.pad[1], dt, pup, pdn, bend, base * 0.6f, blown * 0.5f, 0.002f) * 0.3f;
        f32 m = m_horn(&M.mel, dt, up, dn, bend, base + 0.2f, blown + 0.3f, 0.004f) * 0.42f;
        f32 x = m_horn(&M.sixth, dt, up, dn, bend, base, blown, 0.003f) * 0.26f;
        f32 d = m_drum(&M.deep, dt, ddec, dbend) * 0.6f, bl = m_bell(&M.bell, dt, 3.0f, 0.12f) * 0.25f;
        f32 l = a + b * 0.7f + m * 0.9f + x * 1.2f + d + bl * 0.6f, r = a + b * 1.3f + m * 1.1f + x * 0.8f + d + bl * 1.4f;
        m_fx_tick(&l, &r, a * 0.3f + b + m * 1.1f + x + bl * 2.0f + d * 0.3f, m * 0.6f + bl * 1.5f);
        lr[2 * i] = m_clip(l * 0.52f); lr[2 * i + 1] = m_clip(r * 0.52f);
    }
}

const ZMusic m_scorpio = {scorpio_m_init, scorpio_m_strike, scorpio_m_render};
