/* the list of pieces, and the room and echo they share */
#include "music.h"

extern const ZMusic m_aries, m_taurus, m_gemini, m_cancer, m_leo, m_virgo, m_libra, m_scorpio, m_sagittarius, m_capricorn, m_aquarius, m_pisces;
static const ZMusic *const pieces[] = {&m_aries, &m_taurus, &m_gemini, &m_cancer, &m_leo, &m_virgo, &m_libra,
                                       &m_scorpio, &m_sagittarius, &m_capricorn, &m_aquarius, &m_pisces};
const ZMusic *z_music(int sign) { return sign >= 0 && sign < (int)(sizeof pieces / sizeof pieces[0]) ? pieces[sign] : 0; }

int z_led;
static int events;
void m_event(void) { events++; }
int z_music_event(void) { int n = events; events = 0; return n; }

f32 m_hz(f32 midi)
{
    static const f32 semi[13] = {1.0f, 1.059463f, 1.122462f, 1.189207f, 1.259921f, 1.334840f, 1.414214f,
                                 1.498307f, 1.587401f, 1.681793f, 1.781797f, 1.887749f, 2.0f};
    f32 m = midi - 9.0f;                                    /* 9 = the A at 13.75 Hz */
    int o = (int)(m / 12.0f); if (m < 0.0f) o--;
    f32 r = m - 12.0f * (f32)o; int i = (int)r; if (i > 11) i = 11;
    f32 hz = 13.75f * (semi[i] + (semi[i + 1] - semi[i]) * (r - (f32)i));
    for (; o > 0; o--) hz *= 2.0f;
    for (; o < 0; o++) hz *= 0.5f;
    return hz;
}

/* a small room: four combs and two all-passes a side. Sized for 48 kHz.
 * Z_ECHO16 (the FM-1, short of memory): the echo's line is kept as 16-bit numbers, half the size, good for -2 .. 2
 * in steps of 1/16384. The room stays as it is. */
#define NCOMB 4
#define COMBMAX 1800
#define APMAX 640
#define ECHOMAX 48000
static const int comb_len[NCOMB] = {1557, 1617, 1491, 1422}, ap_len[2] = {556, 225};
#ifdef Z_ECHO16
typedef int16_t echo_t;
static inline echo_t echo_in(f32 x) { x *= 16384.0f; return (echo_t)(x > 32767.0f ? 32767 : x < -32767.0f ? -32767 : (int)(x + (x < 0.0f ? -0.5f : 0.5f))); }
static inline f32 echo_out(echo_t v) { return (f32)v * (1.0f / 16384.0f); }
#else
typedef f32 echo_t;
static inline echo_t echo_in(f32 x) { return x; }
static inline f32 echo_out(echo_t v) { return v; }
#endif
static Z_BIG struct {
    f32 comb[2][NCOMB][COMBMAX], lp[2][NCOMB], ap[2][2][APMAX], echo_lp, sr;
    echo_t echo[ECHOMAX];
    int clen[2][NCOMB], ci[2][NCOMB], alen[2][2], ai[2][2], ei;
} F;
MFx m_fx;

void m_fx_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof F; i++) ((uint8_t *)&F)[i] = 0;
    F.sr = sr;
    for (int s = 0; s < 2; s++) {
        for (int k = 0; k < NCOMB; k++) { int n = (int)((f32)(comb_len[k] + 23 * s) * sr / 44100.0f); F.clen[s][k] = n > COMBMAX ? COMBMAX : n; }
        for (int k = 0; k < 2; k++) { int n = (int)((f32)(ap_len[k] + 23 * s) * sr / 44100.0f); F.alen[s][k] = n > APMAX ? APMAX : n; }
    }
    m_fx.room = 0.6f; m_fx.damp = 0.5f; m_fx.wet = 0.2f; m_fx.echo_secs = 0.5f; m_fx.echo_fb = 0.4f; m_fx.echo_wet = 0.0f;
}

