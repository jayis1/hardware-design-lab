/* ============================================
 * wm8778.c — WaveForge WM8778 Audio Codec Driver
 * I2C control interface, I2S audio data
 * ============================================ */

#include "wm8778.h"
#include "registers.h"
#include "i2c.h"

/* WM8778 I2C address (7-bit) */
#define WM8778_ADDR  WM8778_I2C_ADDR  /* 0x1A */

/* ---- Helper: Write to WM8778 register ---- */
static int WM8778_WriteReg(uint8_t reg, uint16_t value) {
    /* WM8778 uses 7-bit register address in upper 7 bits of 16-bit word:
     * [reg_addr 7:1][data 9:0] = 16 bits total
     * Format: [B15:B9] = register address, [B8:B0] = data
     * Actually, WM8778 uses: first byte = (reg << 1) | ((data >> 8) & 1),
     *                       second byte = data & 0xFF
     */
    uint8_t buf[2];
    buf[0] = (uint8_t)((reg << 1) | ((value >> 8) & 0x01));
    buf[1] = (uint8_t)(value & 0xFF);

    /* The WM8778 uses a simple I2C write without register sub-address.
     * The "register" is encoded in the data itself. */
    return I2C1_Write(WM8778_ADDR, buf[0], &buf[1], 1);
}

int WM8778_Init(void) {
    int ret;

    /* 1. Software reset */
    WM8778_WriteReg(WM8778_REG_RESET, 0x000);
    for (volatile int i = 0; i < 10000; i++);  /* Wait for reset */

    /* 2. Check if codec is present by trying to communicate */
    if (I2C1_IsDeviceReady(WM8778_ADDR, 3) != 0) {
        return -1;  /* Codec not responding */
    }

    /* 3. Configure DAC control register 1:
     *    - DAC interface format: I2S, 24-bit
     *    - MCLK/LRCK ratio: 256x
     */
    ret = WM8778_WriteReg(WM8778_REG_DAC_CTRL1, 0x004);  /* I2S, 24-bit, 256x MCLK */
    if (ret != 0) return -1;

    /* 4. Configure DAC control register 2:
     *    - DAC output enable
     *    - No deemphasis
     */
    ret = WM8778_WriteReg(WM8778_REG_DAC_CTRL2, 0x002);  /* DAC enabled */
    if (ret != 0) return -1;

    /* 5. Set DAC volume to 0 dB (value = 0xFF for 0 dB on each channel) */
    WM8778_SetDACVolume(0);

    /* 6. Set headphone volume to moderate level (-12 dB) */
    WM8778_SetHPVolume(-12);

    /* 7. Configure ADC input:
     *    - Enable left and right ADC
     *    - Line input mode
     */
    ret = WM8778_WriteReg(WM8778_REG_LEFT_ADC, 0x100);   /* Left ADC: +0 dB */
    if (ret != 0) return -1;

    ret = WM8778_WriteReg(WM8778_REG_RIGHT_ADC, 0x100);  /* Right ADC: +0 dB */
    if (ret != 0) return -1;

    /* 8. Configure left input (AINL enabled) */
    ret = WM8778_WriteReg(WM8778_REG_LEFT_IN0, 0x021);   /* AINL enabled, +0 dB */
    if (ret != 0) return -1;

    /* 9. Configure right input (AINR enabled) */
    ret = WM8778_WriteReg(WM8778_REG_RIGHT_IN0, 0x021);  /* AINR enabled, +0 dB */
    if (ret != 0) return -1;

    /* 10. Configure ADC mode: high performance, no deemphasis */
    ret = WM8778_WriteReg(WM8778_REG_ADC_MODE, 0x000);
    if (ret != 0) return -1;

    /* 11. Enable line input */
    WM8778_EnableLineInput(1);

    /* 12. Unmute DAC */
    WM8778_MuteDAC(0);

    /* 13. Unmute headphone */
    WM8778_MuteHP(0);

    return 0;
}

void WM8778_SetDACVolume(int8_t volume_db) {
    /* Volume range: 0 dB (0xFF) to -127 dB (0x00), in 0.5 dB steps
     * volume_db = 0 → register = 0xFF (0 dB)
     * volume_db = -1 → register = 0xFD
     * etc.
     */
    uint16_t val;
    if (volume_db > 0) volume_db = 0;
    if (volume_db < -127) volume_db = -127;
    val = (uint16_t)((volume_db * 2) + 255) & 0x1FF;  /* Convert to 0.5 dB steps */

    WM8778_WriteReg(WM8778_REG_LEFT_DAC, val);
    WM8778_WriteReg(WM8778_REG_RIGHT_DAC, val | 0x100);  /* Update both channels */
}

void WM8778_SetHPVolume(int8_t volume_db) {
    /* HP volume: +6 dB (0x7F) to -73 dB (0x00)
     * Register value = (volume + 73) in 1 dB steps
     * 0 dB → register = 73 → 0x49
     */
    uint16_t val;
    if (volume_db > 6) volume_db = 6;
    if (volume_db < -73) volume_db = -73;
    val = (uint16_t)(volume_db + 73) & 0x7F;

    WM8778_WriteReg(WM8778_REG_LEFT_HP, val);
    WM8778_WriteReg(WM8778_REG_RIGHT_HP, val | 0x100);  /* Update both */
}

void WM8778_SetADCGain(int8_t gain_db) {
    /* ADC gain: +24 dB (0x1F) to -96 dB (0x00), in 3 dB steps */
    uint16_t val;
    if (gain_db > 24) gain_db = 24;
    if (gain_db < -96) gain_db = -96;
    val = (uint16_t)((gain_db + 96) / 3) & 0x1F;

    WM8778_WriteReg(WM8778_REG_LEFT_ADC, val);
    WM8778_WriteReg(WM8778_REG_RIGHT_ADC, val | 0x100);
}

void WM8778_MuteDAC(int mute) {
    if (mute) {
        WM8778_WriteReg(WM8778_REG_DAC_CTRL2, 0x006);  /* Mute both channels */
    } else {
        WM8778_WriteReg(WM8778_REG_DAC_CTRL2, 0x002);  /* Unmute */
    }
}

void WM8778_MuteHP(int mute) {
    if (mute) {
        WM8778_WriteReg(WM8778_REG_LEFT_HP, 0x000);   /* Mute left HP */
        WM8778_WriteReg(WM8778_REG_RIGHT_HP, 0x100);  /* Mute right HP */
    }
    /* Unmute is handled by WM8778_SetHPVolume */
}

void WM8778_EnableLineInput(int enable) {
    if (enable) {
        WM8778_WriteReg(WM8778_REG_LEFT_IN0, 0x021);   /* AINL enabled */
        WM8778_WriteReg(WM8778_REG_RIGHT_IN0, 0x021);  /* AINR enabled */
    } else {
        WM8778_WriteReg(WM8778_REG_LEFT_IN0, 0x000);   /* AINL disabled */
        WM8778_WriteReg(WM8778_REG_RIGHT_IN0, 0x000);  /* AINR disabled */
    }
}

void WM8778_Reset(void) {
    WM8778_WriteReg(WM8778_REG_RESET, 0x000);
}