/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CAM_ENGINE_H
#define CAM_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define CAM_BINS 40u
#define CAM_COUNTS 400u
#define CAM_TIMEOUT 60000u
#define CAM_DEBOUNCE 5u
/* ACTIVE requires a witnessed rising index. No origin is inferred at boot. */
enum cam_state { CAM_WAIT, CAM_ACTIVE, CAM_FAULT };
enum cam_reason {
    CAM_NONE, CAM_REVERSE, CAM_INVALID, CAM_GAP,
    CAM_LENGTH, CAM_TIMEOUT_REASON, CAM_EXHAUSTED
};
typedef struct {
    uint32_t duration;
    uint16_t closed;
    uint16_t transitions;
    uint8_t bins[CAM_BINS];
} CamResult;
typedef struct {
    uint32_t previous;
    uint32_t start;
    uint32_t candidate_since;
    uint32_t completed;
    uint32_t rejected;
    uint32_t gaps;
    uint16_t travel;
    uint16_t closed;
    uint16_t transitions;
    uint8_t phase;
    uint8_t raw;
    uint8_t bins[CAM_BINS];
    bool initialized;
    bool index;
    bool candidate;
    bool follower;
    bool follower_ready;
    bool edge_follower;
    bool have_edge;
    enum cam_state state;
    enum cam_reason reason;
    CamResult result;
} Cam;
void cam_init(Cam *e, uint32_t now);
void cam_tick(Cam *e, uint32_t now, uint8_t raw);
void cam_fault(Cam *e);
bool cam_valid(const Cam *e);
#endif
