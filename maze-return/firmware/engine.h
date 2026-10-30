/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef MAZE_ENGINE_H
#define MAZE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define COUNT_LIMIT 1000000000u
#define QUALIFY_MS 5u
#define CLEAR_MS 100u
#define TIMEOUT_MS 30000u
#define MAX_GAP_MS 5u
enum State { WAIT, ARMED, CAPTURE, HALT };
enum Fault { F_CONFIG=1, F_WATCHDOG=2, F_COUNTER=4, F_GAP=8,
             F_AMBIGUOUS=16, F_TIMEOUT=32 };
typedef struct {
    uint32_t since;
    uint8_t candidate, stable, valid;
} Contact;
typedef struct {
    uint32_t now, raw, mask, qualified, state, fault;
    uint32_t visits, added, loops, repeats, erased, gaps, tx_errors;
    uint32_t depth, path[4], pending, clear_ms, elapsed;
    uint32_t initialized, begun;
    Contact contact[4];
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
uint32_t engine_route(const Engine *e);
#endif
