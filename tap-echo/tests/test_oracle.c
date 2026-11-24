/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <assert.h>
#include <stdio.h>
static uint32_t tick;
static void hold(Engine *e, unsigned mask, unsigned ms)
{
    while (ms--) {
        engine_sample(e, ++tick, mask);
        assert(engine_valid(e));
    }
}
static void phrase(Engine *e, unsigned word, unsigned spacing, unsigned mode)
{
    hold(e, mode, 10);
    hold(e, mode | 8, 10);
    for (unsigned i = 0; i < 3; ++i) {
        unsigned pad = (word & (1u << i)) ? 2 : 1;
        hold(e, mode | 8 | pad, 20);
        hold(e, mode | 8, spacing - 20);
    }
    hold(e, mode, 10);
}
int main(void)
{
    unsigned checked = 0;
    for (unsigned reference = 0; reference < 8; ++reference) {
        for (unsigned attempt = 0; attempt < 8; ++attempt) {
            for (unsigned spacing = 129; spacing <= 131; ++spacing) {
                Engine e; engine_init(&e); tick = 0;
                phrase(&e, reference, 100, 4);
                phrase(&e, attempt, spacing, 0);
                unsigned wrong = 0;
                for (unsigned i = 0; i < 3; ++i) {
                    if (((reference ^ attempt) & (1u << i)) || (i && spacing > 130))
                        ++wrong;
                }
                assert(e.reference_count == 3 && e.count == 3);
                assert(e.wrong == wrong);
                assert(e.matches == (wrong == 0));
                assert(e.max_error == spacing - 100);
                ++checked;
            }
        }
    }
    /* End just before the deadline versus exactly on its rejecting sample. */
    for (unsigned endpoint = DEADLINE - 1; endpoint <= DEADLINE; ++endpoint) {
        Engine e; engine_init(&e); tick = 0;
        phrase(&e, 0, 100, 4);
        hold(&e, 0, 10); hold(&e, 8, 10);
        hold(&e, 9, 20); hold(&e, 8, 80);
        hold(&e, 9, 20); hold(&e, 8, 20);
        hold(&e, 8, endpoint - 6 - e.age);
        hold(&e, 0, 6);
        assert(e.trials == (endpoint < DEADLINE));
        assert(e.timeouts == (endpoint == DEADLINE));
    }
    printf("PASS %u independent phrase/tolerance comparisons and deadline boundaries\n", checked);
}
