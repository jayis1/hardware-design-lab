/*
 * display.h — AeroCast ST7789V display API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_DISPLAY_H
#define AEROCAST_DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_flush(void);
void display_fill(uint16_t color);
void display_rect(int x, int y, int w, int h, uint16_t color);
void display_text(int x, int y, const char *s, uint16_t fg, uint16_t bg);
void display_boot_splash(const char *msg);
void display_status(uint32_t total_count, float flow, float t, float rh,
                     const uint32_t *prev_counts);
void display_alert(const char *msg);

#endif /* AEROCAST_DISPLAY_H */