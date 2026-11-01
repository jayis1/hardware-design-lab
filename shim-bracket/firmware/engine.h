/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SHIM_ENGINE_H
#define SHIM_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { WAIT, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
#define QUALIFY 5u
#define MAX_PRESS_MS 10000u
typedef struct {
    uint32_t now, state, levels, valid, starts, accepted, rejected, aborted;
    uint32_t gaps, faults, tx_errors, pass, refuse, conflict, age, tainted;
    uint32_t selected, outcome, stamp;
    uint8_t candidate[3], stable[3];
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
