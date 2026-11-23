/* SPDX-License-Identifier: MPL-2.0 */
/* Zodiac on the FM-1: the app's half of the contract with fm1-base (device/unity.c, host/plat_host.c).
 * Begun from fm1-base's template, Copyright (C) 2026 Charles Vestal. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "ZODIAC"
#define APP_SPLASH_RGB RGB(242, 120, 72)
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_CANVAS_PX (240u * 32u)       /* the platform's own canvas kept small: the picture has its own buffers */

/* the store: the app's settings, the panel calibration (the platform's) and Bluetooth's (unused), and their offsets */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_BLE, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xDE000u}

/* the contract: the platform calls these */
void app_init(void);                     /* boot: the saved settings (or defaults) */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);   /* audio interrupt: n stereo frames, 24-bit */
void app_panic(void);                    /* silence now (before an update rewrites the flash) */
void app_ui_init(void);                  /* the first picture, after the splash */
void app_frame(void);                    /* input and the next frame of the picture */
void app_input_only(void);               /* between frames: input only */
void app_say(const char *a, const char *b);   /* to a screen reader (nothing is written on the picture) */

/* the host simulator's checks (host/plat_host.c "expect"): sign, dirty, strikes, lp, hp, echo, verb, fx, playing */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
