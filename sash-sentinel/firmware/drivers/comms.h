/*
 * sash-sentinel/firmware/drivers/comms.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_COMMS_H
#define SASH_SENTINEL_COMMS_H

#include "../board.h"

void comms_init(const device_config_t *config);
void comms_encode_telemetry(const device_sample_t *sample, const risk_report_t *risk, char *out, size_t out_size);
void comms_build_dashboard(const sample_history_t *history, const risk_report_t *risk, char *out, size_t out_size);
bool comms_apply_command(const char *command, device_config_t *config, char *response, size_t response_size);

#endif
