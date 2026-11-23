/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RATIO_ENGINE_H
#define RATIO_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define REARM 100u
#define TURN 96u
#define DEADLINE 120000u
#define OUTPUT_LIMIT 4096u
#define LIMIT 1000000000u
enum { WAIT, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_TIME=4, F_OVERFLOW=8, F_INPUT=16 };
enum { R_NONE, R_REVERSE, R_JUMP, R_TIMEOUT, R_CAPACITY, R_PLATFORM };
/* Directions: 0 unknown, 1 positive ring, 2 negative ring. */
typedef struct {
    uint32_t samples, now, state, fault, raw, qualified, valid;
    uint32_t starts, good, bad, same, opposite, still;
    uint32_t minimum, maximum, total, last, last_ms, last_input, last_output;
    uint32_t steps, output, age, input_dir, output_dir, reason, quiet, tx_errors;
    uint8_t candidate[2], stable[2];
    bool seeded;
    bool clean;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint32_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
int quadrature(unsigned before, unsigned after);
#endif
