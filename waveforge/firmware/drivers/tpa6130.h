/* ============================================
 * tpa6130.h — WaveForge TPA6130A2 Headphone Amp Driver
 * ============================================ */

#ifndef WAVEFORGE_TPA6130_H
#define WAVEFORGE_TPA6130_H

#include <stdint.h>

/* Initialize TPA6130A2 headphone amplifier */
int TPA6130_Init(void);

/* Set headphone volume
 * volume: 0x00 = mute, 0x01 = -54 dB, ..., 0x1F = 0 dB, ..., 0x3F = +6 dB
 */
void TPA6130_SetVolume(uint8_t volume);

/* Mute/unmute headphone output */
void TPA6130_Mute(int mute);

/* Enable/disable the amplifier */
void TPA6130_Enable(int enable);

#endif /* WAVEFORGE_TPA6130_H */