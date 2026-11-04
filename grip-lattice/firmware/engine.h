/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef GRIP_ENGINE_H
#define GRIP_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define BASELINE_MS 250u
#define DEADLINE_MS 60000u
#define QUALIFY_SAMPLES 5u
#define NO_BIN 8u
enum Phase { WAIT, READY, ACTIVE, DONE, HALT };
enum Fault { F_NONE, F_GAP, F_OVERFLOW, F_CONFIG, F_WATCHDOG, F_TIMEOUT };
typedef struct {
    uint32_t now, samples, starts, completed, aborted, tx_errors;
    uint32_t age, unknown, baseline, streak;
    uint32_t hist[8], runs[8], longest[8];
    uint8_t phase, fault, raw, mask, valid, bin;
    uint8_t candidate[4], count[4];
    bool clocked, prior_low;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
