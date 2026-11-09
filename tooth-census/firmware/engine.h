/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TOOTH_ENGINE_H
#define TOOTH_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define EDGES 96u
#define TIMEOUT 120000u
enum { SEEK, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_COUNTER=8 };
enum { R_NONE, R_REVERSE, R_QUADRATURE, R_STEPS, R_BOUNDARY, R_TIMEOUT, R_GAP, R_FAULT };
typedef struct {
    uint32_t sequence, now, state, fault, raw, qualified, valid;
    uint32_t starts, good, bad, mismatch, reference, minimum, maximum;
    uint32_t total, last, last_ms, reason, steps, teeth, age, direction;
    uint32_t tx_errors, gaps;
    uint8_t candidate[3], stable[3];
    bool seen, index_armed, tooth_high;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
