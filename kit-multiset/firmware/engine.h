/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef KIT_ENGINE_H
#define KIT_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { WAIT, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
#define QUALIFY 5u
#define MAX_BATCH_MS 60000u
/* All counters are volatile session observations, never verified part counts. */
typedef struct {
    uint32_t now, state, levels, valid, starts, good, bad, aborted;
    uint32_t gaps, faults, tx_errors, age, tainted;
    uint32_t count[3], last[3], last_result, last_age;
    uint32_t stamp;
    uint8_t candidate[4], stable[4];
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
