/*
 * ui.c — OLED UI state machine implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the user interface for AlloyScan using the SH1106 OLED.
 * The UI is a simple state machine that renders different screens based
 * on the current state. All drawing is done to the OLED framebuffer.
 */

#include "ui.h"
#include "drivers/oled_drv.h"
#include "alloy_db.h"
#include "board.h"
#include <string.h>
#include <stdio.h>

static ui_state_t current_state = UI_STATE_IDLE;
static int scan_progress = 0;
static char last_error[64] = "";
static uint32_t state_timer = 0;
static int menu_index = 0;
static const char *menu_items[] = {
    "1. Scan",
    "2. Browse DB",
    "3. Calibrate",
    "4. Scan Log",
    "5. Battery",
    "6. About"
};
#define MENU_ITEM_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

void ui_init(void)
{
    oled_drv_init();
    current_state = UI_STATE_IDLE;
    state_timer = 0;
}

ui_state_t ui_get_state(void)
{
    return current_state;
}

void ui_set_state(ui_state_t state)
{
    current_state = state;
    state_timer = 0;
}

void ui_show_idle(void)
{
    oled_clear();

    /* Title bar */
    oled_draw_string(0, 0, "AlloyScan v1.0", false);
    oled_draw_hline(0, 9, 128, true);

    /* Center prompt */
    oled_draw_string(16, 24, "Ready to scan", false);
    oled_draw_string(8, 36, "Press trigger", false);

    /* Battery indicator (top right) */
    oled_draw_rect(100, 0, 20, 8, true);
    oled_draw_vline(120, 2, 4, true);
    /* Battery fill would go here based on battery_percent() */

    /* Bottom hint */
    oled_draw_string(0, 56, "OK=Menu  Trig=Scan", false);

    oled_flush();
}

void ui_show_scanning(int progress_percent)
{
    oled_clear();

    oled_draw_string(0, 0, "SCANNING...", true);
    oled_draw_hline(0, 9, 128, true);

    /* Progress bar */
    oled_draw_string(0, 24, "Measuring...", false);
    oled_draw_progress(14, 36, 100, 12, progress_percent);

    /* Animated dots */
    int dots = (state_timer / 3) % 4;
    char dot_str[5] = {'.', '.', '.', '.', '\0'};
    for (int i = 0; i < dots; i++)
        dot_str[i] = '.';
    for (int i = dots; i < 4; i++)
        dot_str[i] = ' ';
    oled_draw_string(90, 24, dot_str, false);

    oled_flush();
    state_timer++;
}

void ui_show_result(const classify_result_t *result)
{
    oled_clear();

    /* Title */
    oled_draw_string(0, 0, "RESULT", true);
    oled_draw_hline(0, 9, 128, true);

    if (result->alloy_index < 0 || result->uncertain) {
        /* Uncertain result */
        oled_draw_string(0, 16, "UNCERTAIN", true);
        oled_draw_string(0, 28, "Reposition", false);
        oled_draw_string(0, 38, "and retry", false);

        if (result->edge_warning) {
            oled_draw_string(0, 50, "! EDGE DETECTED", true);
        }
    } else {
        const alloy_entry_t *alloy = &alloy_database[result->alloy_index];

        /* Alloy name (large, centered) */
        oled_draw_string(0, 16, alloy->name, false);

        /* Family name */
        oled_draw_string(0, 28, alloy_family_name(alloy->family), false);

        /* Confidence */
        int conf_pct = (int)(result->confidence * 100.0f);
        char conf_str[20];
        /* Build "Conf: NN%" manually since we avoid snprintf */
        oled_draw_string(0, 40, "Conf:", false);

        /* Simple integer to string for confidence */
        char num[8];
        int pos = 0;
        int v = conf_pct;
        if (v == 0) {
            num[pos++] = '0';
        } else {
            char tmp[8];
            int t = 0;
            while (v > 0) { tmp[t++] = '0' + (v % 10); v /= 10; }
            while (t > 0) num[pos++] = tmp[--t];
        }
        num[pos] = '\0';
        oled_draw_string(36, 40, num, false);
        oled_draw_char(36 + pos * 6, 40, '%', false);

        /* Confidence bar */
        oled_draw_progress(0, 50, 128, 8, conf_pct);

        /* Alternatives (if space) */
        if (result->alt_count > 0 && result->alternatives[0] >= 0) {
            const alloy_entry_t *alt = &alloy_database[result->alternatives[0]];
            oled_draw_string(0, 58, "Alt:", false);
            oled_draw_string(24, 58, alt->name, false);
        }
    }

    /* LED indicators (GPIO C pins 1-3) */
    if (result->confidence >= CONFIDENCE_HIGH) {
        GPIOC->BSRR = (1UL << LED_GREEN_PIN);   /* Green on */
        GPIOC->BRR  = (1UL << LED_YELLOW_PIN);
        GPIOC->BRR  = (1UL << LED_RED_PIN);
    } else if (result->confidence >= CONFIDENCE_MEDIUM) {
        GPIOC->BRR  = (1UL << LED_GREEN_PIN);
        GPIOC->BSRR = (1UL << LED_YELLOW_PIN);  /* Yellow on */
        GPIOC->BRR  = (1UL << LED_RED_PIN);
    } else {
        GPIOC->BRR  = (1UL << LED_GREEN_PIN);
        GPIOC->BRR  = (1UL << LED_YELLOW_PIN);
        GPIOC->BSRR = (1UL << LED_RED_PIN);     /* Red on */
    }

    oled_flush();
}

