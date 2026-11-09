/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef LABEL_ENGINE_H
#define LABEL_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define TIMEOUT 60000u
/* Contact bits: dock, encoded class B, response A, response B. */
enum { WAIT, READY, ACTIVE, HALT };
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_COUNTER=8 };
enum { R_NONE, R_AMBIGUOUS, R_UNANSWERED, R_TIMEOUT, R_GAP, R_FAULT };
typedef struct {
    uint32_t sequence, now, state, fault, valid;
    uint32_t starts, completed, rejected, cells[4];
    uint32_t age, answer, tainted, last_truth, last_answer, last_ms, reason;
    uint32_t tx_errors, gaps;
    /* These acquisition fields never go on the wire: provisional truth is private. */
    uint8_t raw, qualified, candidate[3], stable[3], truth, envelope;
    bool seen, removing;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
