/* Zodiac's music: each sign has a piece that plays itself. No hardware here; a port asks for blocks of sound. */
#pragma once
#include "zodiac.h"
#include "mathx.h"

typedef struct {
    void (*init)(f32 sr);                                   /* sr: samples a second */
    void (*strike)(void);                                   /* a note, a key */
    void (*render)(f32 *lr, int n, const ZParams *p);       /* n frames, left and right in turn, about -1 .. 1 */
} ZMusic;
const ZMusic *z_music(int sign);
void z_music_start(const ZMusic *m, f32 sr);                 /* then z_music_render: the piece, making its own gesture now and then */
void z_music_render(const ZMusic *m, f32 *lr, int n, const ZParams *p);                            /* 0: this sign has no piece yet */

/* so that no two starts are alike, a port gives a number that differs each time (0, the default: always the same, for measuring) */
void z_music_seed(uint32_t s);
extern uint32_t m_seed;                                     /* this start's number */
static inline uint32_t m_seeded(uint32_t c) { uint32_t s = c ^ m_seed; return s ? s : c; }

/* the slow layer: what changes over minutes. Every piece reads it, none writes it. For the first minute it is at rest. */
typedef struct {
    f32 secs;
    f32 fill;                                               /* -1 thin .. 1 full; 0 is the piece as written */
    f32 v[4];                                               /* each voice: 1 present, 0 fallen silent. 0 only recedes, and 0 and 1 are never both out */
    int reg;                                                /* the tune's register: -2 .. 2 steps of the scale */
    int key;                                                /* semitones from home: a piece moves there at its next phrase */
} MSlow;
extern MSlow m_slow;

/* the music leads the picture: when a piece wants the picture to strike it says so here */
extern int z_led;                                           /* 1 while a piece is playing: pictures don't strike by themselves */
void m_event(void);
int z_music_event(void);                                    /* how many strikes the music asked for since last asked */

/* shared parts */
f32 m_hz(f32 midi);                                         /* a note number (fractions allowed) as a frequency */
static inline f32 m_k(f32 secs, f32 dt) { return dt / (secs + dt); }   /* one-pole step for a time constant */
static inline uint32_t m_rng(uint32_t *s) { uint32_t x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return *s = x; }
static inline f32 m_rnd(uint32_t *s) { return (f32)(m_rng(s) >> 8) * (1.0f / 16777216.0f); }
static inline f32 m_noise(uint32_t *s) { return m_rnd(s) * 2.0f - 1.0f; }
static inline f32 m_clip(f32 x) { x = x < -1.5f ? -1.5f : x > 1.5f ? 1.5f : x; return x - x * x * x * (4.0f / 27.0f); }

static inline int m_scale(const int8_t *sc, int idx) { int o = idx >= 0 ? idx / 7 : -((6 - idx) / 7); return 12 * o + sc[idx - 7 * o]; }
static inline f32 m_saw(f32 ph, f32 inc)                    /* a saw without the worst of its roughness */
{
    f32 v = 2.0f * ph - 1.0f;
    if (ph < inc) { f32 t = ph / inc; v -= t + t - t * t - 1.0f; }
    else if (ph > 1.0f - inc) { f32 t = (ph - 1.0f) / inc; v -= t * t + t + t + 1.0f; }
    return v;
}

/* a horn: a sine bent by another at the same pitch, brighter the harder it is blown */
typedef struct { f32 ph, mph, hz, aim, env, gate, vib, age, pan; } MHorn;
static inline void m_horn_on(MHorn *h, f32 midi, f32 scoop) { h->aim = m_hz(midi); h->hz = h->aim * scoop; h->gate = 1.0f; h->age = 0.0f; }
static inline f32 m_horn(MHorn *h, f32 dt, f32 up, f32 down, f32 bend, f32 base, f32 blown, f32 vibrato)
{
    h->env += (h->gate - h->env) * (h->gate > h->env ? up : down);
    h->hz += (h->aim - h->hz) * bend;
    h->age += dt;
    h->vib += 5.1f * dt; if (h->vib >= 1.0f) h->vib -= 1.0f;
    f32 hz = h->hz * (1.0f + vibrato * sat(h->age * 1.4f - 0.5f) * sin_turns(h->vib));
    h->ph += hz * dt; if (h->ph >= 1.0f) h->ph -= 1.0f;
    h->mph += hz * dt; if (h->mph >= 1.0f) h->mph -= 1.0f;
    f32 e = h->env;
    return sin_turns(h->ph + (base + blown * e * e) * 0.159155f * sin_turns(h->mph)) * e;
}

