/*
 * storage.h — SD card / FatFS storage interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_STORAGE_H
#define MUONSCAPE_DRV_STORAGE_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

muon_status_t storage_init(void);
uint64_t      storage_capacity_bytes(void);
uint64_t      storage_free_bytes(void);
int           storage_exists(const char *path);
muon_status_t storage_list(const char *dir, char *out, uint16_t max);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_STORAGE_H */