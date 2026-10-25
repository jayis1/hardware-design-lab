/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RC_ENGINE_H
#define RC_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 3u
#define CLEAR_MS 100u
#define FLIGHT_MS 30000u
#define BLOCK_MS 2000u
#define HISTOGRAM_BINS 6u
/* Every session begins unarmed. Clear means qualified clear, not raw clear. */
typedef enum { DRAIN, READY, FLIGHT, SETTLE, FAULT } State;
typedef enum { NONE, OVERLAP, SIMULTANEOUS, MULTIPLE, EXPIRED, GAP, BLOCKED, LIMIT } Reason;
typedef struct {
    bool candidate;
    bool stable;
    uint32_t age;
    uint32_t blocked_ms;
} Gate;
typedef struct {
    uint32_t count;
    uint32_t minimum;
    uint32_t maximum;
    uint64_t sum;
    uint32_t bins[HISTOGRAM_BINS];
} Route;
typedef struct {
    uint32_t time;
    uint32_t start;
    uint32_t clear_ms;
    uint32_t entries;
    uint32_t accepted;
    uint32_t rejected;
    uint32_t orphans;
    uint32_t gaps;
    uint32_t last_ms;
    uint32_t last_route;
    uint32_t block_events;
    uint32_t fault_mask;
    uint32_t pending_route;
    uint32_t pending_ms;
    uint8_t levels;
    State state;
    Reason reason;
    Gate gate[4];
    Route route[3];
} Engine;
void engine_init(Engine *e, uint32_t now, uint8_t raw);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e);
uint32_t engine_flags(const Engine *e);
uint32_t route_mean(const Route *r);
unsigned route_bin(uint32_t duration);
bool engine_consistent(const Engine *e);
#endif
