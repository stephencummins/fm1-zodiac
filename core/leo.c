/* LEO: the lion, fire. Warm, proud, generous, ruled by the Sun: it sits crowned in its own light.
 * At a note it throws its head up to the right and roars, mouth wide, and the sun behind it flares.
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

void leo_frame(f32 dt, const ZParams *want)
{
    sc_step(&L.s, dt, want);
    f32 t = L.s.t;
    /* one big roar, head thrown up to the right: quick to open, held, slower to close */
    L.age += dt;
    f32 w = L.age < 0.22f ? L.age / 0.22f : L.age < 1.5f ? 1.0f : L.age < 2.2f ? (2.2f - L.age) / 0.7f : 0.0f;
    w = w * w * (3.0f - 2.0f * w); L.roar = w;
    f32 lean = w;
    sc_begin(&L.s);
    sun(w);
    sc_mist(&L.s, K_ORANGE);
    /* the jaw drops and trembles, the mouth open behind it, a tongue, no teeth */
    int jaw = (int)(w * (16.0f + 1.6f * sin_turns(t * 10.0f)) + 0.5f);
    p_begin(leo_pic);
    if (jaw > 0) {
        p_fill(102, 156, 36, jaw, 0xC0 | 63);
        p_fill(105, 157, 30, jaw, 0xC0 | 20);
        if (jaw > 6) { p_fill(110, 157 + jaw - 6, 20, 6, 0x80 | 63); p_fill(119, 157 + jaw - 6, 2, 4, 0x80 | 40); }
        p_move(leo_pic, 100, 156, 40, 22, 0, jaw);
    }
    /* then the whole head, mouth and all, lifts and turns to that side */
    for (int i = 0; i < GW * GH; i++) head[i] = g_work[i];
    p_warp(head, 120.0f, 126.0f, 86.0f, 86.0f, 0.05f * w + 0.006f * sin_turns(t * 0.09f),
           0.034f * lean + 0.003f * sin_turns(t * 0.05f), 9.0f * lean, -8.0f * w, 0);
    g_blit(g_work, 1.0f + 0.008f * sin_turns(t * 0.08f), 0.0f, 0.3f + 1.3f * L.s.p.heat, 0.03f, t * 0.6f, 0.95f + 0.25f * w);
}

void leo_palette(uint16_t pal[256])
{
    static const uint32_t inks[G_INKS] = {0xF4E8C8u, 0xF2B830u, 0xD87020u, 0x4A2890u};
    /* Blaze deep orange, Echo royal purple, Mist rose, Light gold */
    static const f32 tint[4][3] = {{150, 50, 6}, {70, 20, 120}, {120, 40, 60}, {150, 110, 20}};
    sc_palette_tint(&L.s, inks, 0.25f * L.roar, tint, L.roar, pal);
}
