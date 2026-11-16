/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef AISLE_ENGINE_H
#define AISLE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define CELLS 9u
/* Per drawer code: 00=before early, 01=between, 11=beyond late.
 * 10 is forbidden. Bits 0/1 are drawer A; bits 2/3 are drawer B. */
enum { WAIT=0, OBSERVE=1, HALT=3 };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_OVERFLOW=8 };
typedef struct {
    uint32_t sequence, now, fault, intervals, unknown, invalid;
    uint32_t episodes, run, maximum, gaps, tx_errors;
    uint32_t bins[CELLS];
    uint8_t raw, qualified, valid, candidate[2], streak[2], phase;
    bool started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
int engine_cell(uint8_t raw);
uint32_t engine_shared(const Engine *e);
#endif
