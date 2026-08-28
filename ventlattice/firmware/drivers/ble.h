/* BLE formatter for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_BLE_H
#define VENTLATTICE_BLE_H

#include "../board.h"

size_t ble_build_status_packet(const vent_snapshot_t *snapshot, char *buffer, size_t buffer_size);
size_t ble_build_report_packet(const vent_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t buffer_size);

#endif
