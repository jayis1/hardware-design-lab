/*
 * touch.h — Capacitive button sense (single GPIO + RC discharge)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_TOUCH_H
#define GLYPHFLOW_TOUCH_H

#include <stdint.h>

int    touch_init(void);
uint8_t touch_read(void);             /* 1 if pressed, 0 if not */
uint8_t touch_debounce(void);         /* debounced press, 1 on rising edge */
void   touch_reset(void);

#endif /* GLYPHFLOW_TOUCH_H */