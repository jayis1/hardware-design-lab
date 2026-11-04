/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SNAP_ENGINE_H
#define SNAP_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define BASELINE_MS 250u
#define SETTLE_MS 250u
#define DEADLINE_MS 60000u
#define QUALIFY_SAMPLES 5u
#define MAX_EVENTS 32u
enum Phase { WAIT, READY, ACTIVE, DONE, HALT };
enum Fault { F_NONE, F_GAP, F_OVERFLOW, F_CONFIG, F_WATCHDOG, F_TIMEOUT, F_CAPACITY };
typedef struct { uint32_t at; uint8_t mask; } Event;
typedef struct {
    uint32_t now, samples, age, quiet, baseline, tx_errors;
    Event event[MAX_EVENTS];
    uint8_t phase, fault, raw, mask, valid, count;
    uint8_t candidate[4], stable[4];
    bool clocked, prior_open, prior_closed;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
