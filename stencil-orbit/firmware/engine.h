/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ORBIT_ENGINE_H
#define ORBIT_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define ORBIT_STATIONS 4u
#define ORBIT_STEPS 16u
#define ORBIT_DEBOUNCE 5u
#define ORBIT_MAX_EPISODE 60000u
/* Input bits denote closures, not electrical high levels. */
#define INPUT_REGISTERED 4u
#define INPUT_PARKED 8u
#define FLAG_READY 1u
#define FLAG_KNOWN 2u
#define FLAG_ACTIVE 4u
#define FLAG_BAD_EPISODE 8u
#define FLAG_OVERDUE 16u
/* Counters saturate; modulo position never implies an absolute home sensor. */
typedef struct {
    uint32_t now;
    uint32_t last_sample;
    uint32_t candidate_age;
    uint32_t episode_ms;
    uint32_t last_duration;
    uint32_t accepted;
    uint32_t rejected;
    uint32_t repeats;
    uint32_t skips;
    uint32_t quad_faults;
    uint32_t motion_faults;
    uint32_t registration_faults;
    uint32_t gaps;
    uint32_t transport_losses;
    uint32_t station_counts[ORBIT_STATIONS];
    uint8_t levels;
    uint8_t candidate;
    uint8_t position;
    uint8_t episode_station;
    uint8_t last_station;
    bool sampled;
    bool ready;
    bool known;
    bool home_attempted;
    bool active;
    bool episode_bad;
    bool overdue;
    bool last_station_valid;
} Engine;
void engine_init(Engine *e, uint32_t now);
bool engine_reset_allows_home(uint32_t reset_cause);
void engine_sample(Engine *e, uint8_t closures, uint32_t now);
void engine_transport_loss(Engine *e);
uint32_t engine_flags(const Engine *e);
#endif
