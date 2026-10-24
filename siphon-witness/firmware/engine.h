/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SW_ENGINE_H
#define SW_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define DEBOUNCE_MS 30u
#define READY_MS 1000u
#define TRIAL_LIMIT_MS 120000u
#define UNKNOWN_MS UINT32_MAX
/* Bit order follows J2 A0,A1,A2,A3. A closed reed means submerged. */
enum { SOURCE_LOW = 1, SOURCE_HIGH = 2, RECEIVER_LOW = 4, RECEIVER_HIGH = 8 };
typedef enum {
    PH_SETTLE, PH_READY, PH_ARMED, PH_TRANSFER, PH_COMPLETE, PH_FAULT
} Phase;
enum {
    FL_TOPOLOGY = 1, FL_REVERSE = 2, FL_TIMEOUT = 4,
    FL_GAP = 8, FL_SKIPPED = 16, FL_TRANSPORT = 32
};
typedef struct {
    bool initialized;
    bool qualified;
    uint8_t candidate;
    uint8_t mask;
    uint32_t candidate_ms;
    uint32_t last_ms;
    uint32_t ready_ms;
    uint32_t elapsed_ms;
    uint32_t drain_age;
    uint32_t fill_age;
    uint32_t drain_ms;
    uint32_t fill_ms;
    bool draining;
    bool filling;
    Phase phase;
    uint32_t flags;
    uint32_t trial;
    uint32_t cycles;
    uint32_t failed;
    uint32_t gaps;
    uint32_t drops;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_transport_loss(Engine *e);
uint32_t engine_sat_add(uint32_t a, uint32_t b);
bool engine_active(const Engine *e);
#endif
