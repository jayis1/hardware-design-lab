/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RAIL_ENGINE_H
#define RAIL_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define MAX_TRAVEL_MS 60000u
enum State { WAIT, READY, ACTIVE, HALTED };
enum Fault { F_GAP=1, F_CODE=2, F_OVERFLOW=4, F_CONFIG=8, F_WATCHDOG=16 };
/* Each rail uses a thermometer code: 00, 01, 11. Never 10.
 * Bins index 3*direction + abs(left_stage-right_stage).
 * Direction zero opens (origin 0); direction one closes (origin 15).
 * Evidence includes abandoned traverses and excludes unsettled intervals. */
typedef struct {
    uint32_t time;
    uint32_t starts;
    uint32_t completed[2];
    uint32_t aborted;
    uint32_t active_ms;
    uint32_t bins[6];
    uint32_t last_ms;
    uint32_t gaps;
    uint32_t tx;
    uint8_t raw;
    uint8_t levels;
    uint8_t known;
    uint8_t candidate;
    uint8_t stable[2];
    uint8_t state;
    uint8_t anchor; /* 0 none, 1 closed, 2 open */
    uint8_t peak;
    uint8_t last_peak;
    uint8_t last_dir;
    uint8_t flags;
    bool sampled;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t time, uint8_t raw);
void engine_fault(Engine *e, uint8_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
