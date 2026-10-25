/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PEDAL_ENGINE_H
#define PEDAL_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define KEY_A 1u
#define KEY_B 2u
#define P_REST 4u
#define P_DOWN 8u
#define DEBOUNCE_MS 8u
#define MAX_SAMPLE_MS 4u
#define TRIAL_MS 5000u
#define FLAG_SIMULTANEOUS 1u
#define FLAG_CONTRADICTION 2u
#define FLAG_TIMEOUT 4u
#define FLAG_GAP 8u
#define FLAG_KEY_RELEASE 16u
#define FLAG_REATTACK 32u
#define FLAG_UNQUALIFIED 64u
#define RESULT_NONE 0u
#define RESULT_COMPLETE 1u
#define RESULT_REJECTED 2u
/* Baseline is mandatory after every trial, including failed trials. */
typedef enum { WAIT_BASELINE, READY, CAPTURE } Phase;
typedef struct {
    uint32_t id;
    uint32_t outcome;
    uint32_t flags;
    int32_t key_gap;
    int32_t release_delta;
    int32_t down_delta;
} Result;
typedef struct {
    uint32_t now;
    uint32_t previous;
    uint32_t candidate_since[4];
    uint32_t started;
    uint32_t a_off;
    uint32_t b_on;
    uint32_t pedal_rest;
    uint32_t pedal_down;
    uint32_t attempts;
    uint32_t completed;
    uint32_t rejected;
    uint32_t gaps;
    uint32_t transport_losses;
    uint32_t flags;
    uint8_t levels;
    uint8_t candidate;
    uint8_t qualified_mask;
    bool qualified;
    bool seen_a_off;
    bool seen_b_on;
    bool seen_rest;
    bool seen_down;
    Phase phase;
    Result last;
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint32_t now, uint8_t closed);
void engine_transport_loss(Engine *e);
uint32_t engine_flags(const Engine *e);
#endif
