/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TEXTURE_ENGINE_H
#define TEXTURE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { WAIT, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_OVERFLOW=4 };
#define QUALIFY 5u
#define MAX_PRESS_MS 10000u
typedef struct {
    uint32_t now, state, levels, valid, starts, references, preferences;
    uint32_t rejected, aborted, gaps, faults, tx_errors;
    uint32_t graph, cycle, anchored, anchor, age, tainted, selected, outcome;
    uint32_t stamp;
    uint8_t candidate[3], stable[3];
    uint8_t paddle_seen;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
uint32_t engine_cycle(uint32_t graph);
#endif
