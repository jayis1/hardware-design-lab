/* Author: jayis1 */
#ifndef DFG_ANALYTICS_H
#define DFG_ANALYTICS_H
#include "../board.h"
void analytics_init(dfg_baseline_t *baseline);
void analytics_update(const dfg_sensor_frame_t *history, uint32_t count,
                      dfg_baseline_t *baseline,
                      dfg_health_metrics_t *metrics,
                      dfg_sensor_frame_t *current);
#endif
