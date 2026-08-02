/*
 * drivers/display.h — SSD1306 OLED driver + result rendering
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_DISPLAY_H
#define HYDRASCAN_DISPLAY_H
#include "../board.h"

hydra_err_t display_init(void);
void        display_clear(void);
void        display_text(uint8_t row, uint8_t col, const char *str);
/* Render the measurement result (identity + confidence + adulteration). */
void        display_result(const char *name, float confidence,
                           uint8_t adulterant, float ratio, float temp_c);
#endif