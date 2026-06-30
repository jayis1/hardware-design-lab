/*
 * drivers/imx519.h — Sony IMX519 camera driver interface
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef IMX519_H
#define IMX519_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IMX519_WIDTH     2592
#define IMX519_HEIGHT    1944

int  imx519_init(void);
void imx519_reset(void);
int  imx519_capture_bayer(uint16_t *out_buf, uint32_t max_pixels);
int  imx519_set_exposure(uint32_t us);
void imx519_set_strobe(uint8_t enable);
void imx519_set_ir_led(uint8_t enable);
void imx519_power_down(void);
void imx519_power_up(void);

/* Downsample a full-resolution Bayer frame to a small RGB565 buffer.
 * Returns 0 on success. The output buffer must be at least
 * (out_w * out_h * 2) bytes. */
int imx519_downsample_rgb565(const uint16_t *bayer, uint32_t src_w, uint32_t src_h,
                              uint16_t *out, uint32_t out_w, uint32_t out_h);

#ifdef __cplusplus
}
#endif

#endif /* IMX519_H */