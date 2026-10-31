/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PATTERN_ENGINE_H
#define PATTERN_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define PATTERN_SIZE 16u
#define SETUP_MS 20u
#define REARM_MS 20u
#define MIN_HIGH_MS 10u
#define MAX_HIGH_MS 1000u
/* Faults are terminal: losing one clock destroys positional alignment. */
enum Fault { F_NONE, F_CONFIG, F_WATCHDOG, F_GAP, F_SETUP, F_HOLD,
             F_SHORT, F_LONG, F_REARM, F_OVERFLOW };
enum State { WAIT, READY, HIGH, HALTED };
typedef struct {
    uint32_t now, tx, words, bad, high_ms;
    uint32_t errors[PATTERN_SIZE];
    uint8_t reference[PATTERN_SIZE], latest[PATTERN_SIZE];
    uint8_t raw, state, fault;
    /* Internal sampled-endpoint evidence; never inferred from host time. */
    uint8_t previous, captured;
    uint32_t data_ms, low_ms;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
