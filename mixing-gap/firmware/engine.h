/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef MIXING_ENGINE_H
#define MIXING_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define HOLD_MS 100u

/* Fault values are exclusive terminal causes, not combinable flags. */
enum { F_NONE, F_CONFIG, F_WATCHDOG, F_INPUT, F_EXHAUST };
typedef struct {
    uint32_t now;
    uint32_t samples;
    uint32_t gaps;
    uint32_t missed;
    uint32_t tx_errors;
    uint32_t visits;
    uint32_t coverage;
    uint32_t qualified_ms;
    uint32_t unknown_ms;
    uint16_t settings;
    uint16_t hold;
    uint8_t raw;
    uint8_t credited;
    uint8_t seeded;
    uint8_t fault;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint8_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
uint32_t engine_project(uint16_t settings);
uint32_t engine_relation_cell(uint8_t mask);
/* Odd splits 1..13 represent seven unordered nonempty bipartitions.
 * parents packs 16*left_word+right_word; 256 means no missing witness. */
unsigned engine_missing(const Engine *e, unsigned split);
unsigned engine_parents(const Engine *e, unsigned split, unsigned word);
unsigned engine_complete(const Engine *e);
unsigned engine_missing_total(const Engine *e);
unsigned engine_popcount(uint32_t value);
unsigned engine_suggest(const Engine *e);
#endif
