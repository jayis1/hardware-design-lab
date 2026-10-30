/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TEARDOWN_ENGINE_H
#define TEARDOWN_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define CHANNELS 4u
#define QUALIFY_MS 10u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_OVERFLOW 4u
#define F_INPUT 8u
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
    uint32_t depth;
    uint32_t stack[CHANNELS];
    uint32_t started;
    uint32_t completed;
    uint32_t aborted;
    uint32_t pushes;
    uint32_t pops;
    uint32_t abandoned;
    uint32_t order_errors;
    uint32_t collisions;
    uint32_t gaps;
    uint32_t tx;
    uint32_t fault;
    Contact contact[CHANNELS];
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t mask);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
