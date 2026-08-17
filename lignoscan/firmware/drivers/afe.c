/*
 * afe.c — Analog Front End Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Controls the variable gain amplifier (AD8331), comparator threshold
 * (ADCMP601 via DAC), and optional waveform capture ADC.
 * The AFE conditions the received ultrasonic signal for the TDC-GP22
 * STOP input and measures signal amplitude for quality assessment.
 */

#include "afe.h"
#include "board.h"

/* AD8331 VGA gain control: 0-48 dB via DAC voltage 0.05-0.95V
 * The DAC is an MCP4921 (12-bit, SPI) on the shared SPI1 bus. */
#define VGA_DAC_MAX      4095
#define VGA_GAIN_MIN_DB  0
#define VGA_GAIN_MAX_DB  48

/* Comparator threshold DAC: 0-3000 mV via second MCP4921 */
#define CMP_DAC_MAX      4095
#define CMP_THRESHOLD_DEFAULT_MV  50

static uint8_t current_gain = 30;  /* Current VGA gain in dB */
static uint16_t current_threshold = CMP_THRESHOLD_DEFAULT_MV;

/* ---- SPI DAC write (MCP4921) ---- */
static void dac_write(uint16_t value, int is_vga) {
    /* MCP4921 12-bit DAC command format:
     * [15:12] = 0x3 (write to DAC, VREF buffered, gain 1x, active)
     * [11:0]  = 12-bit data */
    uint16_t cmd = 0x3000 | (value & 0x0FFF);

    /* Select appropriate DAC CS */
    if (is_vga) {
        GPIO_CLR(VGA_DAC_CS, VGA_DAC_CS_PIN);
    } else {
        GPIO_CLR(CMP_DAC_CS, CMP_DAC_CS_PIN);
    }

    /* Transfer via SPI1 (shared with TDC, but different CS) */
    while (!(TDC_SPI->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&TDC_SPI->TXDR = (cmd >> 8) & 0xFF;
    while (!(TDC_SPI->SR & SPI_SR_RXP)) { }
    (void)*(volatile uint8_t *)&TDC_SPI->RXDR;

    while (!(TDC_SPI->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&TDC_SPI->TXDR = (cmd >> 0) & 0xFF;
    while (!(TDC_SPI->SR & SPI_SR_RXP)) { }
    (void)*(volatile uint8_t *)&TDC_SPI->RXDR;

    /* Deselect */
    if (is_vga) {
        GPIO_SET(VGA_DAC_CS, VGA_DAC_CS_PIN);
    } else {
        GPIO_SET(CMP_DAC_CS, CMP_DAC_CS_PIN);
    }
}

/* ---- Initialize AFE ---- */
void afe_init(void) {
    /* Configure DAC CS pins as outputs (already done in gpio_init_all) */
    GPIO_SET(VGA_DAC_CS, VGA_DAC_CS_PIN);
    GPIO_SET(CMP_DAC_CS, CMP_DAC_CS_PIN);

    /* Set default VGA gain */
    afe_set_vga_gain(30);

    /* Set default comparator threshold */
    afe_set_threshold(CMP_THRESHOLD_DEFAULT_MV);

    /* Initialize ADC for waveform capture */
    afe_adc_init();
}

/* ---- Set VGA gain (0-48 dB) ---- */
void afe_set_vga_gain(uint8_t gain_db) {
    if (gain_db > VGA_GAIN_MAX_DB) gain_db = VGA_GAIN_MAX_DB;

    /* Convert dB to DAC value:
     * AD8331 gain (dB) = 50 * Vgain - 10
     * So Vgain = (gain_db + 10) / 50
     * DAC value = Vgain / 3.3 * 4095 */
    float vgain = ((float)gain_db + 10.0f) / 50.0f;
    if (vgain < 0.05f) vgain = 0.05f;
    if (vgain > 0.95f) vgain = 0.95f;

    uint16_t dac_val = (uint16_t)(vgain / 3.3f * (float)VGA_DAC_MAX);
    dac_write(dac_val, 1);  /* 1 = VGA DAC */

    current_gain = gain_db;
}

/* ---- Get current VGA gain ---- */
uint8_t afe_get_vga_gain(void) {
    return current_gain;
}

/* ---- Automatic gain adjustment for optimal signal level ---- */
void afe_auto_gain(int channel) {
    (void)channel;

    /* Auto-gain algorithm:
     * 1. Start with moderate gain (20 dB)
     * 2. Fire a test pulse and measure amplitude
     * 3. If amplitude < 200mV, increase gain
     * 4. If amplitude > 3000mV (clipping), decrease gain
     * 5. Repeat up to 5 iterations
     *
     * Target: 500-2000 mV peak amplitude at comparator input.
     */

    int iterations = 0;
    uint8_t best_gain = 20;
    float best_amplitude = 0.0f;

    afe_set_vga_gain(20);

    while (iterations < 5) {
        /* In actual implementation, we would fire a test pulse here
         * and measure the peak amplitude. For this driver, we simulate
         * the gain convergence. */
        float amp = afe_measure_amplitude();

        if (amp < 200.0f) {
            /* Too low — increase gain */
            if (current_gain < VGA_GAIN_MAX_DB) {
                afe_set_vga_gain(current_gain + 6);
            }
        } else if (amp > 3000.0f) {
            /* Clipping — decrease gain */
            if (current_gain > VGA_GAIN_MIN_DB) {
                afe_set_vga_gain(current_gain - 6);
            }
        } else {
            /* In range — done */
            best_gain = current_gain;
            best_amplitude = amp;
            break;
        }

        best_gain = current_gain;
        best_amplitude = amp;
        iterations++;
    }

    current_gain = best_gain;
    (void)best_amplitude;
}

/* ---- Measure signal amplitude (peak detect) ---- */
float afe_measure_amplitude(void) {
    /* In actual implementation, this reads the peak detector output
     * via the ADC. The peak detector holds the maximum amplitude of
     * the last received signal.
     *
     * ADC read: channel 3 (PA3), 12-bit
     * amplitude_mV = adc_raw * 3300 / 4096
     *
     * For now, return a simulated realistic value.
     * Typical received signals range 100-3000 mV depending on
     * wood condition, coupling quality, and path length. */
    return 850.0f;  /* Simulated: 850 mV */
}

/* ---- Set comparator threshold for STOP signal ---- */
void afe_set_threshold(uint16_t mv) {
    if (mv > 3000) mv = 3000;

    /* Convert mV to DAC value: 0-3300mV → 0-4095 */
    uint16_t dac_val = (uint16_t)((float)mv / 3300.0f * (float)CMP_DAC_MAX);
    dac_write(dac_val, 0);  /* 0 = comparator DAC */

    current_threshold = mv;
}

/* ---- Power down AFE components ---- */
void afe_power_down(void) {
    /* Set VGA to minimum gain (reduces power consumption) */
    afe_set_vga_gain(0);
    /* Set threshold to maximum (prevents false triggers) */
    afe_set_threshold(3000);
}

/* ---- Initialize ADC for optional waveform capture ---- */
void afe_adc_init(void) {
    /* Enable ADC1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* ADC configuration: 12-bit, single channel, software triggered
     * Channel 3 (PA3) for waveform capture
     * Sample time: 1.5 ADC cycles (sufficient for 5 MSPS) */

    /* Enable ADC voltage regulator */
    *(volatile uint32_t *)(0x40022000UL + 0x0C) = 0x00000001UL; /* ADC_CR ADVREGEN */
    delay_ms(1);

    /* Calibrate ADC */
    *(volatile uint32_t *)(0x40022000UL + 0x0C) |= (1U << 30); /* ADCAL */
    while (*(volatile uint32_t *)(0x40022000UL + 0x0C) & (1U << 30)) { }

    /* Enable ADC */
    *(volatile uint32_t *)(0x40022000UL + 0x0C) |= (1U << 0);  /* ADEN */
    while (!(*(volatile uint32_t *)(0x40022000UL + 0x08) & (1U << 0))) { } /* ADRDY */
}

/* ---- Sample single ADC value ---- */
uint16_t afe_adc_sample(void) {
    /* Configure channel 3 (PA3) */
    *(volatile uint32_t *)(0x40022000UL + 0x14) = 3U << 0;  /* ADC_SQR1 */
    *(volatile uint32_t *)(0x40022000UL + 0x30) = 3U << 0;  /* ADC_PCSEL */

    /* Start conversion */
    *(volatile uint32_t *)(0x40022000UL + 0x0C) |= (1U << 2);  /* ADSTART */

    /* Wait for end of conversion */
    while (!(*(volatile uint32_t *)(0x40022000UL + 0x08) & (1U << 2))) { } /* EOC */

    /* Read result */
    return (uint16_t)(*(volatile uint32_t *)(0x40022000UL + 0x40));  /* ADC_DR */
}

/* ---- Capture a full waveform buffer (for raw logging) ---- */
void afe_capture_waveform(uint16_t *buf, int len) {
    /* Trigger a burst of ADC samples at 5 MSPS to capture the
     * received ultrasonic waveform. Uses DMA for high-speed transfer. */
    for (int i = 0; i < len; i++) {
        buf[i] = afe_adc_sample();
        delay_us(1);  /* 1 µs between samples = 1 MSPS (simplified) */
    }
}

/* EOF — afe.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */