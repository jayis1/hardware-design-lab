/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef BALANCE_ENGINE_H
#define BALANCE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY 5u
#define SETTLE_MS 500u
#define MAX_TRIAL_MS 10000u
enum { WAIT, READY, ACTIVE, DONE, HALT };
enum { F_GAP=1, F_OVERFLOW=2, F_CONFIG=4, F_WATCHDOG=8 };
typedef struct {
    uint32_t now,state,levels,valid,starts,accepted,rejected,aborted;
    uint32_t gaps,faults,tx_errors,age,dwell,selected,outcome,evidence,conflict;
    uint32_t counts[9];
    uint32_t stamp;
    uint8_t candidate[2],stable[2],prior_raw;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool balance_conflict(uint32_t evidence);
bool engine_valid(const Engine *e);
#endif
