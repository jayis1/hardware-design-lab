/*
 * command.h — JSON command parser / dispatcher
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_COMMAND_H
#define MUONSCAPE_DRV_COMMAND_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parse a JSON command string and dispatch to the appropriate handler.
 * Returns a JSON response string written to resp (max resp_len bytes).
 * Returns response length, or 0 on error.
 */
uint16_t command_dispatch(const char *cmd, uint16_t cmd_len,
                           char *resp, uint16_t resp_len);

/* Build a status JSON into resp. Returns length. */
uint16_t command_build_status(char *resp, uint16_t max);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_COMMAND_H */