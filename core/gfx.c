#include "gfx.h"
#include "mathx.h"

Z_BIG uint8_t g_fb[GW * GH];
static int ink_bits, mode;

void g_clear(void) { for (int i = 0; i < GW * GH; i++) g_fb[i] = 0; }

void g_fade(int mul256)
{
    for (int i = 0; i < GW * GH; i++) {
        uint8_t p = g_fb[i];
        int lv = ((p & 63) * mul256) >> 8;
        g_fb[i] = lv ? (uint8_t)((p & 0xC0) | lv) : 0;                       /* what fades out, and last frame's black, is ground again */
    }
}

void g_ink(int ink) { ink_bits = (ink & 3) << 6; }
void g_mode(int m) { mode = m; }

/* cover pixel (x, y) by c (0 .. 1) of ink at strength a (0 .. 1) */
static inline void put(int x, int y, f32 c, f32 a)
{
    if ((unsigned)x >= GW || (unsigned)y >= GH || c <= 0.0f) return;
    uint8_t *p = &g_fb[y * GW + x];
    if (c > 1.0f) c = 1.0f;
    if (mode == G_GROUND) { if (c >= 0.5f) *p = 0; return; }   /* back to the bare ground */
    if (mode == G_MAX) {
        int lv = (int)(c * a * 63.0f + 0.5f);
        if (lv > (*p & 63)) *p = (uint8_t)(ink_bits | lv);
    } else {
        f32 was = (f32)(*p & 63) * (1.0f - c), now = a * 63.0f * c;        /* the stronger part keeps its ink */
        int lv = (int)(was + now + 0.5f);
        if (lv) *p = (uint8_t)((now >= was ? ink_bits : (*p & 0xC0)) | lv);
        else if (c >= 0.5f) *p = 0x40;                                       /* black, which is not the ground (0) */
    }
}

void g_line(f32 x0, f32 y0, f32 x1, f32 y1, f32 w, f32 a)
{
    f32 h = w * 0.5f, pad = h + 1.0f;
    int xa = (int)(minf(x0, x1) - pad), xb = (int)(maxf(x0, x1) + pad) + 1;
    int ya = (int)(minf(y0, y1) - pad), yb = (int)(maxf(y0, y1) + pad) + 1;
    f32 dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy, il2 = l2 > 1e-6f ? 1.0f / l2 : 0.0f;
    if (xa < 0) xa = 0; if (ya < 0) ya = 0; if (xb > GW - 1) xb = GW - 1; if (yb > GH - 1) yb = GH - 1;
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            f32 px = (f32)x + 0.5f - x0, py = (f32)y + 0.5f - y0;
            f32 t = sat((px * dx + py * dy) * il2);
            f32 ex = px - t * dx, ey = py - t * dy, d2 = ex * ex + ey * ey;
            if (d2 < pad * pad) put(x, y, h + 0.5f - sqrtf_(d2), a);
        }
}

void g_poly(const f32 *xy, int n, int closed, f32 w, f32 a)
{
    for (int i = 0; i + 1 < n; i++) g_line(xy[2 * i], xy[2 * i + 1], xy[2 * i + 2], xy[2 * i + 3], w, a);
    if (closed && n > 2) g_line(xy[2 * n - 2], xy[2 * n - 1], xy[0], xy[1], w, a);
}

void g_ring(f32 cx, f32 cy, f32 r, f32 w, f32 a)
{
    f32 h = w * 0.5f, pad = r + h + 1.0f;
    int xa = (int)(cx - pad), xb = (int)(cx + pad) + 1, ya = (int)(cy - pad), yb = (int)(cy + pad) + 1;
    if (xa < 0) xa = 0; if (ya < 0) ya = 0; if (xb > GW - 1) xb = GW - 1; if (yb > GH - 1) yb = GH - 1;
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            f32 px = (f32)x + 0.5f - cx, py = (f32)y + 0.5f - cy, d = sqrtf_(px * px + py * py) - r;
            if (d < 0.0f) d = -d;
            put(x, y, h + 0.5f - d, a);
        }
}

void g_disc(f32 cx, f32 cy, f32 r, f32 a)
{
    f32 pad = r + 1.0f;
    int xa = (int)(cx - pad), xb = (int)(cx + pad) + 1, ya = (int)(cy - pad), yb = (int)(cy + pad) + 1;
    if (xa < 0) xa = 0; if (ya < 0) ya = 0; if (xb > GW - 1) xb = GW - 1; if (yb > GH - 1) yb = GH - 1;
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            f32 px = (f32)x + 0.5f - cx, py = (f32)y + 0.5f - cy;
            put(x, y, r + 0.5f - sqrtf_(px * px + py * py), a);
        }
}

void g_dot(f32 x, f32 y, f32 a) { put((int)x, (int)y, 1.0f, a); }

