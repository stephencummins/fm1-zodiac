/* A sign's drawing as data (made by tools/svg2sign.py from an SVG) and the code that draws it, moved. */
#pragma once
#include "gfx.h"

typedef struct {
    uint8_t kind;                /* 0 a line in ink, 1 a filled shape (covers what is behind), 2 a black line (cuts) */
    uint8_t ink, level;          /* which ink, how much (0 .. 63) */
    uint8_t width;               /* a line's width, 1/16 pixel */
    uint8_t group, closed;
    uint16_t first, n;           /* its points in pts */
} SignPath;

typedef struct {
    const SignPath *paths; int npaths;
    const int16_t *pts; int npts;        /* x, y in 1/16 pixel */
} Sign;

/* called for each path with its points (n pairs) to move in place, and its ink's strength to scale */
typedef void (*SignMove)(int group, f32 *xy, int n, f32 *gain, void *ctx);

void sign_draw(const Sign *s, SignMove move, void *ctx);
