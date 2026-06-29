/*
 * drivers/hx711.h — HX711 load cell ADC driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef HIVEVOX_HX711_H
#define HIVEVOX_HX711_H

#include <stdint.h>

#define HX711_READ_TIMEOUT  ((int32_t)0x80000000)

void    hx711_init(void);
void    hx711_power_down(void);
void    hx711_power_up(void);
int     hx711_wait_ready(uint32_t timeout_ms);
int32_t hx711_read_raw(void);
int32_t hx711_read_avg(uint8_t n);
int32_t hx711_read_grams(void);
void    hx711_tare(void);
void    hx711_set_scale(float counts_per_gram);
float   hx711_get_scale(void);
int32_t hx711_get_offset(void);
void    hx711_set_offset(int32_t off);

void delay_ms(uint32_t ms);

#endif /* HIVEVOX_HX711_H */