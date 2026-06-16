// led_controller.h — LED Controller Header
#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H
#include <stdint.h>

int  led_controller_init(void);
void led_set_all(uint32_t color);
void led_set_single(uint8_t index, uint32_t color);
void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void led_off(void);
void led_sequence_start(const uint32_t *colors, uint8_t count, uint32_t period_ms);
void led_sequence_stop(void);
#endif
