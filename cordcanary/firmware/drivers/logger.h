/*
 * CordCanary logger
 * Author: jayis1
 */

#ifndef CORDCANARY_LOGGER_H
#define CORDCANARY_LOGGER_H

#include "../board.h"

void logger_init(cc_logger_t *logger);
void logger_push(cc_logger_t *logger, unsigned tick, const cc_inference_t *inf);
void logger_print(const cc_logger_t *logger);

#endif
