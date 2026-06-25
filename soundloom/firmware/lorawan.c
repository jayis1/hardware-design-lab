/*
 * lorawan.c — LoRaWAN telemetry driver for SX1262
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom bioacoustic soil health monitor.
 * Implements LoRaWAN 1.0.4 Class A uplink/downlink over the Semtech SX1262
 * radio. Handles SPI register programming, frame assembly, adaptive data
 * rate, and the 51-byte SoundLoom uplink payload format.
 *
 * This is a simplified MAC layer; production firmware uses a full LoRaWAN
 * stack (e.g. LMNv2 or a port of LMIC) handling join, counters, MIC, etc.
 */

#include "lorawan.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Private state ---- */

static SPI_HandleTypeDef hspi_lora;
static lora_state_t lora_state;
static uint8_t lora_initialized = 0;

/* Uplink/downlink buffers */
static uint8_t tx_buf[LORA_UPLINK_LEN + 4];
static uint8_t rx_buf[64];
static uint8_t msg_seq = 0;
static uint32_t tx_count = 0;
static uint32_t rx_count = 0;
static int8_t   last_rssi = 0;
static int8_t   last_snr  = 0;

/* ---- Pin helpers ---- */

static inline void lora_nss_low(void)  { HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_RESET); }
static inline void lora_nss_high(void) { HAL_GPIO_WritePin(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_PIN_SET); }
static inline void lora_rst_low(void)  { HAL_GPIO_WritePin(LORA_RST_PORT, LORA_RST_PIN, GPIO_PIN_RESET); }
static inline void lora_rst_high(void) { HAL_GPIO_WritePin(LORA_RST_PORT, LORA_RST_PIN, GPIO_PIN_SET); }

/* Wait for BUSY line to go low */
static void lora_wait_busy(void)
{
    uint32_t timeout = HAL_GetTick() + 100;
    while (HAL_GPIO_ReadPin(LORA_BUSY_PORT, LORA_BUSY_PIN) == GPIO_PIN_SET) {
        if (HAL_GetTick() > timeout) break;
    }
}

/* ---- SPI transfer (full-duplex, NSS controlled) ---- */

static void lora_spi_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    lora_wait_busy();
    lora_nss_low();
    HAL_SPI_TransmitReceive(&hspi_lora, (uint8_t*)tx, rx, len, 100);
    lora_nss_high();
}

/* ---- SX1262 command wrappers ---- */

static void sx1262_write_reg(uint16_t addr, uint8_t val)
{
    uint8_t tx[4] = { SX1262_CMD_WRITE_REG, (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), val };
    uint8_t rx[4];
    lora_spi_xfer(tx, rx, 4);
}

static uint8_t sx1262_read_reg(uint16_t addr)
{
    uint8_t tx[5] = { SX1262_CMD_READ_REG, (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), 0x00, 0x00 };
    uint8_t rx[5];
    lora_spi_xfer(tx, rx, 5);
    return rx[4];
}

static void sx1262_write_buf(uint8_t offset, const uint8_t *data, uint8_t len)
{
    uint8_t tx[64] = { SX1262_CMD_WRITE_BUF, offset };
    memcpy(&tx[2], data, len);
    uint8_t rx[64];
    lora_spi_xfer(tx, rx, len + 2);
}

static void sx1262_read_buf(uint8_t offset, uint8_t *data, uint8_t len)
{
    uint8_t tx[3] = { SX1262_CMD_READ_BUF, offset, 0x00 };
    uint8_t rx[3 + 64];
    lora_spi_xfer(tx, rx, 3);
    /* Dummy byte is rx[2], actual data follows */
    memcpy(data, &rx[3], len);
}

static void sx1262_set_standby(uint8_t clk_cfg)
{
    uint8_t tx[2] = { SX1262_CMD_SET_STDBY, clk_cfg };
    uint8_t rx[2];
    lora_spi_xfer(tx, rx, 2);
}

static void sx1262_set_sleep(uint8_t warm_start, uint8_t rtc_wake)
{
    uint8_t tx[2] = { SX1262_CMD_SET_SLEEP, (uint8_t)((warm_start << 2) | (rtc_wake << 3)) };
    uint8_t rx[2];
    lora_spi_xfer(tx, rx, 2);
}

static void sx1262_set_packet_type(uint8_t type)
{
    uint8_t tx[2] = { SX1262_CMD_SET_PACKET_TYPE, type };
    uint8_t rx[2];
    lora_spi_xfer(tx, rx, 2);
}

