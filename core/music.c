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

/* a small room: four combs and two all-passes a side. Sized for 48 kHz. (For the FM-1 these want halving.) */
#define NCOMB 4
#define COMBMAX 1800
#define APMAX 640
#define ECHOMAX 48000
static const int comb_len[NCOMB] = {1557, 1617, 1491, 1422}, ap_len[2] = {556, 225};
static struct {
    f32 comb[2][NCOMB][COMBMAX], lp[2][NCOMB], ap[2][2][APMAX], echo[ECHOMAX], echo_lp, sr;
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
    f32 e = F.echo[rd];
    F.echo_lp += (e - F.echo_lp) * 0.25f;
    F.echo[F.ei] = to_echo + F.echo_lp * m_fx.echo_fb;
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
void z_music_start(const ZMusic *m, f32 sr) { m->init(sr); own_dt = 1.0f / sr; own_in = 18.0f; own_rng = 0x0DDBA11u; }
void z_music_render(const ZMusic *m, f32 *lr, int n, const ZParams *p)
{
    own_in -= (f32)n * own_dt;
    if (own_in < 0.0f) { own_in = 25.0f + 45.0f * m_rnd(&own_rng); m->strike(); }
    m->render(lr, n, p);
}
