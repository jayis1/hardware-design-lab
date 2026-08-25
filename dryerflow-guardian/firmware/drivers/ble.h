/* Author: jayis1 */
#ifndef DFG_BLE_H
#define DFG_BLE_H
#include <stddef.h>
#include "../board.h"
size_t ble_encode_packet(const char *device_id,
                         const dfg_sensor_frame_t *frame,
                         const dfg_health_metrics_t *metrics,
                         char *buffer,
                         size_t buffer_size);
#endif
