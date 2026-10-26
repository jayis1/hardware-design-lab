/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef WICK_ENGINE_H
#define WICK_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define CHANNELS 4u
#define PERIOD_MS 20u
#define TRIAL_MS 600000u
#define NEVER UINT32_MAX
#define WET_MAX 1200u
#define DRY_MIN 3000u
#define QUALIFY 5u
#define BASELINE_SAMPLES 50u
enum State { BASELINE, READY, ACTIVE, COMPLETE, EXPIRED, INVALID };
enum Fault { F_ORDER=1, F_BYPASS=2, F_DRY=4, F_GAP=8, F_ADC=16, F_TX=32, F_WATCHDOG=64 };
typedef struct {
    enum State state;
    uint32_t now;
    uint32_t previous;
    uint32_t started;
    uint32_t elapsed;
    uint32_t first[CHANNELS];
    uint16_t adc[CHANNELS];
    uint8_t streak[CHANNELS];
    uint8_t candidate;
    uint8_t wet;
    uint8_t faults;
    uint8_t baseline;
    bool initialized;
} Engine;
void engine_init(Engine *e);
void engine_fault(Engine *e, uint8_t flag);
void engine_sample(Engine *e,uint32_t now,const uint16_t adc[CHANNELS],bool valid);
bool engine_valid(const Engine *e);
#endif
