/* LIBRA, the piece. Violins that float: four high lines, each moving in its own time and sliding to its next note,
 * no beat anywhere; and the wind, coming and going. A key sends a gust through.
 * Breeze: more wind, the lines move sooner. Echo, Mist: as everywhere. Light: brighter. */
#include "music.h"

static const int8_t lydian[7] = {0, 2, 4, 6, 7, 9, 11};
static const int8_t open[6] = {0, 1, 2, 4, 5, 7};            /* steps of the scale that sit well together */

static struct {
    f32 dt, t; uint32_t rng, n;
    MBow v[4]; f32 wait[4];
    f32 w_low[2], w_band[2], w_f, w_amp[2], gust, gust_t, breeze;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }

static void move(int k, int up)
{
    int idx = open[(int)(rnd() * 6.0f)] + (k >= 2 ? 4 : 0) + (up ? 7 : 0);
    m_bow_on(&M.v[k], (f32)(65 + m_scale(lydian, idx)) + (rnd() - 0.5f) * 0.08f, 1);
    M.v[k].gate = 0.35f + 0.65f * rnd();
    M.wait[k] = (5.0f + 10.0f * rnd()) * (1.3f - 0.6f * M.breeze);
}

static void control(f32 dt)
{
    M.t += dt;
    for (int k = 0; k < 4; k++) if ((M.wait[k] -= dt) < 0.0f) {
        if (rnd() < 0.15f) { M.v[k].gate = 0.0f; M.wait[k] = 3.0f + 4.0f * rnd(); }      /* it rests */
        else move(k, 0);
    }
    if (M.gust_t >= 0.0f) { M.gust_t += dt; f32 aim = M.gust_t < 0.5f ? 1.0f : 0.0f; M.gust += (aim - M.gust) * m_k(aim > M.gust ? 0.2f : 1.4f, dt); if (M.gust_t > 7.0f) M.gust_t = -1.0f; }
    f32 a = noise1(M.t * 0.13f, 5u), b = noise1(M.t * 0.13f + 0.4f, 5u);
    f32 lev = 0.10f + 0.5f * M.breeze;
    M.w_amp[0] = a * a * lev + M.gust * 0.6f; M.w_amp[1] = b * b * lev + M.gust * 0.6f;
    M.w_f = sat((350.0f + 1300.0f * noise1(M.t * 0.21f, 9u) + 1200.0f * M.gust) * 6.2832f * M.dt);
}

static void libra_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0x71B2Au; M.gust_t = -1.0f; M.breeze = 0.45f;
    for (int k = 0; k < 4; k++) { M.v[k].pan = 0.15f + 0.23f * (f32)k; M.v[k].vib = 0.21f * (f32)k; move(k, 0); M.wait[k] = 2.0f + 3.0f * (f32)k; }
    m_fx_init(sr);
}

static void libra_m_strike(void) { M.gust_t = 0.0f; move((int)(rnd() * 4.0f), 1); m_event(); }

static void libra_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.breeze = sat(p->heat);
    m_fx.room = 0.8f + 0.18f * mist; m_fx.damp = 0.3f + 0.4f * mist - 0.15f * light; m_fx.wet = 0.35f + 0.45f * mist;
    m_fx.echo_secs = 0.7f; m_fx.echo_fb = 0.35f + 0.4f * echo; m_fx.echo_wet = 0.15f + 0.85f * echo;
    f32 up = m_k(1.6f, dt), dn = m_k(2.2f, dt), glide = m_k(0.5f, dt), cut = sat((1500.0f + 3000.0f * light) * 6.2832f * dt);
    for (int i = 0; i < n; i++) {
        if ((M.n++ & 63u) == 0) control(64.0f * dt);
        f32 l = 0.0f, r = 0.0f;
        for (int k = 0; k < 4; k++) { f32 x = m_bow(&M.v[k], dt, up, dn, glide, 0.006f, cut) * 0.3f; l += x * (1.0f - M.v[k].pan); r += x * M.v[k].pan; }
        f32 w[2];
        for (int c = 0; c < 2; c++) {
            f32 nz = m_noise(&M.rng);
            M.w_low[c] += M.w_f * M.w_band[c];
            f32 hp = nz - M.w_low[c] - 0.5f * M.w_band[c];
            M.w_band[c] += M.w_f * hp;
            w[c] = M.w_band[c] * M.w_amp[c] * 0.35f;
        }
        f32 send = (l + r) * 1.2f + (w[0] + w[1]) * 0.4f;
        l += w[0]; r += w[1];
        m_fx_tick(&l, &r, send, (l + r) * 0.5f);
        lr[2 * i] = m_clip(l * 0.9f); lr[2 * i + 1] = m_clip(r * 0.9f);
    }
}

const ZMusic m_libra = {libra_m_init, libra_m_strike, libra_m_render};
