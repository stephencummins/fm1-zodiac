/* SAGITTARIUS, the piece. Heroic, the kind of music a film gives its hero: horns leaping up the chord and holding,
 * three quick notes leading into each long one, strings driving underneath in threes, kettledrums. The chords are
 * the bright ones with the borrowed flat sixth and seventh. A key looses the arrow: the brass rips upward.
 * Flight: trumpets join an octave up, harder strings. Echo, Mist: as everywhere. Light: opens the brass. */
#include "music.h"

#define STEPS 6                                   /* steps a beat, so the beat divides in three */
#define BPM 108.0f
typedef struct { int8_t root, minor; } Chord;
static const Chord progs[4][4] = {{{0, 0}, {8, 0}, {10, 0}, {0, 0}}, {{0, 0}, {5, 0}, {10, 0}, {7, 0}},
                                  {{0, 0}, {0, 0}, {5, 0}, {7, 0}}, {{9, 1}, {5, 0}, {0, 0}, {7, 0}}};

static struct {
    f32 secs; int beats;
    f32 dt, clock; uint32_t rng;
    int step, bar, pr, pt, mi, off, rip, key, reg;
    Chord c; int8_t tones[9];
    MHorn hn[2], tp; MBow st[2]; MDrum timp;
    f32 whoosh, w_low, w_band, thump, flight;
} M;

static f32 rnd(void) { return m_rnd(&M.rng); }

static void chord(Chord c)                                  /* the chord's notes upward from the A below middle C */
{
    int n = 0; M.c = c;
    for (int m = 57; m < 90 && n < 9; m++) {
        int pc = ((m - 62 - M.key - c.root) % 12 + 24) % 12;
        if (pc == 0 || pc == 7 || pc == (c.minor ? 3 : 4)) M.tones[n++] = (int8_t)m;
    }
}
static void sing(int mi, f32 scoop)
{
    if (mi < 0) mi = 0; if (mi > 7) mi = 7;
    m_horn_on(&M.hn[0], (f32)M.tones[mi], scoop); m_horn_on(&M.hn[1], (f32)M.tones[mi] - 12.0f, scoop);
    if (M.flight + 0.3f * m_slow.fill > 0.5f) m_horn_on(&M.tp, (f32)M.tones[mi] + 12.0f, scoop); else M.tp.gate = 0.0f;
}

static void step(void)
{
    /* each tune rhythm: where the long notes start, then how long each lasts */
    static const uint8_t at[4][3] = {{0, 99, 99}, {0, 99, 99}, {0, 6, 12}, {0, 9, 12}}, len[4][3] = {{12, 0, 0}, {17, 0, 0}, {5, 5, 11}, {8, 3, 11}};
    int s = M.step, end = M.beats * STEPS;
    if (s == 0 && (M.bar & 7) == 0 && M.bar && rnd() < (M.beats == 4 ? 0.2f : 0.7f)) M.beats = 7 - M.beats;      /* four, or a broad three */
    if (s == 0 && (M.bar & 7) == 0) { M.key = m_slow.key; M.reg = m_slow.reg; }
    if (s == 0) {
        int b = M.bar & 3;
        if (b == 0) M.pr = (int)(rnd() * 4.0f);
        chord(progs[M.pr][b]); M.pt = (int)(rnd() * 4.0f); if (m_slow.fill < -0.4f) M.pt &= 1;                 /* thin: one long note a bar */
        m_drum_on(&M.timp, m_hz((f32)(38 + M.key + (M.c.root > 6 ? M.c.root - 12 : M.c.root))), 1.0f, 1.12f); M.thump = 1.0f;
    }
    if (s == 12) { m_drum_on(&M.timp, m_hz((f32)(38 + M.key + (M.c.root > 6 ? M.c.root - 12 : M.c.root)) - 5.0f), 0.6f, 1.12f); M.thump = 0.5f; }
    if (!(s & 1)) {                                          /* the strings: low, low, high */
        int k = (s >> 1) % 3;
        m_bow_on(&M.st[0], (f32)(M.tones[k == 2 ? 2 : 0] - 12), 0); m_bow_on(&M.st[1], (f32)(M.tones[k == 2 ? 3 : 1] - 12), 0);
        M.st[0].env *= 0.3f; M.st[1].env *= 0.3f;
    } else M.st[0].gate = M.st[1].gate = 0.0f;
    if (M.off > 0 && --M.off == 0) M.hn[0].gate = M.hn[1].gate = M.tp.gate = 0.0f;
    if (M.rip > 0) M.rip--;
    else {
        for (int k = 0; k < 3; k++) if (at[M.pt][k] == s) {
            static const int8_t mv[6] = {1, 2, -1, 1, -2, 2};
            if (s == 0 || k) M.mi += mv[(int)(rnd() * 6.0f)];
            if (M.mi < 2 + M.reg) M.mi += 2; if (M.mi > 7 + (M.reg < 0 ? M.reg : 0)) M.mi -= 3;
            sing(M.mi, 0.97f); M.off = len[M.pt][k];
        }
        if (s >= end - 6 && !(s & 1) && M.pt < 2) { sing(M.mi - 3 + ((s - end + 6) >> 1), 0.985f); M.off = 2; }      /* three quick notes in */
    }
    if (++M.step >= end) { M.step = 0; M.bar++; }
}

