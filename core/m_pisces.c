/* PISCES, the piece. A rushing torrent, a waterfall: the low roar, the rush, the spray, never still, with drops
 * and bubbles in it; and far inside the water three soft notes that change now and then. A key: a fish darts,
 * a splash and a string of bubbles.
 * Dream: the water stands back and the notes come forward. Echo, Mist: as everywhere. Light: more spray. */
#include "music.h"

#define NBUB 6
static const int8_t sets[4][3] = {{49, 56, 63}, {47, 54, 61}, {44, 56, 60}, {42, 54, 61}};

static struct {
    f32 dt, t; uint32_t rng, n;
    f32 lo1, lo2, m_low[2], m_band[2], hi[2], m_f, m_amp, h_amp, lo_amp;
    struct { f32 ph, hz, env; } bub[NBUB]; int bi;
    MHorn pad[3]; f32 next_set; int set;
    f32 splash, s_low, s_band, dream;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }
static void bubble(f32 vel) { int k = M.bi; M.bi = (M.bi + 1) % NBUB; M.bub[k].hz = 500.0f + 2200.0f * rnd() * rnd(); M.bub[k].env = vel; }

static void control(f32 dt)
{
    M.t += dt;
    M.m_f = sat((500.0f + 1100.0f * noise1(M.t * 0.6f, 3u)) * 6.2832f * M.dt);
    M.m_amp = 0.6f + 0.4f * noise1(M.t * 0.9f, 4u); M.h_amp = 0.5f + 0.5f * noise1(M.t * 1.3f, 6u); M.lo_amp = 0.7f + 0.3f * noise1(M.t * 0.3f, 8u);
    if (rnd() < (5.0f + 9.0f * M.dream) * dt) bubble(0.25f + 0.4f * rnd());
    if ((M.next_set -= dt) < 0.0f) {
        M.set = (M.set + 1 + (int)(rnd() * 3.0f)) & 3; M.next_set = 10.0f + 10.0f * rnd();
        for (int k = 0; k < 3; k++) { M.pad[k].aim = m_hz((f32)sets[M.set][k]); M.pad[k].gate = 1.0f; }
    }
}

static void pisces_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0x915CE5u;
    for (int k = 0; k < 3; k++) { M.pad[k].hz = m_hz((f32)sets[0][k]); M.pad[k].pan = 0.25f + 0.25f * (f32)k; M.pad[k].vib = 0.3f * (f32)k; }
    m_fx_init(sr);
}

static void pisces_m_strike(void) { M.splash = 1.0f; for (int k = 0; k < 4; k++) bubble(0.5f + 0.4f * rnd()); m_event(); }

static void pisces_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.dream = sat(p->heat);
    m_fx.room = 0.6f + 0.38f * mist; m_fx.damp = 0.4f + 0.35f * mist - 0.15f * light; m_fx.wet = 0.2f + 0.5f * mist;
    m_fx.echo_secs = 0.45f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 klo = sat(170.0f * 6.2832f * dt), khi = sat(2800.0f * 6.2832f * dt), bdec = m_k(0.035f, dt), sdec = m_k(0.22f, dt);
    f32 up = m_k(2.5f, dt), bend = m_k(1.5f, dt), water = 1.0f - 0.65f * M.dream, pad = 0.07f + 0.16f * M.dream, spray = 0.05f + 0.16f * light;
    for (int i = 0; i < n; i++) {
        if ((M.n++ & 63u) == 0) control(64.0f * dt);
        f32 nz[2] = {m_noise(&M.rng), m_noise(&M.rng)}, w[2];
        M.lo1 += (nz[0] + nz[1] - M.lo1) * klo; M.lo2 += (M.lo1 - M.lo2) * klo;
        for (int c = 0; c < 2; c++) {
            M.m_low[c] += M.m_f * M.m_band[c];
            f32 hp = nz[c] - M.m_low[c] - 1.2f * M.m_band[c];
            M.m_band[c] += M.m_f * hp;
            M.hi[c] += (nz[c] - M.hi[c]) * khi;
            w[c] = (M.lo2 * 2.2f * M.lo_amp + M.m_band[c] * 0.28f * M.m_amp + (nz[c] - M.hi[c]) * spray * M.h_amp) * water;
        }
        f32 bb = 0.0f;
        for (int k = 0; k < NBUB; k++) if (M.bub[k].env > 0.001f) {
            M.bub[k].env -= M.bub[k].env * bdec; M.bub[k].hz *= 1.0f + 9.0f * dt;
            M.bub[k].ph += M.bub[k].hz * dt; if (M.bub[k].ph >= 1.0f) M.bub[k].ph -= 1.0f;
            bb += sin_turns(M.bub[k].ph) * M.bub[k].env;
        }
        M.splash -= M.splash * sdec;
        f32 sf = sat((500.0f + 2600.0f * M.splash) * 6.2832f * dt);
        M.s_low += sf * M.s_band; f32 shp = nz[0] - M.s_low - 0.8f * M.s_band; M.s_band += sf * shp;
        f32 sp = (M.s_band + shp * 0.3f) * M.splash * 0.5f;
        f32 l = w[0] + bb * 0.14f + sp * 0.8f, r = w[1] + bb * 0.18f + sp * 1.2f, ps = 0.0f;
        for (int k = 0; k < 3; k++) { f32 x = m_horn(&M.pad[k], dt, up, up, bend, 0.1f, 0.3f, 0.004f) * pad; l += x * (1.0f - M.pad[k].pan) * 2.0f; r += x * M.pad[k].pan * 2.0f; ps += x; }
        m_fx_tick(&l, &r, ps * 2.5f + bb * 0.3f + sp * 0.6f + (w[0] + w[1]) * 0.15f, bb * 0.2f + sp * 0.4f + ps);
        lr[2 * i] = m_clip(l * 0.85f); lr[2 * i + 1] = m_clip(r * 0.85f);
    }
}

const ZMusic m_pisces = {pisces_m_init, pisces_m_strike, pisces_m_render};
