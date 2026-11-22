/* Zodiac's core: no hardware here. A port (the computer, the browser, the FM-1) calls these. */
#pragma once
#include "gfx.h"

/* what the player's hands set, each 0 .. 1. No numbers are shown for them: the picture is the readout. */
typedef struct { f32 heat, echo, mist, light; } ZParams;

void aries_init(void);
void aries_frame(f32 dt, const ZParams *want);     /* move on by dt seconds and draw into g_fb */
void aries_palette(uint16_t pal[256]);             /* this frame's colours (after aries_frame) */
