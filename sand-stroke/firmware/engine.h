/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SAND_ENGINE_H
#define SAND_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 10u
#define MAX_STROKE_MS 10000u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_OVERFLOW 4u
#define F_INPUT 8u
#define LEFT 1u
#define RIGHT 2u
#define ENGAGED 4u
#define FACE_B 8u
typedef enum { WAIT, READY, ACTIVE, HALT } State;
typedef struct {
    uint8_t candidate;
    uint8_t age;
    uint8_t stable;
    bool known;
} Contact;
typedef struct {
    uint32_t now;
    uint32_t state;
    uint32_t mask;
    uint32_t known;
    uint32_t anchor;
    uint32_t face;
    uint32_t elapsed;
    uint32_t started;
    uint32_t full_a;
    uint32_t full_b;
    uint32_t aborted;
    uint32_t ms_a;
    uint32_t ms_b;
    uint32_t short_returns;
    uint32_t lifts;
    uint32_t flips;
    uint32_t collisions;
    uint32_t timeouts;
    uint32_t gaps;
    uint32_t tx;
    uint32_t fault;
    uint32_t departed;
    Contact contact[4];
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t mask);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
