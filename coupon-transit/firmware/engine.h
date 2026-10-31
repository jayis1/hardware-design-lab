/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TRANSIT_ENGINE_H
#define TRANSIT_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_SAMPLES 10u
#define BASELINE_MS 50u
#define DEADLINE_MS 5000u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_EXHAUST 4u
#define F_RAW 8u
enum { WAIT, DOCKED, TRANSIT };
enum { NONE, DELIVERED, RETURNED };
typedef struct {
    uint32_t state, origin, elapsed;
    uint32_t started, delivered, returned, abandoned;
    uint32_t sum_ms, max_ms, last_ms, last_kind;
    uint32_t ambiguities, timeouts;
    /* Internal qualification and time anchors are never host-clock values. */
    uint32_t departure, baseline;
    uint8_t candidate;
} Holder;
typedef struct {
    uint32_t now, mask, known, gaps, tx, fault, halted;
    Holder holder[2];
    uint8_t raw_candidate[4], stable[4];
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, unsigned raw);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
