/*
 * Pantry Warden logger interface
 * Author: jayis1
 */

#ifndef PANTRY_WARDEN_LOGGER_H
#define PANTRY_WARDEN_LOGGER_H

#include "../board.h"

typedef struct {
    unsigned tick;
    pw_state_t state;
    float health;
    float spoilage;
    float pest;
    float condensation;
} pw_log_entry_t;

typedef struct {
    pw_log_entry_t entries[PW_LOG_DEPTH];
    unsigned count;
} pw_logger_t;

void logger_init(pw_logger_t *logger);
void logger_push(pw_logger_t *logger, unsigned tick, const pw_inference_t *inf);
void logger_print(const pw_logger_t *logger);

#endif
