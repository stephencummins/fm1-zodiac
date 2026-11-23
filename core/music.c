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


/* The DJ's hands (z_dj): four knobs and twenty-six held effects over whatever piece is playing. Nothing here keeps any
 * sound of its own: Repeat and Roll borrow the echo's line, Half, Tape stop and Wow play the piece itself slower.
 * With every knob at 0 and nothing held, D.any is 0 and not one number of the piece changes. */
const char *const z_fx_name[Z_NFX] = {"Echo", "Dub", "Repeat", "Roll", "Gate", "Pump", "Half time", "Tape stop", "Wow",
    "Drive", "Crush", "Ring", "Phaser", "Wash", "Freeze", "Close", "Rise", "Sweep", "Wah", "Drop", "Radio", "Riser",
    "Siren", "Vinyl", "Spin", "Cut"};
typedef struct { f32 ic1, ic2; } DSvf;
static struct {
    f32 t_lp, t_hp, t_echo, t_verb; uint32_t fx, was;      /* as asked */
    f32 lp, hp, echo, verb, e[Z_NFX];                       /* as heard: followed smoothly */
    int any, post, src;
    /* for m_fx_tick */
    f32 esend, ek, edub, ewet, tail, rsend, rfb, rwet, frz, dry;
    int rmode, rpos, rcap, rloop, rstart;                   /* Repeat: 0 off, 1 catching, 2 round and round */
    f32 siren_ph, siren_lfo, rz_lo, rz_bd; uint32_t rng;
    /* the piece slower */
    uint32_t rate, frac; f32 s0[2], s1[2], tape, wow_ph; int slow;
    /* the last stage */
    DSvf flp[2], fhp[2]; f32 g_lp, g_hp, k_lp, k_hp, m_lp, m_hp, t_lpg, t_hpg;
    f32 sweep_ph, wah_ph, ring_ph, ph_lfo, ph_z[2][4], ph_fb[2], hold[2], gate_ph, gate_g, pump_ph, spin_ph, crk;
    int hold_n;
} D;
static inline f32 ease(f32 v, int on, f32 up, f32 down, f32 dt) { f32 a = on ? 1.0f : 0.0f; v += (a - v) * (dt / ((on ? up : down) + dt)); return v < 1e-4f && !on ? 0.0f : v; }
static inline f32 tan_pi(f32 f) { f32 x = 3.14159265f * f; if (x > 1.1f) x = 1.1f; f32 x2 = x * x; return x * (1.0f + x2 * (0.333333f + x2 * 0.133333f)); }
void z_dj(f32 lp, f32 hp, f32 echo, f32 verb, uint32_t fx) { D.t_lp = sat(lp); D.t_hp = sat(hp); D.t_echo = sat(echo); D.t_verb = sat(verb); D.fx = fx; }
static void dj_start(void)
{
    f32 a = D.t_lp, b = D.t_hp, c = D.t_echo, d = D.t_verb; uint32_t fx = D.fx;
    for (unsigned i = 0; i < sizeof D; i++) ((uint8_t *)&D)[i] = 0;
    D.t_lp = D.lp = a; D.t_hp = D.hp = b; D.t_echo = D.echo = c; D.t_verb = D.verb = d; D.fx = fx;
    D.rate = 65536u; D.tape = 1.0f; D.rng = 0x1234567u; D.dry = 1.0f; D.gate_g = 1.0f;
}
#define ON(k) (int)(D.fx >> (k) & 1u)
static void dj_ctl(int n, f32 dt1)                           /* once a block: the easing, and what the samples need */
{
    static const f32 up[Z_NFX] = {0.01f, 0.05f, 0, 0, 0.005f, 0.01f, 0, 0, 0.1f, 0.25f, 0.05f, 0.02f, 0.05f, 0.25f, 0.03f,
                                  1.4f, 2.0f, 0.05f, 0.03f, 0.03f, 0.03f, 4.0f, 0.02f, 0.05f, 0.05f, 0.02f};
    static const f32 down[Z_NFX] = {0.02f, 0.4f, 0, 0, 0.01f, 0.05f, 0, 0, 0.2f, 0.08f, 0.02f, 0.03f, 0.2f, 0.5f, 0.6f,
                                    0.25f, 0.25f, 0.2f, 0.1f, 0.06f, 0.05f, 0.12f, 0.06f, 0.08f, 0.1f, 0.03f};
    f32 dt = (f32)n * dt1, kk = dt / (0.05f + dt), any = 0.0f;
    uint32_t rise = D.fx & ~D.was; D.was = D.fx;
    D.lp += (D.t_lp - D.lp) * kk; D.hp += (D.t_hp - D.hp) * kk; D.echo += (D.t_echo - D.echo) * kk; D.verb += (D.t_verb - D.verb) * kk;
    if (D.t_lp == 0.0f && D.lp < 1e-4f) D.lp = 0.0f;
    if (D.t_hp == 0.0f && D.hp < 1e-4f) D.hp = 0.0f;
    if (D.t_echo == 0.0f && D.echo < 1e-4f) D.echo = 0.0f;
    if (D.t_verb == 0.0f && D.verb < 1e-4f) D.verb = 0.0f;
    for (int k = 0; k < Z_NFX; k++) { if (up[k] > 0.0f) D.e[k] = ease(D.e[k], ON(k), up[k], down[k], dt); any += D.e[k]; }
    if (rise >> Z_FX_SWEEP & 1u) D.sweep_ph = 0.0f;
    if (rise >> Z_FX_SIREN & 1u) D.siren_lfo = 0.0f;
    const f32 *e = D.e;
    /* the echo: the knob and Echo send the whole piece into it; what was sent goes on round after the key is let go */
    f32 th = maxf(D.echo, e[Z_FX_ECHO]);
    D.tail = th > D.tail ? th : D.tail * (1.0f - dt / (3.0f + dt));
    if (D.tail < 1e-4f) D.tail = 0.0f;
    D.esend = 0.5f * th + 0.25f * e[Z_FX_DUB]; D.ek = maxf(D.echo, 0.9f * D.tail); D.edub = e[Z_FX_DUB];
    D.ewet = 0.6f * maxf(D.echo, D.tail) + 0.3f * e[Z_FX_DUB];
    /* the room: the knob a hall, Wash a flood with the piece far behind it, Freeze what is in the room held */
    f32 v = D.verb, w = e[Z_FX_WASH], z = e[Z_FX_FREEZE], c = e[Z_FX_CUT];
    D.rsend = (0.35f * v + 0.35f * w) * (1.0f - z); D.rfb = maxf(v, w); D.frz = z;
    D.rwet = 0.15f * v + 0.12f * w + 0.5f * z + 0.25f * c;
    D.dry = (1.0f - 0.3f * v) * (1.0f - 0.85f * w) * (1.0f - c) * (1.0f - 0.25f * maxf(th, e[Z_FX_DUB]));
    D.src = e[Z_FX_SIREN] > 0.0f || e[Z_FX_RISER] > 0.0f;
    /* Repeat, Roll: a slice of the piece caught in the echo's line, then round and round */
    int rep = ON(Z_FX_REPEAT), roll = ON(Z_FX_ROLL);
    if (rep || roll) {
        f32 s = m_fx.echo_secs; while (s > 0.42f) s *= 0.5f; while (s < 0.2f) s *= 2.0f;      /* near an eighth of this piece */
        int len = (int)(s * F.sr);
        if (D.rmode == 0) { D.rmode = 1; D.rpos = 0; D.rstart = F.ei; D.rcap = rep ? len : len / 2; }
        D.rloop = roll ? (rep ? D.rcap / 2 : D.rcap) : D.rcap;
        if (D.rloop > D.rcap) D.rloop = D.rcap;
        if (D.rloop < 64) D.rloop = 64;
    } else D.rmode = 0;
    /* the piece slower: Half time at once, Tape stop down to nothing, Wow a worn tape */
    D.tape = ON(Z_FX_TAPE) ? maxf(D.tape - dt / 0.8f, 0.0f) : minf(D.tape + dt / 0.2f, 1.0f);
    D.wow_ph += 4.3f * dt; if (D.wow_ph >= 1.0f) D.wow_ph -= 1.0f;
    f32 r = D.tape * (ON(Z_FX_HALF) ? 0.5f : 1.0f) * (1.0f - 0.03f * e[Z_FX_WOW] * (1.0f + sin_turns(D.wow_ph)));
    D.rate = (uint32_t)(r * 65536.0f); if (D.rate > 65536u) D.rate = 65536u;
    if (D.rate == 65536u && D.slow) { D.slow = 0; D.frac = 0; } else if (D.rate != 65536u) D.slow = 1;
    /* the filters */
    D.sweep_ph += 0.16f * dt; if (D.sweep_ph >= 1.0f) D.sweep_ph -= 1.0f;
    D.wah_ph += 1.7f * dt; if (D.wah_ph >= 1.0f) D.wah_ph -= 1.0f;
    f32 a = D.lp;
    a = maxf(a, 0.9f * e[Z_FX_CLOSE]);
    a = maxf(a, e[Z_FX_SWEEP] * (0.12f + 0.6f * (0.5f - 0.5f * cos_turns(D.sweep_ph))));
    a = maxf(a, e[Z_FX_WAH] * (0.42f + 0.2f * sin_turns(D.wah_ph)));
    a = maxf(a, 0.3f * e[Z_FX_RADIO]); a = maxf(a, 0.26f * e[Z_FX_VINYL]);
    f32 q = 0.7f + 2.0f * sat(a * 5.0f) + 3.0f * maxf(e[Z_FX_SWEEP], 0.5f * e[Z_FX_WAH]);
    D.g_lp = tan_pi(m_hz(126.0f - 79.0f * a) / F.sr); D.k_lp = 1.0f / q; D.m_lp = sat(a * 10.0f); D.t_lpg = 0.45f + 0.55f * 0.7f / q;
    f32 b = D.hp;
    b = maxf(b, 0.82f * e[Z_FX_RISE]); b = maxf(b, 0.45f * e[Z_FX_DROP]); b = maxf(b, 0.56f * e[Z_FX_RADIO]); b = maxf(b, 0.42f * e[Z_FX_VINYL]);
    q = 0.7f + 1.8f * sat(b * 5.0f) + 1.5f * e[Z_FX_RISE];
    D.g_hp = tan_pi(m_hz(15.5f + 96.0f * b) / F.sr); D.k_hp = 1.0f / q; D.m_hp = sat(b * 10.0f); D.t_hpg = 0.45f + 0.55f * 0.7f / q;
    D.post = D.m_lp > 0.0f || D.m_hp > 0.0f || e[Z_FX_DRIVE] > 0.0f || e[Z_FX_CRUSH] > 0.0f || e[Z_FX_RING] > 0.0f || e[Z_FX_PHASER] > 0.0f
             || e[Z_FX_GATE] > 0.0f || e[Z_FX_PUMP] > 0.0f || e[Z_FX_SPIN] > 0.0f || e[Z_FX_VINYL] > 0.0f;
    D.any = any > 0.0f || D.lp > 0.0f || D.hp > 0.0f || D.echo > 0.0f || D.verb > 0.0f || D.tail > 0.0f || D.rmode;
}
static inline f32 dj_source(void)                            /* the Siren and the Riser: sounds of the hands' own */
{
    f32 dt = 1.0f / F.sr, x = 0.0f, es = D.e[Z_FX_SIREN], er = D.e[Z_FX_RISER];
    if (es > 0.0f) {
        D.siren_lfo += 2.6f * dt; if (D.siren_lfo >= 1.0f) D.siren_lfo -= 1.0f;
        D.siren_ph += (1250.0f - 700.0f * D.siren_lfo) * dt; if (D.siren_ph >= 1.0f) D.siren_ph -= 1.0f;
        f32 s = sin_turns(D.siren_ph); x += (s + 0.3f * s * s * s) * 0.16f * es;
    }
    if (er > 0.0f) {
        f32 f = 6.2832f * (250.0f + 6500.0f * er * er) * dt;
        D.rz_lo += f * D.rz_bd; D.rz_bd += f * (m_noise(&D.rng) - D.rz_lo - 0.35f * D.rz_bd);
        x += D.rz_bd * 0.22f * er * (0.25f + 0.75f * er);
    }
    return x;
}
static inline f32 svf(DSvf *s, f32 x, f32 g, f32 k, int high)
{
    f32 a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, v3 = x - s->ic2, v1 = a1 * s->ic1 + a2 * v3, v2 = s->ic2 + a2 * s->ic1 + g * a2 * v3;
    s->ic1 = 2.0f * v1 - s->ic1; s->ic2 = 2.0f * v2 - s->ic2;
    return high ? x - k * v1 - v2 : v2;
}
static void dj_post(f32 *lr, int n)                          /* the last stage, on the whole piece */
{
    if (!D.post) { for (int s = 0; s < 2; s++) { D.flp[s].ic1 = D.flp[s].ic2 = D.fhp[s].ic1 = D.fhp[s].ic2 = 0.0f; } return; }
    const f32 *e = D.e; f32 dt = 1.0f / F.sr;
    f32 dr = e[Z_FX_DRIVE], cr = e[Z_FX_CRUSH], rg = e[Z_FX_RING], ph = e[Z_FX_PHASER], gt = e[Z_FX_GATE], pu = e[Z_FX_PUMP], sp = e[Z_FX_SPIN], vy = e[Z_FX_VINYL];
    f32 drg = 1.0f + 9.0f * dr, drn = 1.0f / (1.0f + 3.2f * dr);
    int cn = 1 + (int)(11.0f * cr); f32 cl = 40.0f - 34.0f * cr;
    f32 gper = m_fx.echo_secs; while (gper > 0.17f) gper *= 0.5f; while (gper < 0.07f) gper *= 2.0f;
    f32 pper = m_fx.echo_secs; while (pper > 0.75f) pper *= 0.5f; while (pper < 0.36f) pper *= 2.0f;
    for (int i = 0; i < n; i++) {
        f32 x[2] = {lr[2 * i], lr[2 * i + 1]};
        if (dr > 0.0f) for (int s = 0; s < 2; s++) x[s] = m_clip(x[s] * drg) * drn;
        if (cr > 0.0f) {
            if (++D.hold_n >= cn) { D.hold_n = 0; for (int s = 0; s < 2; s++) { f32 v = x[s] * cl; D.hold[s] = (f32)(int)(v + (v < 0.0f ? -0.5f : 0.5f)) / cl; } }
            for (int s = 0; s < 2; s++) x[s] += (D.hold[s] - x[s]) * sat(cr * 4.0f);
        }
        if (rg > 0.0f) { D.ring_ph += 233.0f * dt; if (D.ring_ph >= 1.0f) D.ring_ph -= 1.0f; f32 m = sin_turns(D.ring_ph) * 1.5f; for (int s = 0; s < 2; s++) x[s] += (x[s] * m - x[s]) * rg; }
        if (ph > 0.0f) {
            D.ph_lfo += 0.35f * dt; if (D.ph_lfo >= 1.0f) D.ph_lfo -= 1.0f;
            for (int s = 0; s < 2; s++) {
                f32 a = 0.2f + 0.65f * (0.5f + 0.5f * sin_turns(D.ph_lfo + 0.25f * (f32)s)), y = x[s] + 0.6f * D.ph_fb[s];
                for (int k = 0; k < 4; k++) { f32 o = -a * y + D.ph_z[s][k]; D.ph_z[s][k] = y + a * o; y = o; }
                D.ph_fb[s] = y; x[s] += (0.45f * (x[s] + y) - x[s]) * ph;
            }
        }
        if (D.m_lp > 0.0f) for (int s = 0; s < 2; s++) x[s] += (svf(&D.flp[s], x[s], D.g_lp, D.k_lp, 0) * D.t_lpg - x[s]) * D.m_lp;
        if (D.m_hp > 0.0f) for (int s = 0; s < 2; s++) x[s] += (svf(&D.fhp[s], x[s], D.g_hp, D.k_hp, 1) * D.t_hpg - x[s]) * D.m_hp;
        if (vy > 0.0f) {
            if (m_rnd(&D.rng) < 0.0009f) D.crk = (m_rnd(&D.rng) < 0.5f ? -0.5f : 0.5f) * (0.3f + 0.7f * m_rnd(&D.rng));
            f32 c = D.crk + 0.012f * m_noise(&D.rng); D.crk *= 0.55f;
            x[0] += c * vy; x[1] += c * vy;
        }
        f32 g = 1.0f;
        if (gt > 0.0f) { D.gate_ph += dt / gper; if (D.gate_ph >= 1.0f) D.gate_ph -= 1.0f; D.gate_g += ((D.gate_ph < 0.5f ? 1.0f : 0.0f) - D.gate_g) * 0.02f; g *= 1.0f + (D.gate_g - 1.0f) * gt; }
        if (pu > 0.0f) { D.pump_ph += dt / pper; if (D.pump_ph >= 1.0f) D.pump_ph -= 1.0f; f32 r = D.pump_ph * (2.0f - D.pump_ph); g *= 1.0f + (0.08f + 0.92f * r - 1.0f) * pu; }
        x[0] *= g; x[1] *= g;
        if (sp > 0.0f) { D.spin_ph += 2.2f * dt; if (D.spin_ph >= 1.0f) D.spin_ph -= 1.0f; f32 p = 0.5f + 0.5f * sin_turns(D.spin_ph); x[0] *= 1.0f + (1.4f * p - 1.0f) * sp; x[1] *= 1.0f + (1.4f * (1.0f - p) - 1.0f) * sp; }
        lr[2 * i] = x[0]; lr[2 * i + 1] = x[1];
    }
}
/* the piece slower than it was written (rate below 65536): fewer of its samples made, drawn out over the block */
static int dj_need(int n) { return D.slow ? (int)((D.frac + (uint32_t)n * D.rate) >> 16) : n; }
static void dj_slow(f32 *lr, int n, int made)
{
    if (!D.slow) { if (n >= 2) for (int s = 0; s < 2; s++) { D.s0[s] = lr[2 * (n - 2) + s]; D.s1[s] = lr[2 * (n - 1) + s]; } return; }
    f32 o0[2] = {D.s0[0], D.s0[1]}, o1[2] = {D.s1[0], D.s1[1]}, g = sat(D.tape * 5.0f);
    for (int s = 0; s < 2; s++) {
        if (made >= 2) { D.s0[s] = lr[2 * (made - 2) + s]; D.s1[s] = lr[2 * (made - 1) + s]; }
        else if (made == 1) { D.s0[s] = o1[s]; D.s1[s] = lr[s]; }
    }
    for (int i = n - 1; i >= 0; i--) {
        uint32_t a = D.frac + (uint32_t)(i + 1) * D.rate; int adv = (int)(a >> 16); f32 fr = (f32)(a & 0xFFFFu) * (1.0f / 65536.0f);
        for (int s = 0; s < 2; s++) {
            f32 c = adv >= 1 ? lr[2 * (adv - 1) + s] : o1[s], p = adv >= 2 ? lr[2 * (adv - 2) + s] : adv == 1 ? o1[s] : o0[s];
            lr[2 * i + s] = (p + (c - p) * fr) * g;
        }
    }
    D.frac = (D.frac + (uint32_t)n * D.rate) & 0xFFFFu;
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
    const int dj = D.any;
    if (D.rmode == 2) {                                      /* Repeat: the slice round and round, everything else waiting */
        int k = D.rstart + D.rpos; if (k >= ECHOMAX) k -= ECHOMAX;
        int edge = D.rpos < D.rloop - D.rpos ? D.rpos : D.rloop - D.rpos; if (edge < 0) edge = 0;
        *l = *r = echo_out(F.echo[k]) * (edge < 48 ? (f32)edge * (1.0f / 48.0f) : 1.0f);
        if (++D.rpos >= D.rloop) D.rpos = 0;
        return;
    }
    if (dj) {
        f32 mono = (*l + *r) * 0.5f;
        if (D.src) { f32 x = dj_source(); *l += x; *r += x; mono += x; }
        to_echo += mono * D.esend; to_room += mono * D.rsend;
    }
    /* the echo: one line, darker each time round */
    int len = (int)(m_fx.echo_secs * F.sr); if (len < 1) len = 1; if (len > ECHOMAX - 1) len = ECHOMAX - 1;
    int rd = F.ei - len; if (rd < 0) rd += ECHOMAX;
    f32 e = echo_out(F.echo[rd]);
    F.echo_lp += (e - F.echo_lp) * 0.25f;
    f32 efb = m_fx.echo_fb + 0.15f * H.echo; if (efb > 0.85f) efb = 0.85f;
    f32 ein = to_echo * (1.0f + 0.6f * H.echo);
    if (dj) { efb += (0.82f - efb) * D.ek; efb += (0.97f - efb) * D.edub; ein = m_clip(ein + F.echo_lp * efb); } else ein += F.echo_lp * efb;
    const int wr = F.ei;
    F.echo[wr] = echo_in(ein);
    if (++F.ei >= ECHOMAX) F.ei = 0;
    f32 ew = e * (m_fx.echo_wet + 0.35f * H.echo + (dj ? D.ewet : 0.0f));
    f32 in = (to_room + ew * 0.6f) * 0.25f, fb = 0.72f + 0.25f * m_fx.room, d = 0.15f + 0.7f * m_fx.damp, out[2];
    if (dj) {                                                /* a longer tail holds more: less goes in, so it is no louder */
        f32 fb0 = fb; fb += (0.975f - fb) * D.rfb;
        f32 c = (1.0f - fb) / (1.0f - fb0); in *= c + (1.0f - c) * 0.3f;
        fb += (1.0f - fb) * D.frz; in *= 1.0f - D.frz; d *= 1.0f - D.frz;
    }
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
    if (dj) { wet += D.rwet; dry *= D.dry; }
    *l = *l * dry + (ew + out[0] * wet); *r = *r * dry + (ew + out[1] * wet);
    if (D.rmode == 1) {                                      /* Repeat catching: the piece as heard into the echo's line */
        F.echo[wr] = echo_in((*l + *r) * 0.5f);
        if (++D.rpos >= D.rcap) { D.rmode = 2; D.rpos = 0; }
    }
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
    m->init(sr); dj_start(); own_dt = 1.0f / sr; own_in = 18.0f; own_rng = m_seeded(0x0DDBA11u);
}
void z_music_render(const ZMusic *m, f32 *lr, int n, const ZParams *p)
{
    dj_ctl(n, own_dt);
    int made = dj_need(n);
    slow_run((f32)made * own_dt);
    own_in -= (f32)made * own_dt;
    if (own_in < 0.0f) { own_in = 25.0f + 45.0f * m_rnd(&own_rng); m->strike(); }
    hands_run(p);
    if (made > 0) m->render(lr, made, p);
    dj_slow(lr, n, made);
    dj_post(lr, n);
    hands_post(lr, n);
}
