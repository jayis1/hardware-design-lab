/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PUNCH_ENGINE_H
#define PUNCH_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define COUNT_LIMIT 1000000000u
#define QUALIFY_MS 5u
#define SEAT_MS 100u
#define EMPTY_MS 100u
#define TIMEOUT_MS 30000u
#define MAX_GAP_MS 5u
enum State { WAIT, READY, ACTIVE, DRAIN, HALT };
enum Reason {
    R_EARLY=1, R_LOST=2, R_NO_BOTTOM=4, R_REPEAT=8,
    R_CONFLICT=16, R_TIMEOUT=32, R_GAP=64, R_EXHAUSTED=128
};
enum Fault { F_CONFIG=1, F_WATCHDOG=2, F_COUNTER=4 };
typedef struct {
    uint8_t candidate;
    uint8_t stable;
    uint8_t valid;
    uint32_t since;
} Contact;
typedef struct {
    uint32_t now;
    uint32_t raw;
    uint32_t mask;
    uint32_t qualified;
    uint32_t state;
    uint32_t fault;
    uint32_t starts;
    uint32_t good;
    uint32_t bad;
    uint32_t aborted;
    uint32_t extra;
    uint32_t gaps;
    uint32_t tx_errors;
    uint32_t reason;
    uint32_t elapsed;
    uint32_t bottoms;
    uint32_t last_reason;
    uint32_t last_duration;
    uint32_t seat_ms;
    uint32_t empty_ms;
    uint32_t begun;
    uint32_t initialized;
    Contact contact[4];
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