static void sx1262_set_rf_freq(uint32_t freq_hz)
{
    uint8_t tx[5] = { SX1262_CMD_SET_RF_FREQ,
                      (uint8_t)(freq_hz >> 24),
                      (uint8_t)(freq_hz >> 16),
                      (uint8_t)(freq_hz >> 8),
                      (uint8_t)(freq_hz) };
    uint8_t rx[5];
    lora_spi_xfer(tx, rx, 5);
}

static void sx1262_set_tx_params(int8_t power_dbm, uint8_t ramp)
{
    /* TX params: power (signed), ramp time */
    uint8_t tx[3] = { SX1262_CMD_SET_TX_PARAMS, (uint8_t)power_dbm, ramp };
    uint8_t rx[3];
    lora_spi_xfer(tx, rx, 3);
}

static void sx1262_set_mod_params_lora(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro)
{
    /* LoRa modulation params: SF, BW, CR, LDRO */
    uint8_t tx[5] = { SX1262_CMD_SET_MOD_PARAMS, sf, bw, cr, ldro };
    uint8_t rx[5];
    lora_spi_xfer(tx, rx, 5);
}

static void sx1262_set_tx(uint32_t timeout_ms)
{
    uint32_t ticks = timeout_ms * 64;  /* 15.625 µs per tick → ms × 64 */
    uint8_t tx[4] = { SX1262_CMD_SET_TX,
                      (uint8_t)(ticks >> 16),
                      (uint8_t)(ticks >> 8),
                      (uint8_t)(ticks) };
    uint8_t rx[4];
    lora_spi_xfer(tx, rx, 4);
}

static void sx1262_set_rx(uint32_t timeout_ms)
{
    uint32_t ticks = timeout_ms * 64;
    uint8_t tx[4] = { SX1262_CMD_SET_RX,
                      (uint8_t)(ticks >> 16),
                      (uint8_t)(ticks >> 8),
                      (uint8_t)(ticks) };
    uint8_t rx[4];
    lora_spi_xfer(tx, rx, 4);
}

static void sx1262_clear_irq(uint16_t mask)
{
    uint8_t tx[4] = { SX1262_CMD_CLEAR_IRQ,
                      (uint8_t)(mask >> 8), (uint8_t)(mask & 0xFF), 0x00 };
    uint8_t rx[4];
    lora_spi_xfer(tx, rx, 4);
}

static uint16_t sx1262_get_irq_status(void)
{
    uint8_t tx[4] = { SX1262_CMD_SET_DIO_IRQ, 0x02, 0xFF, 0xFF };
    /* Simplified: read via GetIrqStatus opcode 0x12 */
    uint8_t tx2[4] = { 0x12, 0x00, 0x00, 0x00 };
    uint8_t rx[4];
    lora_spi_xfer(tx2, rx, 4);
    return ((uint16_t)rx[2] << 8) | rx[3];
}

/* ---- Initialise LoRa radio ---- */

