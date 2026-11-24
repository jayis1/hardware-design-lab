/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TOUR_ENGINE_H
#define TOUR_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define DEADLINE 60000u
enum { F_CONFIG=1, F_WATCHDOG=2, F_TIME=4, F_OVERFLOW=8, F_INPUT=16 };
typedef struct {
    uint32_t now, samples, missed, tx_errors, fault;
    uint32_t raw, qualified, valid, armed;
    uint32_t starts, rounds, aborted, repeats, discarded;
    uint32_t disruptions, timeouts;
    uint32_t active, mask, age, order;
    uint32_t last_age, last_order;
    uint32_t visits[4];
    /* Internal whole-word qualifier and raw-envelope guard. */
    uint32_t candidate, stable, envelope;
    bool seeded, blocked;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint32_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
unsigned tour_population(uint32_t mask);
#endif
