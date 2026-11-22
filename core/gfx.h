/* Zodiac's drawing kit: a 240 x 240 picture of ink, drawn afresh each frame.
 * One byte a pixel: the top two bits say which of four inks, the low six how much of it (0 none .. 63 full).
 * A palette turns ink into colour when the frame is sent, so a picture has four colours on black. */
#pragma once
#include <stdint.h>

#define GW 240
#define GH 240
#define G_INKS 4
#define G_LEVELS 64

typedef float f32;

enum { G_MAX, G_PAINT };                 /* G_MAX: the brighter ink wins (line art). G_PAINT: covers what is under it */

extern uint8_t g_fb[GW * GH];

void g_clear(void);
void g_fade(int mul256);                 /* every pixel's ink times mul256 / 256: trails */
void g_ink(int ink);                     /* 0 .. 3 */
void g_mode(int mode);
void g_line(f32 x0, f32 y0, f32 x1, f32 y1, f32 w, f32 a);
void g_poly(const f32 *xy, int n, int closed, f32 w, f32 a);
void g_disc(f32 x, f32 y, f32 r, f32 a);
void g_ring(f32 x, f32 y, f32 r, f32 w, f32 a);
void g_fill(const f32 *xy, int n, f32 a);          /* a filled polygon, up to 96 points */
void g_glow(f32 x, f32 y, f32 r, f32 a);           /* a soft light, dithered */
void g_dot(f32 x, f32 y, f32 a);                   /* one grain */

/* the palette: four ink colours (0xRRGGBB) into 256 RGB565 entries */
void g_palette(const uint32_t rgb[G_INKS], f32 light, uint16_t out[256]);
void g_palette_on(const uint32_t rgb[G_INKS], f32 light, uint32_t ground, uint16_t out[256]);