void m_fx_tick(f32 *l, f32 *r, f32 to_room, f32 to_echo)
{
    /* the echo: one line, darker each time round */
    int len = (int)(m_fx.echo_secs * F.sr); if (len < 1) len = 1; if (len > ECHOMAX - 1) len = ECHOMAX - 1;
    int rd = F.ei - len; if (rd < 0) rd += ECHOMAX;
    f32 e = echo_out(F.echo[rd]);
    F.echo_lp += (e - F.echo_lp) * 0.25f;
    F.echo[F.ei] = echo_in(to_echo + F.echo_lp * m_fx.echo_fb);
    if (++F.ei >= ECHOMAX) F.ei = 0;
    f32 ew = e * m_fx.echo_wet;
    f32 in = (to_room + ew * 0.6f) * 0.25f, fb = 0.72f + 0.25f * m_fx.room, d = 0.15f + 0.7f * m_fx.damp, out[2];
    for (int s = 0; s < 2; s++) {
        f32 acc = 0.0f;
        for (int k = 0; k < NCOMB; k++) {
            f32 *b = F.comb[s][k]; int i = F.ci[s][k];
            f32 y = b[i];
            F.lp[s][k] += (y - F.lp[s][k]) * (1.0f - d);
            b[i] = in + F.lp[s][k] * fb;
            if (++i >= F.clen[s][k]) i = 0;
            F.ci[s][k] = i; acc += y;
        }
        for (int k = 0; k < 2; k++) {
            f32 *b = F.ap[s][k]; int i = F.ai[s][k];
            f32 z = b[i], x = acc + z * 0.5f;
            b[i] = x; acc = z - x * 0.5f;
            if (++i >= F.alen[s][k]) i = 0;
            F.ai[s][k] = i;
        }
        out[s] = acc;
    }
    *l += ew + out[0] * m_fx.wet; *r += ew + out[1] * m_fx.wet;
}

/* left alone, a piece still huffs, clicks or roars by itself from time to time */
static f32 own_dt, own_in; static uint32_t own_rng;

/* the slow layer */
MSlow m_slow; uint32_t m_seed;
static uint32_t seed_next, slow_rng; static f32 key_in;
static uint32_t stir(uint32_t x) { x ^= x >> 16; x *= 0x7FEB352Du; x ^= x >> 15; x *= 0x846CA68Bu; x ^= x >> 16; return x; }
void z_music_seed(uint32_t s) { seed_next = s ? stir(s) | 1u : 0u; }                  /* stirred: near numbers give far playings */
static void slow_start(void)
{
    m_slow.secs = 0.0f; m_slow.fill = 0.0f; m_slow.reg = 0; m_slow.key = 0;
    for (int k = 0; k < 4; k++) m_slow.v[k] = 1.0f;
    slow_rng = m_seeded(0x51A0E5u);
    key_in = 150.0f + 90.0f * m_rnd(&slow_rng);
}
static void slow_run(f32 dt)
{
    MSlow *s = &m_slow;
    f32 t = s->secs += dt, h = sat((t - 60.0f) / 90.0f); h = h * h * (3.0f - 2.0f * h);          /* at rest, then coming in */
    s->fill = h * (2.0f * sat(0.5f + 1.7f * (noise1(t / 97.0f, m_seed + 1u) - 0.5f)) - 1.0f);
    f32 lift = h * (2.0f * sat(0.5f + 1.7f * (noise1(t / 131.0f, m_seed + 2u) - 0.5f)) - 1.0f);
    int reg = lift > 0.6f ? 2 : lift > 0.2f ? 1 : lift < -0.6f ? -2 : lift < -0.2f ? -1 : 0;
    if ((s->key >= 3 && reg > 0) || (s->key <= -3 && reg < 0)) reg = 0;                           /* a higher key and a higher register don't pile up */
    s->reg = reg;
    int on[4];
    for (int k = 0; k < 4; k++) on[k] = h < 0.5f || noise1(t / 53.0f, m_seed + 10u + (uint32_t)k) > (k == 1 ? 0.18f : 0.26f);
    if (!on[0] && !on[1]) on[1] = 1;
    f32 kk = m_k(4.0f, dt);
    for (int k = 0; k < 4; k++) s->v[k] += ((on[k] ? 1.0f : k == 0 ? 0.3f : 0.0f) - s->v[k]) * kk;      /* the ground only recedes: no piece falls silent */
    if ((key_in -= dt) < 0.0f) {                             /* to a key near the one it is in, or home */
        static const int8_t mv[6] = {5, -5, 3, -3, 2, -2};
        int k = s->key;
        if (k && m_rnd(&slow_rng) < 0.4f) k = 0;
        else { k += mv[(int)(m_rnd(&slow_rng) * 6.0f)]; if (k > 6) k -= 12; if (k < -5) k += 12; }
        s->key = k; key_in = 150.0f + 180.0f * m_rnd(&slow_rng);
    }
}

void z_music_start(const ZMusic *m, f32 sr)
{
    m_seed = seed_next;
    if (seed_next) { seed_next = seed_next * 1664525u + 1013904223u; if (!seed_next) seed_next = 1u; }      /* the next start differs again */
    slow_start();
    m->init(sr); own_dt = 1.0f / sr; own_in = 18.0f; own_rng = m_seeded(0x0DDBA11u);
}
void z_music_render(const ZMusic *m, f32 *lr, int n, const ZParams *p)
{
    slow_run((f32)n * own_dt);
    own_in -= (f32)n * own_dt;
    if (own_in < 0.0f) { own_in = 25.0f + 45.0f * m_rnd(&own_rng); m->strike(); }
    m->render(lr, n, p);
}
