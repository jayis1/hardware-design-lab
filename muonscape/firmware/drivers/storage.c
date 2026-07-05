/*
 * storage.c — SD card / FatFS storage interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Wraps the FatFS library (ff.c) over SDIO. The event.c and command.c
 * layers use the fs_* shims declared there; this file provides the
 * higher-level queries (capacity, free space, listing).
 */
#include "storage.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <stdio.h>

extern void *ff_init_sdio(void);
extern int ff_mount(void);
extern uint64_t ff_capacity(void);
extern uint64_t ff_free(void);
extern int ff_exists(const char *path);
extern int ff_list_dir(const char *dir, char *out, uint16_t max);

muon_status_t storage_init(void)
{
    /* Enable SDMMC1 clock */
    RCC_AHB3ENR |= RCC_AHB3ENR_SDMMC1EN;
    /* GPIO PC8-PC12 AF12 for SDIO — omitted for brevity */
    if (ff_init_sdio() == NULL) return MUON_ERR_SD;
    if (ff_mount() != 0) return MUON_ERR_SD;
    return MUON_OK;
}

uint64_t storage_capacity_bytes(void) { return ff_capacity(); }
uint64_t storage_free_bytes(void)     { return ff_free(); }
int      storage_exists(const char *path) { return ff_exists(path); }

muon_status_t storage_list(const char *dir, char *out, uint16_t max)
{
    if (!dir || !out || max == 0) return MUON_ERR_INVALID_ARG;
    if (ff_list_dir(dir, out, max) < 0) return MUON_ERR_SD;
    return MUON_OK;
}

/* ---- FatFS weak stubs ---- */
__attribute__((weak)) void *ff_init_sdio(void) { return (void*)1; }
__attribute__((weak)) int ff_mount(void) { return 0; }
__attribute__((weak)) uint64_t ff_capacity(void) { return 32ULL * 1024 * 1024 * 1024; }
__attribute__((weak)) uint64_t ff_free(void) { return 16ULL * 1024 * 1024 * 1024; }
__attribute__((weak)) int ff_exists(const char *p) { (void)p; return 0; }
__attribute__((weak)) int ff_list_dir(const char *d, char *o, uint16_t m)
{ (void)d; if (o && m) o[0] = 0; return 0; }