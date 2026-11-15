/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef REACH_ENGINE_H
#define REACH_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define WINDOW 100u
#define DEADLINE 30000u
/* Phases, faults and rows are fixed by the LR1 protocol. */
enum { BASELINE, READY, TRAVEL, RETURN, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_OVERFLOW=8 };
typedef struct {
    uint32_t sequence, now, fault, starts, completed, rejected;
    uint32_t bins[4][3]; /* witnessed NEAR/FAR mask x open/closed/disturbed */
    uint32_t age, quiet, gaps, tx_errors;
    uint8_t raw, qualified, valid, candidate[4], run[4];
    uint8_t phase, seen, latch, disturbed;
    bool started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
