/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CLAMP_ENGINE_H
#define CLAMP_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 5u
#define SETTLE_MS 50u
#define TRIAL_MS 10000u
/* Result describes only the retained trial, not the current jaw positions. */
enum Result { NONE, ACTIVE, DONE, RETURNED, TIMEOUT, GAP, FAULT };
enum Fault { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
typedef struct {
    uint32_t now;
    uint32_t starts;
    uint32_t completed;
    uint32_t aborted;
    uint32_t gaps;
    uint32_t tx_errors;
    uint32_t faults;
    uint32_t elapsed_ms;
    uint32_t gap_ms;
    uint32_t overlap_ms;
    uint32_t max_gap_ms;
    uint32_t run_gap_ms;
    uint16_t stable[4];
    uint16_t settle;
    uint8_t raw;
    uint8_t levels;
    uint8_t qualified;
    uint8_t grips;
    uint8_t owner;
    uint8_t candidate;
    uint8_t direction;
    uint8_t result;
    uint8_t direct;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
