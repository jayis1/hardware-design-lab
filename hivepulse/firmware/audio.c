/*
 * audio.c — CS42448 24-bit audio codec driver with I2S DMA for HivePulse
 *
 * Implements double-buffered DMA capture of 4 microphone channels at 48 kHz,
 * 24-bit resolution (packed into 32-bit slots). The DMA half-transfer and
 * full-transfer interrupts signal the DSP task that a new window is ready.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "audio.h"
#include "registers.h"

/* ---- CS42448 I2C Register Addresses ---- */
#define CS42448_REG_CHIP_ID       0x01
#define CS42448_REG_PWR_CTRL      0x02
#define CS42448_REG_DAI_CTRL      0x03
#define CS42448_REG_DAC_CTRL      0x04
#define CS42448_REG_ADC_CTRL      0x05
#define CS42448_REG_MCLK_FREQ     0x06
#define CS42448_REG_ADC_GAIN      0x07
#define CS42448_REG_ADC_MIX       0x08
#define CS42448_REG_STATUS        0x18

#define CS42448_I2C_ADDR          0x24  /* 7-bit address, CS pin low */

/* CS42448 power control bits */
#define CS42448_PWR_ADC_EN        (1U << 3)
#define CS42448_PWR_DAC_EN        (1U << 2)
#define CS42448_PWR_REF_EN        (1U << 0)

/* CS42448 DAI control: TDM mode, 24-bit */
#define CS42448_DAI_TDM_24BIT     0x30

/* MCLK frequency settings: 256*Fs = 12.288 MHz for 48 kHz */
#define CS42448_MCLK_256FS        0x00

/* ---- Internal State ---- */
static int16_t dma_buf_a[AUDIO_BUF_SAMPLES]; /* DMA buffer A (16-bit samples) */
static int16_t dma_buf_b[AUDIO_BUF_SAMPLES]; /* DMA buffer B (double buffer) */

static audio_snapshot_t snapshot_a;  /* Processed snapshot from buffer A */
static audio_snapshot_t snapshot_b;  /* Processed snapshot from buffer B */
static audio_snapshot_t *ready_snapshot = NULL;
static volatile bool snapshot_ready = false;
static volatile uint32_t snapshot_seq = 0;

static float current_levels[AUDIO_CHANNELS];
static uint8_t mic_gain = 50; /* Default 50% gain */

static bool audio_initialized = false;
static bool capture_active = false;

/* ---- I2C Helper Functions ---- */
static int i2c2_write(uint8_t dev_addr, uint8_t reg, uint8_t value)
{
    /* Wait for I2C bus to be free */
    uint32_t timeout = 10000;
    while ((I2C2->ISR & I2C_ISR_BUSY) && timeout--);
    if (timeout == 0) return -1;

    /* Configure transfer: write 2 bytes (reg + data), auto-end */
    I2C2->CR2 = ((uint32_t)dev_addr << 1) |    /* Address (W) */
                (2U << 16) |                   /* NBYTES=2 */
                (1U << 25) |                   /* AUTOEND */
                (1U << 13);                    /* START */

    /* Write register address */
    while (!(I2C2->ISR & I2C_ISR_TXE));
    I2C2->TXDR = reg;

    /* Write data */
    while (!(I2C2->ISR & I2C_ISR_TXE));
    I2C2->TXDR = value;

    /* Wait for completion */
    timeout = 10000;
    while (!(I2C2->ISR & (1U << 6)) && timeout--); /* TC flag */
    return (timeout == 0) ? -1 : 0;
}

static int i2c2_read(uint8_t dev_addr, uint8_t reg, uint8_t *value)
{
    uint32_t timeout = 10000;

    /* Phase 1: write register address */
    I2C2->CR2 = ((uint32_t)dev_addr << 1) |
                (1U << 16) |               /* NBYTES=1 */
                (1U << 25) |               /* AUTOEND */
                (1U << 13);                /* START */
    while (!(I2C2->ISR & I2C_ISR_TXE));
    I2C2->TXDR = reg;
    while (!(I2C2->ISR & (1U << 6)) && timeout--);

    /* Phase 2: read 1 byte */
    I2C2->CR2 = ((uint32_t)dev_addr << 1) |
                (1U << 0) |                /* RD_WRn=1 (read) */
                (1U << 16) |               /* NBYTES=1 */
                (1U << 25) |               /* AUTOEND */
                (1U << 13);                /* START */

    while (!(I2C2->ISR & I2C_ISR_RXNE) && timeout--);
    *value = (uint8_t)I2C2->RXDR;

    return (timeout == 0) ? -1 : 0;
}

