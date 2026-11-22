/* SPDX-License-Identifier: MPL-2.0 */
/* Zodiac on the FM-1: twelve signs, each a picture and a piece of music that plays itself (the core, ../core).
 * PRESET (or SELECT) steps through the signs; any key, and any MIDI note, strikes; K1 .. K4 are the sign's own
 * knob, echo, mist and light; a MIDI program change 0 .. 11 chooses a sign; SAVE keeps the sign and the knobs.
 * Nothing is written on the picture: a screen reader is told the names.
 * Part of the unity build, after the platform. Begun from fm1-base's template, Copyright (C) 2026 Charles Vestal.
 *
 * Two things run at once: the audio interrupt (app_render: the music) and the main loop (app_frame: the picture).
 * Each owns its half. They meet only in single numbers: `want` (the sign asked for; each side moves to it in its
 * own time) and counters that one side adds to and the other only reads. */
#include "zodiac.h"
#include "music.h"

#define SET_MAGIC 0x5A4F4431u            /* "ZOD1" */
#define P_MAX 200                        /* a knob's range in the settings; one detent is P_STEP */
#define P_STEP 4
typedef struct {
    uint32_t magic;
    uint8_t sign;                        /* 0 Aries .. 11 Pisces */
    uint8_t p[4];                        /* heat (the sign's own knob), echo, mist, light: 0 .. P_MAX */
    uint8_t rsv[3];
} settings_t;
static settings_t set;
static uint8_t set_dirty;

static volatile int32_t want;            /* the sign asked for: the encoder (main loop) or a program change (interrupt) */
static volatile uint32_t key_strikes;    /* keys pressed: the main loop adds, the interrupt plays them */
static volatile uint32_t pic_strikes;    /* strikes for the picture: the interrupt adds (the music's, MIDI's) */
static volatile uint8_t silent;          /* app_panic */
static volatile ZParams P = {0.45f, 0.0f, 0.15f, 0.7f};   /* the knobs as the core reads them */

/* ------------------------------------------------------------------ music --- */
static const ZMusic *Mu;                 /* the interrupt's: the piece playing */
static int32_t mu_sign = -1;
static uint32_t keys_done;
static float mbuf[2u * APP_BLOCK], gain_now;

static void midi_in(void)
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 0x7Fu, d2 = pkt >> 24;
        if (st == 0x90u && d2) {
            if (Mu)
                Mu->strike();
            else
                pic_strikes++;
        } else if (st == 0xC0u && d1 < (uint32_t)z_nsigns) {
            want = (int32_t)d1;
        }
    }
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i;
    int32_t w = want;
    float gain = (float)master_q12 / 4096.0f * 8300000.0f;
    ZParams p = P;
    midi_in();
    if (w != mu_sign) {                  /* another sign: its piece from the start, no two playings alike */
        mu_sign = w;
        Mu = z_music(w);
        z_led = Mu != 0;
        if (Mu) {
            z_music_seed(((plat_ms() ^ master_q12 << 20) * 2654435761u) | 1u);
            z_music_start(Mu, 44100.0f);
        }
        (void)z_music_event();
    }
    while (keys_done != key_strikes) {
        keys_done++;
        if (Mu)
            Mu->strike();
        else
            pic_strikes++;
    }
    if (n > APP_BLOCK)
        n = APP_BLOCK;
    if (Mu && !silent) {
        z_music_render(Mu, mbuf, (int)n, &p);
        pic_strikes += (uint32_t)z_music_event();    /* the music leads, the picture follows */
    } else {
        for (i = 0; i < 2u * n; i++)
            mbuf[i] = 0.0f;
    }
    for (i = 0; i < 2u * n; i++) {
        float s;
        if (!(i & 1u))
            gain_now += (gain - gain_now) * 0.004f;      /* turning MASTER does not click */
        s = mbuf[i] * gain_now;
        lr[i] = s > 8388607.0f ? 8388607 : s < -8388607.0f ? -8388607 : (int32_t)s;
    }
}

void app_panic(void) { silent = 1; }

/* --------------------------------------------------------------- settings --- */
static void params_set(void)
{
    ZParams p;
    p.heat = (float)set.p[0] / (float)P_MAX;
    p.echo = (float)set.p[1] / (float)P_MAX;
    p.mist = (float)set.p[2] / (float)P_MAX;
    p.light = (float)set.p[3] / (float)P_MAX;
    P = p;
}

static void settings_load(void)
{
    if (plat_store_load(OBJ_SETTINGS, &set, sizeof set) != (int)sizeof set || set.magic != SET_MAGIC
        || set.sign >= (uint32_t)z_nsigns || set.p[0] > P_MAX || set.p[1] > P_MAX || set.p[2] > P_MAX
        || set.p[3] > P_MAX) {
        set.magic = SET_MAGIC;
        set.sign = 0;
        set.p[0] = 90;                   /* 0.45, 0, 0.15, 0.7: where the pieces were written and heard */
        set.p[1] = 0;
        set.p[2] = 30;
        set.p[3] = 140;
    }
    params_set();
    want = set.sign;
}

void app_init(void) { settings_load(); }

