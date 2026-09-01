/*
 * CordCanary logger
 * Author: jayis1
 */

#include <stdio.h>

#include "logger.h"

void logger_init(cc_logger_t *logger)
{
    size_t i;
    logger->count = 0U;
    for (i = 0U; i < CC_HISTORY_DEPTH; ++i) {
        logger->entries[i].tick = 0U;
        logger->entries[i].state = CC_STATE_NOMINAL;
        logger->entries[i].risk_score = 0.0f;
        logger->entries[i].note[0] = '\0';
    }
}

void logger_push(cc_logger_t *logger, unsigned tick, const cc_inference_t *inf)
{
    size_t slot = logger->count % CC_HISTORY_DEPTH;
    logger->entries[slot].tick = tick;
    logger->entries[slot].state = inf->state;
    logger->entries[slot].risk_score = inf->risk_score;
    (void) snprintf(logger->entries[slot].note,
                    sizeof(logger->entries[slot].note),
                    "%.20s | %.70s",
                    cc_state_name(inf->state),
                    inf->advisory);
    logger->count += 1U;
}

void logger_print(const cc_logger_t *logger)
{
    size_t start = logger->count > CC_HISTORY_DEPTH ? logger->count - CC_HISTORY_DEPTH : 0U;
    size_t end = logger->count;
    size_t idx;

    puts("\nEvent log:");
    puts("tick,state,risk,note");
    for (idx = start; idx < end; ++idx) {
        const cc_log_entry_t *entry = &logger->entries[idx % CC_HISTORY_DEPTH];
        printf("%u,%s,%.2f,%s\n",
               entry->tick,
               cc_state_name(entry->state),
               entry->risk_score,
               entry->note);
    }
}
