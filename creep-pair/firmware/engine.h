/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CREEP_ENGINE_H
#define CREEP_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define PERIOD_MS 20u
#define CHANNELS 4u
#define HOLD_MIN 60000u
#define HOLD_MAX 180000u
#define RECOVERY_MS 60000u
#define BASELINE_SAMPLES 50u
#define QUALIFY 5u
#define NEVER UINT32_MAX
enum State { BASELINE, READY, LOADED, RECOVERY, DONE, INVALID };
enum Fault { F_RANGE=1, F_GAP=2, F_ADC=4, F_TX=8, F_WATCHDOG=16 };
typedef struct {
    enum State state;
    uint32_t baseline;
    uint32_t sum;
    uint32_t low;
    uint32_t high;
    uint32_t hold_ms;
    uint32_t recovery_ms;
    uint32_t loaded[3];
    uint32_t recovered[3];
    uint32_t residual;
    uint8_t baseline_count;
    uint8_t streak;
    uint8_t candidate;
    uint8_t contact;
    uint8_t milestones;
    uint8_t recovery_marks;
} Lane;
typedef struct {
    uint32_t now;
    uint32_t previous;
    uint16_t adc[4];
    uint32_t ohms[2];
    Lane lane[2];
    uint8_t faults;
    bool initialized;
    bool acquired;
} Engine;
uint32_t resistance(uint16_t adc);
void engine_init(Engine *e);
void engine_fault(Engine *e,uint8_t flag);
void engine_sample(Engine *e,uint32_t now,const uint16_t adc[4],bool valid);
bool engine_valid(const Engine *e);
#endif
