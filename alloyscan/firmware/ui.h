/*
 * ui.h — OLED UI state machine for AlloyScan
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_UI_H
#define ALLOYSCAN_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "classifier.h"

/* UI states */
typedef enum {
    UI_STATE_IDLE = 0,
    UI_STATE_SCANNING,
    UI_STATE_RESULT,
    UI_STATE_MENU,
    UI_STATE_CALIBRATE,
    UI_STATE_LOG,
    UI_STATE_LOW_BATTERY,
    UI_STATE_ERROR
} ui_state_t;

/* Initialize UI subsystem */
void ui_init(void);

/* Update the UI (call from main loop, ~10 Hz) */
void ui_update(void);

/* Show scan result on display */
void ui_show_result(const classify_result_t *result);

/* Show scanning animation */
void ui_show_scanning(int progress_percent);

/* Show idle screen */
void ui_show_idle(void);

/* Show low battery warning */
void ui_show_low_battery(uint8_t percent);

/* Show error message */
void ui_show_error(const char *msg);

/* Get current UI state */
ui_state_t ui_get_state(void);

/* Set UI state */
void ui_set_state(ui_state_t state);

/* Show calibration step */
void ui_show_calibration(int step, const char *msg);

/* Show menu */
void ui_show_menu(int selected_index);

/* Show scan log entry */
void ui_show_log_entry(int index, const char *alloy_name, uint32_t timestamp);

#endif /* ALLOYSCAN_UI_H */