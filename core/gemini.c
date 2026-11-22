/* GEMINI: the twins, air. Quick, curious, talkative, never the same for long: two faces that talk in turn, glance
 * about, and whose words fly between them on the wind. A strike turns them over like cards and they change colours.
 * The faces are art/gemini.svg; the hair, the words, the stars and the wind are drawn here. */
#include "scene.h"
#include "sign.h"
#include "art_gemini.h"

enum { K_LINE, K_ROSE, K_SUN, K_SKY };

#define NWORD 36
#define SCALE 1.16f
typedef struct { f32 x, y, vx, age, life; int ink, kind; } Word;

static struct {
    Scene s;
    int speaker; f32 next_turn, env[2], open;
    f32 flip; int swapped; f32 next_flip;
    f32 blink2, next_blink2;
    Word w[NWORD]; f32 spawn;
} G;

void gemini_init(void)
{
    sc_init(&G.s, 23u);
    G.speaker = 0; G.next_turn = 2.5f; G.env[0] = G.env[1] = 0.0f; G.open = 0.0f;
    G.flip = -1.0f; G.swapped = 0; G.next_flip = 14.0f; G.blink2 = 0.0f; G.next_blink2 = 4.4f; G.spawn = 0.0f;
    for (int i = 0; i < NWORD; i++) G.w[i].life = 0.0f;
}

void gemini_strike(void)
{
    if (G.flip < 0.0f) G.flip = 0.0f;
    G.speaker ^= 1; G.next_turn = 1.5f + sc_rnd(&G.s) * 2.0f;
}

