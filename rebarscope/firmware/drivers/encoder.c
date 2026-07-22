/*
 * drivers/encoder.c — Quadrature wheel encoder + IMU position fusion
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Uses TIM3 in encoder-mode 3 to decode the 200 PPR quadrature wheel encoder.
 * The IMU yaw is integrated on the companion gyro path; a complementary
 * filter fuses the two when the operator walks a serpentine path.
 */
#include "../board.h"
#include "../registers.h"
#include "encoder.h"
#include "imu.h"
#include <math.h>

/* Position state */
static float g_pos_x_mm = 0.0f;
static float g_pos_y_mm = 0.0f;
static float g_heading_deg = 0.0f;
static int32_t g_last_cnt = 0;
static float g_scale = 1.0f;   /* user calibration factor (e.g., 1.00) */

void encoder_init(void)
{
    /* TIM3 clock enable */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3;
    /* Configure PA6/PA7 as AF (TIM3_CH1/CH2) */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << 12)) | (0x2u << 12);  /* PA6 AF */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << 14)) | (0x2u << 14);  /* PA7 AF */
    GPIOA->AFRL  = (GPIOA->AFRL & ~(0xFu << 24)) | (0x2u << 24);    /* PA6 = AF2 */
    GPIOA->AFRL  = (GPIOA->AFRL & ~(0xFu << 28)) | (0x2u << 28);    /* PA7 = AF2 */

    TIM3->SMCR = TIM_SMCR_SMS_ENC3;  /* encoder mode 3 */
    TIM3->CCMR1 = (TIM_CCMR1_CC1S_TI2) | (TIM_CCMR1_CC2S_TI2 << 8);
    TIM3->CCER = 0;
    TIM3->ARR = 0xFFFF;
    TIM3->CNT = 0;
    TIM3->CR1 = TIM_CR1_CEN;

    g_last_cnt = 0;
    g_pos_x_mm = 0.0f;
    g_pos_y_mm = 0.0f;
    g_heading_deg = 0.0f;

    imu_init();
}

void encoder_reset_origin(void)
{
    TIM3->CNT = 0;
    g_last_cnt = 0;
    g_pos_x_mm = 0.0f;
    g_pos_y_mm = 0.0f;
    g_heading_deg = 0.0f;
    imu_reset_yaw();
}

void encoder_set_scale(float s)
{
    g_scale = s;
}

/* Called at 50 Hz from the main loop */
void encoder_update(void)
{
    int32_t cnt = (int16_t)TIM3->CNT;
    int32_t delta = cnt - g_last_cnt;
    if (delta > 32768)  delta -= 65536;
    if (delta < -32768) delta += 65536;
    g_last_cnt = cnt;

    /* Distance traveled this tick */
    float dist_mm = delta * ENC_RES_MM * g_scale;

    /* Update heading from IMU yaw rate (degrees) */
    float yaw_rate = imu_get_yaw_rate_dps();
    g_heading_deg += yaw_rate * 0.02f;   /* 50 Hz tick = 0.02 s */
    if (g_heading_deg >= 360.0f) g_heading_deg -= 360.0f;
    if (g_heading_deg < 0.0f)    g_heading_deg += 360.0f;

    /* Dead-reckon in XY */
    float rad = g_heading_deg * 3.14159265f / 180.0f;
    g_pos_x_mm += dist_mm * cosf(rad);
    g_pos_y_mm += dist_mm * sinf(rad);
}

void encoder_get_position(float *x_mm, float *y_mm, float *heading_deg)
{
    if (x_mm)        *x_mm = g_pos_x_mm;
    if (y_mm)        *y_mm = g_pos_y_mm;
    if (heading_deg) *heading_deg = g_heading_deg;
}

float encoder_get_total_distance_mm(void)
{
    /* Magnitude of position vector — a proxy for distance walked */
    return sqrtf(g_pos_x_mm * g_pos_x_mm + g_pos_y_mm * g_pos_y_mm);
}