void ui_show_low_battery(uint8_t percent)
{
    oled_clear();
    oled_draw_string(0, 0, "LOW BATTERY!", true);
    oled_draw_hline(0, 9, 128, true);

    oled_draw_string(0, 24, "Charge soon!", false);

    /* Battery icon */
    oled_draw_rect(40, 36, 48, 20, true);
    oled_draw_vline(88, 40, 12, true);
    oled_fill_rect(42, 38, (44 * percent) / 100, 16, true);

    oled_flush();
}

void ui_show_error(const char *msg)
{
    oled_clear();
    oled_draw_string(0, 0, "ERROR", true);
    oled_draw_hline(0, 9, 128, true);
    oled_draw_string(0, 20, msg, false);
    oled_draw_string(0, 56, "Press OK to cont.", false);
    oled_flush();
}

void ui_show_calibration(int step, const char *msg)
{
    oled_clear();
    oled_draw_string(0, 0, "CALIBRATE", true);
    oled_draw_hline(0, 9, 128, true);

    /* Step indicator */
    char step_str[24] = "Step ";
    step_str[5] = '1' + step;
    step_str[6] = ' ';
    step_str[7] = '/';
    step_str[8] = '4';
    step_str[9] = '\0';
    oled_draw_string(0, 16, step_str, false);

    /* Progress dots */
    for (int i = 0; i < 4; i++) {
        if (i <= step)
            oled_fill_rect(8 + i * 12, 28, 8, 8, true);
        else
            oled_draw_rect(8 + i * 12, 28, 8, 8, true);
    }

    /* Instruction message */
    if (msg && msg[0])
        oled_draw_string(0, 44, msg, false);

    oled_draw_string(0, 56, "OK=confirm", false);
    oled_flush();
}

void ui_show_menu(int selected_index)
{
    oled_clear();
    oled_draw_string(0, 0, "MENU", true);
    oled_draw_hline(0, 9, 128, true);

    /* Show up to 7 items, scroll if needed */
    int start = 0;
    if (selected_index > 5)
        start = selected_index - 5;

    for (int i = 0; i < 7 && (start + i) < (int)MENU_ITEM_COUNT; i++) {
        int y = 12 + i * 8;
        bool sel = (start + i == selected_index);
        if (sel) {
            oled_fill_rect(0, y, 128, 8, true);
            oled_draw_string(2, y, menu_items[start + i], true);  /* Inverted */
        } else {
            oled_draw_string(2, y, menu_items[start + i], false);
        }
    }

    oled_flush();
}

void ui_show_log_entry(int index, const char *alloy_name, uint32_t timestamp)
{
    oled_clear();
    oled_draw_string(0, 0, "SCAN LOG", true);
    oled_draw_hline(0, 9, 128, true);

    /* Entry number */
    char num_str[16];
    /* Simple itoa */
    int pos = 0;
    int v = index;
    if (v == 0) { num_str[pos++] = '0'; }
    else { char tmp[8]; int t = 0;
        while (v > 0) { tmp[t++] = '0' + (v % 10); v /= 10; }
        while (t > 0) num_str[pos++] = tmp[--t];
    }
    num_str[pos] = '\0';
    oled_draw_string(0, 16, "#", false);
    oled_draw_string(6, 16, num_str, false);

    /* Alloy name */
    oled_draw_string(0, 28, "Alloy:", false);
    if (alloy_name)
        oled_draw_string(36, 28, alloy_name, false);

    oled_draw_string(0, 56, "Up/Down=browse OK=back", false);
    oled_flush();
}

void ui_update(void)
{
    switch (current_state) {
        case UI_STATE_IDLE:
            ui_show_idle();
            break;
        case UI_STATE_SCANNING:
            ui_show_scanning(scan_progress);
            break;
        case UI_STATE_RESULT:
            /* Result stays until button press; no auto-update needed */
            break;
        case UI_STATE_MENU:
            ui_show_menu(menu_index);
            break;
        case UI_STATE_CALIBRATE:
            /* Calibration screens are set explicitly */
            break;
        case UI_STATE_LOG:
            /* Log entries are set explicitly */
            break;
        case UI_STATE_LOW_BATTERY:
            /* Set explicitly with battery percent */
            break;
        case UI_STATE_ERROR:
            if (last_error[0])
                ui_show_error(last_error);
            break;
    }
    state_timer++;
}