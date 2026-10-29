/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FLOW_ENGINE_H
#define FLOW_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define QUALIFY_MS 8u
#define SILENCE_MS 5000u
#define MAX_PULSE_MS 2000u
#define CHANNELS 4u
enum { WAIT, READY, WATCH, PAUSED, HALT };
enum { F_OVERFLOW=1, F_CONFIG=2, F_WATCHDOG=4 };
typedef struct {
    uint8_t candidate, stable, age;
    bool known;
} Contact;
typedef struct {
    uint32_t now, gaps, tx, fault;
    uint32_t state, mask, known;
    uint32_t runs, finished, aborted;
    uint32_t pulses, bad_pulses, pauses, recovered, assisted, abandoned;
    uint32_t taps, silent, pause_age, pause_total, pause_max, episode_taps;
    uint32_t pulse_age;
    bool sampled, pulse_active;
    Contact contact[CHANNELS];
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t mask);
void engine_fault(Engine *e, uint32_t reason);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
