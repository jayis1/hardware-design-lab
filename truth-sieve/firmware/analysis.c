/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
/* A row is a SET of witnessed responses, not a menu of permitted guesses.
 * Two set bits contradict every deterministic memoryless truth table. */
unsigned engine_row(const Engine *e, unsigned input)
{
    if (!e || input >= 4)
        return 0;
    return (e->coverage >> (4 * input)) & 15u;
}
unsigned engine_candidates(const Engine *e)
{
    if (!e)
        return 0;
    unsigned count = 1;
    for (unsigned input = 0; input < 4; ++input) {
        unsigned row = engine_row(e, input);
        if (!row) {
            /* Any of four responses remains possible in an unobserved row. */
            count *= 4;
        } else if (row & (row - 1)) {
            return 0;
        }
    }
    return count;
}
