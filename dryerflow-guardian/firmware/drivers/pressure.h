/* Author: jayis1 */
#ifndef DFG_PRESSURE_H
#define DFG_PRESSURE_H
#include "../board.h"
void pressure_init(void);
void pressure_sample(dfg_sensor_frame_t *frame, uint32_t tick);
#endif
