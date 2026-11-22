/* LEO: the lion, fire. Warm, proud, generous, ruled by the Sun: it sits crowned in its own light.
 * At a note it raises its head and roars, mouth wide, and the sun behind it flares.
 * The lion is art/pic/leo.jpg; the rays and the sparks are drawn here. */
#include "scene.h"
#include "pic_leo.h"

enum { K_CREAM, K_GOLD, K_ORANGE, K_PURPLE };

static struct { Scene s; f32 age, roar; } L;      /* age: seconds since the note; roar: 0 .. 1, how open the mouth */
static uint8_t head[GW * GH];             /* the head with its mouth open, before it is turned */

void leo_init(void) { sc_init(&L.s, 53u); L.age = 99.0f; L.roar = 0.0f; }
void leo_strike(void) { L.age = 0.0f; }

static void sun(f32 w)
{
    f32 blaze = L.s.p.heat, t = L.s.t;
    enum { N = 36 };
    g_mode(G_MAX);
    for (int i = 0; i < N; i++) {
        f32 th = ((f32)i + 0.5f) / (f32)N + t * 0.006f, dx = cos_turns(th), dy = sin_turns(th);
        f32 len = (6.0f + 58.0f * blaze * (0.5f + 0.5f * hash01((uint32_t)i * 13u + 3u))) * (0.75f + 0.25f * sin_turns(t * 0.19f + (f32)i * 0.37f)) + 46.0f * w;
        f32 r0 = 106.0f + 4.0f * sin_turns(t * 0.11f + (f32)i * 0.5f);
        g_ink(i & 1 ? K_GOLD : K_ORANGE);
        g_line(CX + dx * r0, CY + dy * r0, CX + dx * (r0 + len), CY + dy * (r0 + len), 1.6f + 1.4f * w, 0.35f + 0.5f * blaze + 0.4f * w);
    }
    int n = (int)(6.0f + 44.0f * blaze);
    for (int i = 0; i < n; i++) {                                   /* sparks rising */
        f32 u = t * (0.05f + 0.07f * hash01((uint32_t)i * 7u + 1u)) + hash01((uint32_t)i * 11u); u -= (f32)(int)u;
        f32 x = hash01((uint32_t)i * 29u + 5u) * 240.0f + sin_turns(t * 0.2f + (f32)i * 0.3f) * 6.0f;
        g_ink(i % 3 ? K_GOLD : K_ORANGE); g_dot(x, 244.0f - 250.0f * u, (1.0f - u) * 0.9f);
        g_dot(x + 1.0f, 244.0f - 250.0f * u, (1.0f - u) * 0.6f);
    }
}

/* the open mouth, from row y0 down to y1, hw pixels to each side of the middle: dark lips round a dark throat,
 * a tongue lying in the lower half, the four canines showing */
static void mouth(int y0, int y1, f32 hw)
{
    int H = y1 - y0;
    f32 cr = minf(7.0f, (f32)H * 0.5f);
    for (int y = y0; y <= y1; y++)
        for (int x = 96; x <= 144; x++) {
            f32 ax = absf((f32)x - 119.5f), fy = (f32)(y - y0), by = (f32)(y1 - y), half = hw;
            f32 ex = ax - (half - cr), ey = (fy < cr ? cr - fy : by < cr ? cr - by : 0.0f), d;
            if (ex > 0.0f && ey > 0.0f) d = cr - sqrtf_(ex * ex + ey * ey); else { d = half - ax; if (fy < d) d = fy; if (by < d) d = by; }
            if (d < 0.0f) continue;                                  /* d: how far inside the edge */
            uint8_t v = d < 2.0f ? (0xC0 | 46) : (0xC0 | 9);
            f32 tx = ax / 11.0f, ty = ((f32)y - ((f32)y1 - 7.0f)) / 6.0f;
            if (H > 10 && d >= 2.0f && tx * tx + ty * ty < 1.0f) v = ax < 1.0f && ty < 0.3f ? (0x80 | 36) : (0x80 | 63);
            if (H > 14 && d >= 1.0f) {
                f32 up = absf(ax - (half - 5.5f)), lo = absf(ax - (half - 7.5f));
                if (fy < 7.0f && up < 1.8f - fy * 0.22f) v = 0x00 | 63;                  /* upper canines */
                if (by < 5.0f && lo < 1.5f - by * 0.25f) v = 0x00 | 58;                  /* lower canines */
            }
            g_work[y * GW + x] = v;
        }
}

