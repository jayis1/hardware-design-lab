/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SECTOR_ENGINE_H
#define SECTOR_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define SECTORS 16u
enum { WAIT, TRACK, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_COUNTER=8 };
typedef struct {
    uint32_t sequence, now, state, fault, raw, qualified, valid, sector;
    uint32_t unknown, forward, backward, skipped, reversals, direction;
    uint32_t run, tx_errors, gaps;
    uint32_t dwell[SECTORS], longest[SECTORS];
    uint8_t candidate, stable;
    bool seen, anchored, clean;
} Engine;
uint8_t engine_decode(uint8_t gray);
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
