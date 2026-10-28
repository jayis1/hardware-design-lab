/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TILT_ENGINE_H
#define TILT_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define PERIOD_MS 20u
#define ZERO_SAMPLES 50u
#define MAX_RAMP_MS 120000u
#define MAX_SLIDE_MS 10000u
typedef enum { WAIT, ZERO, READY, RAMP, SLIDE, DONE, INVALID } State;
enum {
    F_RANGE=1, F_GAP=2, F_ADC=4, F_TX=8, F_WATCHDOG=16,
    F_ORDER=32, F_REVERSE=64, F_SPEED=128, F_TIMEOUT=256, F_CANCEL=512
};
typedef struct {
    bool level, candidate, known;
    unsigned count;
    uint32_t since;
} Contact;
typedef struct {
    State state;
    uint32_t faults, now, last, start, release_ms, transit_ms, elapsed;
    uint16_t adc[4];
    bool valid, sampled, released, have_release;
    Contact contacts[3];
    uint32_t zero_count, zero_min, zero_max, baseline;
    int32_t height, previous, maximum, last_home, candidate_lo, candidate_hi;
    int32_t release_lo, release_hi;
} Engine;
void engine_init(Engine *e);
void engine_fault(Engine *e,uint32_t fault);
void engine_sample(Engine *e,uint32_t now,const uint16_t raw[4],bool valid);
uint32_t engine_resistance(uint16_t adc);
bool engine_consistent(const Engine *e);
#endif
