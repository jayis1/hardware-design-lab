/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Return the observed binary response set for one full three-control word. */
unsigned engine_row(const Engine *e, unsigned input)
{
    if (!e || input >= 8)
        return 0;
    return (e->coverage >> (2 * input)) & 3u;
}

/* A counterexample is two retained words with equal selected controls and
 * unequal response. Return the lexicographically first pair packed as a+16*b.
 * 256 is an out-of-band sentinel; it never names an observation. */
unsigned engine_witness(const Engine *e, unsigned mask)
{
    if (!e || mask >= 8)
        return 256;
    for (unsigned a = 0; a < 16; ++a) {
        if (!(e->settings & (1u << a)))
            continue;
        for (unsigned b = a + 1; b < 16; ++b) {
            if (!(e->settings & (1u << b)))
                continue;
            if ((a >> 3) == (b >> 3))
                continue;
            if ((a & mask) == (b & mask))
                return a + 16 * b;
        }
    }
    return 256;
}

/* Bit m denotes a compatible sensor mask, not a sensor identity itself.
 * Compatibility is an empirical absence of counterexamples, never a proof
 * that omitted controls cannot matter in an unseen setting or future trial. */
unsigned engine_subsets(const Engine *e)
{
    if (!e)
        return 0;
    unsigned result = 0;
    for (unsigned mask = 0; mask < 8; ++mask) {
        if (engine_witness(e, mask) == 256)
            result |= 1u << mask;
    }
    return result;
}

/* Inclusion-minimal is not minimum cardinality. Multiple incomparable
 * explanations can survive, including explanations with different sizes.
 * In an empty history all masks survive and the empty mask is minimal. */
unsigned engine_minimal(const Engine *e)
{
    unsigned compatible = engine_subsets(e);
    unsigned result = 0;
    for (unsigned mask = 0; mask < 8; ++mask) {
        if (!(compatible & (1u << mask)))
            continue;
        bool minimal = true;
        for (unsigned sub = 0; sub < 8; ++sub) {
            if (sub == mask || (sub & mask) != sub)
                continue;
            if (compatible & (1u << sub)) {
                minimal = false;
                break;
            }
        }
        if (minimal)
            result |= 1u << mask;
    }
    return result;
}
