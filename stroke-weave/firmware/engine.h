/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef STROKE_ENGINE_H
#define STROKE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000u
#define DEADLINE 60000u
#define QUALIFY 5u
enum { RUNNING, HALT };
enum { UNKNOWN, HOME, END, OUTBOUND, RETURNING };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_DEADLINE=8,
       F_BOUNDARY=16, F_COUNTER=32, F_GEOMETRY=64 };
typedef struct {
    uint32_t phase, starts, deliveries, refills, canceled, abandoned, age, last_ms;
} Lane;
typedef struct {
    uint32_t sequence, now, state, fault, raw, qualified, valid;
    Lane lane[2];
    uint32_t alternating, repeated, ties, anchor, streak, peak, tx_errors, gaps;
    uint8_t candidate[4], stable[4];
    bool seen;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
