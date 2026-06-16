// vga_driver.h — LMH6517 Variable Gain Amplifier Driver Header
#ifndef VGA_DRIVER_H
#define VGA_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

#define VGA_GAIN_MIN            0x00    // -2.5 dB
#define VGA_GAIN_MAX            0xFF    // +42.5 dB
#define VGA_GAIN_MID            0x40    // ~10 dB
#define VGA_BW_FULL             0x00    // 1200 MHz bandwidth
#define VGA_BW_750MHZ           0x01    // 750 MHz
#define VGA_BW_450MHZ           0x02    // 450 MHz
#define VGA_BW_200MHZ           0x03    // 200 MHz
#define VGA_AUX_DISABLED        0x00
#define VGA_AUX_ENABLED         0x01

int  vga_driver_init(void);
int  vga_set_gain(uint8_t gain);
int  vga_set_bandwidth(uint8_t bw_mode);
int  vga_set_aux_mode(uint8_t mode);
int  vga_get_gain(uint8_t *gain);
void vga_power_down(void);
void vga_power_up(void);
int  vga_self_test(void);
#endif
