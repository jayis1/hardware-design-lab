/*
 * drivers/onewire.h — DS18B20 1-Wire temperature probe driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef HIVEVOX_ONEWIRE_H
#define HIVEVOX_ONEWIRE_H

#include <stdint.h>

int  ow_reset(void);
void ow_write_byte(uint8_t b);
uint8_t ow_read_byte(void);

int  ds18b20_start_all(void);
int  ds18b20_wait_done(uint32_t timeout_ms);
int  ds18b20_read_scratch(int16_t *temp_centi);
int  ds18b20_read_by_rom(const uint8_t rom[8], int16_t *temp_centi);

void delay_ms(uint32_t ms);
volatile uint32_t dwt_cycle_count(void);

#endif /* HIVEVOX_ONEWIRE_H */