/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef LAY_ENGINE_H
#define LAY_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define MAX_TRAVERSE_MS 600000u
enum State { WAIT, READY, ACTIVE, HALTED };
enum Fault {
    F_GAP=1, F_ENDPOINTS=2, F_OVERFLOW=4, F_CONFIG=8, F_WATCHDOG=16
};
typedef struct {
    uint32_t time;
    uint32_t starts;
    uint32_t completed;
    uint32_t aborted;
    uint32_t pulses;
    uint32_t abandoned;
    uint32_t completed_pulses;
    uint32_t active_pulses;
    uint32_t active_ms;
    uint32_t last_pulses;
    uint32_t last_ms;
    uint32_t bad_total;
    uint32_t gaps;
    uint32_t tx;
    uint8_t raw;
    uint8_t levels;
    uint8_t known;
    uint8_t candidate;
    uint8_t stable[4];
    uint8_t state;
    uint8_t anchor;
    uint8_t active_bad;
    uint8_t last_bad;
    uint8_t last_end;
    uint8_t flags;
    bool sampled;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t time, uint8_t raw);
void engine_fault(Engine *e, uint8_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
