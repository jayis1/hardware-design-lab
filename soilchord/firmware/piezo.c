/*
 * piezo.c — Soilchord piezo signal chain: ADC, PGA, charge-amp power
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * The piezo chain is:
 *   piezo disc → OPA2333 charge amplifier → Sallen-Key BP (300 Hz-8 kHz)
 *     → MCP6S22 PGA (SPI1, gains 1..32x) → STM32 ADC1 IN1 (12-bit, 1 MSPS)
 *
 * We sample at 16 kHz for 512 ms (8192 samples) per chord. The analog LDO
 * is power-gated to save energy between measurements.
 */
#include "soilchord.h"
#include "registers.h"
#include "board.h"

/* ---- GPIO port helper ---- */
static volatile uint32_t *port_base(char p)
{
    switch (p) {
    case 'A': return (volatile uint32_t *)GPIOA_BASE_REG;
    case 'B': return (volatile uint32_t *)GPIOB_BASE_REG;
    case 'C': return (volatile uint32_t *)GPIOC_BASE_REG;
    default: return (volatile uint32_t *)GPIOA_BASE_REG;
    }
}

static void gpio_write(char port, int pin, int val)
{
    volatile uint32_t *b = port_base(port);
    if (val) GPIO_BSRR(b) = (1U << pin);
    else     GPIO_BSRR(b) = (1U << (pin + 16));
}

static int gpio_read(char port, int pin)
{
    volatile uint32_t *b = port_base(port);
    return (GPIO_IDR(b) >> pin) & 1;
}

/* ---- Analog LDO enable (PB1) ---- */
void piezo_chain_on(void)
{
    gpio_write('B', 1, 1);
}

void piezo_chain_off(void)
{
    gpio_write('B', 1, 0);
}

/* ---- PGA (MCP6S22) over SPI1 (software-bitbang for clarity) ---- */
static void pga_spi_start(void)
{
    gpio_write('B', 4, 0);                 /* NSS low */
}
static void pga_spi_stop(void)
{
    gpio_write('B', 4, 1);                 /* NSS high */
}
static uint8_t pga_spi_xfer(uint8_t tx)
{
    uint8_t rx = 0;
    for (int i = 7; i >= 0; i--) {
        gpio_write('B', 3, (tx >> i) & 1);  /* SCK low → set MOSI */
        /* clock rising */
        gpio_write('B', 3, 1);
        if (gpio_read('B', 6)) rx |= (1U << i);   /* MISO (PB6) */
        gpio_write('B', 3, 0);                     /* SCK falling */
    }
    return rx;
}

static const uint8_t pga_gain_codes[7] = {
    0x00,  /* 1x  */
    0x01,  /* 2x  */
    0x02,  /* 4x  */
    0x03,  /* 5x  */
    0x04,  /* 8x  */
    0x05,  /* 16x */
    0x06   /* 32x */
};

void piezo_set_pga_gain(uint8_t gain_idx)
{
    if (gain_idx > 6) gain_idx = 6;
    pga_spi_start();
    pga_spi_xfer(0x40);                 /* write gain register */
    pga_spi_xfer(pga_gain_codes[gain_idx]);
    pga_spi_stop();
}

/* ---- ADC1 (12-bit, single-channel, software-triggered) ---- */
sc_err_t adc_init(void)
{
    /* Enable ADC clock. */
    RCC_AHB2ENR |= (1U << 13);           /* ADCEN */

    /* Deep-power-down exit + voltage regulator enable. */
    ADC_CR = 0;                          /* clear */
    ADC_CR |= (1U << 28);                /* ADVREGEN */
    for (volatile int i = 0; i < 10000; i++) __asm__("nop"); /* settle */

    /* Calibrate (single-ended). */
    ADC_CR |= (1U << 31);                /* ADCAL */
    while (ADC_CR & (1U << 31)) { }

    /* Enable ADC. */
    ADC_CR |= ADC_CR_ADEN;
    while ((ADC_ISR & ADC_ISR_ADRDY) == 0) { }

    /* Configure: 12-bit, right-aligned, single conversion, channel 1 (PA0). */
    ADC_CFGR = (0U << 5)                 /* DISCEN=0 */
             | (0U << 3)                 /* CONT=0 (single) */
             | (0U << 2);               /* EXTEN=0 (soft trig) */
    ADC_SQR1 = (1U << 0);               /* 1 conversion in sequence, ch1 */

    piezo_set_pga_gain(3);               /* default 5x gain */
    return SC_OK;
}

sc_err_t adc_sample_block(int16_t *out, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ADC_CR |= ADC_CR_ADSTART;
        uint32_t timeout = 100000;
        while (((ADC_ISR & ADC_ISR_EOC) == 0) && (timeout-- > 0)) { }
        if (timeout == 0) return SC_ERR_TIMEOUT;
        uint16_t v = (uint16_t)ADC_DR;
        /* Store as signed 16-bit by subtracting the mid-rail (2048) — the
         * charge amp biases the signal around Vref/2. */
        out[i] = (int16_t)((int)v - 2048);
        ADC_ISR = ADC_ISR_EOC;           /* clear */
    }
    return SC_OK;
}

/* ---- Full per-chord capture ---- */
sc_err_t piezo_capture(uint8_t chord, int16_t *buf, size_t n)
{
    /* For the reference design all piezo pickups share a single ADC channel
     * via an analog mux; we select the mux channel here. */
    static const uint8_t mux_pins[3] = { 0, 1, 2 };   /* 3 control bits → 6 inputs */
    static const uint8_t mux_codes[NCHORDS][3] = {
        {0,0,0}, {0,0,1}, {0,1,0}, {0,1,1}, {1,0,0}, {1,0,1}
    };
    if (chord >= NCHORDS) return SC_ERR_RANGE;
    /* Mux select pins live on PC0..PC2 for the reference board. */
    for (int b = 0; b < 3; b++) {
        gpio_write('C', b, mux_codes[chord][b]);
    }
    /* Allow the mux to settle. */
    for (volatile int i = 0; i < 200; i++) __asm__("nop");

    /* Auto-scale gain: start at 5x, if the peak is near full-scale drop to 1x,
     * if it's tiny go to 16x. We do a quick 256-sample probe. */
    int16_t probe[256];
    piezo_set_pga_gain(3);                /* 5x */
    adc_sample_block(probe, 256);
    int16_t pk = 0;
    for (int i = 0; i < 256; i++) {
        int16_t a = (probe[i] >= 0) ? probe[i] : (int16_t)(-probe[i]);
        if (a > pk) pk = a;
    }
    if (pk > 1800)      piezo_set_pga_gain(0);   /* 1x  */
    else if (pk < 120)  piezo_set_pga_gain(5);   /* 16x */
    else                piezo_set_pga_gain(3);   /* 5x  */

    return adc_sample_block(buf, n);
}