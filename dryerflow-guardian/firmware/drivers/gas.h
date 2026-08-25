/* Author: jayis1 */
#ifndef DFG_GAS_H
#define DFG_GAS_H
#include "../board.h"
void gas_init(void);
void gas_sample(dfg_sensor_frame_t *frame, uint32_t tick, bool gas_dryer);
#endif
