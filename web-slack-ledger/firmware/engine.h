/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef WEB_SLACK_ENGINE_H
#define WEB_SLACK_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    int32_t position;
    uint32_t forward;
    uint32_t backward;
    uint32_t invalid;
    uint32_t edges;
    uint8_t previous;
} Axis;
typedef struct {
    Axis axis[2];
    uint32_t time;
    uint32_t gaps;
    uint32_t reversals;
    uint32_t idle_ms;
    uint32_t leg_ms;
    uint32_t previous_leg_ms;
    int64_t origin;
    int64_t leg_min;
    int64_t leg_max;
    int64_t leg_peak;
    int8_t direction;
    bool saturated;
} Engine;
void engine_init(Engine *e, uint32_t time, uint8_t raw);
void engine_sample(Engine *e, uint32_t time, uint8_t raw);
int64_t engine_difference(const Engine *e);
bool engine_valid(const Engine *e);
uint32_t engine_flags(const Engine *e);
#endif
