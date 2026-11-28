/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Every pair is considered. Repeated visits never act as new identities. */
bool engine_witness(const Engine *e, unsigned *a, unsigned *b)
{
    if (!a || !b)
        return false;
    unsigned minimum = 5;
    bool found = false;
    for (unsigned first = 0; first < 16; ++first) {
        if (!(e->settings & (1u << first)))
            continue;
        for (unsigned second = first + 1; second < 16; ++second) {
            if (!(e->settings & (1u << second)))
                continue;
            unsigned distance = engine_popcount(first ^ second);
            if (distance < minimum) {
                minimum = distance;
                *a = first;
                *b = second;
                found = true;
            }
        }
    }
    return found;
}

unsigned engine_distance(const Engine *e)
{
    unsigned a, b;
    if (!engine_witness(e, &a, &b))
        return 5; /* Undefined, NOT five-bit protection on four channels. */
    return engine_popcount(a ^ b);
}

unsigned engine_candidates(const Engine *e, unsigned reading)
{
    if (reading > 15)
        return 0;
    /* A radius-one ball is exactly the word plus its four neighbors.
     * This formulation keeps the telemetry path bounded and cheap. */
    unsigned ball = 1u << reading;
    for (unsigned bit = 0; bit < 4; ++bit)
        ball |= 1u << (reading ^ (1u << bit));
    return ball & e->settings;
}

unsigned engine_ambiguous(const Engine *e)
{
    unsigned result = 0;
    for (unsigned reading = 0; reading < 16; ++reading) {
        unsigned candidates = engine_candidates(e, reading);
        /* Clear the lowest bit: nonzero remainder means multiple candidates.
         * Include exact codewords if another code is one bit away. */
        if (candidates && (candidates & (candidates - 1)))
            result |= 1u << reading;
    }
    return result;
}
