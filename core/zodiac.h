/* Zodiac's core: no hardware here. A port (the computer, the browser, the FM-1) calls these. */
#pragma once
#include "gfx.h"

/* what the player's hands set, each 0 .. 1. No numbers are shown for them: the picture is the readout. */
/* heat is the sign's own element: Aries' fire, Taurus' growth, Gemini's wind, Cancer's tide */
typedef struct { f32 heat, echo, mist, light; } ZParams;

/* a sign: its picture and how it behaves */
typedef struct {
    const char *name, *element, *knob;             /* knob: what the first knob is called for this sign */
    void (*init)(void);
    void (*frame)(f32 dt, const ZParams *want);    /* move on by dt seconds and draw into g_fb */
    void (*strike)(void);                          /* a note, a key */
    void (*palette)(uint16_t pal[256]);            /* this frame's colours (after frame) */
} ZSign;
extern const ZSign z_signs[];
extern const int z_nsigns;

#define Z_SIGN(n) void n##_init(void); void n##_frame(f32 dt, const ZParams *want); void n##_strike(void); void n##_palette(uint16_t pal[256]);
Z_SIGN(aries) Z_SIGN(taurus) Z_SIGN(gemini) Z_SIGN(cancer)
