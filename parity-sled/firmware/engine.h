/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PARITY_ENGINE_H
#define PARITY_ENGINE_H
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
    /* Qualified odd-parity excursions, independent of raw-word segments. */
    uint32_t starts;
    uint32_t same;
    uint32_t changed;
    uint32_t orphan;
    uint32_t aborted;
    uint32_t active;
    uint32_t known;
    uint32_t anchor;
    uint32_t age;
    uint32_t last_age;
    uint32_t last_from;
    uint32_t last_to;
    uint32_t last_kind;
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
uint32_t parity_odd(uint32_t word);
#endif
