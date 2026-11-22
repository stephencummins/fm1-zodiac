/* The core on a computer: runs the picture for a while and writes frames as PPM files.
 *   zhost OUT_DIR                     a set of stills at different settings */
#include <stdio.h>
#include <stdlib.h>
#include "../../core/zodiac.h"

static void shot(const char *dir, const char *name)
{
    static uint16_t pal[256];
    char path[512];
    aries_palette(pal);
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", GW, GH);
    for (int i = 0; i < GW * GH; i++) {
        uint16_t c = pal[g_fb[i]];
        unsigned char rgb[3] = {(unsigned char)(((c >> 11) & 31) * 255 / 31), (unsigned char)(((c >> 5) & 63) * 255 / 63), (unsigned char)((c & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

static void run(f32 secs, const ZParams *p) { for (int i = 0; i < (int)(secs * 40.0f); i++) aries_frame(1.0f / 40.0f, p); }

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    static const struct { const char *name; ZParams p; f32 secs; } set[] = {
        {"1-rest",      {0.45f, 0.0f, 0.15f, 0.70f}, 6.0f},
        {"1b-roar",     {0.45f, 0.0f, 0.15f, 0.70f}, -0.45f},
        {"2-heat-low",  {0.05f, 0.0f, 0.15f, 0.70f}, 4.0f},
        {"3-heat-full", {1.00f, 0.0f, 0.15f, 0.80f}, 3.0f},
        {"4-echo",      {0.80f, 0.9f, 0.10f, 0.70f}, 4.0f},
        {"5-mist",      {0.35f, 0.0f, 1.00f, 0.60f}, 5.0f},
        {"6-dark",      {0.30f, 0.0f, 0.30f, 0.05f}, 4.0f},
    };
    aries_init();
    for (unsigned i = 0; i < sizeof set / sizeof set[0]; i++) { if (set[i].secs < 0) aries_strike(); run(set[i].secs < 0 ? -set[i].secs : set[i].secs, &set[i].p); shot(dir, set[i].name); }
    return 0;
}
