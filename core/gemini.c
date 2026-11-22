/* GEMINI: the twins, air. Quick, curious, talkative, never the same for long: two faces that talk in turn, glance
 * about, and whose words fly between them on the wind. A strike turns them over like cards and they change colours.
 * The picture is art/pic/gemini.jpg, brought in whole; the words, the wind and its sway are drawn here.
 * (The first, drawn twins are in art/gemini.svg and this file's history.) */
#include "scene.h"
#include "sign.h"
#include "pic_gemini.h"

enum { K_LINE, K_ROSE, K_SUN, K_SKY };

#define NWORD 36
#define SCALE 1.16f
typedef struct { f32 x, y, vx, age, life; int ink, kind; } Word;

static struct {
    Scene s;
    int speaker; f32 next_turn, env[2], open;
    f32 flip; int swapped; f32 next_flip;
    f32 blink2, next_blink2;
    f32 wink_t, wink, smile; int winker;      /* a note: one of them winks, both smile */
    Word w[NWORD]; f32 spawn;
} G;

void gemini_init(void)
{
    sc_init(&G.s, 23u);
    G.speaker = 0; G.next_turn = 2.5f; G.env[0] = G.env[1] = 0.0f; G.open = 0.0f;
    G.flip = -1.0f; G.swapped = 0; G.next_flip = 14.0f; G.blink2 = 0.0f; G.next_blink2 = 4.4f; G.spawn = 0.0f;
    G.wink_t = -1.0f; G.wink = 0.0f; G.smile = 0.0f; G.winker = 1;
    for (int i = 0; i < NWORD; i++) G.w[i].life = 0.0f;
}

static void turn_over(void)
{
    if (G.flip < 0.0f) G.flip = 0.0f;
    G.speaker ^= 1; G.next_turn = 1.5f + sc_rnd(&G.s) * 2.0f;
}

/* a note: the twins take it in turns to wink, and both smile */
void gemini_strike(void) { G.winker ^= 1; G.wink_t = 0.0f; }

/* the faces' moving parts, put into the working picture: an eye shut to a curve, the mouths' corners lifted */
static void faces(void)
{
    p_begin(gemini_pic);
    uint8_t rose = gemini_pic[130 * GW + 92], sky = gemini_pic[135 * GW + 150];
    if (G.wink > 0.5f) {
        if (G.winker == 0) {
            p_fill(93, 116, 12, 10, rose);
            p_fill(95, 122, 7, 1, 0); p_fill(94, 121, 1, 1, 0); p_fill(102, 121, 1, 1, 0);
        } else {
            p_fill(135, 115, 14, 11, sky);
            p_fill(138, 122, 8, 1, 0); p_fill(137, 121, 1, 1, 0); p_fill(146, 121, 1, 1, 0);
        }
    }
    int n = (int)(G.smile * 4.0f + 0.5f);
    uint8_t dr = (uint8_t)(0x40 | 16), ds = (uint8_t)(0xC0 | 16);
    for (int k = 1; k <= n; k++) {
        int up = k < 3 ? k - 1 : k;                    /* the corner runs back, then turns up */
        p_fill(100 - k, 143 - up, 1, 1, dr); p_fill(139 + k, 143 - up, 1, 1, ds);
    }
    if (n >= 3) { p_fill(95, 139, 1, 1, dr); p_fill(144, 139, 1, 1, ds); }            /* and dimples the cheek */
}

static void wind(void)
{
    f32 wv = G.s.p.heat, t = G.s.t;
    enum { NS = 20, N = 8 };
    g_mode(G_MAX);
    for (int i = 0; i < NS; i++) {
        f32 r = 95.0f + 46.0f * hash01((uint32_t)i * 13u + 1u);
        f32 th0 = hash01((uint32_t)i * 29u) + t * (0.015f + 0.11f * wv) * (0.6f + 0.8f * hash01((uint32_t)i * 5u + 2u));
        f32 len = 0.035f + 0.11f * wv * (0.5f + 0.5f * hash01((uint32_t)i * 11u + 4u));
        f32 xy[N * 2];
        for (int k = 0; k < N; k++) {
            f32 th = th0 - len * (f32)k / (f32)(N - 1);
            f32 rr = r + 4.0f * sin_turns(th * 3.0f + t * 0.17f + (f32)i * 0.4f) + 5.0f * wv * sin_turns(th * 7.0f - t * 0.5f);
            xy[2 * k] = CX + cos_turns(th) * rr; xy[2 * k + 1] = CY + sin_turns(th) * rr;
        }
        f32 a = 0.22f + 0.60f * wv;
        g_ink(i & 1 ? K_SKY : K_SUN);
        g_poly(xy, N, 0, 1.2f, a * 0.5f);
        g_poly(xy, N / 2, 0, 1.4f, a);
        g_ink(K_LINE); g_disc(xy[0], xy[1], 1.2f, a);
    }
}

