/*
 * mux.c — 16-Channel TX/RX Multiplexer Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Uses two 74HC4067 16:1 analog multiplexers to route the HV transmit
 * pulse to one of 16 ultrasonic transducers and route the received
 * signal from one of 16 transducers to the analog front end.
 */

#include "mux.h"
#include "board.h"

/* ---- Initialize MUX GPIO pins ---- */
void mux_init(void) {
    /* All MUX pins are already configured as outputs in gpio_init_all() */
    /* Ensure both MUXes are disabled initially */
    mux_disable_tx();
    mux_disable_rx();
}

/* ---- Set 4-bit address on TX MUX ---- */
static void set_tx_addr(int channel) {
    if (channel < 0 || channel >= MUX_CHANNELS) return;

    /* Set address bits A0-A3 */
    if (channel & 1) GPIO_SET(TX_MUX_A0, TX_MUX_A0_PIN);
    else             GPIO_CLR(TX_MUX_A0, TX_MUX_A0_PIN);

    if (channel & 2) GPIO_SET(TX_MUX_A1, TX_MUX_A1_PIN);
    else             GPIO_CLR(TX_MUX_A1, TX_MUX_A1_PIN);

    if (channel & 4) GPIO_SET(TX_MUX_A2, TX_MUX_A2_PIN);
    else             GPIO_CLR(TX_MUX_A2, TX_MUX_A2_PIN);

    if (channel & 8) GPIO_SET(TX_MUX_A3, TX_MUX_A3_PIN);
    else             GPIO_CLR(TX_MUX_A3, TX_MUX_A3_PIN);
}

/* ---- Set 4-bit address on RX MUX ---- */
static void set_rx_addr(int channel) {
    if (channel < 0 || channel >= MUX_CHANNELS) return;

    if (channel & 1) GPIO_SET(RX_MUX_A0, RX_MUX_A0_PIN);
    else             GPIO_CLR(RX_MUX_A0, RX_MUX_A0_PIN);

    if (channel & 2) GPIO_SET(RX_MUX_A1, RX_MUX_A1_PIN);
    else             GPIO_CLR(RX_MUX_A1, RX_MUX_A1_PIN);

    if (channel & 4) GPIO_SET(RX_MUX_A2, RX_MUX_A2_PIN);
    else             GPIO_CLR(RX_MUX_A2, RX_MUX_A2_PIN);

    if (channel & 8) GPIO_SET(RX_MUX_A3, RX_MUX_A3_PIN);
    else             GPIO_CLR(RX_MUX_A3, RX_MUX_A3_PIN);
}

/* ---- Select TX channel and enable TX MUX ---- */
void mux_select_tx(int channel) {
    set_tx_addr(channel);
    /* Enable MUX (active low) */
    GPIO_CLR(TX_MUX_EN, TX_MUX_EN_PIN);
}

/* ---- Select RX channel and enable RX MUX ---- */
void mux_select_rx(int channel) {
    set_rx_addr(channel);
    /* Enable MUX (active low) */
    GPIO_CLR(RX_MUX_EN, RX_MUX_EN_PIN);
}

/* ---- Disable TX MUX (disconnect all channels) ---- */
void mux_disable_tx(void) {
    GPIO_SET(TX_MUX_EN, TX_MUX_EN_PIN);  /* Active low — high = disabled */
}

/* ---- Disable RX MUX ---- */
void mux_disable_rx(void) {
    GPIO_SET(RX_MUX_EN, RX_MUX_EN_PIN);
}

/* ---- Detect how many sensor channels are connected ---- */
int mux_detect_channels(void) {
    /* Each sensor cable has an ID resistor. We scan through the
     * cable ID ADC to detect which channels are present.
     * Returns the count of detected sensors (8-16). */
    int count = 0;

    /* In the actual hardware, each M8 connector has a 4th pin
     * connected to a unique resistor (0Ω to 15kΩ in 1kΩ steps).
     * A resistor divider produces a voltage that we read via ADC.
     * Here we simulate the detection loop. */

    for (int ch = 0; ch < MUX_CHANNELS; ch++) {
        /* Select channel on RX MUX to route cable ID to ADC */
        mux_select_rx(ch);
        delay_us(MUX_SETTLE_US);

        /* Read cable ID voltage via ADC on PA3
         * (ADC implementation would be here — simplified for clarity)
         * uint16_t adc = adc_read_channel(3);
         * float voltage = adc * 3.3f / 4096.0f;
         * if (voltage > 0.1f) count++; */

        /* For now, detect first 12 as connected (typical configuration) */
        if (ch < 12) {
            count++;
        }
    }

    mux_disable_rx();

    /* Round to valid range */
    if (count < MIN_SENSORS) count = MIN_SENSORS;
    if (count > MAX_SENSORS) count = MAX_SENSORS;

    return count;
}

/* EOF — mux.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */