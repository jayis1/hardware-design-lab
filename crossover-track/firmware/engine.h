/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CROSSOVER_ENGINE_H
#define CROSSOVER_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 10u
#define BASELINE_MS 100u
#define MAX_SKEW_MS 2000u
#define MAX_TRIAL_MS 30000u
#define UNKNOWN UINT32_MAX
typedef enum { WAITING, ARMED, RUNNING } Phase;
typedef enum { NONE, ORDER, RETURNED, SKEW, TIMEOUT, GAP, LINK, SATURATED } Reason;
typedef struct {
    uint32_t id;
    bool ok;
    Reason reason;
    uint32_t start[2];
    uint32_t finish[2];
} Result;
typedef struct {
    uint32_t now;
    uint32_t trial;
    uint32_t good;
    uint32_t bad;
    uint32_t gaps;
    uint32_t loss;
    Phase phase;
    uint8_t levels;
    uint8_t candidates;
    uint8_t qualified;
    uint32_t stable[4];
    uint32_t baseline;
    uint32_t origin;
    uint32_t start[2];
    uint32_t finish[2];
    bool started[2];
    bool finished[2];
    Result last;
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_transport_loss(Engine *e);
#endif
