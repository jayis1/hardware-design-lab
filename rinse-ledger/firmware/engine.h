/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RINSE_ENGINE_H
#define RINSE_ENGINE_H
#include <stdint.h>
#include <stdbool.h>
#define DEBOUNCE_MS 8u
#define MAX_GAP_MS 4u
#define BASKETS 3u
#define UNCERTAIN 1u
#define SATURATED 2u
/* Every duration is qualified-contact time, not chemical exposure. */
typedef struct {
    uint32_t total;
    uint32_t epoch_ms;
    uint32_t streak;
    uint32_t longest;
    uint32_t entries;
    uint32_t baths;
    uint32_t last_epoch;
    uint32_t flags;
} Basket;
typedef struct {
    bool candidate;
    bool level;
    bool valid;
    uint32_t age;
} Contact;
typedef struct {
    uint32_t now;
    uint32_t epoch;
    uint32_t rejected;
    uint32_t gaps;
    uint32_t losses;
    bool ready;
    uint8_t levels;
    Contact contact[4];
    Basket basket[BASKETS];
} Engine;
void engine_init(Engine *e, uint32_t now);
void engine_sample(Engine *e, uint32_t now, uint8_t raw);
void engine_transport_loss(Engine *e);
#endif
