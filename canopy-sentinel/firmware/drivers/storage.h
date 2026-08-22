/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_STORAGE_H
#define CANOPY_SENTINEL_STORAGE_H

#include "../board.h"
#include <stddef.h>

void storage_init(cs_log_t *log);
void storage_append(cs_log_t *log, const cs_scan_result_t *result);
void storage_export_csv(const cs_log_t *log, char *buffer, size_t buffer_size);

#endif
