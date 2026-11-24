/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef QUORUM_ENGINE_H
#define QUORUM_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
enum { F_NONE, F_TIME, F_INPUT, F_OVERFLOW, F_CONFIG, F_WATCHDOG };
/* Five clean intervals establish each input independently. Evidence starts
 * only on the interval AFTER that endpoint. No allocation or flash writes. */
typedef struct {
    uint32_t now, samples, intervals, gaps, missed, tx_errors;
    uint32_t voter_known, compare_known;
    uint32_t total[5], segments[5], peak[5], run[5];
    uint8_t raw, qualified, valid, fault;
    uint8_t candidate, age[4], started;
} Engine;
void engine_init(Engine *e);
void engine_sample(Engine *e, uint32_t now, unsigned raw);
void engine_fault(Engine *e, unsigned fault);
void engine_tx_error(Engine *e);
bool engine_valid(const Engine *e);
#endif
