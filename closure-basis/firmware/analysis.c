/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

unsigned engine_support(const Engine *e, unsigned antecedent)
{
    if (antecedent > 15)
        return 0;
    unsigned count = 0;
    for (unsigned word = 0; word < 16; ++word) {
        if ((e->settings & (1u << word)) && (word & antecedent) == antecedent)
            ++count;
    }
    return count;
}

unsigned engine_closure(const Engine *e, unsigned antecedent)
{
    /* The intersection of matching observed configurations contains every
     * consequence currently supported by this antecedent. Empty support is
     * exposed as unknown instead of the mathematical vacuous closure 15. */
    if (antecedent > 15)
        return 16;
    unsigned common = 15;
    bool seen = false;
    for (unsigned word = 0; word < 16; ++word) {
        if (!(e->settings & (1u << word)))
            continue;
        if ((word & antecedent) != antecedent)
            continue;
        common &= word;
        seen = true;
    }
    return seen ? common : 16;
}

unsigned engine_witness(const Engine *e, unsigned antecedent, unsigned bit)
{
    /* A single configuration, not a pair, refutes a positive implication. */
    if (antecedent > 15 || bit > 3)
        return 16;
    for (unsigned word = 0; word < 16; ++word) {
        if (!(e->settings & (1u << word)))
            continue;
        if ((word & antecedent) != antecedent)
            continue;
        if (!(word & (1u << bit)))
            return word;
    }
    return 16;
}

unsigned engine_supported(const Engine *e)
{
    unsigned result = 0;
    for (unsigned antecedent = 0; antecedent < 16; ++antecedent) {
        if (engine_support(e, antecedent))
            result |= 1u << antecedent;
    }
    return result;
}

unsigned engine_minimal_for(const Engine *e, unsigned bit)
{
    /* Inclusion-minimal antecedents for ONE consequent. This is not a
     * canonical stem basis and can contain transitively redundant rules. */
    if (bit > 3)
        return 0;
    unsigned candidates = 0;
    for (unsigned a = 0; a < 16; ++a) {
        if (a & (1u << bit))
            continue; /* Do not report the tautology A -> A. */
        unsigned closure = engine_closure(e, a);
        if (closure != 16 && (closure & (1u << bit)))
            candidates |= 1u << a;
    }
    unsigned result = candidates;
    for (unsigned a = 0; a < 16; ++a) {
        if (!(candidates & (1u << a)))
            continue;
        for (unsigned b = 0; b < 16; ++b) {
            if (a != b && (a & b) == b && (candidates & (1u << b))) {
                result &= ~(1u << a);
                break;
            }
        }
    }
    return result;
}

unsigned engine_rules(const Engine *e)
{
    unsigned total = 0;
    for (unsigned bit = 0; bit < 4; ++bit)
        total += engine_popcount(engine_minimal_for(e, bit));
    return total;
}
