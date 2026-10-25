/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef NS_ENGINE_H
#define NS_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define NS_CHANNELS 4u
#define NS_DEBOUNCE 5u
#define NS_BASELINE 100u
#define NS_SETTLE 250u
#define NS_LIMIT 30000u
enum ns_state { NS_WAIT, NS_READY, NS_ACTIVE, NS_FAULT };
enum ns_reason { NS_NONE, NS_OK, NS_UNSETTLED, NS_TIMEOUT, NS_GAP };
typedef struct {
    uint32_t id;
    enum ns_reason reason;
    uint32_t duration;
    uint32_t first_settle;
    uint32_t full_ms;
    uint32_t longest;
    uint32_t drops;
    uint8_t drop_mask;
    uint8_t seen;
} NSResult;
typedef struct {
    enum ns_state state;
    uint32_t previous;
    uint32_t candidate_since[NS_CHANNELS];
    uint8_t candidate;
    uint8_t qualified;
    uint8_t mask;
    bool baseline;
    uint32_t baseline_since;
    uint32_t start;
    bool full;
    bool ever_full;
    bool settled;
    uint32_t run;
    uint32_t full_ms;
    uint32_t longest;
    uint32_t first_settle;
    uint32_t drops;
    uint8_t drop_mask;
    uint8_t seen;
    uint32_t gaps;
    NSResult result;
} NS;
void ns_init(NS *e, uint32_t now);
void ns_tick(NS *e, uint32_t now, uint8_t raw);
void ns_fault(NS *e);
#endif