/* scanline fill, even-odd, four sub-rows a row and exact edges along the row */
#define FILL_MAX 96
void g_fill(const f32 *xy, int n, f32 a)
{
    static f32 cov[GW];
    f32 ymin = 1e9f, ymax = -1e9f, xs[FILL_MAX];
    if (n < 3 || n > FILL_MAX) return;
    for (int i = 0; i < n; i++) { ymin = minf(ymin, xy[2 * i + 1]); ymax = maxf(ymax, xy[2 * i + 1]); }
    int ya = (int)ymin, yb = (int)ymax + 1;
    if (ya < 0) ya = 0; if (yb > GH - 1) yb = GH - 1;
    for (int y = ya; y <= yb; y++) {
        int lo = GW, hi = -1;
        for (int s = 0; s < 4; s++) {
            f32 sy = (f32)y + ((f32)s + 0.5f) * 0.25f;
            int k = 0;
            for (int i = 0; i < n; i++) {
                int j = i + 1 == n ? 0 : i + 1;
                f32 ay = xy[2 * i + 1], by = xy[2 * j + 1];
                if ((ay <= sy) != (by <= sy))
                    xs[k++] = xy[2 * i] + (sy - ay) / (by - ay) * (xy[2 * j] - xy[2 * i]);
            }
            for (int i = 1; i < k; i++) {                 /* few crossings: insertion sort */
                f32 v = xs[i]; int j = i - 1;
                while (j >= 0 && xs[j] > v) { xs[j + 1] = xs[j]; j--; }
                xs[j + 1] = v;
            }
            for (int i = 0; i + 1 < k; i += 2) {
                f32 l = maxf(xs[i], 0.0f), r = minf(xs[i + 1], (f32)GW);
                if (r <= l) continue;
                int il = (int)l, ir = (int)r;
                if (ir > GW - 1) ir = GW - 1;
                if (il < lo) { for (int x = il; x < lo && x < GW; x++) cov[x] = 0.0f; lo = il; }
                if (ir > hi) { for (int x = hi < lo ? lo : hi + 1; x <= ir; x++) cov[x] = 0.0f; hi = ir; }
                if (il == ir) cov[il] += (r - l) * 0.25f;
                else {
                    cov[il] += ((f32)(il + 1) - l) * 0.25f;
                    for (int x = il + 1; x < ir; x++) cov[x] += 0.25f;
                    cov[ir] += (r - (f32)ir) * 0.25f;
                }
            }
        }
        for (int x = lo; x <= hi; x++) put(x, y, cov[x], a);
    }
}

static const uint8_t bayer[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

void g_glow(f32 cx, f32 cy, f32 r, f32 a)
{
    int xa = (int)(cx - r), xb = (int)(cx + r) + 1, ya = (int)(cy - r), yb = (int)(cy + r) + 1;
    f32 ir2 = 1.0f / (r * r);
    if (xa < 0) xa = 0; if (ya < 0) ya = 0; if (xb > GW - 1) xb = GW - 1; if (yb > GH - 1) yb = GH - 1;
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            f32 px = (f32)x + 0.5f - cx, py = (f32)y + 0.5f - cy, u = 1.0f - (px * px + py * py) * ir2;
            if (u <= 0.0f) continue;
            int lv = (int)(u * u * a * 63.0f + (f32)bayer[(y & 3) * 4 + (x & 3)] * (1.0f / 16.0f));
            uint8_t *p = &g_fb[y * GW + x];
            if (lv > (*p & 63)) *p = (uint8_t)(ink_bits | (lv > 63 ? 63 : lv));
        }
}

void g_palette(const uint32_t rgb[G_INKS], f32 light, uint16_t out[256]) { g_palette_on(rgb, light, 0u, out); }

