/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FLAP_ENGINE_H
#define FLAP_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define BASELINE_MS 250u
#define SETTLE_MS 250u
#define DEADLINE_MS 10000u
#define QUALIFY_SAMPLES 5u
#define MAX_CLOSURES 1000u
/* READY means the common retaining bar is still engaged. */
enum Phase { WAIT, READY, ACTIVE, DONE, HALT };
enum Fault { F_NONE, F_GAP, F_OVERFLOW, F_CONFIG, F_WATCHDOG,
             F_TIMEOUT, F_PRECLOSED, F_REHELD };
typedef struct {
    uint32_t first, last, closures, reopens, quiet;
} Lane;
typedef struct {
    uint32_t now, age, baseline, tx_errors;
    Lane lane[3];
    uint8_t phase, fault, raw, mask, valid;
    uint8_t candidate[4], stable[4];
    bool clocked, prior_baseline;
    uint8_t prior_agree;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