/* ---- Codec Register Access (Public) ---- */
int codec_write_reg(uint8_t reg, uint8_t value)
{
    return i2c2_write(CS42448_I2C_ADDR, reg, value);
}

int codec_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c2_read(CS42448_I2C_ADDR, reg, value);
}

/* ---- GPIO Configuration for I2S2 ---- */
static void i2s2_gpio_init(void)
{
    /* Configure I2S2 pins: WS(PI4), SCK(PI5), SDO(PI6), SDI(PI7) as AF5 */
    volatile uint32_t *pi_afrl = &GPIOI->AFR[0];
    volatile uint32_t *pi_afrh = &GPIOI->AFR[1];

    /* PI4 = AF5 (bits 16-19 in AFR[0]) */
    *pi_afrl &= ~(0xFU << 16);
    *pi_afrl |= (5U << 16);
    GPIOI->OSPEEDR |= (GPIO_SPEED_VHIGH << 8); /* PI4 high speed */

    /* PI5 = AF5 (bits 20-23 in AFR[0]) */
    *pi_afrl &= ~(0xFU << 20);
    *pi_afrl |= (5U << 20);
    GPIOI->OSPEEDR |= (GPIO_SPEED_VHIGH << 10);

    /* PI6 = AF5 (bits 24-27 in AFR[0]) */
    *pi_afrl &= ~(0xFU << 24);
    *pi_afrl |= (5U << 24);
    GPIOI->OSPEEDR |= (GPIO_SPEED_VHIGH << 12);

    /* PI7 = AF5 (bits 28-31 in AFR[0]) */
    *pi_afrl &= ~(0xFU << 28);
    *pi_afrl |= (5U << 28);
    GPIOI->OSPEEDR |= (GPIO_SPEED_VHIGH << 14);

    /* Set all four pins to AF mode */
    for (int i = 4; i <= 7; i++) {
        GPIOI->MODER &= ~(0x3U << (i * 2));
        GPIOI->MODER |= (GPIO_MODE_AF << (i * 2));
    }

    /* MCLK on PC3 = AF5 */
    GPIOC->AFR[0] &= ~(0xFU << 12);
    GPIOC->AFR[0] |= (5U << 12);
    GPIOC->MODER &= ~(0x3U << 6);
    GPIOC->MODER |= (GPIO_MODE_AF << 6);
    GPIOC->OSPEEDR |= (GPIO_SPEED_VHIGH << 6);
}

/* ---- I2S2 Peripheral Configuration ---- */
static void i2s2_config(void)
{
    /* Enable SPI2 clock */
    RCC_APB1LENR |= RCC_APB1LENR_SPI2EN;

    /* Disable SPI2 before config */
    SPI2->CR1 &= ~SPI_CR1_SPE;

    /* Configure I2S mode:
     * - Master receive (we generate MCLK and SCK)
     * - 24-bit data in 32-bit frame
     * - I2S Philips standard
     */
    SPI2->I2SCFGR = I2S_CFGR_I2SMOD |     /* I2S mode */
                    (0x2 << 8) |          /* Master receive */
                    (0x3 << 1) |          /* 24-bit data length */
                    (0x1 << 3) |          /* 32-bit channel length */
                    I2S_CFGR_I2SE;        /* Enable I2S */

    /* I2S prescaler: MCLK = 256*Fs, Fs = 48 kHz
     * With APB1 at 140 MHz:
     * I2SDIV = (140e6 / (256 * 48000)) - 1 ≈ 10.4, so div=11, odd=1
     */
    SPI2->I2SPR = (1U << 8) |   /* MCKOE: master clock output enable */
                  (1U << 9) |   /* ODD divider */
                  11;           /* I2SDIV = 11 */
}