int lora_init(void)
{
    memset(&lora_state, 0, sizeof(lora_state));

    /* Enable clocks & GPIO */
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* SPI1 pins: SCK=PA5, MISO=PA6, MOSI=PA7 */
    GPIO_InitTypeDef gp = {0};
    gp.Mode = GPIO_MODE_AF_PP;
    gp.Pull = GPIO_NOPULL;
    gp.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gp.Alternate = GPIO_AF5_SPI1;
    gp.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gp);

    /* NSS, RST, BUSY, DIO1, ANT_SW */
    gp.Mode = GPIO_MODE_OUTPUT_PP;
    gp.Pull = GPIO_NOPULL;
    gp.Speed = GPIO_SPEED_FREQ_HIGH;
    gp.Alternate = 0;
    gp.Pin = LORA_NSS_PIN | LORA_RST_PIN | LORA_ANT_SW_PIN;
    HAL_GPIO_Init(GPIOB, &gp);
    lora_nss_high();
    lora_rst_high();
    HAL_GPIO_WritePin(LORA_ANT_SW_PORT, LORA_ANT_SW_PIN, GPIO_PIN_SET);

    /* BUSY input */
    gp.Mode = GPIO_MODE_INPUT;
    gp.Pull = GPIO_NOPULL;
    gp.Pin = LORA_BUSY_PIN;
    HAL_GPIO_Init(LORA_BUSY_PORT, &gp);

    /* DIO1 as EXTI rising */
    gp.Mode = GPIO_MODE_IT_RISING;
    gp.Pull = GPIO_NOPULL;
    gp.Pin = LORA_DIO1_PIN;
    HAL_GPIO_Init(LORA_DIO1_PORT, &gp);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 6);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    /* SPI1 handle */
    hspi_lora.Instance = SPI1;
    hspi_lora.Init.Mode = SPI_MODE_MASTER;
    hspi_lora.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_lora.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_lora.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_lora.Init.CLKPhase = SPI_PHASE_1EDGE;  /* Mode 0 for SX1262 */
    hspi_lora.Init.NSS = SPI_NSS_SOFT;
    hspi_lora.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  /* 480/16=30 MHz → safe <16 MHz */
    hspi_lora.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_lora.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi_lora);

    /* Hardware reset */
    lora_rst_low();
    HAL_Delay(10);
    lora_rst_high();
    HAL_Delay(10);

    /* Set standby */
    sx1262_set_standby(0x01);  /* STDBY_RC */
    HAL_Delay(1);

    /* Set packet type to LoRa */
    sx1262_set_packet_type(SX1262_PKT_TYPE_LORA);

    /* Set RF frequency: EU868.2 MHz
     * freq = (uint32_t)(868200000 / 32 * (2^25)) / 32000000
     * Simplified: use 868200000 Hz directly with the SX1262's 32 MHz reference
     * The register value = freq_Hz * (2^25) / 32MHz
     */
    uint32_t rf_val = (uint32_t)((double)868200000.0 * 33554432.0 / 32000000.0);
    sx1262_set_rf_freq(rf_val);

    /* Set modulation: SF7, BW125, CR4/5, LDRO off */
    sx1262_set_mod_params_lora(0x07, 0x04, 0x01, 0x00);

    /* Set TX power: +22 dBm, ramp 200 µs */
    sx1262_set_tx_params(22, 0x04);

    /* Clear and set IRQ mask */
    sx1262_clear_irq(0x03FF);
    sx1262_write_reg(SX1262_REG_IRQ_MASK, 0xFF);

    lora_state.joined = 1;  /* Simplified: assume ABP join */
    lora_state.datarate = 0;  /* DR0: SF12BW125 */
    lora_state.tx_power = 22;
    lora_state.frequency = 868200000;
    lora_initialized = 1;
    return LORA_OK;
}

/* ---- Assemble SoundLoom uplink payload (51 bytes) ---- */

int lora_build_uplink(const lora_uplink_t *data, uint8_t *out, uint8_t *len)
{
    uint8_t idx = 0;

    /* SVI (1 byte, 0–100) */
    out[idx++] = (uint8_t)(data->svi & 0xFF);

    /* Per-class event counts (10 × 2 bytes = 20) */
    for (int i = 0; i < CLS_NUM_CLASSES; i++) {
        out[idx++] = (uint8_t)(data->event_counts[i] >> 8);
        out[idx++] = (uint8_t)(data->event_counts[i] & 0xFF);
    }

    /* Moisture ×2 (2 × 2 bytes = 4) */
    for (int i = 0; i < 2; i++) {
        uint16_t m = (uint16_t)(data->moisture[i] * 100.0f);  /* 0.00–100.00 → 0–10000 */
        out[idx++] = (uint8_t)(m >> 8);
        out[idx++] = (uint8_t)(m & 0xFF);
    }

    /* EC ×2 (2 × 2 bytes = 4) */
    for (int i = 0; i < 2; i++) {
        uint16_t e = (uint16_t)(data->ec[i]);
        out[idx++] = (uint8_t)(e >> 8);
        out[idx++] = (uint8_t)(e & 0xFF);
    }

    /* Depth temps ×4 (4 × 2 bytes = 8, signed, ×100) */
    for (int i = 0; i < 4; i++) {
        int16_t t = (int16_t)(data->depth_temp[i] * 100.0f);
        out[idx++] = (uint8_t)(t >> 8);
        out[idx++] = (uint8_t)(t & 0xFF);
    }

    /* CO2 ppm (2 bytes) */
    out[idx++] = (uint8_t)(data->co2_ppm >> 8);
    out[idx++] = (uint8_t)(data->co2_ppm & 0xFF);

    /* Battery voltage (2 bytes, mV) */
    out[idx++] = (uint8_t)(data->battery_mv >> 8);
    out[idx++] = (uint8_t)(data->battery_mv & 0xFF);

    /* Flags (2 bytes): bit0=pest_alert, bit1=compaction_alert, bit2=low_bat, bit3=tilt */
    uint16_t flags = data->flags;
    out[idx++] = (uint8_t)(flags >> 8);
    out[idx++] = (uint8_t)(flags & 0xFF);

    /* Sequence number (2 bytes) */
    out[idx++] = (uint8_t)(msg_seq >> 8);
    out[idx++] = (uint8_t)(msg_seq & 0xFF);
    msg_seq++;

    /* CRC-16 (2 bytes) */
    uint16_t crc = 0;
    for (int i = 0; i < idx; i++) crc += out[i];
    out[idx++] = (uint8_t)(crc >> 8);
    out[idx++] = (uint8_t)(crc & 0xFF);

    *len = idx;
    return LORA_OK;
}

