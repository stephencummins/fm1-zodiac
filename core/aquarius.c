/* AQUARIUS: the water-bearer, air. Original, friendly, far-sighted: a jar that never runs dry, under a star.
 * At a note the water surges up out of the jar, the star flares and the waves below run higher.
 * The jar is art/pic/aquarius.jpg; the running water and the twinkles are drawn here. */
#include "scene.h"
#include "pic_aquarius.h"

enum { K_ICE, K_BLUE, K_AQUA, K_LEMON };

static struct { Scene s; f32 surge; } Q;

void aquarius_init(void) { sc_init(&Q.s, 131u); Q.surge = 0.0f; }
void aquarius_strike(void) { Q.surge = 1.0f; }

/* the water running: bright beads over the two arcs, up from the jar's mouth, over and down */
static void run(f32 w)
{
    f32 h = Q.s.p.heat, t = Q.s.t;
    int n = (int)(8.0f + 16.0f * h + 12.0f * w);
    g_mode(G_PAINT); g_ink(K_AQUA);
    for (int i = 0; i < n; i++) {
        f32 u = t * (0.16f + 0.3f * h + 0.3f * w) + (f32)i / (f32)n; u -= (f32)(int)u;
        f32 side = i & 1 ? 1.0f : -1.0f, off = (hash01((uint32_t)i * 13u + 3u) - 0.5f) * 9.0f, x, y;
        if (u < 0.4f) { f32 a = 0.5f * u / 0.4f; x = 26.0f * (1.0f - cos_turns(a)) * 0.5f + 2.0f; y = 78.0f - 12.0f * sin_turns(a); x = 6.0f + 21.0f * (1.0f - cos_turns(a)); }
        else { x = 48.0f; y = 78.0f + 76.0f * (u - 0.4f) / 0.6f; }
        g_disc(CX + side * (x + off * 0.5f), y + (u < 0.4f ? off * 0.4f : 0.0f), 1.5f, 0.9f);
    }
}

static void twinkle(void)
{
    f32 t = Q.s.t, h = Q.s.p.heat;
    g_mode(G_MAX);
    for (int i = 0; i < 16; i++) {
        f32 x = hash01((uint32_t)i * 29u + 5u) * 240.0f, y = hash01((uint32_t)i * 31u + 9u) * 240.0f;
        f32 b = maxf(0.0f, sin_turns(t * (0.10f + 0.1f * hash01((uint32_t)i * 3u)) + (f32)i * 0.17f)) * (0.3f + 0.7f * h);
        g_ink(i & 1 ? K_LEMON : K_ICE); g_line(x - 3.0f, y, x + 3.0f, y, 1.0f, b); g_line(x, y - 3.0f, x, y + 3.0f, 1.0f, b);
    }
}

void aquarius_frame(f32 dt, const ZParams *want)
{
    sc_step(&Q.s, dt, want);
    f32 t = Q.s.t, h = Q.s.p.heat;
    Q.surge = maxf(0.0f, Q.surge - dt * 0.7f);
    f32 w = Q.surge * Q.surge * (3.0f - 2.0f * Q.surge);
    sc_begin(&Q.s);
    twinkle();
    sc_mist(&Q.s, K_BLUE);
    p_begin(aquarius_pic);
    /* the fountain lifts and swells; the jar itself (blue and its aqua line) stays where it is */
    p_warp(aquarius_pic, 120.0f, 92.0f, 66.0f, 44.0f, 0.07f * w + 0.012f * sin_turns(t * 0.25f), 0.0f, 0.0f, -6.0f * w, 2);
    p_warp(aquarius_pic, 120.0f, 48.0f, 12.0f, 12.0f, 0.10f * sin_turns(t * 0.35f) + 0.45f * w, 0.0f, 0.0f, 0.0f, 0);
    g_blit(g_work, 1.0f + 0.006f * sin_turns(t * 0.06f), 150.0f, 0.8f + 1.6f * h + 2.5f * w, 0.03f, t * (0.25f + 0.3f * h), 0.95f + 0.2f * w);
    run(w);
    if (w > 0.0f) { g_mode(G_MAX); g_ink(K_LEMON); g_ring(120.0f, 48.0f, 8.0f + 70.0f * (1.0f - Q.surge), 1.3f, w); }
}

void aquarius_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xD8F8F4u, 0x1478E0u, 0x28E4E8u, 0xF0E040u};
    /* Flow deep blue, Echo violet, Mist teal, Light pale gold */
    static const f32 tint[4][3] = {{8, 30, 130}, {80, 30, 130}, {10, 100, 110}, {130, 120, 60}};
    sc_palette_tint(&Q.s, inks, 0.12f * Q.surge, tint, 0.5f * Q.surge, pal);
}
