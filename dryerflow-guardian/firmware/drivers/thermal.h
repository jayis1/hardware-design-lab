/* Author: jayis1 */
#ifndef DFG_THERMAL_H
#define DFG_THERMAL_H
#include "../board.h"
void thermal_init(void);
void thermal_sample(dfg_sensor_frame_t *frame, uint32_t tick);
float thermal_estimate_dryness_minutes(const dfg_sensor_frame_t *history, uint32_t count);
#endif
