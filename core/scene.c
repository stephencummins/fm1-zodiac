#include "scene.h"

void sc_init(Scene *s, uint32_t seed)
{
    s->t = 0.0f; s->seed = seed; s->blink = 0.0f; s->next_blink = 3.0f;
    s->p.heat = 0.5f; s->p.echo = 0.0f; s->p.mist = 0.2f; s->p.light = 0.7f;
}

f32 sc_rnd(Scene *s) { return hash01(s->seed++); }

f32 sc_follow(f32 v, f32 aim, f32 dt, f32 up, f32 down) { return v + (aim - v) * (1.0f - 1.0f / (1.0f + dt * (aim > v ? up : down))); }

void sc_step(Scene *s, f32 dt, const ZParams *want)
{
    f32 k = 1.0f - 1.0f / (1.0f + dt * 5.0f);
    s->p.heat += (sat(want->heat) - s->p.heat) * k;
    s->p.echo += (sat(want->echo) - s->p.echo) * k;
    s->p.mist += (sat(want->mist) - s->p.mist) * k;
    s->p.light += (sat(want->light) - s->p.light) * k;
    s->t += dt;
    s->next_blink -= dt;
    if (s->next_blink < 0.0f) s->next_blink = 3.0f + sc_rnd(s) * 6.0f;
    s->blink = sc_follow(s->blink, s->next_blink < 0.16f ? 1.0f : 0.0f, dt, 30.0f, 30.0f);
}

void sc_begin(const Scene *s)
{
    if (s->p.echo > 0.02f) g_fade((int)(256.0f * (0.62f + 0.36f * s->p.echo))); else g_clear();
}

void sc_mist(const Scene *s, int ink)
{
    f32 m = s->p.mist, t = s->t;
    if (m < 0.01f) return;
    g_mode(G_MAX); g_ink(ink);
    for (int i = 0; i < 6; i++) {
        f32 sp = 3.0f + 4.0f * hash01((uint32_t)i * 7u + 1u);
        f32 x = hash01((uint32_t)i * 13u) * 340.0f + t * sp;
        x = x - 340.0f * (f32)(int)(x / 340.0f) - 50.0f;
        f32 y = 70.0f + 150.0f * hash01((uint32_t)i * 29u + 3u) + sin_turns(t * 0.03f + (f32)i * 0.3f) * 12.0f;
        g_glow(x, y, 46.0f + 34.0f * hash01((uint32_t)i * 5u + 9u), m * (0.30f + 0.25f * noise1(t * 0.2f + (f32)i, 77u)));
    }
    int n = (int)(m * 260.0f);
    for (int i = 0; i < n; i++) {
        f32 x = hash01((uint32_t)i * 3u + 100u) * 240.0f + t * (1.5f + 3.0f * hash01((uint32_t)i + 500u));
        x -= 240.0f * (f32)(int)(x / 240.0f);
        f32 y = hash01((uint32_t)i * 3u + 101u) * 240.0f + sin_turns(t * 0.05f + (f32)i * 0.13f) * 4.0f;
        g_dot(x, y, m * (0.35f + 0.5f * noise1(t * 0.4f + (f32)i * 0.7f, 9u)));
    }
}

void sc_hex(f32 r, f32 xy[12])
{
    for (int k = 0; k < 6; k++) {
        f32 th = (f32)k / 6.0f + 0.25f;
        xy[2 * k] = CX + cos_turns(th) * r; xy[2 * k + 1] = CY + sin_turns(th) * r;
    }
}

void sc_frame_back(f32 beat, int ink, f32 glow)
{
    f32 hx[12];
    sc_hex(HEXR * beat, hx);
    (void)ink; g_mode(G_GROUND); g_fill(hx, 6, 1.0f);
    g_mode(G_MAX);
    if (glow > 0.0f) g_glow(CX, CY + 10.0f, 70.0f, glow);
}

void sc_frame_line(f32 beat, f32 rays)
{
    f32 hx[12], in[12];
    sc_hex(HEXR * beat, hx); sc_hex((HEXR - 6.0f) * beat, in);
    g_mode(G_MAX); g_ink(0);
    if (rays > 0.0f) for (int k = 0; k < 6; k++) g_line(CX, CY, in[2 * k], in[2 * k + 1], 0.8f, rays);
    g_poly(hx, 6, 1, 1.6f, 0.95f);
    g_poly(in, 6, 1, 0.8f, 0.45f);
}

void sc_palette(const Scene *s, const uint32_t inks[G_INKS], f32 boost, uint16_t pal[256])
{
    g_palette(inks, 0.30f + 0.85f * s->p.light + boost, pal);
}

void sc_rot(f32 *xy, int n, f32 cx, f32 cy, f32 turns)
{
    f32 c = cos_turns(turns), s = sin_turns(turns);
    for (int k = 0; k < n; k++) {
        f32 x = xy[2 * k] - cx, y = xy[2 * k + 1] - cy;
        xy[2 * k] = cx + x * c - y * s; xy[2 * k + 1] = cy + x * s + y * c;
    }
}
void sc_squash(f32 *xy, int n, f32 y0, f32 by) { for (int k = 0; k < n; k++) xy[2 * k + 1] = y0 + (xy[2 * k + 1] - y0) * by; }
void sc_squashx(f32 *xy, int n, f32 x0, f32 by) { for (int k = 0; k < n; k++) xy[2 * k] = x0 + (xy[2 * k] - x0) * by; }
void sc_shift(f32 *xy, int n, f32 dx, f32 dy) { for (int k = 0; k < n; k++) { xy[2 * k] += dx; xy[2 * k + 1] += dy; } }

/* the ground behind everything takes its colour from the four settings, so a turn of any knob shows at once.
 * tint: one colour for each of the sign's knob, Echo, Mist, Light; flash lifts it for a moment (a strike) */
void sc_palette_tint(const Scene *s, const uint32_t inks[G_INKS], f32 boost, const f32 tint[4][3], f32 flash, uint16_t pal[256])
{
    f32 l = s->p.light;
    f32 wt[4] = {s->p.heat * s->p.heat, s->p.echo, s->p.mist, l * l * 0.8f}, c[3];
    for (int k = 0; k < 3; k++) {
        f32 v = 0.0f;
        for (int j = 0; j < 4; j++) v += tint[j][k] * wt[j];
        c[k] = minf(170.0f, v * (0.58f + 0.45f * flash));
    }
    g_palette_on(inks, 0.30f + 0.85f * l + boost, ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | (uint32_t)c[2], pal);
}
