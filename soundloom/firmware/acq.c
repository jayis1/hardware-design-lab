/*
 * acq.c — Acoustic acquisition driver for 4× ADS131M08 (24-channel, 8 kSPS)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom bioacoustic soil health monitor.
 * Manages SPI daisy-chain configuration, DMA double-buffered acquisition,
 * DC-offset removal, decimation, and per-channel sample delivery to the
 * DSP pipeline.
 */

#include "acq.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <math.h>

/* ---- Private state ---- */

static struct {
    volatile uint8_t  dma_half;      /* 1 = half-complete interrupt fired */
    volatile uint8_t  dma_full;      /* 1 = full-complete interrupt fired */
    volatile uint32_t blocks;        /* total blocks processed */
    int32_t           dc_offset[ADC_USED_CHS];
    int32_t           samples[ADC_USED_CHS * ADC_BLOCK_SAMPLES];
    int32_t           scratch[ADC_USED_CHS * ADC_BLOCK_SAMPLES];
    uint8_t           enabled;
    uint8_t           gain_db;       /* digital gain in dB */
    acq_callback_t    callback;
    SPI_HandleTypeDef  hspi;
    DMA_HandleTypeDef  hdma_rx;
    DMA_HandleTypeDef  hdma_tx;
} acq;

/* Raw SPI receive buffer: 4 devices × 30 bytes/frame × 2 buffers (double) */
static uint8_t spi_rx_buf[ADS_TOTAL_FRAME * 2] __attribute__((aligned(32)));
static uint8_t spi_tx_buf[ADS_TOTAL_FRAME * 2] __attribute__((aligned(32)));

/* ---- Low-level pin helpers ---- */

static inline void adc_start_high(void) {
    HAL_GPIO_WritePin(ADC_START_PORT, ADC_START_PIN, GPIO_PIN_SET);
}
static inline void adc_start_low(void) {
    HAL_GPIO_WritePin(ADC_START_PORT, ADC_START_PIN, GPIO_PIN_RESET);
}
static inline void adc_rst_low(void) {
    HAL_GPIO_WritePin(ADC_RST_PORT, ADC_RST_PIN, GPIO_PIN_RESET);
}
static inline void adc_rst_high(void) {
    HAL_GPIO_WritePin(ADC_RST_PORT, ADC_RST_PIN, GPIO_PIN_SET);
}
static inline void adc_nss_high(void) {
    HAL_GPIO_WritePin(ADC_NSS_PORT, ADC_NSS_PIN, GPIO_PIN_SET);
}
static inline void adc_nss_low(void) {
    HAL_GPIO_WritePin(ADC_NSS_PORT, ADC_NSS_PIN, GPIO_PIN_RESET);
}
static inline void adc_sync_high(void) {
    HAL_GPIO_WritePin(ADC_SYNC_PORT, ADC_SYNC_PIN, GPIO_PIN_SET);
}
static inline void adc_sync_low(void) {
    HAL_GPIO_WritePin(ADC_SYNC_PORT, ADC_SYNC_PIN, GPIO_PIN_RESET);
}

/* ---- SPI init ---- */

static void spi_init(void)
{
    acq.hspi.Instance               = ADC_SPI;
    acq.hspi.Init.Mode              = SPI_MODE_MASTER;
    acq.hspi.Init.Direction         = SPI_DIRECTION_2LINES;
    acq.hspi.Init.DataSize          = SPI_DATASIZE_8BIT;
    acq.hspi.Init.CLKPolarity       = SPI_POLARITY_LOW;
    acq.hspi.Init.CLKPhase          = SPI_PHASE_2EDGE;  /* Mode 1 for ADS131M08 */
    acq.hspi.Init.NSS               = SPI_NSS_SOFT;
    acq.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;  /* 480/32 = 15 MHz → /2 ≈ 7.5 MHz, safe */
    acq.hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    acq.hspi.Init.TIMode            = SPI_TIMODE_DISABLE;
    acq.hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&acq.hspi);
}

/* ---- Register write/read (single device, command frame) ---- */

static void ads_write_reg(uint8_t reg, uint16_t val)
{
    uint16_t cmd = ADS_CMD_WREG | ((uint16_t)reg << 8) | 0x0000u; /* count=1 */
    uint8_t tx[6] = {
        (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFFu), 0x00u,
        (uint8_t)(val >> 8), (uint8_t)(val & 0xFFu), 0x00u
    };
    uint8_t rx[6];
    adc_nss_low();
    HAL_SPI_TransmitReceive(&acq.hspi, tx, rx, 6, 100);
    adc_nss_high();
}

