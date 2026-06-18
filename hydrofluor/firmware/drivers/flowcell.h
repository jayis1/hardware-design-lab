/*
 * flowcell.h — HydroFluor peristaltic pump & flow cell control
 * Author: jayis1
 * License: MIT
 */
#ifndef FLOWCELL_H
#define FLOWCELL_H

#include "board.h"
#include <stdint.h>

/* Initialize pump PWM (TIM2) + enable pin. */
void flowcell_init(void);

/* Start pump at given duty cycle (0-100%). */
void flowcell_pump_start(uint8_t duty_pct);

/* Stop pump. */
void flowcell_pump_stop(void);

/* Run pump for a fixed duration to prime / flush the flow cell.
 * Blocks for duration_ms. */
void flowcell_prime(uint32_t duration_ms);

/* Bubble detect: returns 1 if the flow-cell path is likely interrupted
 * (based on recent scatter/absorbance readings). */
uint8_t flowcell_bubble_detected(void);

/* Set the bubble-detection flag from the fluorometry acquisition result. */
void flowcell_set_bubble_flag(uint8_t b);

#endif /* FLOWCELL_H */