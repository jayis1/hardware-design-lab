/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CATCH_ENGINE_H
#define CATCH_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY 5u
#define REARM_MS 50u
#define SETTLE_MS 200u
#define CREDIT_MS 500u
#define MAX_TRIAL_MS 30000u
enum { WAIT, READY, SETTLING, LATCHED, DONE, HALT };
enum { F_GAP=1, F_OVERFLOW=2, F_CONFIG=4, F_WATCHDOG=8 };
enum { DOOR=1, BOLT=2, HANDLE=4, TUG=8 };
typedef struct {
    uint32_t now,state,levels,valid,starts,settled,intentional,challenged;
    uint32_t unexplained,unseated,timed_out,aborted,gaps,faults,tx_errors;
    uint32_t age,dwell,handle_age,tug_age,handle_credit,tug_credit,bolt_losses;
    uint32_t stamp,rearm;
    uint8_t candidate[4],stable[4],prior_raw;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
