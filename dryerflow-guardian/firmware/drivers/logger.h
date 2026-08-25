/* Author: jayis1 */
#ifndef DFG_LOGGER_H
#define DFG_LOGGER_H
#include <stddef.h>
#include "../board.h"
void logger_init(void);
void logger_push_sample(const dfg_sensor_frame_t *frame);
void logger_finalize_cycle(const dfg_sensor_frame_t *history, uint32_t count,
                           const dfg_health_metrics_t *metrics,
                           dfg_cycle_record_t *record);
size_t logger_render_alerts(uint32_t alerts, char *buffer, size_t buffer_size);
#endif