/* ---- DMA Configuration for Audio Capture ---- */
static void audio_dma_config(void)
{
    /* Enable DMA1 clock */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    /* Disable DMA stream before config */
    DMA1_Stream3->CR &= ~DMA_CR_EN;
    while (DMA1_Stream3->CR & DMA_CR_EN); /* Wait for disable */

    /* Configure DMA1 Stream3, Channel 0 (SPI2_RX) */
    DMA1_Stream3->CR = DMA_CHANNEL_0 |          /* Channel 0 */
                       DMA_CR_DIR_P2M |          /* Peripheral to memory */
                       DMA_CR_CIRC |             /* Circular mode */
                       DMA_CR_MINC |             /* Memory increment */
                       DMA_CR_HTIE |             /* Half-transfer interrupt */
                       DMA_CR_TCIE |             /* Transfer complete interrupt */
                       DMA_PRIO_VHIGH |          /* Highest priority */
                       (0x3 << 11) |             /* MSIZE: 16-bit */
                       (0x3 << 13) |             /* PSIZE: 16-bit */
                       (1U << 18) |              /* Double buffer mode */
                       (0x2 << 21);              /* Burst: 4-beat inc */

    /* Set peripheral address: SPI2 DR register */
    DMA1_Stream3->PAR = (uint32_t)&SPI2->DR;

    /* Set memory addresses for double buffer */
    DMA1_Stream3->M0AR = (uint32_t)dma_buf_a;
    DMA1_Stream3->M1AR = (uint32_t)dma_buf_b;

    /* Number of data items to transfer */
    DMA1_Stream3->NDTR = AUDIO_BUF_SAMPLES;

    /* Enable DMA FIFO (for burst transfers) */
    DMA1_Stream3->FCR = (0x3 << 0) |  /* FIFO threshold: full */
                        (1U << 2);    /* Direct mode disable */

    /* Enable DMA interrupt in NVIC */
    NVIC_ISER0 |= (1U << (DMA1_Stream3_IRQn & 0x1F));
}

/* ---- De-interleave DMA Buffer into Snapshot ---- */
static void deinterleave(const int16_t *dma_buf, audio_snapshot_t *snap)
{
    /* DMA buffer is interleaved: [ch0_s0, ch1_s0, ch2_s0, ch3_s0,
     *                            ch0_s1, ch1_s1, ch2_s1, ch3_s1, ...]
     * De-interleave into separate channel arrays */
    for (int i = 0; i < AUDIO_FFT_SIZE; i++) {
        for (int ch = 0; ch < AUDIO_CHANNELS; ch++) {
            snap->samples[ch][i] = dma_buf[i * AUDIO_CHANNELS + ch];
        }
    }

    /* Compute RMS levels per channel */
    for (int ch = 0; ch < AUDIO_CHANNELS; ch++) {
        float sum_sq = 0.0f;
        for (int i = 0; i < AUDIO_FFT_SIZE; i++) {
            float s = (float)snap->samples[ch][i] / 32768.0f;
            sum_sq += s * s;
        }
        current_levels[ch] = sqrtf(sum_sq / AUDIO_FFT_SIZE);
    }

    snap->timestamp_ms = systick_ms;
    snap->seq_number = ++snapshot_seq;
    snap->valid = true;
}

/* ---- DMA Interrupt Handler ---- */
void audio_dma_irq_handler(void)
{
    /* Check half-transfer flag (buffer A complete) */
    if (DMA1->HISR & (1U << 10)) {  /* HTIF3: stream 3 half transfer */
        /* Clear flag */
        DMA1->HIFCR = (1U << 10);

        if (capture_active) {
            /* Process buffer A while DMA fills buffer B */
            deinterleave(dma_buf_a, &snapshot_a);
            ready_snapshot = &snapshot_a;
            snapshot_ready = true;
        }
    }

    /* Check transfer-complete flag (buffer B complete) */
    if (DMA1->HISR & (1U << 11)) {  /* TCIF3: stream 3 complete */
        /* Clear flag */
        DMA1->HIFCR = (1U << 11);

        if (capture_active) {
            /* Process buffer B while DMA fills buffer A */
            deinterleave(dma_buf_b, &snapshot_b);
            ready_snapshot = &snapshot_b;
            snapshot_ready = true;
        }
    }
}