/* what they say: small marks that leave the speaker's mouth and cross to the other */
static void words(f32 dt)
{
    f32 wv = G.s.p.heat;
    if (G.open > 0.3f) G.spawn += dt * (7.0f + 8.0f * wv);
    g_mode(G_MAX);
    for (int i = 0; i < NWORD; i++) {
        Word *w = &G.w[i];
        if (w->life <= 0.0f) {
            if (G.spawn < 1.0f) continue;
            G.spawn -= 1.0f;
            f32 m[2] = {G.speaker ? 128.0f : 112.0f, 139.0f};
            w->x = m[0]; w->y = m[1] + (sc_rnd(&G.s) - 0.5f) * 3.0f;
            w->vx = (G.speaker ? -1.0f : 1.0f) * (16.0f + 22.0f * wv + sc_rnd(&G.s) * 10.0f);
            w->age = 0.0f; w->life = 1.3f + sc_rnd(&G.s) * 1.3f; w->ink = G.speaker ? K_SKY : K_SUN; w->kind = (int)(sc_rnd(&G.s) * 3.0f);
            continue;
        }
        w->age += dt;
        if (w->age >= w->life) { w->life = 0.0f; continue; }
        f32 u = w->age / w->life;
        w->x += w->vx * dt * (1.0f - 0.6f * u);
        w->y += (-(10.0f + 26.0f * u) + sin_turns(w->age * 0.9f + (f32)i * 0.3f) * 14.0f) * dt;
        f32 a = (u < 0.1f ? u * 10.0f : 1.0f - (u - 0.1f) / 0.9f);
        g_ink(w->ink);
        if (w->kind == 0) g_disc(w->x, w->y, 1.3f, a);
        else if (w->kind == 1) g_line(w->x - 2.2f, w->y + 0.8f, w->x + 2.2f, w->y - 0.8f, 1.2f, a);
        else g_ring(w->x, w->y, 2.0f, 0.9f, a);
    }
}

void gemini_frame(f32 dt, const ZParams *want)
{
    sc_step(&G.s, dt, want);
    f32 wv = G.s.p.heat, t = G.s.t;
    G.next_turn -= dt;
    if (G.next_turn < 0.0f) { G.speaker ^= 1; G.next_turn = (1.2f + sc_rnd(&G.s) * 3.2f) * (1.3f - 0.6f * wv); }
    G.env[0] = sc_follow(G.env[0], G.speaker == 0 ? 1.0f : 0.0f, dt, 6.0f, 4.0f);
    G.env[1] = sc_follow(G.env[1], G.speaker == 1 ? 1.0f : 0.0f, dt, 6.0f, 4.0f);
    f32 rate = 2.2f + 2.0f * wv;                                              /* they talk faster in a wind */
    G.open = G.env[G.speaker] * absf(sin_turns(t * rate)) * (0.35f + 0.65f * noise1(t * 2.3f, 71u));
    G.next_blink2 -= dt;
    if (G.next_blink2 < 0.0f) G.next_blink2 = 2.5f + sc_rnd(&G.s) * 6.0f;
    G.blink2 = sc_follow(G.blink2, G.next_blink2 < 0.16f ? 1.0f : 0.0f, dt, 30.0f, 30.0f);
    G.next_flip -= dt;
    if (G.next_flip < 0.0f) { G.next_flip = (14.0f + sc_rnd(&G.s) * 14.0f) * (1.3f - 0.8f * wv); turn_over(); }
    if (G.wink_t >= 0.0f) { G.wink_t += dt; if (G.wink_t > 1.6f) G.wink_t = -1.0f; }
    G.wink = sc_follow(G.wink, G.wink_t >= 0.0f && G.wink_t < 0.5f ? 1.0f : 0.0f, dt, 40.0f, 25.0f);
    G.smile = sc_follow(G.smile, G.wink_t >= 0.0f && G.wink_t < 1.1f ? 1.0f : 0.0f, dt, 18.0f, 4.0f);
    if (G.flip >= 0.0f) {
        f32 was = G.flip;
        G.flip += dt / 0.55f;
        if (was < 0.5f && G.flip >= 0.5f) G.swapped ^= 1;
        if (G.flip >= 1.0f) G.flip = -1.0f;
    }

    sc_begin(&G.s);
    wind();
    sc_mist(&G.s, K_SKY);
    /* the picture breathes, sways in the wind from the neck up to the curls, and narrows to nothing on a flip */
    f32 fl = G.flip >= 0.0f ? 1.0f - absf(cos_turns(G.flip * 0.5f)) : 0.0f;
    faces();
    g_blit(g_work, 1.0f + 0.010f * sin_turns(t * 0.11f) - 0.10f * fl, -400.0f,
           (0.6f + 3.2f * wv) * (1.0f + 2.0f * fl), 0.004f, t * (0.10f + 0.35f * wv), 0.92f + 0.10f * G.open + 0.3f * fl);
    words(dt);
}

void gemini_palette(uint16_t pal[256])
{
    uint32_t inks[G_INKS] = {0xF6EDC4u, 0xC8409Cu, 0xF2C230u, 0x58B0E8u};
    if (G.swapped) { uint32_t x = inks[K_SUN]; inks[K_SUN] = inks[K_SKY]; inks[K_SKY] = x; }
    f32 fl = G.flip >= 0.0f ? 1.0f - absf(cos_turns(G.flip * 0.5f)) : 0.0f;
    /* Wind sky blue, Echo magenta, Mist lilac, Light yellow */
    static const f32 tint[4][3] = {{20, 90, 160}, {140, 24, 96}, {70, 50, 150}, {150, 128, 20}};
    sc_palette_tint(&G.s, inks, 0.25f * fl, tint, fl, pal);
}
