/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FLOAT_ENGINE_H
#define FLOAT_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define LIMIT 1000000000u
#define QUALIFY 5u
#define UNKNOWN 4u
/* Faults are sticky and terminal for acquisition, not for transmission. */
enum { F_CONFIG=1, F_WATCHDOG=2, F_GAP=4, F_OVERFLOW=8 };
typedef struct {
    uint32_t sequence, now, fault, edges;
    uint32_t bins[2][5]; /* open/close x below, low-mid, mid-high, above, unknown */
    uint32_t noise, invalid_ticks, skips, reversals, gaps, tx_errors;
    uint8_t raw, refs, ref_candidate, ref_run, band, direction;
    uint8_t dut, dut_candidate, dut_run;
    uint8_t captured, tainted;
    bool started, ref_valid, dut_valid, envelope;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_fault(Engine *e, uint32_t fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
