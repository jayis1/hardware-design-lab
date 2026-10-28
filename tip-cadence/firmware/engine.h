/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TC_ENGINE_H
#define TC_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define QUALIFY_MS 5u
#define MIN_PULSE_MS 10u
#define MAX_PULSE_MS 2000u
#define MIN_PERIOD_MS 100u
#define MAX_PERIOD_MS 60000u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_OVERFLOW 4u
#define F_INPUT 8u
typedef enum { WAIT_CLEAR, READY, PULSE } Phase;
typedef struct {
    uint8_t candidate, stable, age;
} Filter;
typedef struct {
    uint32_t tips, intervals, sum[2], bins[2];
    uint32_t minimum, maximum, jitter, previous_period;
    uint32_t rejected, breaks, pulse_age, since_tip;
    bool anchored, handled;
    uint8_t parity;
    Phase phase;
} Lane;
typedef struct {
    Lane lane[2];
    Filter filter[2];
    uint32_t now, gaps, tx_errors, faults;
    uint8_t raw;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
