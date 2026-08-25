/* Author: jayis1 */
#ifndef DFG_AIRFLOW_H
#define DFG_AIRFLOW_H
#include "../board.h"
void airflow_init(void);
void airflow_estimate(dfg_sensor_frame_t *frame, const dfg_baseline_t *baseline);
#endif
