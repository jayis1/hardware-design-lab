/*
 * hal.h — TensilGuard hardware-abstraction-layer function declarations
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Declares the low-level functions every driver relies on (gpio, delay, SPI,
 * I2C, ADC, TIM, COMP, RTC). Each function is defined as a weak stub in the
 * file that owns the corresponding peripheral so the firmware links cleanly
 * on a host compiler without vendor CMSIS headers; the real firmware
 * replaces the stubs with register-level implementations in board.c.
 *
 * Pin macros in board.h are written as { 'A', 10 } compound literals; we
 * also expose named-pin helpers so they can be used in function-argument
 * position without tripping the preprocessor.
 */
#ifndef TENSILGUARD_HAL_H
#define TENSILGUARD_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "board.h"

/* ----------------------------- GPIO & timing ----------------------------- */
void     gpio_set(pin_t p, int level);
int      gpio_get(pin_t p);
void     delay_ms(uint32_t ms);
void     delay_us(uint32_t us);
uint32_t millis(void);

/* ----------------------------- ADC --------------------------------------- */
void     adc_capture_dual(uint8_t ch1, uint8_t ch2,
                           int16_t *buf1, int16_t *buf2, uint16_t n);
void     adc_capture_single(uint8_t ch, int16_t *buf, uint32_t n);
void     adc_start_circular(uint8_t ch, int16_t *buf, uint32_t n);
float    adc_read_mv(uint8_t ch);

/* ----------------------------- Timers / PWM ------------------------------ */
void     tim1_set_pwm_freq(float f_hz);
void     tim1_pwm_disable(void);

/* ----------------------------- Comparator -------------------------------- */
void     comp1_arm(float threshold_uv);
bool     comp1_triggered(void);

/* ----------------------------- RTC ---------------------------------------- */
void     rtc_set_alarm(uint32_t seconds);
uint32_t rtc_get_unix(void);
uint32_t read_reset_cause(void);

/* ----------------------------- SPI (accel, LoRa, flash) ------------------- */
uint8_t  spi3_read_reg(uint8_t reg);
void     spi3_write_reg(uint8_t reg, uint8_t val);
void     spi3_read_burst(uint8_t reg, uint8_t *buf, uint16_t n);

/* ----------------------------- I2C (TMP117, MAX17055) --------------------- */
uint16_t i2c_read16(uint8_t addr, uint8_t reg);
void     i2c_write16(uint8_t addr, uint8_t reg, uint16_t val);

/* ----------------------------- Flash ------------------------------------- */
bool     flash_init(void);
bool     flash_read_cal(cal_page_t *cal);
bool     flash_write_cal(const cal_page_t *cal);
void     flash_log_measurement(const measurement_t *m, uint32_t seq, uint32_t t);
uint32_t flash_log_ae(const ae_event_t *ev);
int      flash_read_log_range(uint32_t from_seq, uint32_t max_entries,
                               uint8_t *out, uint32_t out_len);

/* ----------------------------- Sensors / drivers ------------------------- */
bool     temp_init(void);
float    temp_read_c(void);
float    temp_last_c(void);
float    temp_compensate_mu_eff(float mu_eff_raw, float t_c, const cal_page_t *cal);
float    temp_compensate_f1(float f1_meas, float t_c, float L0_m);

bool     accel_init(void);
bool     accel_measure_tension(float *f1_out, float *t_vib_out);
int16_t  accel_rms_mg(void);

void     ae_init(void);
void     ae_isr(void);
void     ae_set_threshold(float uv);
uint8_t  ae_count_drain(void);
const ae_event_t *ae_last_event(void);
float    goertzel_power(const int16_t *buf, uint32_t n, float f_hz, float fs);

bool     mag_sweep(float *mu_eff_out, float *t_mag_out);
void     mag_calibrate_step(float t_kn, float mu_eff);
void     mag_set_baseline(float mu_eff);

/* ----------------------------- Power ------------------------------------- */
bool     power_init(void);
void     power_poll(void);
float    power_battery_pct(void);
float    power_battery_mv(void);
float    power_panel_mv(void);
float    power_current_ma(void);
uint32_t power_adapt_interval(uint32_t base_s);
void     power_enter_stop2(void);

/* ----------------------------- LoRa / mesh ------------------------------- */
void     lora_init(const uint8_t node_id[6]);
int      lora_send(const uint8_t *payload, uint8_t len, bool relay, int8_t snr);
void     lora_listen(void);
void     lora_isr(void);
void     lora_set_hop_limit(uint8_t h);
int8_t   lora_last_snr(void);
int16_t  lora_last_rssi(void);

/* ----------------------------- Misc math --------------------------------- */
static inline float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

#endif /* TENSILGUARD_HAL_H */