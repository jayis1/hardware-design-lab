/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ECHO_ENGINE_H
#define ECHO_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY 5u
#define DEADLINE 30000u
#define CAPACITY 16u
#define PAD_MASK 3u
#define RECORD_MASK 4u
#define GATE_MASK 8u
enum { F_CONFIG=1, F_WATCHDOG=2, F_TIME=4, F_OVERFLOW=8, F_INPUT=16 };
enum { RESULT_NONE, RESULT_REFERENCE, RESULT_MATCH, RESULT_MISMATCH, RESULT_ABORT };
typedef struct { uint32_t pad, interval; } Event;
typedef struct {
    uint32_t now, samples, missed, tx_errors, fault;
    uint32_t raw, qualified, valid, armed, active, recording;
    uint32_t references, trials, matches, aborted, timeouts;
    uint32_t reference_count, count, result, max_error, wrong, age;
    Event reference[CAPACITY], events[CAPACITY];
    /* Internal state is never serialized as application evidence. */
    uint32_t candidate, stable, started, last_onset, envelope;
    bool seeded, released;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint32_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
