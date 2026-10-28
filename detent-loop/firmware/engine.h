/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef DETENT_ENGINE_H
#define DETENT_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define PERIOD_MS 20u
#define ZERO_SAMPLES 50u
#define OHMS_PER_MM 2222
#define SPRING_MN_PER_MM 4000
/* Physical calibration is required; these are compile-time nominal policies. */
enum State { WAIT, ZERO, READY, OUT, BACK, DONE, INVALID };
enum Fault {
    F_RANGE=1, F_GAP=2, F_ADC=4, F_TX=8, F_WATCHDOG=16,
    F_SEQUENCE=32, F_DIRECTION=64, F_SPEED=128, F_TIMEOUT=256
};
typedef struct {
    unsigned count;
    bool candidate;
    bool value;
    bool known;
} Contact;
typedef struct {
    enum State state;
    uint32_t now;
    uint32_t faults;
    bool acquired;
    bool started;
    uint16_t adc[4];
    Contact arm;
    Contact turn;
    unsigned zero_count;
    uint32_t zero_sum[2];
    uint32_t zero_low[2];
    uint32_t zero_high[2];
    uint32_t base[2];
    bool released;
    uint32_t elapsed;
    int32_t x_um;
    int32_t force_mn;
    int32_t last_x;
    int32_t last_force;
    int32_t extreme;
    int32_t span_um;
    int32_t peak_out_mn;
    int32_t peak_back_mn;
    int64_t out_nj2;
    int64_t back_nj2;
} Engine;
uint32_t engine_ohms(uint16_t counts);
void engine_init(Engine *e);
void engine_fault(Engine *e, uint32_t fault);
void engine_sample(Engine *e,uint32_t now,const uint16_t adc[4],bool valid);
#endif
