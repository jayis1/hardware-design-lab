/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef BELLOWS_ENGINE_H
#define BELLOWS_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define MAX_STROKE_MS 60000u
enum State { WAIT, READY, ACTIVE, HALTED };
enum Fault { F_GAP=1, F_ENDPOINTS=2, F_OVERFLOW=4, F_CONFIG=8, F_WATCHDOG=16 };
/* Bins are observed millisecond intervals, not flow or pressure.
 * Index: 4*direction + valve mask; direction 0 compression, 1 expansion.
 * Valve mask: 0 neither, 1 inlet only, 2 outlet only, 3 both. */
typedef struct {
    uint32_t time;
    uint32_t starts;
    uint32_t completed[2];
    uint32_t aborted;
    uint32_t active_ms;
    uint32_t bins[8];
    uint32_t last_ms;
    uint32_t gaps;
    uint32_t tx;
    uint8_t raw;
    uint8_t levels;
    uint8_t known;
    uint8_t candidate;
    uint8_t stable[4];
    uint8_t state;
    uint8_t anchor;
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