/* --------------------------------------------------------------------- UI --- */
static const ZSign *Z;                   /* the main loop's: the picture showing */
static int32_t pic_sign = -1;
static uint32_t pic_done, last_ms, strikes_seen;
static uint32_t was_keys, was_btn;         /* (not held_keys / held_btn: the host simulator has those, and C would merge them) */
#define STRIP 24u                        /* the picture goes to the screen in strips of this many lines */
static uint16_t strip[240u * STRIP] PLAT_POOL;

static void text_cat(char *d, const char *s, uint32_t room)
{
    uint32_t n = 0;
    while (d[n])
        n++;
    while (*s && n + 1u < room) {
        char c = *s++;
        d[n++] = (char)(c >= 'a' && c <= 'z' ? c - 32 : c);
    }
    d[n] = 0;
}

void app_say(const char *a, const char *b)
{
    char t[48];
    if (!ann_on())
        return;
    t[0] = 0;
    text_cat(t, a ? a : "", sizeof t);
    if (b && b[0]) {
        text_cat(t, " ", sizeof t);
        text_cat(t, b, sizeof t);
    }
    ann_say(t, 0, ANN_NOW);
}

static void speak(void)                  /* a screen reader that has just started listening hears where it is */
{
    char t[48];
    if (ann_on() && ann_where() && Z) {
        t[0] = 0;
        text_cat(t, APP_NAME ". ", sizeof t);
        text_cat(t, Z->name, sizeof t);
        ann_say(t, 0, ANN_NOW);
    }
    ann_tick(plat_ms());
}

static void sign_show(void)              /* the picture moves to the sign asked for */
{
    int32_t w = want;
    if (w == pic_sign)
        return;
    pic_sign = w;
    Z = &z_signs[w];
    g_clear();
    Z->init();
    pic_done = pic_strikes;              /* strikes meant for the last picture are let go */
    if (set.sign != (uint8_t)w) {
        set.sign = (uint8_t)w;
        set_dirty = 1;
    }
    app_say(Z->name, "");
}

static void input(void)
{
    static const char *const KNOB[4] = {0, "ECHO", "MIST", "LIGHT"};
    uint32_t keys = plat_keys(), btn = plat_buttons(), k;
    int32_t e;
    if (keys & ~was_keys)               /* a key down: a strike (several at once are one) */
        key_strikes++;
    if ((btn & ~was_btn) >> B_SAVE & 1u) {
        if (plat_store_save(OBJ_SETTINGS, &set, sizeof set) == 0) {
            set_dirty = 0;
            app_say("SAVED", "");
        } else {
            app_say("SAVE", "FAILED");
        }
    }
    e = plat_enc(EN_PRESET) + plat_enc(EN_SELECT);
    if (e) {
        int32_t s = (want + e) % z_nsigns;
        want = s < 0 ? s + z_nsigns : s;
    }
    for (k = 0; k < 4u; k++) {
        int32_t v;
        if ((e = plat_enc((int)(EN_K1 + k))) == 0)
            continue;
        v = clamp((int32_t)set.p[k] + e * P_STEP, 0, P_MAX);
        if (v != set.p[k]) {
            set.p[k] = (uint8_t)v;
            set_dirty = 1;
            params_set();
        }
        app_say(k ? KNOB[k] : Z ? Z->knob : "", "");
    }
    was_keys = keys;
    was_btn = btn;
    plat_glow(1);
    plat_leds(set_dirty ? 1u << B_SAVE : 0u, keys);
}

void app_input_only(void) { input(); }

static void draw(void)                   /* g_fb through this frame's colours to the screen */
{
    static uint16_t pal[256];
    uint32_t y, i;
    Z->palette(pal);
    for (i = 0; i < 256u; i++)
        pal[i] = (uint16_t)(pal[i] >> 8 | pal[i] << 8);      /* the screen's byte order */
    for (y = 0; y < 240u; y += STRIP) {
        const uint8_t *src = g_fb + y * 240u;
        lcd_sync();                      /* the last strip may still be on its way */
        for (i = 0; i < 240u * STRIP; i++)
            strip[i] = pal[src[i]];
        lcd_blit(0, y, 240, STRIP, strip);
    }
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, 0);
    last_ms = plat_ms();
    sign_show();
}

void app_frame(void)
{
    uint32_t now = plat_ms(), s;
    float dt = (float)(now - last_ms) * 0.001f;
    ZParams p = P;
    last_ms = now;
    if (dt > 0.1f)
        dt = 0.1f;
    input();
    sign_show();
    speak();
    for (s = pic_strikes; pic_done != s; pic_done++) {
        Z->strike();
        strikes_seen++;
    }
    Z->frame(dt, &p);
    draw();
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "sign"))
        *got = pic_sign;
    else if (!strcmp(what, "playing"))   /* the piece the interrupt is on */
        *got = mu_sign;
    else if (!strcmp(what, "dirty"))
        *got = set_dirty;
    else if (!strcmp(what, "strikes"))   /* strikes the picture has had */
        *got = (int)strikes_seen;
    else if (!strcmp(what, "heat"))
        *got = set.p[0];
    else if (!strcmp(what, "echo"))
        *got = set.p[1];
    else if (!strcmp(what, "mist"))
        *got = set.p[2];
    else if (!strcmp(what, "light"))
        *got = set.p[3];
    else
        return 0;
    return 1;
}
#endif
