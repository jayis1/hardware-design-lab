/* ============================================
 * wm8778.h — WaveForge WM8778 Audio Codec Driver
 * ============================================ */

#ifndef WAVEFORGE_WM8778_H
#define WAVEFORGE_WM8778_H

#include <stdint.h>

/* Initialize WM8778 codec:
 * - Configure for I2S mode, 48 kHz, 24-bit
 * - Set DAC volume to 0 dB
 * - Enable DAC output
 * - Configure line input gain
 * Returns: 0 on success, -1 on error
 */
int WM8778_Init(void);

/* Set DAC volume (0 dB = 0, range: -127 dB to 0 dB) */
void WM8778_SetDACVolume(int8_t volume_db);

/* Set headphone volume (0 dB = 0, range: -73 dB to +6 dB) */
void WM8778_SetHPVolume(int8_t volume_db);

/* Set ADC input gain (0 dB = 0, range: -96 dB to +24 dB) */
void WM8778_SetADCGain(int8_t gain_db);

/* Mute/unmute DAC output */
void WM8778_MuteDAC(int mute);

/* Mute/unmute headphone output */
void WM8778_MuteHP(int mute);

/* Enable/disable line input */
void WM8778_EnableLineInput(int enable);

/* Software reset (all registers to default) */
void WM8778_Reset(void);

#endif /* WAVEFORGE_WM8778_H */