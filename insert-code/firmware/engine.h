/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef INSERT_ENGINE_H
#define INSERT_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { F_NONE, F_TIME, F_INPUT, F_OVERFLOW, F_CONFIG, F_WATCHDOG };
enum { WAIT_EMPTY, READY, ACTIVE, HALT };
/* A code is a two-of-four closure mask; identity zero means unidentified.
 * Qualification is whole-word: independently delayed bits must not synthesize
 * a code that never physically appeared at an acquired endpoint. */
typedef struct {
    uint32_t now, samples, gaps, missed, tx_errors;
    uint32_t starts, completed, abandoned, unidentified, changed, invalid;
    uint32_t seats[6];
    uint32_t age, latest_age;
    uint8_t raw, qualified, valid, fault, state, identity;
    uint8_t changed_here, invalid_here, candidate, stable, started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, unsigned raw);
void engine_fault(Engine *e, unsigned fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
unsigned engine_identity(unsigned word);
#endif
