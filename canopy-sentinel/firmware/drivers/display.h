/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_DISPLAY_H
#define CANOPY_SENTINEL_DISPLAY_H

#include "../board.h"

void display_init(void);
void display_show_boot(const cs_device_state_t *device);
void display_show_result(const cs_scan_result_t *result, const cs_power_state_t *power);
void display_show_storage(const cs_log_t *log);

#endif
