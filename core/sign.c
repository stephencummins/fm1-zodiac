#include "sign.h"
#include "mathx.h"

#define SIGN_PATH_MAX 128

void sign_draw(const Sign *s, SignMove move, void *ctx)
{
    static f32 xy[SIGN_PATH_MAX * 2];
    for (int i = 0; i < s->npaths; i++) {
        const SignPath *p = &s->paths[i];
        int n = p->n > SIGN_PATH_MAX ? SIGN_PATH_MAX : p->n;
        const int16_t *q = &s->pts[p->first * 2];
        f32 gain = 1.0f;
        for (int k = 0; k < n * 2; k++) xy[k] = (f32)q[k] * (1.0f / 16.0f);
        if (move) move(p->group, xy, n, &gain, ctx);
        f32 a = sat((f32)p->level * (1.0f / 63.0f) * gain);
        g_ink(p->ink);
        if (p->kind == 1) { g_mode(G_PAINT); g_fill(xy, n, a); }
        else if (p->kind == 2) { g_mode(G_PAINT); g_poly(xy, n, p->closed, (f32)p->width * (1.0f / 16.0f), 0.0f); }
        else { g_mode(G_MAX); g_poly(xy, n, p->closed, (f32)p->width * (1.0f / 16.0f), a); }
    }
    g_mode(G_MAX);
}
