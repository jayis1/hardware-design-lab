/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SHUTTER_ENGINE_H
#define SHUTTER_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define DARK_MS 250u
#define LIMIT_MS 10000u
enum State { WAIT, READY, ACTIVE, HALTED };
enum Fault { F_GAP=1, F_OVERFLOW=4, F_CONFIG=8, F_WATCHDOG=16 };
typedef struct {
    uint32_t time, starts, completed, aborted, timeouts, gaps, tx;
    uint32_t age;
    /* Published last completed capture; never replaced by an abort. */
    uint32_t span;
    uint32_t first[4], last[4], lit[4], pulses[4];
    /* Working capture, separate from retained evidence. */
    uint32_t opening[4], closing[4], exposure[4], segments[4];
    uint16_t dark;
    uint8_t raw, levels, known, candidate, stable[4];
    uint8_t state, flags, seen, working_seen;
    bool sampled;
    bool prior_dark;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t time, uint8_t raw);
void engine_fault(Engine *e, uint8_t flag);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