static uint16_t ads_read_reg(uint8_t reg)
{
    uint16_t cmd = ADS_CMD_RREG | ((uint16_t)reg << 8) | 0x0000u;
    uint8_t tx[6] = {
        (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFFu), 0x00u,
        0x00u, 0x00u, 0x00u
    };
    uint8_t rx[6];
    adc_nss_low();
    HAL_SPI_TransmitReceive(&acq.hspi, tx, rx, 6, 100);
    adc_nss_high();
    /* The response is in the 2nd word, 4th & 5th bytes */
    return ((uint16_t)rx[3] << 8) | rx[4];
}

/* ---- Initialise ADS131M08 chain ---- */

int acq_init(void)
{
    memset(&acq, 0, sizeof(acq));

    /* Enable clocks for SPI, DMA, GPIO */
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* Configure SPI pins: SCK, MISO, MOSI */
    GPIO_InitTypeDef gp = {0};
    gp.Mode  = GPIO_MODE_AF_PP;
    gp.Pull  = GPIO_NOPULL;
    gp.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gp.Alternate = ADC_SPI_AF;

    gp.Pin = ADC_SPI_SCK_PIN;
    HAL_GPIO_Init(ADC_SPI_SCK_PORT, &gp);
    gp.Pin = ADC_SPI_MISO_PIN;
    HAL_GPIO_Init(ADC_SPI_MISO_PORT, &gp);
    gp.Pin = ADC_SPI_MOSI_PIN;
    HAL_GPIO_Init(ADC_SPI_MOSI_PORT, &gp);

    /* NSS as output */
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Pin  = ADC_NSS_PIN;
    HAL_GPIO_Init(ADC_NSS_PORT, &gp);
    adc_nss_high();

    /* START, SYNC, RST outputs */
    gp.Pin = ADC_START_PIN;
    HAL_GPIO_Init(ADC_START_PORT, &gp);
    adc_start_low();

    gp.Pin = ADC_SYNC_PIN;
    HAL_GPIO_Init(ADC_SYNC_PORT, &gp);
    adc_sync_high();  /* idle high */

    gp.Pin = ADC_RST_PIN;
    HAL_GPIO_Init(ADC_RST_PORT, &gp);
    adc_rst_low();

    /* DRDY as EXTI falling edge */
    gp.Mode = GPIO_MODE_IT_FALLING;
    gp.Pull = GPIO_PULLUP;
    gp.Pin  = ADC_DRDY_PIN;
    HAL_GPIO_Init(ADC_DRDY_PORT, &gp);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 5);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);

    /* Configure DMA streams for SPI2 RX/TX */
    acq.hdma_rx.Instance                 = DMA1_Stream4;
    acq.hdma_rx.Init.Request             = DMA_REQUEST_SPI2_RX;
    acq.hdma_rx.Init.Direction            = DMA_PERIPH_TO_MEMORY;
    acq.hdma_rx.Init.PeriphInc            = DMA_PINC_DISABLE;
    acq.hdma_rx.Init.MemInc               = DMA_MINC_ENABLE;
    acq.hdma_rx.Init.PeriphDataAlignment  = DMA_PDATAALIGN_BYTE;
    acq.hdma_rx.Init.MemDataAlignment     = DMA_MDATAALIGN_BYTE;
    acq.hdma_rx.Init.Mode                = DMA_CIRCULAR;
    acq.hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&acq.hdma_rx);
    __HAL_LINKDMA(&acq.hspi, hdmarx, acq.hdma_rx);

    acq.hdma_tx.Instance               = DMA1_Stream5;
    acq.hdma_tx.Init.Request           = DMA_REQUEST_SPI2_TX;
    acq.hdma_tx.Init.Direction         = DMA_MEMORY_TO_PERIPH;
    acq.hdma_tx.Init.PeriphInc          = DMA_PINC_DISABLE;
    acq.hdma_tx.Init.MemInc             = DMA_MINC_ENABLE;
    acq.hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    acq.hdma_tx.Init.MemDataAlignment   = DMA_MDATAALIGN_BYTE;
    acq.hdma_tx.Init.Mode               = DMA_CIRCULAR;
    acq.hdma_tx.Init.FIFOMode           = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&acq.hdma_tx);
    __HAL_LINKDMA(&acq.hspi, hdmatx, acq.hdma_tx);

    NVIC_SetPriority(DMA1_Stream4_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream4_IRQn);

    spi_init();

    /* Hardware reset pulse */
    HAL_Delay(2);
    adc_rst_high();
    HAL_Delay(10);

    /* Unlock register access */
    ads_write_reg(0x00, ADS_CMD_UNLOCK);
    HAL_Delay(1);

    /* Read ID to verify */
    uint16_t id = ads_read_reg(ADS_REG_ID);
    if ((id & 0xFF00u) != 0x0100u) {
        /* ADS131M08 ID expected = 0x01xx */
        return ACQ_ERR_NO_DEVICE;
    }

    /* Configure all 8 channels: input mux = input, PGA gain = 32 (max) */
    for (uint8_t ch = 0; ch < 8; ch++) {
        uint16_t val = (ADS_CHSET_MUX_INPUT << 2) | ADS_GAIN_32;
        ads_write_reg(ADS_REG_CH0SET + ch, val);
    }

    /* Set oversampling rate for 8 kSPS */
    ads_write_reg(ADS_REG_OSR, ADS_OSR_2048);

    /* Enable internal reference and acquisition channels */
    ads_write_reg(ADS_REG_CLOCK, ADS_CLOCK_PWR_HD | ADS_CLOCK_PWR_REF | ADS_CLOCK_PWR_ACV);

    /* Clear any pending interrupt flags */
    ads_write_reg(ADS_REG_STATUS, 0);

    /* Set DRDY interrupt mask */
    ads_write_reg(ADS_REG_INTR_MASK, ADS_STATUS_DRDY);

    /* Lock registers */
    ads_write_reg(0x00, ADS_CMD_LOCK);

    /* Prepare TX buffer with NULLCMD for daisy chain */
    memset(spi_tx_buf, 0x00, sizeof(spi_tx_buf));

    acq.gain_db = 0;
    acq.enabled = 1;
    return ACQ_OK;
}

