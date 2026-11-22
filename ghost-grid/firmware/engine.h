/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef GHOST_ENGINE_H
#define GHOST_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define MASKS 16u
/* Terminal faults; accounting remains frozen, time/transport may advance. */
enum { F_CONFIG=1, F_WATCHDOG=2, F_TIME=4, F_OVERFLOW=8, F_INPUT=16 };
typedef struct {
    uint32_t now;
    uint32_t samples;
    uint32_t missed;
    uint32_t unknown;
    uint32_t tx_errors;
    uint32_t fault;
    uint32_t raw;
    uint32_t qualified;
    uint32_t valid;
    uint32_t run;
    uint32_t bins[MASKS];
    uint32_t segments[MASKS];
    uint32_t peaks[MASKS];
    uint8_t candidate;
    uint8_t stable;
    bool seeded;
    bool clean;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint32_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
uint32_t matrix_scan(uint32_t physical);
uint32_t matrix_ghost(uint32_t physical);
#endif
