/* Author: jayis1 */
#ifndef DFG_ACOUSTIC_H
#define DFG_ACOUSTIC_H
#include "../board.h"
void acoustic_init(void);
void acoustic_sample(dfg_sensor_frame_t *frame, uint32_t tick);
#endif