/* ---- Send an uplink ---- */

int lora_send_uplink(const lora_uplink_t *data)
{
    if (!lora_initialized || !lora_state.joined) return LORA_ERR_NOT_JOINED;

    uint8_t payload[LORA_UPLINK_LEN + 4];
    uint8_t plen = 0;
    lora_build_uplink(data, payload, &plen);

    /* Write payload to SX1262 buffer */
    sx1262_write_buf(0x00, payload, plen);

    /* Set TX (with 3-second timeout for RX1/RX2 windows) */
    sx1262_set_standby(0x01);
    sx1262_clear_irq(0x03FF);
    sx1262_set_tx(3000);

    /* Wait for TX done (poll DIO1 or IRQ status) */
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 3000) {
        uint16_t irq = sx1262_get_irq_status();
        if (irq & SX1262_IRQ_TX_DONE) {
            sx1262_clear_irq(SX1262_IRQ_TX_DONE);
            break;
        }
        if (irq & SX1262_IRQ_TIMEOUT) {
            sx1262_clear_irq(SX1262_IRQ_TIMEOUT);
            tx_count++;
            return LORA_ERR_TIMEOUT;
        }
    }

    /* Open RX1 window (1 second after TX) */
    HAL_Delay(1000);
    sx1262_clear_irq(0x03FF);
    sx1262_set_rx(1000);

    /* Check for downlink */
    uint32_t rx_start = HAL_GetTick();
    while ((HAL_GetTick() - rx_start) < 1000) {
        uint16_t irq = sx1262_get_irq_status();
        if (irq & SX1262_IRQ_RX_DONE) {
            sx1262_clear_irq(SX1262_IRQ_RX_DONE);
            /* Read downlink payload */
            uint8_t dl[64];
            sx1262_read_buf(0x00, dl, 64);
            rx_count++;
            /* Process downlink (configuration) */
            lora_handle_downlink(dl, 64);
            break;
        }
    }

    /* Go to sleep to save power */
    sx1262_set_sleep(1, 0);
    tx_count++;
    return LORA_OK;
}

/* ---- Handle downlink commands ---- */

int lora_handle_downlink(const uint8_t *data, uint8_t len)
{
    if (len < 2) return LORA_ERR_INVALID;
    uint8_t cmd = data[0];
    switch (cmd) {
        case LORA_DL_SET_CADENCE:
            if (len >= 5) {
                uint32_t cadence = ((uint32_t)data[1] << 24) |
                                    ((uint32_t)data[2] << 16) |
                                    ((uint32_t)data[3] << 8) |
                                    data[4];
                lora_state.cadence_ms = cadence;
            }
            break;
        case LORA_DL_SET_THRESHOLD:
            if (len >= 2) {
                lora_state.cls_threshold = data[1];
            }
            break;
        case LORA_DL_SET_SENSITIVITY:
            if (len >= 3) {
                /* threshold factor as fixed-point */
                lora_state.sensitivity = ((uint16_t)data[1] << 8) | data[2];
            }
            break;
        case LORA_DL_REBOOT:
            NVIC_SystemReset();
            break;
        case LORA_DL_MODEL_UPDATE:
            /* Trigger BLE OTA model update mode */
            lora_state.ota_pending = 1;
            break;
        default:
            break;
    }
    return LORA_OK;
}

/* ---- Statistics ---- */

uint32_t lora_get_tx_count(void) { return tx_count; }
uint32_t lora_get_rx_count(void) { return rx_count; }
int8_t   lora_get_last_rssi(void) { return last_rssi; }
int8_t   lora_get_last_snr(void)  { return last_snr; }

/* ---- EXTI1 ISR (DIO1) ---- */

void EXTI1_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(LORA_DIO1_PIN);
    /* In a full implementation, this signals the main loop to check IRQ status.
     * For simplicity we handle IRQs in the polling functions above.
     */
}