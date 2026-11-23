/* SPDX-License-Identifier: MPL-2.0 */
/* Zodiac on the FM-1: twelve signs, each a picture and a piece of music that plays itself (the core, ../core).
 * PRESET steps through the signs; SELECT turns the wheel of their symbols, and the one it rests on opens; K1 .. K4 are a DJ's hands on the piece: low pass, high pass, echo and
 * reverb; the lowest white key, and any MIDI note, strikes (the sign answers); every other key is an effect while it
 * is held (the white keys Z_FX_ECHO .. Z_FX_FREEZE, the black keys Z_FX_CLOSE .. Z_FX_CUT, music.h); PLAY stops the
 * music and starts it where it stopped (OCT-); the buttons FX .. REC are the twelve signs; a MIDI program change 0 .. 11 chooses a sign; OCT+ keeps the sign and the knobs.
 * Nothing is written on the picture: a screen reader is told the names.
 * Part of the unity build, after the platform. Begun from fm1-base's template, Copyright (C) 2026 Charles Vestal.
 *
 * Two things run at once: the audio interrupt (app_render: the music) and the main loop (app_frame: the picture).
 * Each owns its half. They meet only in single numbers: `want` (the sign asked for; each side moves to it in its
 * own time) and counters that one side adds to and the other only reads. */
#include "zodiac.h"
#include "music.h"
#include "wheel.h"

#define SET_MAGIC 0x5A4F4432u            /* "ZOD2" (ZOD1: the knobs were heat, echo, mist, light) */
#define P_MAX 200                        /* a knob's range in the settings; one detent is P_STEP */
#define P_STEP 10                        /* twenty detents end to end: a turn is seen and heard */
typedef struct {
    uint32_t magic;
    uint8_t sign;                        /* 0 Aries .. 11 Pisces */
    uint8_t p[4];                        /* low pass, high pass, echo, reverb: 0 .. P_MAX, 0 the piece as it is */
    uint8_t rsv[3];
} settings_t;
static settings_t set;
static uint8_t set_dirty;

static volatile int32_t want;            /* the sign asked for: the encoder (main loop) or a program change (interrupt) */
static volatile uint32_t key_strikes;    /* keys pressed: the main loop adds, the interrupt plays them */
static volatile uint32_t pic_strikes;    /* strikes for the picture: the interrupt adds (the music's, MIDI's) */
static volatile uint8_t silent;          /* app_panic */
static volatile uint8_t stopped;         /* PLAY: the main loop sets it, the interrupt fades out and holds the piece still */
static const ZParams HOME = {0.45f, 0.0f, 0.15f, 0.7f};     /* where the pieces were written and heard: they stay there */
static volatile ZParams P = {0.45f, 0.0f, 0.15f, 0.7f};   /* what the picture is shown: it follows the knobs */
static volatile float dj[4];             /* the knobs as the core reads them (z_dj) */
static volatile float dip;               /* the wheel is showing: the piece sinks under the low pass (the main loop writes) */
static volatile uint8_t rise_ask;        /* the wheel has opened a sign: its piece comes up, as at switch-on */
static volatile uint32_t fx_held;        /* the effects held now, a bit each (Z_FX_*): the main loop writes */

/* ------------------------------------------------------------------ music --- */
static const ZMusic *Mu;                 /* the interrupt's: the piece playing */
static int32_t mu_sign = -1;
static uint32_t keys_done;
static float mbuf[2u * APP_BLOCK], gain_now;
static float rise;                       /* 0 at switch-on, 1 some seconds later: the piece comes up, it does not burst in */
#define RISE_SECS 5.0f

static void midi_in(void)
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 0x7Fu, d2 = pkt >> 24;
        if (st == 0x90u && d2) {
            if (Mu && !stopped)
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
    ZParams p = HOME;
    midi_in();
    z_dj(dj[0] > dip ? dj[0] : dip, dj[1], dj[2], dj[3], fx_held);
    if (w != mu_sign) {
        if (rise_ask && rise > 0.3f)
            rise = 0.3f;
        rise_ask = 0;                  /* another sign: its piece from the start, no two playings alike */
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
        if (Mu && !stopped)
            Mu->strike();
        else
            pic_strikes++;
    }
    if (n > APP_BLOCK)
        n = APP_BLOCK;
    if (stopped)
        gain = 0.0f;
    if (Mu && !silent && !(stopped && gain_now < 64.0f)) {       /* stopped and faded out: the piece waits where it is */
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
        if (!(i & 1u) && rise < 1.0f) {
            rise += 1.0f / (RISE_SECS * 44100.0f);
            if (rise > 1.0f)
                rise = 1.0f;
        }
        s = mbuf[i] * gain_now * rise * rise;
        lr[i] = s > 8388607.0f ? 8388607 : s < -8388607.0f ? -8388607 : (int32_t)s;
    }
}

void app_panic(void) { silent = 1; }

/* --------------------------------------------------------------- settings --- */
static void params_set(void)
{
    ZParams p = HOME;
    float v[4];
    int k;
    for (k = 0; k < 4; k++)
        dj[k] = v[k] = (float)set.p[k] / (float)P_MAX;
    p.light = 0.7f * (1.0f - v[0]) + 0.3f * v[1];     /* the picture: darker as the low pass closes, brighter with the high pass */
    p.echo = v[2];
    p.mist = 0.15f + 0.85f * v[3];
    P = p;
}

static void settings_load(void)
{
    if (plat_store_load(OBJ_SETTINGS, &set, sizeof set) != (int)sizeof set || set.magic != SET_MAGIC
        || set.sign >= (uint32_t)z_nsigns || set.p[0] > P_MAX || set.p[1] > P_MAX || set.p[2] > P_MAX
        || set.p[3] > P_MAX) {
        set.magic = SET_MAGIC;
        set.sign = 0;
        set.p[0] = set.p[1] = set.p[2] = set.p[3] = 0;       /* the piece as it is */
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

/* the wheel (SELECT): 0 the picture; 1 the wheel, turning or resting; 2 the chosen symbol comes to the middle;
 * 3 the picture opens out from the middle */
static uint8_t wh_mode;
static int32_t wh_sel;                   /* the sign asked for, not wrapped: the wheel turns the way the hand did */
static int32_t wh_want;                  /* `want` when the wheel came up: if it moves, something else chose */
static float wh_at, wh_vel, wh_idle, wh_t, wh_clock;
#define WH_REST 0.7f                     /* seconds at rest before the sign opens */
#define WH_GATHER 0.45f
#define WH_OPEN 0.75f
#define WH_DIP 0.72f

static float own_wait;
static uint32_t own_rng;
static int32_t own_sign = -1;

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
    if (wh_mode != 3)                    /* the wheel has said it already */
        app_say(Z->name, "");
}

static void wheel_step(float dt)         /* the wheel moves on by dt, and draws */
{
    float left = dt;
    wh_clock += dt;
    while (left > 0.0f) {                /* a spring, in steps short enough to stay steady at any frame rate */
        float h = left > 0.008f ? 0.008f : left;
        wh_vel += (170.0f * ((float)wh_sel - wh_at) - 26.0f * wh_vel) * h;
        wh_at += wh_vel * h;
        left -= h;
    }
    if (wh_mode == 1) {
        float off = (float)wh_sel - wh_at;
        wh_idle += dt;
        if (wh_idle > WH_REST && off < 0.03f && off > -0.03f) {
            wh_mode = 2;
            wh_t = 0.0f;
        }
    } else {
        wh_t += dt * (1.0f / WH_GATHER);
    }
    wheel_frame(wh_at, wh_mode == 2 ? wh_t : 0.0f, wh_clock);
    if (wh_mode == 2 && wh_t >= 1.0f) {  /* the sign is chosen: its picture and its piece, from the start */
        int32_t s = wh_sel % z_nsigns;
        if (s < 0)
            s += z_nsigns;
        if (s != want)
            rise_ask = 1;
        want = s;
        pic_sign = -1;
        wh_mode = 3;
        wh_t = 0.0f;
        dip = 0.0f;
    }
}

static void input(void)
{
    static const char *const KNOB[4] = {"LOW PASS", "HIGH PASS", "ECHO", "REVERB"};
    /* key 0 is the lowest F: it strikes. The other white keys, upward, are effects 0 .. 14; the black keys 15 .. 25 */
    static const int8_t KEYFX[27] = {-1, 15, 0, 16, 1, 17, 2, 3, 18, 4, 19, 5, 6, 20, 7, 21, 8, 22, 9, 10, 23, 11, 24, 12, 13, 25, 14};
    uint32_t keys = plat_keys(), btn = plat_buttons(), k, fx = 0;
    int32_t e;
    if (keys & ~was_keys & 1u)          /* the lowest key down: a strike */
        key_strikes++;
    for (k = 1; k < 27u; k++) {
        if (!(keys >> k & 1u))
            continue;
        fx |= 1u << KEYFX[k];
        if (!(was_keys >> k & 1u))
            app_say(z_fx_name[KEYFX[k]], "");
    }
    fx_held = fx;
    if ((btn & ~was_btn) >> B_OCTUP & 1u) {          /* OCT+: save */
        if (plat_store_save(OBJ_SETTINGS, &set, sizeof set) == 0) {
            set_dirty = 0;
            app_say("SAVED", "");
        } else {
            app_say("SAVE", "FAILED");
        }
    }
    if ((btn & ~was_btn) >> B_OCTDN & 1u) {          /* OCT-: play, stop */
        stopped = !stopped;
        app_say(stopped ? "STOP" : "PLAY", "");
    }
    for (k = 0; k < 12u; k++) {          /* the twelve buttons FX .. REC: a sign each, Aries .. Pisces */
        if (!((btn & ~was_btn) >> k & 1u))
            continue;
        if ((int32_t)k != want || wh_mode == 1 || wh_mode == 2) {
            if ((int32_t)k != want)
                rise_ask = 1;
            want = wh_want = (int32_t)k;
            pic_sign = -1;               /* its picture opens out from the middle */
            wh_mode = 3;
            wh_t = 0.0f;
            dip = 0.0f;
        }
        app_say(z_signs[k].name, "");
    }
    if ((e = plat_enc(EN_PRESET)) != 0) {            /* PRESET: straight to the next sign */
        int32_t s = (want + e) % z_nsigns;
        want = s < 0 ? s + z_nsigns : s;
    }
    if ((e = plat_enc(EN_SELECT)) != 0) {            /* SELECT: the wheel */
        int32_t s;
        if (wh_mode == 0 || wh_mode == 3) {
            wh_sel = wh_want = want;
            wh_at = (float)wh_sel;
            wh_vel = 0.0f;
        }
        wh_mode = 1;
        wh_idle = 0.0f;
        wh_sel += e;
        dip = WH_DIP;
        s = wh_sel % z_nsigns;
        app_say(z_signs[s < 0 ? s + z_nsigns : s].name, "");
    }
    if ((wh_mode == 1 || wh_mode == 2) && want != wh_want) {     /* PRESET or a program change chose meanwhile: that wins */
        wh_mode = 0;
        pic_sign = -1;
        dip = 0.0f;
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
        app_say(KNOB[k], "");
    }
    was_keys = keys;
    was_btn = btn;
    plat_glow(1);
    plat_leds((set_dirty ? 1u << B_OCTUP : 0u) | (stopped ? 0u : 1u << B_OCTDN) | 1u << (want % 12), keys);    /* the sign's own button is lit */
}

void app_input_only(void) { input(); }

/* g_fb through this frame's colours to the screen. iris: under 0 all of it; or only a disc that many pixels across
 * its half about the middle, the rest black, a bright rim at its edge */
static void draw(void (*palette)(uint16_t pal[256]), float iris)
{
    static uint16_t pal[256];
    uint32_t y, i;
    palette(pal);
    for (i = 0; i < 256u; i++)
        pal[i] = (uint16_t)(pal[i] >> 8 | pal[i] << 8);      /* the screen's byte order */
    for (y = 0; y < 240u; y += STRIP) {
        const uint8_t *src = g_fb + y * 240u;
        lcd_sync();                      /* the last strip may still be on its way */
        for (i = 0; i < 240u * STRIP; i++)
            strip[i] = pal[src[i]];
        if (iris >= 0.0f) {
            uint32_t row;
            for (row = 0; row < STRIP; row++) {
                float dy = (float)(y + row) - 119.5f, hw2 = iris * iris - dy * dy, hw = 0.0f;
                uint16_t *q = strip + row * 240u;
                int32_t a = 120, b = 120, x;         /* the disc's span on this row: a .. b - 1 */
                if (hw2 > 0.0f) {
                    hw = hw2 * 0.5f + 0.5f;
                    for (x = 0; x < 6; x++)          /* a square root, near enough */
                        hw = 0.5f * (hw + hw2 / hw);
                    a = 120 - (int32_t)hw;
                    b = 120 + (int32_t)hw;
                    if (a < 0)
                        a = 0;
                    if (b > 240)
                        b = 240;
                }
                for (x = 0; x < a; x++)
                    q[x] = 0;
                for (x = b; x < 240; x++)
                    q[x] = 0;
                if (hw2 > 0.0f && iris < 168.0f) {
                    for (x = a; x < a + 2 && x < b; x++)
                        q[x] = 0xFFFFu;
                    for (x = b - 2 > a ? b - 2 : a; x < b; x++)
                        q[x] = 0xFFFFu;
                }
            }
        }
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
    if (wh_mode == 1 || wh_mode == 2) {  /* the wheel has the screen; the piece plays on under it */
        wheel_step(dt);
        speak();
        draw(wheel_palette, -1.0f);
        return;
    }
    sign_show();
    speak();
    for (s = pic_strikes; pic_done != s; pic_done++) {
        Z->strike();
        strikes_seen++;
    }
    own_wait -= dt;                      /* and now and then the sign stirs by itself: the picture only, no sound */
    if (own_wait < 0.0f) {
        own_rng = own_rng * 1664525u + 1013904223u + now;
        own_wait = 9.0f + 16.0f * (float)(own_rng >> 8) * (1.0f / 16777216.0f);
        if (pic_sign == own_sign)
            Z->strike();
        own_sign = pic_sign;             /* a sign just arrived waits its first turn out */
    }
    Z->frame(dt, &p);
    if (wh_mode == 3) {                  /* the picture opens out from the middle */
        float u;
        wh_t += dt * (1.0f / WH_OPEN);
        u = wh_t > 1.0f ? 1.0f : wh_t;
        draw(Z->palette, 6.0f + 166.0f * u * u);
        if (wh_t >= 1.0f)
            wh_mode = 0;
    } else {
        draw(Z->palette, -1.0f);
    }
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "sign"))
        *got = pic_sign;
    else if (!strcmp(what, "playing"))   /* the piece the interrupt is on */
        *got = mu_sign;
    else if (!strcmp(what, "stopped"))
        *got = stopped;
    else if (!strcmp(what, "dirty"))
        *got = set_dirty;
    else if (!strcmp(what, "strikes"))   /* strikes the picture has had */
        *got = (int)strikes_seen;
    else if (!strcmp(what, "lp"))
        *got = set.p[0];
    else if (!strcmp(what, "hp"))
        *got = set.p[1];
    else if (!strcmp(what, "echo"))
        *got = set.p[2];
    else if (!strcmp(what, "verb"))
        *got = set.p[3];
    else if (!strcmp(what, "wheel"))     /* 0 the picture, 1 the wheel, 2 the symbol gathers, 3 the picture opens */
        *got = wh_mode;
    else if (!strcmp(what, "fx"))        /* the effects held, a bit each */
        *got = (int)fx_held;
    else
        return 0;
    return 1;
}
#endif
