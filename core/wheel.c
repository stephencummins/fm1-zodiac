/* The wheel: the twelve signs' symbols in a ring, the one asked for at the top. It turns to bring a sign up, and
 * when it has rested there the symbol comes to the middle and grows as the rest go out past the edges.
 * Line art only: each symbol is a few strokes and arcs, drawn at whatever size is wanted. No buffers of its own. */
#include "zodiac.h"
#include "mathx.h"
#include "wheel.h"

/* a symbol's strokes, in half units on a square -20 .. 20 (y down):
 *   P n x y ...      a line through n points
 *   A cx cy r a b    an arc about (cx, cy) from a to b, in 32nds of a turn (0 right, 8 down, 16 left, 24 up)
 *   E                the end */
enum { E, P, A };
static const int8_t aries[] = {P, 2, 0, 20, 0, -6, A, -9, -6, 9, 12, 32, A, 9, -6, 9, 16, 36, E};
static const int8_t taurus[] = {A, 0, 7, 11, 0, 32, A, 0, -18, 14, 1, 15, E};
static const int8_t gemini[] = {P, 2, -7, -14, -7, 14, P, 2, 7, -14, 7, 14, A, 0, -32, 19, 4, 12, A, 0, 32, 19, 20, 28, E};
static const int8_t cancer[] = {A, -9, -7, 5, 0, 32, A, 0, 0, 15, 19, 31, A, 9, 7, 5, 0, 32, A, 0, 0, 15, 3, 15, E};
static const int8_t leo[] = {A, -8, 3, 5, 0, 32, A, 2, -6, 10, 14, 35, P, 2, 10, 0, 7, 12, A, 12, 12, 5, 0, 16, E};
static const int8_t virgo[] = {P, 2, -17, -14, -17, 14, A, -13, -8, 4, 16, 32, P, 2, -9, -8, -9, 14, A, -5, -8, 4, 16, 32,
                               P, 2, -1, -8, -1, 14, A, 3, -8, 4, 16, 32, P, 2, 7, -8, 7, 0, A, 7, 8, 8, 24, 46, E};
static const int8_t libra[] = {P, 2, -17, 14, 17, 14, P, 2, -17, 6, -6, 6, P, 2, 6, 6, 17, 6, A, 0, -3, 11, 11, 37, E};
static const int8_t scorpio[] = {P, 2, -17, -14, -17, 14, A, -13, -8, 4, 16, 32, P, 2, -9, -8, -9, 14, A, -5, -8, 4, 16, 32,
                                 P, 2, -1, -8, -1, 14, A, 3, -8, 4, 16, 32, P, 2, 7, -8, 7, 8, A, 11, 8, 4, 8, 16,
                                 P, 2, 11, 12, 18, 5, P, 3, 12, 5, 18, 5, 18, 11, E};
static const int8_t sagittarius[] = {P, 2, -15, 15, 15, -15, P, 3, 2, -15, 15, -15, 15, -2, P, 2, -13, -2, 2, 13, E};
static const int8_t capricorn[] = {P, 4, -17, -13, -9, 8, -1, -13, 6, 3, A, 11, 8, 6, 0, 32, P, 2, 6, 11, -3, 17, E};
static const int8_t aquarius[] = {P, 7, -18, -3, -12, -11, -6, -3, 0, -11, 6, -3, 12, -11, 18, -3,
                                  P, 7, -18, 10, -12, 2, -6, 10, 0, 2, 6, 10, 12, 2, 18, 10, E};
static const int8_t pisces[] = {A, -23, 0, 17, 27, 37, A, 23, 0, 17, 11, 21, P, 2, -13, 0, 13, 0, E};
static const int8_t *const glyph[12] = {aries, taurus, gemini, cancer, leo, virgo, libra, scorpio, sagittarius, capricorn, aquarius, pisces};