/* the same over a coloured ground: the empty sheet is that colour and every ink rises out of it; painted black stays black */
void g_palette_on(const uint32_t rgb[G_INKS], f32 light, uint32_t ground, uint16_t out[256])
{
    f32 gr = (f32)((ground >> 16) & 255), gg = (f32)((ground >> 8) & 255), gb = (f32)(ground & 255);
    for (int k = 0; k < G_INKS; k++) {
        f32 r = (f32)((rgb[k] >> 16) & 255), g = (f32)((rgb[k] >> 8) & 255), b = (f32)(rgb[k] & 255);
        for (int l = 0; l < G_LEVELS; l++) {
            f32 v = (f32)l * (1.0f / 63.0f), u = v * light, w = (l || !k) ? (1.0f - v) * (1.0f - v) : 0.0f;
            int R = (int)(r * u + gr * w), G = (int)(g * u + gg * w), B = (int)(b * u + gb * w);
            if (R > 255) R = 255; if (G > 255) G = 255; if (B > 255) B = 255;
            out[k * 64 + l] = (uint16_t)(((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3));
        }
    }
}

void g_blit(const uint8_t *pic, f32 scale, f32 y0, f32 amp, f32 freq, f32 phase, f32 gain)
{
    f32 inv = 1.0f / scale;
    int g = (int)(gain * 256.0f);
    for (int y = 0; y < GH; y++) {
        f32 sy = 120.0f + ((f32)y + 0.5f - 120.0f) * inv;
        if (sy < 0.0f || sy >= (f32)GH) continue;
        f32 d = (sy - y0) * (1.0f / 24.0f), sh = d <= 0.0f ? 0.0f : (d > 1.0f ? 1.0f : d) * amp * sin_turns(sy * freq + phase);
        const uint8_t *row = pic + (int)sy * GW;
        uint8_t *p = &g_fb[y * GW];
        for (int x = 0; x < GW; x++, p++) {
            f32 sx = 120.0f + ((f32)x + 0.5f - 120.0f) * inv + sh;
            if (sx < 0.0f || sx >= (f32)GW) continue;
            uint8_t v = row[(int)sx];
            int lv = ((v & 63) * g) >> 8;
            if (lv > 63) lv = 63;
            if (lv > (*p & 63)) *p = (uint8_t)((v & 0xC0) | lv);
        }
    }
}

Z_BIG uint8_t g_work[GW * GH];
Z_BIG uint8_t g_spare[GW * GH];
#ifdef Z_PACKED
Z_BIG uint8_t z_pic[GW * GH];
void z_unpack(const uint8_t *lz, int n)
{
    uint8_t *o = z_pic, *end = z_pic + GW * GH;
    const uint8_t *p = lz, *pe = lz + n;
    while (p < pe) {
        int tok = *p++, len = tok >> 4;
        if (len == 15) { int b; do { b = *p++; len += b; } while (b == 255 && p < pe); }
        if (len > end - o) len = (int)(end - o);                  /* a bad block cannot write past the picture */
        while (len-- > 0) *o++ = *p++;
        if (p >= pe) break;                                       /* the last run has no match after it */
        int off = p[0] | p[1] << 8; p += 2;
        len = tok & 15;
        if (len == 15) { int b; do { b = *p++; len += b; } while (b == 255 && p < pe); }
        len += 4;
        if (!off || off > o - z_pic) break;
        if (len > end - o) len = (int)(end - o);
        for (const uint8_t *m = o - off; len-- > 0;) *o++ = *m++;
    }
    while (o < end) *o++ = 0;
}
#endif
void p_begin(const uint8_t *pic) { for (int i = 0; i < GW * GH; i++) g_work[i] = pic[i]; }
void p_move(const uint8_t *pic, int x0, int y0, int w, int h, int dx, int dy)
{
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++) {
            int X = x + dx, Y = y + dy;
            if (X >= 0 && X < GW && Y >= 0 && Y < GH) g_work[Y * GW + X] = pic[y * GW + x];
        }
}
void p_fill(int x0, int y0, int w, int h, uint8_t v)
{
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++)
            if (x >= 0 && x < GW && y >= 0 && y < GH) g_work[y * GW + x] = v;
}
void p_warp(const uint8_t *pic, f32 cx, f32 cy, f32 rx, f32 ry, f32 zoom, f32 turns, f32 dx, f32 dy, int keep)
{
    f32 sn = sin_turns(turns);
    int xa = (int)(cx - rx), xb = (int)(cx + rx) + 1, ya = (int)(cy - ry), yb = (int)(cy + ry) + 1;
    if (xa < 0) xa = 0; if (ya < 0) ya = 0; if (xb > GW - 1) xb = GW - 1; if (yb > GH - 1) yb = GH - 1;
    for (int y = ya; y <= yb; y++)
        for (int x = xa; x <= xb; x++) {
            f32 px = (f32)x + 0.5f - cx, py = (f32)y + 0.5f - cy, u = px / rx, v = py / ry, w = (1.0f - u * u - v * v) * 2.5f;
            if (w <= 0.0f) continue;
            if (w > 1.0f) w = 1.0f;
            f32 s = sn * w, c = 1.0f - 0.5f * s * s, k = 1.0f / (1.0f + zoom * w);
            f32 sx = cx + (c * px + s * py) * k - dx * w, sy = cy + (c * py - s * px) * k - dy * w;
            if (sx < 0.0f || sx >= (f32)GW || sy < 0.0f || sy >= (f32)GH) continue;
            uint8_t from = pic[(int)sy * GW + (int)sx], was = pic[y * GW + x];
            if (keep && (((was && (keep >> (was >> 6)) & 1)) || (from && ((keep >> (from >> 6)) & 1)))) continue;
            g_work[y * GW + x] = from;
        }
}
