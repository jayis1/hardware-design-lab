/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef MERGE_ENGINE_H
#define MERGE_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUEUE_CAP 8u
#define QUALIFY 8u
#define ARM_MS 100u
#define TIMEOUT_MS 5000u
#define COUNTER_MAX 1000000000u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_OVERFLOW 4u
#define F_INTERNAL 8u
enum State { WAIT, ARMED, HALT };
typedef struct {
    uint32_t now, admitted, left, right, abandoned, rejected;
    uint32_t alternations, longest, streak, latency_sum, latency_max;
    uint32_t segments, ambiguous, orphans, full, timeouts, gaps, tx, fault;
    uint32_t entered[QUEUE_CAP];
    uint8_t tags[QUEUE_CAP];
    uint8_t state, mask, known, candidate, stable[4], pending, head, last;
    uint16_t arm_ms;
    bool initialized, clear_ready;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