/* sign `s` about (x, y), half as tall as `half` pixels, in lines w wide at strength a */
static void symbol(int s, f32 x, f32 y, f32 half, f32 w, f32 a)
{
    const int8_t *g = glyph[s];
    f32 k = half * (1.0f / 20.0f);
    if (a <= 0.01f) return;
    for (;;) {
        if (*g == P) {
            int n = g[1];
            g += 2;
            for (int i = 0; i + 1 < n; i++)
                g_line(x + k * (f32)g[2 * i], y + k * (f32)g[2 * i + 1], x + k * (f32)g[2 * i + 2], y + k * (f32)g[2 * i + 3], w, a);
            g += 2 * n;
        } else if (*g == A) {
            f32 cx = x + k * (f32)g[1], cy = y + k * (f32)g[2], r = k * (f32)g[3];
            int a0 = g[4], a1 = g[5], n = (a1 - a0) * (half > 30.0f ? 2 : 1);         /* a big one in shorter steps: still round */
            f32 px = 0.0f, py = 0.0f;
            for (int i = 0; i <= n; i++) {
                f32 t = ((f32)a0 + (f32)(a1 - a0) * (f32)i / (f32)n) * (1.0f / 32.0f);
                f32 qx = cx + r * cos_turns(t), qy = cy + r * sin_turns(t);
                if (i) g_line(px, py, qx, qy, w, a);
                px = qx; py = qy;
            }
            g += 6;
        } else {
            return;
        }
    }
}

/* at: the place in the ring that is at the top, in signs (3.0 Cancer; 2.5 half way from Gemini to it); it may run
 * past 12 or below 0. open: 0 the wheel, to 1 the chosen sign grown in the middle and everything else gone.
 * Ink k is element k (fire, earth, air, water): a sign is drawn in its element's. */
void wheel_frame(f32 at, f32 open, f32 t)
{
    int top = (int)(at + 1200.5f) % 12;
    f32 o = sat(open), e = o * o * (3.0f - 2.0f * o), out = 1.0f - e;
    g_clear();
    g_mode(G_MAX);
    /* the ring the symbols ride on, and a mark between each two */
    g_ink(2);
    g_ring(120.0f, 120.0f, 111.0f + 80.0f * e, 1.2f, 0.28f * out);
    g_ring(120.0f, 120.0f, 55.0f + 120.0f * e, 1.2f, 0.22f * out);
    for (int i = 0; i < 12; i++) {
        f32 th = ((f32)i + 0.5f - at) * (1.0f / 12.0f), c = cos_turns(th - 0.25f), s = sin_turns(th - 0.25f);
        f32 r0 = 104.0f + 80.0f * e, r1 = 111.0f + 80.0f * e;
        g_line(120.0f + r0 * c, 120.0f + r0 * s, 120.0f + r1 * c, 120.0f + r1 * s, 1.2f, 0.3f * out);
    }
    /* the middle: the chosen sign's element, breathing */
    g_ink(top & 3);
    g_glow(120.0f, 120.0f, 34.0f + 3.0f * sin_turns(t * 0.2f) + 60.0f * e, 0.3f * out + 0.25f * e * (1.0f - o));
    for (int i = 0; i < 12; i++) {
        f32 d = (f32)i - at;                        /* how far round from the top, -6 .. 6 signs */
        d -= 12.0f * (f32)(int)(d * (1.0f / 12.0f));
        if (d > 6.0f) d -= 12.0f;
        if (d < -6.0f) d += 12.0f;
        f32 near = sat(1.0f - absf(d)), th = d * (1.0f / 12.0f) - 0.25f;
        f32 r = 83.0f, half = 11.0f + 9.0f * near, a = 0.38f + 0.62f * near, w = 1.5f + 1.0f * near;
        f32 x = 120.0f + r * cos_turns(th), y = 120.0f + r * sin_turns(th);
        g_ink(i & 3);
        if (i == top) {                             /* to the middle, growing, and thinning out at the last */
            x += (120.0f - x) * e; y += (120.0f - y) * e;
            half += 62.0f * e; w += 2.0f * e;
            a *= 1.0f - sat((o - 0.55f) * (1.0f / 0.45f));
        } else {                                    /* outward and gone */
            x += (x - 120.0f) * 1.2f * e; y += (y - 120.0f) * 1.2f * e;
            a *= out * out;
        }
        symbol(i, x, y, half, w, a);
    }
    /* the top is marked: a small notch on the outer ring */
    g_ink(2);
    g_line(120.0f, 6.0f, 120.0f, 12.0f, 2.0f, 0.8f * out);
}

void wheel_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xFF7A3Du, 0x86D46Au, 0xF4E7B4u, 0x58A9FFu};     /* fire, earth, air, water */
    g_palette(inks, 1.0f, pal);
}