/* ---- Public API ---- */
int audio_init(void)
{
    if (audio_initialized)
        return 0;

    /* Configure I2S GPIO pins */
    i2s2_gpio_init();

    /* Configure I2S2 peripheral */
    i2s2_config();

    /* Configure DMA for double-buffered audio capture */
    audio_dma_config();

    /* Reset CS42448 codec via hardware pin */
    GPIOB->BSRR = (1U << 16);  /* Reset low */
    for (volatile int i = 0; i < 100000; i++); /* ~1ms delay */
    GPIOB->BSRR = (1U << 0);   /* Reset high */

    /* Wait for codec to stabilize */
    for (volatile int i = 0; i < 500000; i++);

    /* Verify chip ID */
    uint8_t chip_id = 0;
    if (codec_read_reg(CS42448_REG_CHIP_ID, &chip_id) != 0) {
        return -1; /* Codec not responding on I2C */
    }
    /* Expected chip ID: 0x00 (CS42448) with rev in upper nibble */

    /* Power up codec: enable ADC, reference, but not DAC (we only capture) */
    codec_write_reg(CS42448_REG_PWR_CTRL,
                    CS42448_PWR_ADC_EN | CS42448_PWR_REF_EN);

    /* Configure DAI: TDM mode, 24-bit, slave mode (MCU is master) */
    codec_write_reg(CS42448_REG_DAI_CTRL, CS42448_DAI_TDM_24BIT);

    /* Set MCLK frequency to 256*Fs */
    codec_write_reg(CS42448_REG_MCLK_FREQ, CS42448_MCLK_256FS);

    /* Set default ADC gain */
    audio_set_gain(mic_gain);

    /* Un-mute all ADC channels */
    codec_write_reg(CS42448_REG_ADC_CTRL, 0x00); /* All channels un-muted */

    audio_initialized = true;
    return 0;
}

int audio_start_capture(void)
{
    if (!audio_initialized)
        return -1;
    if (capture_active)
        return 0;

    /* Clear DMA flags */
    DMA1->HIFCR = 0x0F00; /* Clear all stream 3 flags */

    /* Re-enable DMA stream */
    DMA1_Stream3->NDTR = AUDIO_BUF_SAMPLES;
    DMA1_Stream3->CR |= DMA_CR_EN;

    /* Enable I2S2 RX via DMA request */
    SPI2->CR2 |= (1U << 0); /* RXDMAEN */

    snapshot_ready = false;
    capture_active = true;
    return 0;
}

int audio_stop_capture(void)
{
    if (!capture_active)
        return 0;

    /* Disable DMA */
    DMA1_Stream3->CR &= ~DMA_CR_EN;
    while (DMA1_Stream3->CR & DMA_CR_EN);

    /* Disable I2S DMA request */
    SPI2->CR2 &= ~(1U << 0);

    capture_active = false;
    return 0;
}

bool audio_get_snapshot(audio_snapshot_t *snap)
{
    if (!snapshot_ready || ready_snapshot == NULL)
        return false;

    /* Copy snapshot to caller (interrupt-safe: copy is atomic enough
     * because the next snapshot won't be ready for 85ms) */
    __disable_irq();
    *snap = *ready_snapshot;
    snapshot_ready = false;
    __enable_irq();

    return true;
}

void audio_get_levels(float levels[AUDIO_CHANNELS])
{
    for (int ch = 0; ch < AUDIO_CHANNELS; ch++)
        levels[ch] = current_levels[ch];
}

void audio_set_gain(uint8_t gain_pct)
{
    if (gain_pct > 100) gain_pct = 100;
    mic_gain = gain_pct;

    /* CS42448 ADC gain: 0-0x3F maps to -6dB to +12dB
     * Map 0-100% to this range */
    uint8_t reg_val = (uint8_t)((gain_pct * 0x3F) / 100);

    /* Set gain for all 4 ADC channels (regs 0x07-0x0A for ch1-ch4) */
    for (uint8_t ch = 0; ch < AUDIO_CHANNELS; ch++) {
        codec_write_reg(CS42448_REG_ADC_GAIN + ch, reg_val);
    }
}