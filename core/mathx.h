/* small maths without libm: the FM-1's chip has single floats only */
#pragma once
#include <stdint.h>
#include "gfx.h"

static inline f32 minf(f32 a, f32 b) { return a < b ? a : b; }
static inline f32 maxf(f32 a, f32 b) { return a > b ? a : b; }
static inline f32 sat(f32 x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
static inline f32 absf(f32 x) { return x < 0.0f ? -x : x; }

static inline f32 sqrtf_(f32 x)
{
    if (x <= 0.0f) return 0.0f;
    union { f32 f; uint32_t i; } u = {x};
    u.i = 0x5f3759dfu - (u.i >> 1);
    f32 y = u.f;
    y = y * (1.5f - 0.5f * x * y * y);
    y = y * (1.5f - 0.5f * x * y * y);
    return x * y;
}

/* sine of an angle in turns (1.0 = once round): a parabola pair, corrected */
static inline f32 sin_turns(f32 t)
{
    t -= (f32)(int)t;
    if (t < 0.0f) t += 1.0f;
    f32 x = t < 0.5f ? t : t - 1.0f;                 /* -0.5 .. 0.5 */
    f32 y = 16.0f * x * (0.5f - absf(x));
    return y * (0.775f + 0.225f * absf(y));
}
static inline f32 cos_turns(f32 t) { return sin_turns(t + 0.25f); }

/* a repeatable number 0 .. 1 from an integer */
static inline f32 hash01(uint32_t n)
{
    n ^= n >> 16; n *= 0x7feb352du; n ^= n >> 15; n *= 0x846ca68bu; n ^= n >> 16;
    return (f32)(n >> 8) * (1.0f / 16777216.0f);
}
/* smooth noise 0 .. 1 along x, a different curve for each seed */
static inline f32 noise1(f32 x, uint32_t seed)
{
    int i = (int)x; if ((f32)i > x) i--;
    f32 f = x - (f32)i, u = f * f * (3.0f - 2.0f * f);
    f32 a = hash01((uint32_t)i * 2654435761u + seed), b = hash01((uint32_t)(i + 1) * 2654435761u + seed);
    return a + (b - a) * u;
}
