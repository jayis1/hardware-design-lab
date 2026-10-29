/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PIPETTE_ENGINE_H
#define PIPETTE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 5u
#define TIMEOUT_MS 30000u
/* FIRST remains closed beyond the first stop; SECOND adds another bit. */
enum Phase { WAIT, READY, APPROACH, FIRST, IMMERSED, ASPIRATED, DISPENSE, BLOWOUT, REMOVED };
enum Reason { E_NONE, E_ORDER, E_SOURCE, E_CODE, E_GAP, E_TIMEOUT, E_HARDWARE };
enum Fault { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
typedef struct {
    uint32_t now;
    uint32_t starts;
    uint32_t completed;
    uint32_t aborted;
    uint32_t elapsed;
    uint32_t last_ms;
    uint32_t last_reason;
    uint32_t gaps;
    uint32_t tx_errors;
    uint32_t faults;
    uint8_t phase;
    uint8_t stable[4];
    uint8_t raw;
    uint8_t levels;
    uint8_t qualified;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
