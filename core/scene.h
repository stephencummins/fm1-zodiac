/* What the signs share: the hands' settings followed smoothly, a blink, the six-sided frame, the mist. */
#pragma once
#include "zodiac.h"
#include "mathx.h"

#define CX 120.0f
#define CY 128.0f
#define HEXR 86.0f

typedef struct {
    f32 t;
    ZParams p;                   /* the hands' settings, followed smoothly */
    uint32_t seed;
    f32 blink, next_blink;       /* 0 open .. 1 shut */
} Scene;

void sc_init(Scene *s, uint32_t seed);
f32 sc_rnd(Scene *s);
void sc_step(Scene *s, f32 dt, const ZParams *want);
void sc_begin(const Scene *s);                                   /* Echo: the last frame fades; else a clean sheet */
void sc_mist(const Scene *s, int ink);
void sc_hex(f32 r, f32 xy[12]);
void sc_frame_back(f32 beat, int ink, f32 glow);                 /* the frame hides what is behind it */
void sc_frame_line(f32 beat, f32 rays);
f32 sc_follow(f32 v, f32 aim, f32 dt, f32 up, f32 down);         /* v towards aim, at one rate rising, another falling */
void sc_palette(const Scene *s, const uint32_t inks[G_INKS], f32 boost, uint16_t pal[256]);

void sc_rot(f32 *xy, int n, f32 cx, f32 cy, f32 turns);
void sc_squash(f32 *xy, int n, f32 y0, f32 by);
void sc_squashx(f32 *xy, int n, f32 x0, f32 by);
void sc_shift(f32 *xy, int n, f32 dx, f32 dy);
