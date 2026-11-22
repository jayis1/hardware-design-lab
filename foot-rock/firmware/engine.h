/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FOOT_ENGINE_H
#define FOOT_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define SETTLE 1000u
#define NONE 4u
/* Sensor order NW, NE, SW, SE; opposite diagonal index is i XOR 3. */
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
    uint32_t anchor;
    uint32_t settle;
    uint32_t seeds;
    uint32_t diagonal;
    uint32_t adjacent;
    uint32_t dwell[6];
    uint32_t visits[4];
    uint32_t edges[16];
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
unsigned support_class(uint32_t mask);
#endif