/* ---- Start streaming ---- */

void acq_start(void)
{
    if (!acq.enabled) return;

    /* Assert START to begin conversions */
    adc_start_high();
    HAL_Delay(1);
    adc_sync_high();
    HAL_DelayMicroseconds(10);
    adc_sync_low();

    /* Start DMA in circular double-buffer mode */
    HAL_SPI_TransmitReceive_DMA(&acq.hspi, spi_tx_buf, spi_rx_buf, ADS_TOTAL_FRAME * 2);
}

void acq_stop(void)
{
    adc_start_low();
    HAL_SPI_DMAStop(&acq.hspi);
}

/* ---- DMA half/full callbacks ---- */

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == ADC_SPI) {
        acq.dma_full = 1u;
    }
}

void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == ADC_SPI) {
        acq.dma_half = 1u;
    }
}

/* ---- Parse a raw SPI frame (4 devices, 30 bytes each) into 24 channels ---- */

static void parse_frame(const uint8_t *frame, int32_t *out)
{
    /* Each device frame: 3 bytes status + 8×3 bytes data + 3 bytes CRC = 30 bytes
       In daisy chain, devices appear in order D0, D1, D2, D3.
       We extract channels 0..5 from D0, 6..11 from D1, 12..17 from D2, 18..23 from D3. */

    for (uint8_t dev = 0; dev < ADC_NUM_DEVICES; dev++) {
        const uint8_t *p = frame + (dev * ADS_FRAME_BYTES);
        /* skip 3 status bytes */
        for (uint8_t ch = 0; ch < ADC_CHS_PER_DEVICE; ch++) {
            uint8_t ch_global = dev * ADC_CHS_PER_DEVICE + ch;
            if (ch_global >= ADC_USED_CHS) continue;

            /* 24-bit two's complement, MSB first */
            int32_t v = ((int32_t)p[3 + ch*3] << 16) |
                        ((int32_t)p[3 + ch*3 + 1] << 8) |
                        ((int32_t)p[3 + ch*3 + 2]);
            if (v & 0x800000u) v |= 0xFF000000u;  /* sign-extend */
            out[ch_global * ADC_BLOCK_SAMPLES + 0] = v;
            /* Note: this is one sample; block processing fills the rest per DRDY */
            (void)v;
        }
    }
}

/* ---- Process one DMA buffer: de-interleave all channels ---- */

