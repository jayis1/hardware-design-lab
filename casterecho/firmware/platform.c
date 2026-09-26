/*
 * CasterEcho hardware abstraction and deterministic host simulator.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "casterecho.h"
#include "registers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef CE_HOST_SIM
static uint32_t sim_ms;
static uint32_t sim_step;
static uint8_t slots[CE_HISTORY_RECORDS][sizeof(ce_pass_t)];
static bool slot_valid[CE_HISTORY_RECORDS];

uint32_t platform_millis(void) { return sim_ms; }
void platform_delay_ms(uint32_t milliseconds) { sim_ms += milliseconds; }
void platform_init(void) { sim_ms = 0u; sim_step = 0u; memset(slot_valid, 0, sizeof(slot_valid)); }
void platform_sleep(void) { platform_delay_ms(10u); }
void platform_set_led(bool enabled) { (void)enabled; }

ce_result_t platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *data, size_t length) {
    if (data == NULL || length == 0u) return CE_ERR_ARGUMENT;
    memset(data, 0, length);
    if (address == LIS3DH_ADDRESS && reg == LIS3DH_WHO_AM_I) {
        data[0] = 0x33u;
        return CE_OK;
    }
    if (address == LIS3DH_ADDRESS && reg == (uint8_t)(LIS3DH_OUT_X_L | 0x80u) && length >= 6u) {
        const float phase = (float)sim_step * 0.1570796f;
        const float wheel = sinf(phase) * 0.13f;
        const float rough = sinf(phase * 9.0f) * 0.035f;
        int16_t x = (int16_t)((wheel + rough) * 16384.0f);
        int16_t y = (int16_t)(cosf(phase * 0.5f) * 0.04f * 16384.0f);
        int16_t z = (int16_t)((1.0f + wheel * 0.5f) * 16384.0f);
        data[0]=(uint8_t)x; data[1]=(uint8_t)((uint16_t)x>>8);
        data[2]=(uint8_t)y; data[3]=(uint8_t)((uint16_t)y>>8);
        data[4]=(uint8_t)z; data[5]=(uint8_t)((uint16_t)z>>8);
        return CE_OK;
    }
    return CE_ERR_IO;
}

ce_result_t platform_i2c_write(uint8_t address, uint8_t reg, const uint8_t *data, size_t length) {
    if (address != LIS3DH_ADDRESS || data == NULL || length == 0u) return CE_ERR_ARGUMENT;
    return (reg == LIS3DH_CTRL1 || reg == LIS3DH_CTRL4) ? CE_OK : CE_ERR_IO;
}

uint16_t platform_adc_mv(uint8_t pin) {
    const float phase = (float)sim_step * 0.1570796f;
    if (pin == PIN_PIEZO_ADC) return (uint16_t)(1650.0f + 280.0f * sinf(phase * 4.0f));
    if (pin == PIN_TEMP_ADC) return (uint16_t)(742.0f + 8.0f * sinf(phase * 0.03f));
    if (pin == PIN_GENERATOR_ADC) return (uint16_t)(920.0f + 90.0f * sinf(phase));
    return 0u;
}

bool platform_hall_edges(uint32_t *period_us, int8_t *direction) {
    if (period_us == NULL || direction == NULL) return false;
    *period_us = 92000u + (sim_step % 17u) * 400u;
    *direction = 1;
    return true;
}

uint16_t platform_harvested_mj(void) { return (uint16_t)(32u + sim_step / 8u); }

ce_result_t platform_store(uint16_t slot, const void *data, size_t length) {
    if (slot >= CE_HISTORY_RECORDS || data == NULL || length > sizeof(slots[0])) return CE_ERR_ARGUMENT;
    memcpy(slots[slot], data, length); slot_valid[slot] = true; return CE_OK;
}

ce_result_t platform_load(uint16_t slot, void *data, size_t length) {
    if (slot >= CE_HISTORY_RECORDS || data == NULL || length > sizeof(slots[0])) return CE_ERR_ARGUMENT;
    if (!slot_valid[slot]) return CE_ERR_STORAGE;
    memcpy(data, slots[slot], length); return CE_OK;
}

void platform_ble_notify(const uint8_t *data, size_t length) {
    if (data != NULL && length > 0u) printf("notify:%zu:%02x\n", length, data[0]);
}
int platform_ble_receive(uint8_t *data, size_t capacity) { (void)data; (void)capacity; return 0; }
bool platform_button_pressed(void) { return false; }
void platform_sim_advance(void) { ++sim_step; platform_delay_ms(1000u / CE_SAMPLE_HZ); }
#else
uint32_t platform_millis(void) { return NRF_TIMER1_CC(0); }
void platform_delay_ms(uint32_t milliseconds) { while (milliseconds-- > 0u) for (volatile uint32_t i=0;i<6400u;++i) {} }
void platform_init(void) {
    NRF_GPIO_DIRSET = BIT(PIN_STATUS_LED);
    NRF_CLOCK_TASKS_HFCLKSTART = 1u;
    while (NRF_CLOCK_EVENTS_HFCLKSTARTED == 0u) {}
    NRF_WDT_CRV = 32768u * 4u; NRF_WDT_RREN = 1u; NRF_WDT_TASKS_START = 1u;
}
void platform_sleep(void) { __asm volatile("wfe"); }
void platform_set_led(bool enabled) { if (enabled) NRF_GPIO_OUTSET=BIT(PIN_STATUS_LED); else NRF_GPIO_OUTCLR=BIT(PIN_STATUS_LED); }
/* Production implementations bind to nrfx_twim, nrfx_saadc, GPIOTE/PPI, FDS, and SoftDevice. */
ce_result_t platform_i2c_read(uint8_t a,uint8_t r,uint8_t*d,size_t n){(void)a;(void)r;(void)d;(void)n;return CE_ERR_IO;}
ce_result_t platform_i2c_write(uint8_t a,uint8_t r,const uint8_t*d,size_t n){(void)a;(void)r;(void)d;(void)n;return CE_ERR_IO;}
uint16_t platform_adc_mv(uint8_t p){(void)p;return 0u;}
bool platform_hall_edges(uint32_t*p,int8_t*d){(void)p;(void)d;return false;}
uint16_t platform_harvested_mj(void){return 0u;}
ce_result_t platform_store(uint16_t s,const void*d,size_t n){(void)s;(void)d;(void)n;return CE_ERR_STORAGE;}
ce_result_t platform_load(uint16_t s,void*d,size_t n){(void)s;(void)d;(void)n;return CE_ERR_STORAGE;}
void platform_ble_notify(const uint8_t*d,size_t n){(void)d;(void)n;}
int platform_ble_receive(uint8_t*d,size_t n){(void)d;(void)n;return 0;}
bool platform_button_pressed(void){return false;}
void platform_sim_advance(void){}
#endif