void leo_frame(f32 dt, const ZParams *want)
{
    sc_step(&L.s, dt, want);
    f32 t = L.s.t;
    /* one big roar, the head raised: quick to open, held, slower to close */
    L.age += dt;
    f32 w = L.age < 0.22f ? L.age / 0.22f : L.age < 1.5f ? 1.0f : L.age < 2.2f ? (2.2f - L.age) / 0.7f : 0.0f;
    w = w * w * (3.0f - 2.0f * w); L.roar = w;
    sc_begin(&L.s);
    sun(w);
    sc_mist(&L.s, K_ORANGE);
    /* the jaw drops and trembles; the open mouth is drawn behind it */
    int jaw = (int)(w * (21.0f + 1.6f * sin_turns(t * 10.0f)) + 0.5f);
    p_begin(leo_pic);
    /* the eyes: gold, looking straight at you from under a lowered brow, and they stay on you through the roar */
    int lid = (int)(L.s.blink * (1.0f - w) * 7.0f + 0.5f);
    for (int e = 0; e < 2; e++) {
        int ex = e ? 133 : 99, in = e ? ex : ex + 4;          /* in: the half of the eye nearer the nose */
        p_fill(ex, 110, 8, 7, 0x40 | 63);
        p_fill(ex + 2, 112, 4, 4, 0xC0 | 10);
        p_fill(ex + 2, 112, 1, 1, 0x00 | 63);
        p_fill(ex, 110, 8, 1, 0xC0 | 63); p_fill(in, 111, 4, 1, 0xC0 | 63); p_fill(e ? ex : ex + 6, 112, 2, 1, 0xC0 | 63);
        if (w > 0.5f) p_fill(ex, 116, 8, 1, 0xC0 | 63);       /* narrowed from below as the cheeks push up */
        if (lid > 0) p_fill(ex, 110, 8, lid, 0xC0 | 63);
    }
    if (jaw > 0) {
        p_move(leo_pic, 100, 156, 40, 22, 0, jaw);            /* the chin goes down with the jaw */
        mouth(151 - (jaw > 6 ? 2 : 0), 158 + jaw, 15.0f + 0.2f * (f32)jaw);
    }
    /* the nose and the upper lip lift */
    for (int i = 0; i < GW * GH; i++) head[i] = g_work[i];
    p_warp(head, 120.0f, 142.0f, 24.0f, 13.0f, 0.0f, 0.0f, 0.0f, -3.0f * w, 0);
    /* then the whole head, mouth and all, is raised */
    for (int i = 0; i < GW * GH; i++) head[i] = g_work[i];
    p_warp(head, 120.0f, 126.0f, 86.0f, 86.0f, 0.05f * w + 0.006f * sin_turns(t * 0.09f),
           0.003f * sin_turns(t * 0.05f), 0.0f, -10.0f * w, 0);
    g_blit(g_work, 1.0f + 0.008f * sin_turns(t * 0.08f), 0.0f, 0.3f + 1.3f * L.s.p.heat, 0.03f, t * 0.6f, 0.95f + 0.25f * w);
}

void leo_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xF4E8C8u, 0xF2B830u, 0xD87020u, 0x4A2890u};
    /* Blaze deep orange, Echo royal purple, Mist rose, Light gold */
    static const f32 tint[4][3] = {{150, 50, 6}, {70, 20, 120}, {120, 40, 60}, {150, 110, 20}};
    sc_palette_tint(&L.s, inks, 0.25f * L.roar, tint, L.roar, pal);
}
