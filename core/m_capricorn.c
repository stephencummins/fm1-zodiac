/* CAPRICORN, the piece. Alpine horns: only the notes a long wooden horn has, called slowly across a valley and
 * coming back off the far side, a second horn answering from a distance. Under it the mountain rumbles, and
 * now and then, or at a key, it shakes.
 * Climb: the calls go higher, the ground is less quiet. Echo: more of the valley. Mist, Light: as everywhere. */
#include "music.h"

#define FUND 46.25f                                /* the horn's lowest note; it plays the fourth to the twelfth above */

static struct {
    f32 dt; uint32_t rng, n;
    MHorn hn[2]; int h[2], left[2]; f32 wait[2], off[2];
    f32 quake, quake_t, next_quake, r_lp1, r_lp2, r_ph, r_wob, climb;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }

static void call(int k)
{
    static const int8_t rest[3] = {4, 6, 8};
    if (M.left[k] == 0) { M.left[k] = 3 + (int)(rnd() * 4.0f); M.h[k] = rest[(int)(rnd() * 3.0f)]; }
    else M.h[k] += (int)(rnd() * 5.0f) - 2 + (rnd() < 0.3f + 0.3f * M.climb);
    int top = 9 + (int)(3.9f * M.climb);
    if (M.h[k] < 4) M.h[k] = 5; if (M.h[k] > top) M.h[k] = top - 1;
    int last = --M.left[k] == 0;
    if (last) M.h[k] = M.h[k] >= 7 ? 8 : M.h[k] >= 5 ? 6 : 4;
    M.hn[k].aim = FUND * (f32)M.h[k]; M.hn[k].hz = M.hn[k].aim * 0.96f; M.hn[k].gate = 1.0f; M.hn[k].age = 0.0f;
    f32 dur = last ? 3.0f + 2.5f * rnd() : 0.7f + 1.6f * rnd();
    M.off[k] = dur * (last ? 1.0f : 0.92f);
    M.wait[k] = last ? dur + 4.0f + 8.0f * rnd() : dur;
    if (last && k == 0 && M.wait[1] > 2.0f && M.left[1] == 0) M.wait[1] = dur + 1.0f;        /* the far horn answers */
}

static void control(f32 dt)
{
    for (int k = 0; k < 2; k++) {
        if (M.off[k] > 0.0f && (M.off[k] -= dt) <= 0.0f) M.hn[k].gate = 0.0f;
        if ((M.wait[k] -= dt) < 0.0f) call(k);
    }
    if ((M.next_quake -= dt) < 0.0f && M.quake_t < 0.0f) { M.quake_t = 0.0f; m_event(); }
    if (M.quake_t >= 0.0f) {
        if (M.quake_t == 0.0f) M.next_quake = (45.0f + 60.0f * rnd()) * (1.3f - 0.8f * M.climb);
        M.quake_t += dt;
        f32 aim = M.quake_t < 2.2f ? 1.0f : 0.0f;
        M.quake += (aim - M.quake) * m_k(aim > M.quake ? 0.5f : 1.8f, dt);
        if (M.quake_t > 9.0f) M.quake_t = -1.0f;
    }
}

static void capricorn_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = 0xCA9210u; M.quake_t = -1.0f; M.next_quake = 30.0f;
    M.wait[0] = 1.0f; M.wait[1] = 14.0f; M.hn[0].pan = 0.45f; M.hn[1].pan = 0.8f;
    m_fx_init(sr);
}

static void capricorn_m_strike(void) { if (M.quake_t < 0.0f || M.quake_t > 2.2f) { M.quake_t = 0.0f; m_event(); } }

static void capricorn_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.climb = sat(p->heat);
    m_fx.room = 0.8f + 0.18f * mist; m_fx.damp = 0.45f + 0.3f * mist - 0.15f * light; m_fx.wet = 0.3f + 0.45f * mist;
    m_fx.echo_secs = 0.95f; m_fx.echo_fb = 0.35f + 0.35f * echo; m_fx.echo_wet = 0.4f + 0.6f * echo;
    f32 up = m_k(0.10f, dt), dn = m_k(0.25f, dt), bend = m_k(0.05f, dt), rk = sat(55.0f * 6.2832f * dt);
    f32 base = 0.5f + 0.5f * light, blown = 1.2f + 1.0f * light;
    for (int i = 0; i < n; i++) {
        if ((M.n++ & 63u) == 0) control(64.0f * dt);
        f32 a = m_horn(&M.hn[0], dt, up, dn, bend, base, blown, 0.002f) * 0.36f;
        f32 b = m_horn(&M.hn[1], dt, up, dn, bend, base * 0.7f, blown * 0.6f, 0.002f) * 0.13f;
        f32 nz = m_noise(&M.rng);
        M.r_lp1 += (nz - M.r_lp1) * rk; M.r_lp2 += (M.r_lp1 - M.r_lp2) * rk;
        M.r_wob += 0.37f * dt; if (M.r_wob >= 1.0f) M.r_wob -= 1.0f;
        M.r_ph += (33.0f + 6.0f * sin_turns(M.r_wob)) * dt; if (M.r_ph >= 1.0f) M.r_ph -= 1.0f;
        f32 ground = 0.06f + 0.10f * M.climb + M.quake;
        f32 rum = (M.r_lp2 * 9.0f + sin_turns(M.r_ph) * (0.5f + M.r_lp2 * 14.0f)) * ground * 0.45f;
        f32 l = a * (1.0f - M.hn[0].pan) * 2.0f + b * 0.8f + rum, r = a * M.hn[0].pan * 2.0f + b * 1.2f + rum;
        m_fx_tick(&l, &r, a * 1.2f + b * 5.0f + rum * 0.15f, a * 1.0f + b * 1.5f);
        lr[2 * i] = m_clip(l * 0.7f); lr[2 * i + 1] = m_clip(r * 0.7f);
    }
}

const ZMusic m_capricorn = {capricorn_m_init, capricorn_m_strike, capricorn_m_render};
