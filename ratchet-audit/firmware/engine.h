/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RA_ENGINE_H
#define RA_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define TRIAL_LIMIT 30000u
#define EDGE_LIMIT 1000000u
#define F_CONFIG 1u
#define F_WATCHDOG 2u
#define F_QUADRATURE 4u
#define F_TIMEOUT 8u
#define F_GAP 16u
#define F_OVERFLOW 32u
#define F_TX 64u
enum Phase { WAIT, READY, ACTIVE, HALTED };
typedef struct {
    uint8_t candidate;
    uint8_t accepted;
    uint8_t count;
} Qualifier;
typedef struct {
    uint32_t now;
    uint32_t starts;
    uint32_t completed;
    uint32_t aborted;
    uint32_t duration;
    uint32_t forward;
    uint32_t reverse;
    uint32_t seated_reverse;
    uint32_t rollback;
    uint32_t faults;
    uint32_t gaps;
    uint32_t tx_errors;
    int32_t position;
    int32_t peak;
    uint16_t baseline;
    uint8_t mask;
    enum Phase phase;
    bool initialized;
    bool encoder_anchored;
    Qualifier quadrature;
    Qualifier home;
    Qualifier pawl;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
