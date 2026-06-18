/*
 * lora_link.h — HydroFluor LoRa (RFM95W) telemetry uplink
 * Author: jayis1
 * License: MIT
 */
#ifndef LORA_LINK_H
#define LORA_LINK_H

#include "board.h"
#include "fluorometry.h"
#include <stdint.h>

/* Initialize SPI2 + RFM95W, set frequency & modulation. */
int  lora_init(uint32_t freq_hz, uint8_t sf, uint32_t bw_hz);

/* Compose a summary uplink from a sample record and transmit.
 * Returns 0 on successful TX start. */
int  lora_uplink_sample(const sample_record_t *rec);

/* Non-blocking poll: handle DIO0 (TxDone) and clear flags. */
void lora_poll(void);

/* Set the uplink interval in seconds (0 = disabled). */
void lora_set_interval(uint16_t seconds);

#endif /* LORA_LINK_H */