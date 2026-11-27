/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

static bool valid_split(unsigned split)
{
    /* A always lies on the left. Complement duplicates are not displayed. */
    return split > 0 && split < 15 && (split & 1u);
}

unsigned engine_missing(const Engine *e, unsigned split)
{
    if (!valid_split(split))
        return 0;
    /* Pack projection membership, then test at most sixteen candidates.
     * Avoid an observed-parent Cartesian loop in the periodic serial path. */
    unsigned left = 0, right = 0;
    for (unsigned word = 0; word < 16; ++word) {
        if (!(e->settings & (1u << word)))
            continue;
        left |= 1u << (word & split);
        right |= 1u << (word & (15u ^ split));
    }
    unsigned product = 0;
    for (unsigned word = 0; word < 16; ++word) {
        if ((left & (1u << (word & split))) &&
            (right & (1u << (word & (15u ^ split)))))
            product |= 1u << word;
    }
    /* Missing is absence from retained evidence, NOT mechanical impossibility.
     * New projections can create new holes while other holes are filled. */
    return product & ~e->settings & 65535u;
}

unsigned engine_parents(const Engine *e, unsigned split, unsigned word)
{
    if (!valid_split(split) || word > 15 || (e->settings & (1u << word)))
        return 256;
    for (unsigned a = 0; a < 16; ++a) {
        if (!(e->settings & (1u << a)) || (a & split) != (word & split))
            continue;
        for (unsigned b = 0; b < 16; ++b) {
            if (!(e->settings & (1u << b)))
                continue;
            if ((b & (15u ^ split)) == (word & (15u ^ split)))
                return 16u * a + b;
        }
    }
    return 256;
}

unsigned engine_complete(const Engine *e)
{
    if (!e->settings)
        return 0; /* Do not label empty history as evidence of independence. */
    unsigned result = 0;
    for (unsigned split = 1; split < 15; split += 2) {
        if (!engine_missing(e, split))
            result |= 1u << (split / 2);
    }
    return result;
}

unsigned engine_missing_total(const Engine *e)
{
    unsigned total = 0;
    for (unsigned split = 1; split < 15; split += 2)
        total += engine_popcount(engine_missing(e, split));
    return total; /* Split-specific holes; the same word can count repeatedly. */
}
