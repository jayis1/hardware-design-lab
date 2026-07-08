/*
 * audio_drv.c — Contact acoustic desiccation click detector.
 *
 * The SPH0641LU421 I²S MEMS microphone is bonded to the spring clip that
 * holds the sensor head against the lichen surface. When a lichen thallus
 * desiccates rapidly, the cortex can micro-fracture, producing faint
 * acoustic clicks. This driver captures ~128 ms of I²S audio at 16 kHz,
 * runs a simple energy-threshold click detector, and counts events.
 *
 * Author: jayis1
 * License: MIT
 */

#include "audio_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* I²S config on STM32L4 uses SPI2 in I²S mode with a dedicated PLLSAI1 clock.
 * For firmware simplicity (and because the full I²S peripheral setup is
 * verbose), we capture via a bit-banged / DMA-assisted SPI2 I²S read into
 * a static buffer and post-process.
 *
 * In the real implementation this uses DMA2 channel in circular mode.
 * Here we model the DMA buffer and the click-detection DSP faithfully.
 */

static int16_t s_audio_buf[MIC_SAMPLE_COUNT];
static int s_initialized = 0;

/* ------------------------------------------------------------------------ */
/* I²S / DMA setup (modeled)                                                 */
/* ------------------------------------------------------------------------ */
static void i2s2_init(void)
{
    /* Configure PB12 (WS), PB13 (CK), PB14 (SD) as AF5 (I2S2). */
    volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
    /* PB12 AFRH nibble 4 */
    pb[8] = (pb[8] & ~(0xFU << (4*4))) | (0x5U << (4*4));
    /* PB13 AFRH nibble 5 */
    pb[8] = (pb[8] & ~(0xFU << (5*4))) | (0x5U << (5*4));
    /* PB14 AFRH nibble 6 */
    pb[8] = (pb[8] & ~(0xFU << (6*4))) | (0x5U << (6*4));
    pb[0] = (pb[0] & ~(0x3U << (12*2))) | (GPIO_MODE_AF << (12*2));
    pb[0] = (pb[0] & ~(0x3U << (13*2))) | (GPIO_MODE_AF << (13*2));
    pb[0] = (pb[0] & ~(0x3U << (14*2))) | (GPIO_MODE_AF << (14*2));

    /* Configure SPI2 for I²S mode (simplified): enable clock, set I2SMOD. */
    SPI2->CR1 = 0;
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    /* I²S configuration register (SPI_I2SCFGR) is at offset 0x30 in the
     * full peripheral; we write it through the SPI base + 0x30. */
    volatile uint32_t *i2scfgr = (volatile uint32_t *)((uint32_t)SPI2 + 0x30);
    *i2scfgr = (1U << 11);   /* I2SMOD=1, I2SCFG=master receive */
    /* Enable I²S. */
    *i2scfgr |= (1U << 10);  /* I2SE */
}

/* ------------------------------------------------------------------------ */
/* DMA buffer fill (modeled capture)                                          */
/* ------------------------------------------------------------------------ */
static void capture_dma(int16_t *buf, uint32_t n)
{
    /* In the real implementation, DMA2 CH2 streams from SPI2->DR into buf.
     * Here we model a read loop that drains the SPI RX FIFO. */
    for (uint32_t i = 0; i < n; i++) {
        while (!(SPI2->SR & SPI_SR_RXNE)) { }
        /* I²S data is 16-bit left-justified in the 32-bit slot. */
        uint16_t raw = (uint16_t)SPI2->DR;
        buf[i] = (int16_t)raw;
    }
}

/* ------------------------------------------------------------------------ */
/* Click detection DSP                                                        */
/* ------------------------------------------------------------------------ */
static uint16_t detect_clicks(const int16_t *buf, uint32_t n, uint16_t *peak)
{
    /* Compute short-term energy on overlapping windows of 32 samples (2 ms). */
    const uint32_t WIN = 32;
    uint16_t clicks = 0;
    int32_t max_energy = 0;

    /* Noise floor estimate: median of window energies. */
    int32_t energies[MIC_SAMPLE_COUNT / WIN];
    uint32_t ne = 0;
    for (uint32_t i = 0; i + WIN < n; i += WIN / 2) {
        int64_t e = 0;
        for (uint32_t j = 0; j < WIN; j++) {
            int32_t s = buf[i + j];
            e += (int64_t)s * s;
        }
        int32_t energy = (int32_t)(e / WIN);
        if (energy > max_energy) max_energy = energy;
        if (ne < (sizeof(energies)/sizeof(energies[0]))) {
            energies[ne++] = energy;
        }
    }

    /* Simple bubble-sort to find median (small array). */
    for (uint32_t i = 0; i + 1 < ne; i++) {
        for (uint32_t j = 0; j + 1 < ne - i - 1; j++) {
            if (energies[j] > energies[j+1]) {
                int32_t t = energies[j];
                energies[j] = energies[j+1];
                energies[j+1] = t;
            }
        }
    }
    int32_t noise_floor = ne > 0 ? energies[ne / 2] : 0;
    int32_t threshold = noise_floor + (noise_floor / 2) + 1000;

    /* Count windows exceeding threshold (with a refractory period to avoid
     * counting the same click multiple times). */
    uint32_t refractory = 0;
    for (uint32_t i = 0; i + WIN < n; i += WIN / 2) {
        int64_t e = 0;
        for (uint32_t j = 0; j < WIN; j++) {
            int32_t s = buf[i + j];
            e += (int64_t)s * s;
        }
        int32_t energy = (int32_t)(e / WIN);
        if (energy > threshold && refractory == 0) {
            clicks++;
            refractory = 4;  /* skip next 4 windows (~4 ms) */
        } else if (refractory > 0) {
            refractory--;
        }
    }

    *peak = (uint16_t)(max_energy > 65535 ? 65535 : max_energy);
    return clicks;
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int audio_init(void)
{
    i2s2_init();
    s_initialized = 1;
    return 0;
}

int audio_capture(audio_result_t *res)
{
    if (!s_initialized || res == NULL) return -1;
    memset(res, 0, sizeof(*res));

    /* Capture 128 ms at 16 kHz = 2048 samples. */
    capture_dma(s_audio_buf, MIC_SAMPLE_COUNT);

    /* Run the click detector. */
    uint16_t peak = 0;
    res->click_count  = detect_clicks(s_audio_buf, MIC_SAMPLE_COUNT, &peak);
    res->peak_energy  = peak;
    return 0;
}