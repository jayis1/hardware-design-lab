/*
 * drivers/optical.c — 8-wavelength LED sweep + photodiode absorbance
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The optical block has eight LEDs (255..940 nm) sharing a common-anode
 * rail. A 74HC595 shift register driven by SPI1 selects one LED at a
 * time; an AL8805 constant-current sink sets drive. Two OPT101
 * photodiode amplifiers are read by ADC1: the "sample" channel through
 * the cuvette, and the "reference" channel tapped from a beamsplitter
 * fibre so LED ageing and temperature drift ratio out.
 *
 * For each wavelength we:
 *   1. shift the one-hot LED select into the '595
 *   2. latch and de-assert OE_N
 *   3. wait 200 us for optical + TIA settling
 *   4. coherently average 16 ADC samples of each PD
 *   5. compute A = -log10(I_sample / I_ref)
 *   6. disable the LED before moving on (low duty cycle, low heat)
 */
#include "optical.h"
#include "../registers.h"
#include <math.h>

/* ---- 74HC595 LED select -------------------------------------------- */
static const hgpio_t led_sck   = PIN_LED_MUX_SCK;
static const hgpio_t led_mosi  = PIN_LED_MUX_MOSI;
static const hgpio_t led_latch  = PIN_LED_MUX_LATCH;
static const hgpio_t led_oe_n   = PIN_LED_OE_N;

const uint16_t optical_wavelength_nm[OPTICAL_WAVELENGHS] = {
    255, 280, 365, 470, 590, 660, 850, 940
};

/* ---- ADC helpers --------------------------------------------------- */
static void adc1_init(void)
{
    /* Enable ADC clock and voltage regulator. */
    RCC_REG32(RCC_AHB2ENR_OF) |= RCC_AHB2ENR_ADC12EN;
    (void)RCC_REG32(RCC_AHB2ENR_OF);
    ADC1.CR = 0;                 /* clear, ADC disabled                  */
    ADC1.CR = ADC_CR_ADVREGEN;   /* enable internal regulator             */
    board_delay_ms(2);           /* regulator start-up                    */
    /* 12-bit single conversion, software trigger, right align, no DMA. */
    ADC1.CFGR  = 0;
    ADC1.SMPR1 = (3u << 6);      /* channel 3: 24.5 ADC cycles sampling  */
    ADC1.SMPR2 = (3u << 0);      /* channel 14: same                     */
    ADC1.CR |= ADC_CR_ADEN;
    while (!(ADC1.ISR & (1u << 0))) { }  /* ADRDY                                    */
}

static uint16_t adc1_sample(uint8_t ch)
{
    ADC1.SQR1 = (0u << 0) | (ch << 6);   /* 1 conversion, channel ch    */
    ADC1.CR |= ADC_CR_ADSTART;
    while (!(ADC1.ISR & ADC_ISR_EOC)) { }
    return (uint16_t)(ADC1.DR & 0xFFFFu);
}

static uint32_t adc1_avg(uint8_t ch, uint8_t n)
{
    uint32_t acc = 0;
    for (uint8_t i = 0; i < n; ++i) acc += adc1_sample(ch);
    return acc / n;
}

/* ---- 74HC595 driver ------------------------------------------------ */
static void led_shift(uint8_t onehot)
{
    /* 8 bits, MSB first into the '595; SPI1 is bit-banged here for clarity. */
    for (int8_t b = 7; b >= 0; --b) {
        uint8_t bit = (onehot >> b) & 1u;
        if (bit) led_mosi.port->BSRR = 1u << led_mosi.pin;          /* set  */
        else     led_mosi.port->BSRR = (1u << led_mosi.pin) << 16;   /* reset*/
        /* rising edge on SCK */
        led_sck.port->BSRR = 1u << led_sck.pin;
        led_sck.port->BSRR = (1u << led_sck.pin) << 16;
    }
    /* latch pulse */
    led_latch.port->BSRR = 1u << led_latch.pin;
    led_latch.port->BSRR = (1u << led_latch.pin) << 16;
}

/* ---- Public API ---------------------------------------------------- */
hydra_err_t optical_init(void)
{
    /* All four LED-mux control pins as push-pull outputs, low speed. */
    hgpio_t pins[4] = { led_sck, led_mosi, led_latch, led_oe_n };
    for (int i = 0; i < 4; ++i) {
        pins[i].port->MODER  &= ~(3u << (2u * pins[i].pin));
        pins[i].port->MODER  |=  (GPIO_MODE_OUTPUT << (2u * pins[i].pin));
        pins[i].port->OTYPER &= ~(1u << pins[i].pin);
        pins[i].port->BSRR   = (1u << pins[i].pin) << 16;  /* low      */
    }
    /* Start with outputs disabled (OE_N high). */
    led_oe_n.port->BSRR = 1u << led_oe_n.pin;   /* OE_N = 1 → off      */
    led_shift(0x00);                           /* no LED selected      */
    adc1_init();
    return HYDRA_OK;
}

void optical_off(void)
{
    led_oe_n.port->BSRR = 1u << led_oe_n.pin;   /* disable outputs     */
    led_shift(0x00);
}

hydra_err_t optical_sweep(float out[OPTICAL_WAVELENGHS])
{
    if (!out) return HYDRA_ERR_IO;
    for (uint8_t w = 0; w < OPTICAL_WAVELENGHS; ++w) {
        led_shift((uint8_t)(1u << w));          /* select one LED      */
        led_oe_n.port->BSRR = (1u << led_oe_n.pin) << 16; /* OE_N = 0 → on */
        board_delay_ms(1);                       /* 200 us + slack      */
        uint32_t i_samp = adc1_avg(ADC_CH_SAMPLE_PD, 16);
        uint32_t i_ref  = adc1_avg(ADC_CH_REF_PD_REV_B, 16);
        led_oe_n.port->BSRR = 1u << led_oe_n.pin;   /* off ASAP          */
        led_shift(0x00);
        if (i_ref < 4u) {
            out[w] = 3.0f;                       /* clip dark ref       */
            continue;
        }
        float ratio = (float)i_samp / (float)i_ref;
        if (ratio < 1e-4f) ratio = 1e-4f;
        out[w] = -log10f(ratio);
        if (out[w] < 0.0f) out[w] = 0.0f;        /* ignore overshoot    */
    }
    optical_off();
    return HYDRA_OK;
}