static void sagittarius_m_init(f32 sr)
{
    for (unsigned i = 0; i < sizeof M; i++) ((uint8_t *)&M)[i] = 0;
    M.dt = 1.0f / sr; M.rng = m_seeded(0x5A6177u); M.clock = 1.0f; M.beats = 4; M.mi = 3;
    M.st[0].pan = 0.3f; M.st[1].pan = 0.7f; chord(progs[0][0]);
    m_fx_init(sr);
}

static void sagittarius_m_strike(void)
{
    if (M.rip > 0) return;
    M.rip = 14; M.whoosh = 1.0f; M.off = 13;
    sing(7, 0.5f);                                           /* up from an octave below */
    m_drum_on(&M.timp, m_hz((f32)(38 + M.key + (M.c.root > 6 ? M.c.root - 12 : M.c.root))), 1.0f, 1.12f); M.thump = 1.0f;
    m_event();
}

static void sagittarius_m_render(f32 *lr, int n, const ZParams *p)
{
    f32 dt = M.dt, light = sat(p->light), mist = sat(p->mist), echo = sat(p->echo);
    M.flight = sat(p->heat);
    m_fx.room = 0.7f + 0.28f * mist; m_fx.damp = 0.35f + 0.35f * mist - 0.15f * light; m_fx.wet = 0.24f + 0.5f * mist;
    m_fx.echo_secs = 60.0f / BPM * 0.6667f; m_fx.echo_fb = 0.3f + 0.4f * echo; m_fx.echo_wet = echo;
    f32 up = m_k(0.03f, dt), dn = m_k(0.12f, dt), bend = m_k(M.rip > 0 ? 0.10f : 0.025f, dt);
    f32 sup = m_k(0.008f, dt), sdn = m_k(0.05f, dt), scut = sat((900.0f + 1600.0f * light + 900.0f * M.flight) * 6.2832f * dt);
    f32 tdec = m_k(0.45f, dt), tbend = m_k(0.04f, dt), thdec = m_k(0.02f, dt), wdec = m_k(0.25f, dt);
    f32 base = 0.8f + 0.6f * light, blown = 1.8f + 1.0f * light;
    f32 bpm = BPM * m_drift(M.secs += (f32)n * dt, 18u, 0.10f);
    f32 v0 = m_slow.v[0], v1 = m_slow.v[1];
    for (int i = 0; i < n; i++) {
        M.clock += (f32)STEPS * bpm / 60.0f * dt;
        if (M.clock >= 1.0f) { M.clock -= 1.0f; step(); }
        f32 h = m_horn(&M.hn[0], dt, up, dn, bend, base, blown, 0.004f), h2 = m_horn(&M.hn[1], dt, up, dn, bend, base * 0.7f, blown * 0.7f, 0.003f); h *= v1; h2 *= v1;
        f32 t = m_horn(&M.tp, dt, up, dn, bend, 1.5f, 2.8f, 0.004f) * v1;
        f32 a = m_bow(&M.st[0], dt, sup, sdn, 1.0f, 0.0f, scut), b = m_bow(&M.st[1], dt, sup, sdn, 1.0f, 0.0f, scut);
        f32 nz = m_noise(&M.rng);
        M.thump -= M.thump * thdec; M.whoosh -= M.whoosh * wdec;
        f32 timp = m_drum(&M.timp, dt, tdec, tbend) + nz * M.thump * 0.12f;
        f32 wf = sat((600.0f + 4000.0f * (1.0f - M.whoosh)) * 6.2832f * dt);
        M.w_low += wf * M.w_band; f32 hp = nz - M.w_low - 0.7f * M.w_band; M.w_band += wf * hp;
        f32 wh = M.w_band * M.whoosh * (1.0f - M.whoosh) * 0.9f;
        f32 str = (0.30f + 0.2f * M.flight) * v0;
        f32 l = h * 0.26f + h2 * 0.22f + t * 0.12f + a * str * 1.3f + b * str * 0.7f + timp * 0.45f + wh * 0.6f;
        f32 r = h * 0.30f + h2 * 0.16f + t * 0.18f + a * str * 0.7f + b * str * 1.3f + timp * 0.45f + wh * 1.2f;
        m_fx_tick(&l, &r, (h + h2) * 0.4f + t * 0.3f + (a + b) * 0.25f + timp * 0.25f + wh, h * 0.2f + t * 0.2f);
        lr[2 * i] = m_clip(l * 0.394f); lr[2 * i + 1] = m_clip(r * 0.394f);
    }
}

const ZMusic m_sagittarius = {sagittarius_m_init, sagittarius_m_strike, sagittarius_m_render};
