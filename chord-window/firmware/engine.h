/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CW_ENGINE_H
#define CW_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define CW_DEBOUNCE 5u
#define CW_BASELINE 100u
#define CW_LIMIT 10000u
#define CW_CHANNELS 4u
enum cw_state { CW_WAIT, CW_READY, CW_ACTIVE, CW_FAULT };
enum cw_reason { CW_NONE, CW_OK, CW_RETAP, CW_TIMEOUT, CW_GAP };
typedef struct {
    uint32_t id;
    enum cw_reason reason;
    uint8_t keys;
    uint32_t spread;
    uint32_t overlap;
    uint32_t release;
    uint32_t duration;
} CWResult;
typedef struct {
    enum cw_state state;
    uint32_t previous;
    uint32_t candidate_since[CW_CHANNELS];
    uint8_t candidate;
    uint8_t qualified;
    uint8_t mask;
    uint8_t seen;
    uint8_t released;
    bool retap;
    bool baseline;
    uint32_t baseline_since;
    uint32_t start;
    uint32_t press[CW_CHANNELS];
    uint32_t lift[CW_CHANNELS];
    uint32_t gaps;
    CWResult result;
} CW;
void cw_init(CW *e, uint32_t now);
void cw_tick(CW *e, uint32_t now, uint8_t raw);
void cw_fault(CW *e);
#endif
