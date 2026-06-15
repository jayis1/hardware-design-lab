/* ============================================
 * tpa6130.c — WaveForge TPA6130A2 Headphone Amp Driver
 * I2C control interface
 * ============================================ */

#include "tpa6130.h"
#include "registers.h"
#include "i2c.h"

#define TPA6130_ADDR  TPA6130_I2C_ADDR  /* 0x1C */

int TPA6130_Init(void) {
    int ret;

    /* Check if device is present */
    if (I2C1_IsDeviceReady(TPA6130_ADDR, 3) != 0) {
        return -1;
    }

    /* Read version register to confirm communication */
    uint8_t version = 0;
    ret = I2C1_ReadByte(TPA6130_ADDR, TPA6130_REG_VERSION, &version);
    if (ret != 0) return -1;

    /* Configure control register:
     * - Enable left and right channels
     * - Stereo operation
     * - Unmuted
     */
    ret = I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_CTRL, 0xC0);
    if (ret != 0) return -1;

    /* Set moderate volume (-12 dB ≈ 0x13) */
    TPA6130_SetVolume(0x13);

    /* Unmute */
    TPA6130_Mute(0);

    return 0;
}

void TPA6130_SetVolume(uint8_t volume) {
    if (volume > 0x3F) volume = 0x3F;

    /* Left channel volume */
    I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_LEFT_VOL, volume);

    /* Right channel volume */
    I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_RIGHT_VOL, volume);
}

void TPA6130_Mute(int mute) {
    uint8_t ctrl = 0;
    I2C1_ReadByte(TPA6130_ADDR, TPA6130_REG_CTRL, &ctrl);

    if (mute) {
        ctrl &= ~(0x03);  /* Mute both channels */
        I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_CTRL, ctrl);
        I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_MUTE, 0x03);
    } else {
        ctrl |= 0xC0;     /* Enable both channels */
        I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_CTRL, ctrl);
        I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_MUTE, 0x00);
    }
}

void TPA6130_Enable(int enable) {
    if (enable) {
        HPAMP_ENABLE();
        I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_CTRL, 0xC0);
    } else {
        I2C1_WriteByte(TPA6130_ADDR, TPA6130_REG_CTRL, 0x00);
        HPAMP_DISABLE();
    }
}