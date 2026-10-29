/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SCOOP_ENGINE_H
#define SCOOP_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 5u
/* Bits: dedicated A dock, A lid closed, dedicated B dock, B lid closed. */
enum Fault { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
typedef struct {
    uint32_t now;
    uint32_t starts[2];
    uint32_t completed[2];
    uint32_t aborted[2];
    uint32_t flagged[2];
    uint32_t away_ms[2];
    uint32_t foreign_ms[2];
    uint32_t gaps;
    uint32_t tx_errors;
    uint32_t faults;
    uint8_t stable[4];
    uint8_t raw;
    uint8_t levels;
    uint8_t qualified;
    uint8_t ready;
    uint8_t active;
    uint8_t marked;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
