/* The music on the computer: zwav <sign> <seconds> <file.wav> [heat echo mist light [a key every so many seconds [seed]]] writes a piece
 * as a sound file, and says what the slow layer did. Seed 0, or none: the same every time. */
#include <stdio.h>
#include <stdlib.h>
#include "../../core/music.h"

static void w32(FILE *f, uint32_t v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); fputc((v >> 16) & 255, f); fputc(v >> 24, f); }
int main(int argc, char **argv)
{
    if (argc < 4) return 1;
    const ZMusic *m = z_music(atoi(argv[1])); if (!m) return 2;
    uint32_t sr = 44100, n = (uint32_t)(atof(argv[2]) * sr);
    ZParams p = {argc > 4 ? (f32)atof(argv[4]) : 0.45f, argc > 5 ? (f32)atof(argv[5]) : 0.0f, argc > 6 ? (f32)atof(argv[6]) : 0.15f, argc > 7 ? (f32)atof(argv[7]) : 0.7f};
    FILE *f = fopen(argv[3], "wb"); if (!f) return 3;
    fwrite("RIFF", 1, 4, f); w32(f, 36 + n * 4); fwrite("WAVEfmt ", 1, 8, f); w32(f, 16); w32(f, 0x00020001u); w32(f, sr); w32(f, sr * 4); w32(f, 0x00100004u);
    fwrite("data", 1, 4, f); w32(f, n * 4);
    z_music_seed(argc > 9 ? (uint32_t)strtoul(argv[9], 0, 0) : 0u);
    z_music_start(m, (f32)sr);
    int key = 0; double out[4] = {0, 0, 0, 0}, thin = 0, full = 0;
    static f32 buf[512]; int ev = 0; double every = argc > 8 ? atof(argv[8]) : 0, next = every;
    for (uint32_t done = 0; done < n; done += 256) {
        if (every > 0 && (double)done / sr >= next) { m->strike(); next += every; }
        z_music_render(m, buf, 256, &p);
        if (m_slow.key != key) { key = m_slow.key; printf("key %+d at %.0f s\n", key, (double)done / sr); }
        for (int k = 0; k < 4; k++) if (m_slow.v[k] < 0.5f) out[k] += 256.0 / sr;
        if (m_slow.fill < -0.4f) thin += 256.0 / sr; if (m_slow.fill > 0.4f) full += 256.0 / sr;
        int e = z_music_event(); if (e) { ev += e; printf("strike at %.2f s\n", (double)done / sr); }
        for (int i = 0; i < 512; i++) { int v = (int)(buf[i] * 32767.0f); v = v > 32767 ? 32767 : v < -32768 ? -32768 : v; fputc(v & 255, f); fputc((v >> 8) & 255, f); }
    }
    fclose(f); printf("voices out for %.0f %.0f %.0f %.0f s; thin %.0f s, full %.0f s; %d strikes\n", out[0], out[1], out[2], out[3], thin, full, ev);
    return 0;
}
