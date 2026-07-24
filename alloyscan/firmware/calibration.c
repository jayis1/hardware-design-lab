/*
 * calibration.c — Factory calibration routines
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Guides the user through a 4-step factory calibration process:
 * 1. Open circuit (probe in air) — records baseline coil impedance
 * 2. Reference block at contact — records maximum signal reference
 * 3. Reference block at 0.5 mm shim — records lift-off variation point 1
 * 4. Reference block at 1.0 mm shim — records lift-off variation point 2
 *
 * Steps 2-4 define the lift-off direction in the impedance plane,
 * allowing the classifier to project out the lift-off component.
 */

#include "calibration.h"
#include "classifier.h"
#include "ui.h"
#include "drivers/dac_drv.h"
#include "drivers/adc_drv.h"
#include "drivers/tof_drv.h"
#include "drivers/button.h"
#include "lockin.h"
#include "board.h"
#include <string.h>

const char *calibration_step_names[4] = {
    "Hold probe in air",
    "Press on ref block",
    "Add 0.5mm shim",
    "Add 1.0mm shim"
};

static const char *calibration_instructions[4] = {
    "Hold probe in air. No metal within 10cm. Press OK.",
    "Press probe firmly on 6061 Al reference block. Press OK.",
    "Place 0.5mm shim on ref block, press probe on it. Press OK.",
    "Place 1.0mm shim on ref block, press probe on it. Press OK."
};

/* Acquire a measurement and return the raw 8-dim feature vector */
static bool acquire_measurement(float feature[8], float *liftoff_mm)
{
    lockin_state_t lockin;
    static uint16_t adc_buffer[ADC_SAMPLE_COUNT];

    lockin_init(&lockin, ADC_SAMPLE_RATE_HZ);

    /* Start excitation */
    dac_drv_start();

    /* Small delay for signal to settle */
    for (volatile int i = 0; i < 10000; i++)
        ;

    /* Read lift-off */
    *liftoff_mm = (float)tof_drv_measure_mm(50) / 10.0f;  /* mm with 0.1mm res */

    /* Acquire ADC samples */
    if (!adc_drv_read_block(adc_buffer, ADC_SAMPLE_COUNT)) {
        dac_drv_stop();
        return false;
    }

    /* Wait for completion */
    if (!adc_drv_wait_block(100)) {
        dac_drv_stop();
        return false;
    }

    /* Stop excitation */
    dac_drv_stop();

    /* Convert to signed */
    int16_t signed_samples[ADC_SAMPLE_COUNT];
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++)
        signed_samples[i] = adc_drv_to_signed(adc_buffer[i], 3300);

    /* Process through lock-in amplifier */
    lockin_result_t lr = lockin_process(&lockin, signed_samples, ADC_SAMPLE_COUNT);

    if (!lr.valid)
        return false;

    /* Pack into 8-dim feature vector */
    for (int k = 0; k < FREQ_COUNT; k++) {
        feature[k * 2]     = lr.I[k];
        feature[k * 2 + 1] = lr.Q[k];
    }

    return true;
}

bool calibration_run_step(int step)
{
    if (step < 0 || step > 3)
        return false;

    /* Show instruction */
    ui_show_calibration(step, calibration_instructions[step]);

    /* Wait for OK button press */
    while (!button_pressed(BUTTON_OK)) {
        button_poll();
    }

    /* Wait for button release */
    while (button_held(BUTTON_OK)) {
        button_poll();
    }

    /* Acquire measurement */
    float feature[8];
    float liftoff_mm = 0.0f;

    ui_show_calibration(step, "Measuring...");

    if (!acquire_measurement(feature, &liftoff_mm)) {
        ui_show_error("Cal measurement failed");
        return false;
    }

    /* Feed to classifier calibration */
    if (!classifier_calibrate(step, feature, liftoff_mm)) {
        ui_show_error("Cal data invalid");
        return false;
    }

    /* Show success */
    ui_show_calibration(step, "OK! Step complete.");

    /* Brief delay */
    for (volatile int i = 0; i < 1000000; i++)
        ;

    return true;
}

bool calibration_run_full(void)
{
    ui_set_state(UI_STATE_CALIBRATE);

    for (int step = 0; step < 4; step++) {
        if (!calibration_run_step(step)) {
            ui_show_error("Calibration aborted");
            return false;
        }
    }

    /* Save calibration */
    if (!classifier_save_calibration()) {
        ui_show_error("Failed to save cal");
        return false;
    }

    ui_show_error("Calibration done!");
    for (volatile int i = 0; i < 2000000; i++)
        ;

    ui_set_state(UI_STATE_IDLE);
    return true;
}

int calibration_status(void)
{
    if (!classifier_is_calibrated())
        return 0;
    /* In production: check calibration drift by comparing
     * current open-circuit reading to stored baseline */
    return 1;
}

void calibration_reset(void)
{
    /* In production: erase calibration sector in flash */
    g_calibration.valid = false;
    g_calibration.magic = 0;
}