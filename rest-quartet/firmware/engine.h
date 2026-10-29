/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef REST_ENGINE_H
#define REST_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define CHANNELS 4u
#define QUALIFY_MS 8u
#define GOAL_MS 60000u
#define LIMIT_MS 3600000u
enum { WAIT, READY, OCCUPIED };
enum { F_OVERFLOW=1, F_CONFIG=2, F_WATCHDOG=4 };
typedef struct {
    uint32_t state;
    uint32_t starts;
    uint32_t early;
    uint32_t rested;
    uint32_t aborted;
    uint32_t age;
    uint32_t last;
    uint32_t minimum;
    uint32_t maximum;
    uint32_t total;
    uint32_t timeouts;
} Slot;
typedef struct {
    uint8_t candidate;
    uint8_t stable;
    uint8_t age;
    bool known;
} Contact;
typedef struct {
    uint32_t now;
    uint32_t gaps;
    uint32_t tx;
    uint32_t fault;
    bool sampled;
    Slot slot[CHANNELS];
    Contact contact[CHANNELS];
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t mask);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
