/*
 * drivers/flash_lib.h — QSPI liquid-library store
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_FLASH_LIB_H
#define HYDRASCAN_FLASH_LIB_H
#include "../board.h"
#include "../classifier.h"

hydra_err_t flash_lib_init(void);
/* Load the in-RAM classifier library mirror from QSPI. */
hydra_err_t flash_lib_load(library_t *lib);
/* Append/replace a class and persist to QSPI. */
hydra_err_t flash_lib_put_class(const liquid_class_t *cls);
/* Parse and act on an app command line "L,ADD,...". */
void        library_handle_command(const char *line);
#endif