static void deinterleave_block(const uint8_t *buf, int32_t *out)
{
    /* In daisy chain at 8 kSPS, the DRDY fires every 1/8000 = 125 µs.
       We collect ADC_BLOCK_SAMPLES (1024) DRDY events into one block.
       But the DMA buffer holds ADS_TOTAL_FRAME (120 bytes) per DRDY;
       double-buffer holds 2 frames = 240 bytes... this is the half-buffer.

       For simplicity and correctness with the circular DMA setup,
       we parse frame-by-frame from the half-buffer (120 bytes = 1 frame).
       The upper layer calls this once per DMA half/full event.
       Each event provides one sample per channel; we accumulate 1024
       samples by processing 1024 successive DMA events.

       In practice the DMA is sized to ADS_TOTAL_FRAME*2 and we use
       the half callback to get a pointer to the first frame and the
       full callback to get the second. This is a simplified model
       that captures the concept: each DMA event = one multi-channel
       sample. Real firmware uses an interrupt accumulator counter. */

    parse_frame(buf, out);
}

/* ---- DC offset removal (running mean) ---- */

static void remove_dc(int32_t *ch_data, uint32_t n, int32_t *offset)
{
    /* Update running mean with exponential filter */
    int64_t sum = 0;
    for (uint32_t i = 0; i < n; i++) sum += ch_data[i];
    int32_t mean = (int32_t)(sum / (int64_t)n);
    *offset = (int32_t)((7 * (*offset) + mean) / 8);  /* smoothing */
    for (uint32_t i = 0; i < n; i++) ch_data[i] -= *offset;
}

/* ---- Decimate from 8 kSPS to 4 kSPS (factor 2, simple averaging) ---- */

static void decimate_2x(const int32_t *in, int32_t *out, uint32_t n)
{
    for (uint32_t i = 0; i < n / 2; i++) {
        out[i] = (in[2*i] + in[2*i + 1]) / 2;
    }
}

/* ---- Polling function: called from main loop ---- */

int acq_poll(int32_t *out_samples)
{
    if (!acq.enabled) return 0;

    const uint8_t *buf = NULL;
    if (acq.dma_half) {
        acq.dma_half = 0;
        buf = spi_rx_buf;
    } else if (acq.dma_full) {
        acq.dma_full = 0;
        buf = spi_rx_buf + ADS_TOTAL_FRAME;
    }
    if (buf == NULL) return 0;

    /* De-interleave one sample set into per-channel arrays.
       In a full implementation we accumulate into a ring buffer;
       here we deliver the single multi-channel snapshot. */
    deinterleave_block(buf, acq.scratch);

    /* Copy first sample of each channel into the output (simplified:
       in real firmware the full 1024-sample block is assembled over
       1024 DRDY events and processed as a block). */
    for (uint8_t c = 0; c < ADC_USED_CHS; c++) {
        out_samples[c] = acq.scratch[c * ADC_BLOCK_SAMPLES];
        /* DC removal per channel */
        remove_dc(&out_samples[c], 1, &acq.dc_offset[c]);
    }

    /* Apply digital gain if set */
    if (acq.gain_db != 0) {
        float scale = powf(10.0f, acq.gain_db / 20.0f);
        for (uint8_t c = 0; c < ADC_USED_CHS; c++) {
            out_samples[c] = (int32_t)(out_samples[c] * scale);
        }
    }

    acq.blocks++;
    if (acq.callback) acq.callback(out_samples, ADC_USED_CHS);
    return 1;
}

/* ---- Register a per-block callback ---- */

void acq_set_callback(acq_callback_t cb)
{
    acq.callback = cb;
}

/* ---- Set digital gain (dB) ---- */

void acq_set_gain(int8_t db)
{
    acq.gain_db = (uint8_t)db;
}

/* ---- Get total blocks processed ---- */

uint32_t acq_get_block_count(void)
{
    return acq.blocks;
}

/* ---- Get current DC offset for a channel (diagnostic) ---- */

int32_t acq_get_offset(uint8_t ch)
{
    if (ch >= ADC_USED_CHS) return 0;
    return acq.dc_offset[ch];
}

/* ---- EXTI4 ISR (DRDY) ---- */

void EXTI4_IRQHandler(void)
{
    /* Clear pending bit */
    __HAL_GPIO_EXTI_CLEAR_IT(ADC_DRDY_PIN);
    /* In DMA mode, DRDY just paces the conversion; the DMA handles
       the actual data transfer automatically. We could use this for
       a sample counter or watchdog. */
}

/* ---- DMA1 Stream4 ISR (SPI2 RX) ---- */

void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&acq.hdma_rx);
}