/* one twin's place: leaning in to speak, back to listen, and turned over on a flip */
static void twin_xf(int right, f32 *xy, int n)
{
    f32 sg = right ? -1.0f : 1.0f;
    f32 lean = 0.006f * sin_turns(G.s.t * 0.05f + (f32)right * 0.3f) + 0.016f * G.env[right] - 0.008f * G.env[!right];
    sc_rot(xy, n, right ? 163.0f : 77.0f, 172.0f, sg * lean);
    if (G.flip >= 0.0f) sc_squashx(xy, n, right ? 156.0f : 84.0f, absf(cos_turns(G.flip * 0.5f)));
    for (int k = 0; k < n; k++) { xy[2 * k] = CX + (xy[2 * k] - CX) * SCALE; xy[2 * k + 1] = 134.0f + (xy[2 * k + 1] - 134.0f) * SCALE; }
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

static void star(f32 x, f32 y, f32 r, f32 a)
{
    f32 p[16];
    for (int k = 0; k < 8; k++) {
        f32 rr = (k & 1) ? r * 0.30f : r, th = (f32)k / 8.0f;
        p[2 * k] = x + cos_turns(th) * rr; p[2 * k + 1] = y + sin_turns(th) * rr;
    }
    g_mode(G_MAX); g_ink(K_LINE); g_fill(p, 8, a);
}

static void hair(int right)
{
    static const f32 root[7][2] = {{86, 82}, {76, 86}, {67, 93}, {61, 104}, {58, 118}, {58, 133}, {61, 148}};
    f32 wv = G.s.p.heat, t = G.s.t;
    enum { N = 8 };
    g_mode(G_MAX);
    for (int j = 0; j < 7; j++) {
        f32 xy[N * 2], len = (16.0f + 20.0f * wv) * (0.75f + 0.5f * hash01((uint32_t)(j * 3 + right * 40)));
        for (int k = 0; k < N; k++) {
            f32 u = (f32)k / (f32)(N - 1);
            f32 wave = sin_turns(u * 1.3f - t * (0.5f + 1.6f * wv) + (f32)j * 0.23f + (f32)right * 0.4f) * (2.0f + 4.0f * wv) * u;
            f32 x = root[j][0] - len * u * (0.55f + 0.45f * wv);
            f32 y = root[j][1] + len * u * u * (0.75f - 0.75f * wv) - len * 0.25f * u * (1.0f - (f32)j / 6.0f) + wave;
            xy[2 * k] = right ? 240.0f - x : x; xy[2 * k + 1] = y;
        }
        twin_xf(right, xy, N);
        g_ink(j & 1 ? K_ROSE : (right ? K_SKY : K_SUN));
        g_poly(xy, N, 0, 1.4f, 0.80f);
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
            f32 m[2] = {G.speaker ? 131.0f : 109.0f, 141.0f};
            twin_xf(G.speaker, m, 1);
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

static void twin_move(int g, f32 *xy, int n, f32 *gain, void *ctx)
{
    (void)ctx;
    int right = g >= GEMINI_G_NECKR;
    f32 t = G.s.t, sg = right ? -1.0f : 1.0f;
    switch (g) {
    case GEMINI_G_JAWL: case GEMINI_G_JAWR:
        if (right == G.speaker) sc_rot(xy, n, right ? 160.0f : 80.0f, 146.0f, sg * 0.040f * G.open);
        break;
    case GEMINI_G_EYEL: case GEMINI_G_EYER:
        sc_squash(xy, n, 112.5f, 1.0f - 0.9f * (right ? G.blink2 : G.s.blink));
        *gain = 0.75f + 0.35f * G.env[right];
        break;
    case GEMINI_G_PUPILL: case GEMINI_G_PUPILR: {
        f32 look = noise1(t * 0.45f + (f32)right * 7.0f, 51u);
        sc_shift(xy, n, sg * (look > 0.7f ? -3.0f : look < 0.2f ? 1.2f : 0.0f), look > 0.85f ? -1.0f : 0.0f);
        sc_squash(xy, n, 112.5f, 1.0f - 0.9f * (right ? G.blink2 : G.s.blink));
        break; }
    case GEMINI_G_HEADL: case GEMINI_G_HEADR: *gain = 0.85f + 0.30f * G.env[right]; break;
    }
    twin_xf(right, xy, n);
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
    if (G.next_flip < 0.0f) { G.next_flip = (14.0f + sc_rnd(&G.s) * 14.0f) * (1.3f - 0.8f * wv); gemini_strike(); }
    if (G.flip >= 0.0f) {
        f32 was = G.flip;
        G.flip += dt / 0.55f;
        if (was < 0.5f && G.flip >= 0.5f) G.swapped ^= 1;
        if (G.flip >= 1.0f) G.flip = -1.0f;
    }

    sc_begin(&G.s);
    wind();
    sc_frame_back(1.0f + 0.008f * sin_turns(t * 0.2f), K_SKY, 0.14f + 0.10f * wv);
    sc_frame_line(1.0f + 0.008f * sin_turns(t * 0.2f), 0.06f);
    sc_mist(&G.s, K_SKY);
    for (int r = 0; r < 2; r++) {
        f32 p[2] = {r ? 154.0f : 86.0f, 66.0f};
        twin_xf(r, p, 1);
        star(p[0], p[1] + 4.0f, 4.5f + 2.5f * G.env[r] + 0.8f * sin_turns(t * 0.7f + (f32)r * 0.5f), 0.45f + 0.5f * G.env[r]);
        hair(r);
    }
    sign_draw(&gemini_art, twin_move, 0);
    words(dt);
}

void gemini_palette(uint16_t pal[256])
{
    uint32_t inks[G_INKS] = {0xF2EAD2u, 0xCC4E7Cu, 0xF4D048u, 0x4AA8ECu};
    if (G.swapped) { uint32_t x = inks[K_SUN]; inks[K_SUN] = inks[K_SKY]; inks[K_SKY] = x; }
    f32 fl = G.flip >= 0.0f ? 1.0f - absf(cos_turns(G.flip * 0.5f)) : 0.0f;
    sc_palette(&G.s, inks, 0.25f * fl, pal);
}