/* a bowed string: a saw, softened, with a vibrato that comes in after the note starts */
typedef struct { f32 ph, hz, aim, env, gate, vib, age, pan, lp1, lp2; } MBow;
static inline void m_bow_on(MBow *b, f32 midi, int slide)
{
    b->aim = m_hz(midi);
    if (!slide || b->env < 0.05f) { b->hz = b->aim; b->age = 0.0f; }
    b->gate = 1.0f;
}
static inline f32 m_bow(MBow *b, f32 dt, f32 up, f32 down, f32 glide, f32 vibrato, f32 cut)
{
    b->env += (b->gate - b->env) * (b->gate > b->env ? up : down);
    b->hz += (b->aim - b->hz) * glide;
    b->age += dt;
    b->vib += 5.6f * dt; if (b->vib >= 1.0f) b->vib -= 1.0f;
    f32 inc = b->hz * (1.0f + vibrato * sat(b->age * 1.6f - 0.4f) * sin_turns(b->vib)) * dt;
    b->ph += inc; if (b->ph >= 1.0f) b->ph -= 1.0f;
    f32 x = m_saw(b->ph, inc) * b->env;
    b->lp1 += (x - b->lp1) * cut; b->lp2 += (b->lp1 - b->lp2) * cut;
    return b->lp2;
}

/* a plucked string of the harpsichord kind: bright, at once, then dying away */
typedef struct { f32 ph, ph2, hz, env, gate, dec, lp, pan; } MPluck;
static inline void m_pluck_on(MPluck *p, f32 midi, f32 vel, f32 dec) { p->hz = m_hz(midi); p->env = vel; p->gate = 1.0f; p->dec = dec; }
static inline f32 m_pluck(MPluck *p, f32 dt, f32 rel, f32 cut)
{
    if (p->env < 0.0004f) { p->lp *= 0.99f; return p->lp; }
    p->env -= p->env * (p->gate > 0.0f ? p->dec : rel);
    f32 inc = p->hz * dt;
    p->ph += inc; if (p->ph >= 1.0f) p->ph -= 1.0f;
    p->ph2 += 2.0f * inc; if (p->ph2 >= 1.0f) p->ph2 -= 1.0f;
    f32 w = p->ph + 0.14f; if (w >= 1.0f) w -= 1.0f;
    f32 x = (m_saw(p->ph, inc) - m_saw(w, inc)) * 0.7f + m_saw(p->ph2, 2.0f * inc) * 0.25f;
    p->lp += (x * p->env - p->lp) * cut;
    return p->lp;
}

/* a drum, or a low plucked note: a sine that falls to its pitch */
typedef struct { f32 ph, hz, base, env, peak; int rise; } MDrum;
static inline void m_drum_on(MDrum *d, f32 hz, f32 vel, f32 from) { d->base = hz; d->hz = hz * from; d->peak = vel; d->rise = d->env < vel; }
static inline f32 m_drum(MDrum *d, f32 dt, f32 dec, f32 bend)
{
    d->hz += (d->base - d->hz) * bend;
    if (d->rise) { d->env += (d->peak - d->env) * sat(900.0f * dt); if (d->env > 0.97f * d->peak) d->rise = 0; }    /* a few thousandths of a second to come up: no click */
    else d->env -= d->env * dec;
    d->ph += d->hz * dt; if (d->ph >= 1.0f) d->ph -= 1.0f;
    return sin_turns(d->ph) * d->env;
}

/* every piece wanders: its speed drifts a little either way over a minute or so */
static inline f32 m_drift(f32 secs, uint32_t seed, f32 depth) { return 1.0f + depth * (2.0f * noise1(secs / 41.0f, seed + m_seed) - 1.0f); }

/* a small bell, celesta kind. High up, its brightness is held back: the bright part of a bell is four times the
 * note, and above about 2 kHz that part and its partners pass the top of what 44.1 kHz can carry and come back down
 * as false tones (heard as distortion). `fit` is how many of those partners still have room; under two and a half
 * of them the brightness thins, and with room for only one the bell is nearly a plain tone. */
typedef struct { f32 ph, mph, hz, env, dec, pan, fit; } MBell;
static inline void m_bell_on(MBell *b, f32 midi, f32 vel, f32 dec) { b->hz = m_hz(midi); b->env = vel; b->dec = dec; b->fit = -1.0f; }
static inline f32 m_bell(MBell *b, f32 dt, f32 ratio, f32 depth)
{
    if (b->env < 0.0004f) return 0.0f;
    if (b->fit < 0.0f) b->fit = sat(((0.45f / dt - b->hz) / (b->hz * ratio) - 1.0f) * (1.0f / 1.5f));     /* once a note */
    b->env -= b->env * b->dec;
    b->ph += b->hz * dt; if (b->ph >= 1.0f) b->ph -= 1.0f;
    b->mph += b->hz * ratio * dt; while (b->mph >= 1.0f) b->mph -= 1.0f;
    return sin_turns(b->ph + depth * b->fit * b->env * sin_turns(b->mph)) * b->env;
}

/* the room and the echo, shared by all pieces: set the fields, then one call a sample */
typedef struct {
    f32 room, damp, wet;                                    /* reverb: how long 0 .. 1, how dark 0 .. 1, how much */
    f32 echo_secs, echo_fb, echo_wet;
} MFx;
extern MFx m_fx;
void m_fx_init(f32 sr);
void m_fx_tick(f32 *l, f32 *r, f32 to_room, f32 to_echo);   /* adds the room and echo to l and r */
