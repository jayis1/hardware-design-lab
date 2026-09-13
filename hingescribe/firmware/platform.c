/*
 * HingeScribe platform layer: deterministic host simulation and nRF hooks
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "hingescribe.h"
#include "registers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef HS_HOST_SIM
static uint32_t simulated_ms;
static float simulated_phase;
static bool led_state;
static uint8_t received[HS_PACKET_MAX];
static size_t received_length;

uint32_t platform_millis(void) { return simulated_ms; }
void platform_delay_ms(uint32_t milliseconds) { simulated_ms += milliseconds; }
void platform_init(void)
{
    simulated_ms = 0u;
    simulated_phase = 0.0f;
    led_state = false;
    received_length = 0u;
}
void platform_sleep(void) { platform_delay_ms(10u); }
void platform_set_led(bool enabled) { led_state = enabled; }

static float simulated_angle(void)
{
    float cycle = fmodf(simulated_phase, 10.0f);
    if (cycle < 2.0f) return 0.0f;
    if (cycle < 4.0f) return (cycle - 2.0f) * 45.0f;
    if (cycle < 6.0f) return 90.0f;
    if (cycle < 9.0f) return 90.0f - (cycle - 6.0f) * 30.0f;
    return 0.0f;
}

hs_result_t platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length)
{
    if (data == NULL || length == 0u) return HS_ERR_ARGUMENT;
    memset(data, 0, length);
    if (address == AS5600_ADDRESS && reg == AS5600_STATUS && length == 1u) {
        data[0] = 0x20u;
        return HS_OK;
    }
    if (address == AS5600_ADDRESS && reg == AS5600_RAW_ANGLE && length == 2u) {
        uint16_t raw = (uint16_t)(simulated_angle() * (4096.0f / 360.0f));
        data[0] = (uint8_t)(raw >> 8);
        data[1] = (uint8_t)(raw & 0xFFu);
        return HS_OK;
    }
    if (address == TMP117_ADDRESS && reg == TMP117_TEMP && length == 2u) {
        int16_t raw = (int16_t)((21.5f + sinf(simulated_phase) * 0.4f) / 0.0078125f);
        data[0] = (uint8_t)((uint16_t)raw >> 8);
        data[1] = (uint8_t)raw;
        return HS_OK;
    }
    return HS_ERR_IO;
}

hs_result_t platform_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0u) return HS_ERR_ARGUMENT;
    return address == TMP117_ADDRESS && reg == TMP117_CONFIG ? HS_OK : HS_ERR_IO;
}

hs_result_t platform_loadcell_read(int32_t *counts, uint32_t timeout_ms)
{
    if (counts == NULL || timeout_ms == 0u) return HS_ERR_ARGUMENT;
    float angle = simulated_angle();
    float force = angle > 1.0f ? 18.0f + 0.12f * angle : 0.0f;
    force += 1.3f * sinf(simulated_phase * 4.3f);
    *counts = (int32_t)(120000.0f + force * 8420.0f);
    return HS_OK;
}

uint16_t platform_adc_mv(uint8_t pin)
{
    (void)pin;
    return (uint16_t)(3900.0f - fmodf(simulated_phase, 100.0f));
}
uint16_t platform_harvest_mj(void) { return (uint16_t)(simulated_angle() * 0.8f); }
void platform_ble_notify(const uint8_t *data, size_t length)
{
    if (data != NULL && length > 0u) printf("notify:%zu\n", length);
}
int platform_ble_receive(uint8_t *data, size_t capacity)
{
    if (data == NULL || received_length == 0u || capacity < received_length) return 0;
    memcpy(data, received, received_length);
    int length = (int)received_length;
    received_length = 0u;
    return length;
}
bool platform_button_pressed(void) { return false; }
void platform_sim_advance(void)
{
    simulated_phase += 0.01f;
    simulated_ms += 10u;
}
#else
uint32_t platform_millis(void) { return NRF_RTC1_BASE; }
void platform_delay_ms(uint32_t milliseconds)
{
    volatile uint32_t loops = milliseconds * 16000u;
    while (loops-- > 0u) { __asm volatile("nop"); }
}
void platform_init(void)
{
    NRF_CLOCK_TASKS_HFCLKSTART = 1u;
    while (NRF_CLOCK_EVENTS_HFCLKSTARTED == 0u) { }
    NRF_GPIO_DIRSET = BIT(PIN_STATUS_LED) | BIT(PIN_LOAD_SCK);
    NRF_GPIO_PIN_CNF(PIN_USER_BUTTON) = 0x0000000Cu;
    NRF_TWIM_PSEL_SCL = PIN_I2C_SCL;
    NRF_TWIM_PSEL_SDA = PIN_I2C_SDA;
    NRF_TWIM_FREQUENCY = 0x06680000u;
    NRF_TWIM_ENABLE = 6u;
    NRF_WDT_CRV = 32768u * 8u;
    NRF_WDT_RREN = 1u;
    NRF_WDT_TASKS_START = 1u;
}
void platform_sleep(void) { __asm volatile("wfe"); NRF_WDT_RR0 = 0x6E524635u; }
void platform_set_led(bool enabled) { if (enabled) NRF_GPIO_OUTCLR = BIT(PIN_STATUS_LED); else NRF_GPIO_OUTSET = BIT(PIN_STATUS_LED); }
hs_result_t platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length)
{ (void)address; (void)reg; (void)data; (void)length; return HS_ERR_IO; }
hs_result_t platform_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length)
{ (void)address; (void)reg; (void)data; (void)length; return HS_ERR_IO; }
hs_result_t platform_loadcell_read(int32_t *counts, uint32_t timeout_ms)
{ (void)counts; (void)timeout_ms; return HS_ERR_IO; }
uint16_t platform_adc_mv(uint8_t pin) { (void)pin; return 0u; }
uint16_t platform_harvest_mj(void) { return 0u; }
void platform_ble_notify(const uint8_t *data, size_t length) { (void)data; (void)length; }
int platform_ble_receive(uint8_t *data, size_t capacity) { (void)data; (void)capacity; return 0; }
bool platform_button_pressed(void) { return (NRF_GPIO_IN & BIT(PIN_USER_BUTTON)) == 0u; }
void platform_sim_advance(void) { }
#endif
