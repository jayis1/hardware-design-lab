/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef REBOUND_ENGINE_H
#define REBOUND_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum State { WAIT, READY, DOWN, TURN, UP, DONE, FAULT };
enum Fault {
    F_ORDER = 1, F_OVERLAP = 2, F_GAP = 4, F_TIMEOUT = 8,
    F_CONFIG = 16, F_WATCHDOG = 32
};
#define QUALIFY_MS 3u
#define CLEAR_MS 250u
#define LIMIT_MS 5000u
/* Every accepted edge is retained. Masks distinguish absent timestamps from
 * the legitimate zero timestamp at the initial upper-gate closure. */
typedef struct {
    uint32_t now;
    uint32_t state;
    uint32_t raw;
    uint32_t stable;
    uint32_t qualified;
    uint32_t faults;
    uint32_t age;
    uint32_t high;
    uint32_t downmask;
    uint32_t upmask;
    uint32_t events;
    uint32_t gaps;
    uint32_t tx_errors;
    uint32_t clear_ms;
    uint32_t down[4];
    uint32_t up[4];
    uint32_t end_ms;
    uint32_t candidate;
    uint32_t candidate_ms;
    uint32_t started;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
