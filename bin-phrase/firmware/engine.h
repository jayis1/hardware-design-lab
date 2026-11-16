/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PHRASE_ENGINE_H
#define PHRASE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define DEADLINE 600000u
#define WORD_SIZE 16u
/* Logical ones: lid OPEN A/B/C in bits 0..2, tray PRESENT in bit 3. */
enum { BASELINE, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_OVERFLOW=8 };
enum { R_NONE, R_REMOVAL, R_TIMEOUT, R_CAPACITY, R_FAULT };
typedef struct {
    uint32_t sequence, now, fault, starts, completed, aborted;
    uint32_t age, last_age, gaps, tx_errors, reason;
    uint8_t raw, qualified, valid, candidate[4], run[4];
    uint8_t phase, count, last_count;
    uint8_t word[WORD_SIZE], last_word[WORD_SIZE];
    bool started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
