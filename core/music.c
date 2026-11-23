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

/* What the hands add to every piece, on top of what the piece itself does with them. Nothing at all where the pieces
 * were written and heard (Echo 0, Mist 0.15, Light 0.7): there every number below is exactly 0 or 1.
 * Echo: more of it and more times round. Mist: below 0.15 the room empties, above it the room fills, the piece itself
 * steps back and dulls. Light: below 0.7 the top goes, down to a murmur; above it the top is lifted. */
#define MIST_HOME 0.15f
#define LIGHT_HOME 0.7f
static struct {
    f32 echo, under, over, dark, bright;         /* each 0 .. 1, followed smoothly */
    f32 lp[2][4];                                /* a side: Light's two poles, Mist's one, the top's one */
    int set;
} H;
static void hands_run(const ZParams *p)
{
    f32 e = sat(p->echo), m = sat(p->mist), l = sat(p->light);
    f32 aim[5] = {e, sat((MIST_HOME - m) / MIST_HOME), sat((m - MIST_HOME) / (1.0f - MIST_HOME)),
                  sat((LIGHT_HOME - l) / LIGHT_HOME), sat((l - LIGHT_HOME) / (1.0f - LIGHT_HOME))};
    f32 *v = &H.echo;
    for (int k = 0; k < 5; k++) v[k] = H.set ? v[k] + (aim[k] - v[k]) * 0.12f : aim[k];      /* a turn does not click */
    H.set = 1;
}
static void hands_post(f32 *lr, int n)
{
    f32 d = H.dark, o = H.over, b = H.bright;
    f32 kd = 1.0f / (1.0f + 17.0f * d * d), ko = 1.0f / (1.0f + 4.0f * o * o);
    f32 trim = 1.0f / ((1.0f + 0.9f * o) * (1.0f + 0.45f * H.echo * H.echo));        /* a fuller room is not a louder piece */
    for (int i = 0; i < 2 * n; i++) {
        f32 *z = H.lp[i & 1], x = lr[i];
        if (d > 0.0f) { z[0] += (x - z[0]) * kd; z[1] += (z[0] - z[1]) * kd; x = z[1] * (1.0f + 0.8f * d); } else { z[0] = z[1] = x; }
        if (o > 0.0f) { z[2] += (x - z[2]) * ko; x = z[2]; } else { z[2] = x; }
        z[3] += (x - z[3]) * 0.35f;
        if (b > 0.0f) x += (x - z[3]) * 2.2f * b;
        x *= trim;
        f32 ax = x < 0.0f ? -x : x;                                  /* past 0.8 it bends, and never reaches 1 */
        if (ax > 0.8f) { ax = 0.8f + 0.2f * (ax - 0.8f) / (ax - 0.6f); x = x < 0.0f ? -ax : ax; }
        lr[i] = x;
    }
}

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
    f32 efb = m_fx.echo_fb + 0.15f * H.echo; if (efb > 0.85f) efb = 0.85f;
    F.echo[F.ei] = echo_in(to_echo * (1.0f + 0.6f * H.echo) + F.echo_lp * efb);
    if (++F.ei >= ECHOMAX) F.ei = 0;
    f32 ew = e * (m_fx.echo_wet + 0.35f * H.echo);
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
    f32 wet = m_fx.wet * (1.0f - H.under) * (1.0f + 2.2f * H.over), dry = 1.0f - 0.4f * H.over;
    *l = *l * dry + (ew + out[0] * wet); *r = *r * dry + (ew + out[1] * wet);
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
    for (unsigned i = 0; i < sizeof H; i++) ((uint8_t *)&H)[i] = 0;
    m->init(sr); own_dt = 1.0f / sr; own_in = 18.0f; own_rng = m_seeded(0x0DDBA11u);
}
void z_music_render(const ZMusic *m, f32 *lr, int n, const ZParams *p)
{
    slow_run((f32)n * own_dt);
    own_in -= (f32)n * own_dt;
    if (own_in < 0.0f) { own_in = 25.0f + 45.0f * m_rnd(&own_rng); m->strike(); }
    hands_run(p);
    m->render(lr, n, p);
    hands_post(lr, n);
}
