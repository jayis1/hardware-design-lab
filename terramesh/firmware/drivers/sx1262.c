/**
 * @file    sx1262.c
 * @brief   Terramesh — Semtech SX1262 LoRa transceiver driver
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * Implements SPI command interface, TX/RX mode switching, CAD, and
 * configuration for the SX1262 LoRa transceiver used in the Terramesh
 * subterranean mesh network.
 */

#include <stdint.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"

/* SX1262 command opcodes */
#define SX1262_CMD_SET_SLEEP        0x84
#define SX1262_CMD_SET_STANDBY      0x80
#define SX1262_CMD_SET_FS           0x81
#define SX1262_CMD_SET_TX           0x83
#define SX1262_CMD_SET_RX           0x82
#define SX1262_CMD_SET_RXDUTYCYCLE  0x94
#define SX1262_CMD_SET_CAD          0xC5
#define SX1262_CMD_SET_TXCONTINUOUS 0x85
#define SX1262_CMD_SET_TXCFGMODE    0x87
#define SX1262_CMD_SET_PACKETTYPE   0x8A
#define SX1262_CMD_GET_STATUS       0xC0
#define SX1262_CMD_WRITE_REGISTER   0x0D
#define SX1262_CMD_READ_REGISTER    0x1D
#define SX1262_CMD_WRITE_BUFFER     0x0E
#define SX1262_CMD_READ_BUFFER      0x1E
#define SX1262_CMD_SET_DIOIRQ       0x08
#define SX1262_CMD_GET_IRQREASON    0x12
#define SX1262_CMD_SET_RFREQUENCY   0x86
#define SX1262_CMD_SET_PACKETPARAMS 0x8C
#define SX1262_CMD_SET_MODULATION   0x8B
#define SX1262_CMD_SET_TXPARAMS     0x8E
#define SX1262_CMD_SET_CADPARAMS    0x88
#define SX1262_CMD_SET_BUFFERBASE   0x8F
#define SX1262_CMD_CALIBRATE        0x89
#define SX1262_CMD_CALIBRATEIMAGE   0x98
#define SX1262_CMD_SET_TCXOMODE     0x97
#define SX1262_CMD_SET_TXMODULATION 0x99
#define SX1262_CMD_GET_RSSIINST     0x15
#define SX1262_CMD_GET_PACKETSTATUS 0x14

/* SX1262 register addresses */
#define SX1262_REG_SYNCWORD_MSB     0x0740
#define SX1262_REG_SYNCWORD_LSB     0x0741
#define SX1262_REG_PACKETPARAMS1    0x0704
#define SX1262_REG_PAYLOADLENGTH    0x0702
#define SX1262_REG_MODULATIONCFG    0x0708
#define SX1262_REG_DIO1_MAPPING     0x0580
#define SX1262_REG_RX_GAIN          0x08AC
#define SX1262_REG_OCP              0x08E7
#define SX1262_REG_RTC_CTRL         0x0902
#define SX1262_REG_XTATEMP          0x0911

/* Status bits */
#define SX1262_STATUS_MODE_MASK     0x70
#define SX1262_STATUS_MODE_STBY_RC  0x10
#define SX1262_STATUS_MODE_STBY_XOSC 0x20
#define SX1262_STATUS_MODE_FS       0x30
#define SX1262_STATUS_MODE_RX       0x40
#define SX1262_STATUS_MODE_TX       0x50

/* IRQ masks */
#define SX1262_IRQ_TX_DONE          (1UL << 0)
#define SX1262_IRQ_RX_DONE          (1UL << 1)
#define SX1262_IRQ_PREAMBLE_DETECT  (1UL << 3)
#define SX1262_IRQ_SYNCWORD_VALID   (1UL << 4)
#define SX1262_IRQ_HEADER_VALID     (1UL << 5)
#define SX1262_IRQ_HEADER_ERR       (1UL << 6)
#define SX1262_IRQ_CRC_ERR         (1UL << 7)
#define SX1262_IRQ_CAD_DONE         (1UL << 8)
#define SX1262_IRQ_CAD_DETECTED     (1UL << 9)
#define SX1262_IRQ_RX_TX_TIMEOUT    (1UL << 11)

/* ======================================================================== *
 *  Private helpers                                                             *
 * ======================================================================== */

static void sx1262_wait_busy(void) {
    while (GPIO_READ_PIN(GPIOA_BASE, PIN_SX1262_BUSY)) { __asm__("nop"); }
}

static void sx1262_spi_cmd(const uint8_t *cmd, uint32_t cmd_len,
                           const uint8_t *tx_data, uint32_t tx_len,
                           uint8_t *rx_data, uint32_t rx_len) {
    sx1262_wait_busy();
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_NSS);
    for (uint32_t i = 0; i < cmd_len; i++) {
        while (!(REG_READ(SPI_SR(SPI1_BASE)) & SPI_SR_TXE)) { __asm__("nop"); }
        REG_WRITE(SPI_DR(SPI1_BASE), cmd[i]);
    }
    if (tx_data) {
        for (uint32_t i = 0; i < tx_len; i++) {
            while (!(REG_READ(SPI_SR(SPI1_BASE)) & SPI_SR_TXE)) { __asm__("nop"); }
            REG_WRITE(SPI_DR(SPI1_BASE), tx_data[i]);
        }
    }
    if (rx_data) {
        for (uint32_t i = 0; i < rx_len; i++) {
            while (!(REG_READ(SPI_SR(SPI1_BASE)) & SPI_SR_RXNE)) { __asm__("nop"); }
            rx_data[i] = (uint8_t)REG_READ(SPI_DR(SPI1_BASE));
        }
    }
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_NSS);
}

/* ======================================================================== *
 *  Public API                                                                  *
 * ======================================================================== */

void sx1262_reset(void) {
    GPIO_CLR_PIN(GPIOA_BASE, PIN_SX1262_RESET);
    for (volatile uint32_t i = 0; i < 10000; i++) { __asm__("nop"); }
    GPIO_SET_PIN(GPIOA_BASE, PIN_SX1262_RESET);
    for (volatile uint32_t i = 0; i < 50000; i++) { __asm__("nop"); }
    sx1262_wait_busy();
}

uint8_t sx1262_get_status(void) {
    uint8_t cmd[] = { SX1262_CMD_GET_STATUS, 0x00 };
    uint8_t rx[2];
    sx1262_spi_cmd(cmd, 2, NULL, 0, rx, 2);
    return rx[1];
}

void sx1262_set_standby(uint8_t mode) {
    uint8_t cmd[] = { SX1262_CMD_SET_STANDBY, mode };
    sx1262_spi_cmd(cmd, 2, NULL, 0, NULL, 0);
}

void sx1262_set_sleep(uint8_t wakeup_config) {
    uint8_t cmd[] = { SX1262_CMD_SET_SLEEP, wakeup_config };
    sx1262_spi_cmd(cmd, 2, NULL, 0, NULL, 0);
}

void sx1262_set_frequency(uint32_t freq_hz) {
    /* Frequency = (freq_hz * 2^25) / 32e6 = freq_hz / 61.035 */
    uint32_t frf = (uint32_t)((float)freq_hz / 61.035f + 0.5f);
    uint8_t cmd[] = {
        SX1262_CMD_SET_RFREQUENCY,
        (uint8_t)(frf >> 16),
        (uint8_t)(frf >> 8),
        (uint8_t)(frf)
    };
    sx1262_spi_cmd(cmd, 4, NULL, 0, NULL, 0);
}

void sx1262_set_tx_params(int8_t power_dbm, uint8_t ramp_time) {
    /* power_dbm: -9 to +22 dBm, mapped to register value */
    uint8_t reg_power;
    if (power_dbm <= -9) reg_power = 0x00;
    else if (power_dbm >= 22) reg_power = 0x1F;
    else reg_power = (uint8_t)(power_dbm + 9);

    uint8_t cmd[] = { SX1262_CMD_SET_TXPARAMS, reg_power, ramp_time };
    sx1262_spi_cmd(cmd, 3, NULL, 0, NULL, 0);
}

void sx1262_set_modulation_params(uint8_t sf, uint8_t bw_khz,
                                    uint8_t cr, uint8_t ldro) {
    uint8_t bw_reg;
    switch (bw_khz) {
        case 7:   bw_reg = 0; break;
        case 10:  bw_reg = 1; break;
        case 15:  bw_reg = 2; break;
        case 20:  bw_reg = 3; break;
        case 31:  bw_reg = 4; break;
        case 41:  bw_reg = 5; break;
        case 62:  bw_reg = 6; break;
        case 125: bw_reg = 7; break;
        case 250: bw_reg = 8; break;
        case 500: bw_reg = 9; break;
        default:  bw_reg = 7; break;
    }

    uint8_t cmd[] = {
        SX1262_CMD_SET_MODULATION,
        sf,
        bw_reg,
        cr,
        ldro,
        0x00, 0x00, 0x00
    };
    sx1262_spi_cmd(cmd, 8, NULL, 0, NULL, 0);
}

void sx1262_set_packet_params(uint8_t preamble_len, uint8_t header_type,
                               uint8_t payload_len, uint8_t crc_type,
                               uint8_t invert_iq) {
    uint8_t cmd[] = {
        SX1262_CMD_SET_PACKETPARAMS,
        preamble_len,
        header_type,
        payload_len,
        crc_type,
        invert_iq,
        0x00, 0x00
    };
    sx1262_spi_cmd(cmd, 8, NULL, 0, NULL, 0);
}

void sx1262_set_dio_irq(uint16_t irq_mask, uint16_t dio1_mask,
                         uint16_t dio2_mask, uint16_t dio3_mask) {
    uint8_t cmd[] = {
        SX1262_CMD_SET_DIOIRQ,
        0x01,  /* IRQ config page */
        (uint8_t)(irq_mask >> 8),
        (uint8_t)(irq_mask),
        (uint8_t)(dio1_mask >> 8),
        (uint8_t)(dio1_mask),
        (uint8_t)(dio2_mask >> 8),
        (uint8_t)(dio2_mask),
        (uint8_t)(dio3_mask >> 8),
        (uint8_t)(dio3_mask)
    };
    sx1262_spi_cmd(cmd, 10, NULL, 0, NULL, 0);
}

void sx1262_set_tx(uint32_t timeout_ms) {
    /* Timeout = timeout_ms * 15.625 µs */
    uint32_t timeout = (uint32_t)((float)timeout_ms * 1000.0f / 15.625f);
    uint8_t cmd[] = {
        SX1262_CMD_SET_TX,
        (uint8_t)(timeout >> 16),
        (uint8_t)(timeout >> 8),
        (uint8_t)(timeout)
    };
    sx1262_spi_cmd(cmd, 4, NULL, 0, NULL, 0);
}

void sx1262_set_rx(uint32_t timeout_ms) {
    uint32_t timeout = (uint32_t)((float)timeout_ms * 1000.0f / 15.625f);
    uint8_t cmd[] = {
        SX1262_CMD_SET_RX,
        (uint8_t)(timeout >> 16),
        (uint8_t)(timeout >> 8),
        (uint8_t)(timeout)
    };
    sx1262_spi_cmd(cmd, 4, NULL, 0, NULL, 0);
}

void sx1262_write_buffer(uint8_t offset, const uint8_t *data, uint32_t len) {
    uint8_t cmd[] = { SX1262_CMD_WRITE_BUFFER, 0x00, offset };
    sx1262_spi_cmd(cmd, 3, data, len, NULL, 0);
}

uint32_t sx1262_read_buffer(uint8_t offset, uint8_t *data, uint32_t max_len) {
    uint8_t cmd[] = { SX1262_CMD_READ_BUFFER, 0x00, offset };
    uint8_t status[2];
    sx1262_spi_cmd(cmd, 3, NULL, 0, status, 2);
    /* status[1] contains payload length */
    uint8_t payload_len = status[1];
    if (payload_len > max_len) payload_len = max_len;
    sx1262_spi_cmd(NULL, 0, NULL, 0, data, payload_len);
    return payload_len;
}

void sx1262_write_register(uint16_t reg, uint8_t val) {
    uint8_t cmd[] = {
        SX1262_CMD_WRITE_REGISTER,
        (uint8_t)(reg >> 8),
        (uint8_t)(reg),
        val
    };
    sx1262_spi_cmd(cmd, 4, NULL, 0, NULL, 0);
}

uint8_t sx1262_read_register(uint16_t reg) {
    uint8_t cmd[] = {
        SX1262_CMD_READ_REGISTER,
        (uint8_t)(reg >> 8),
        (uint8_t)(reg),
        0x00
    };
    uint8_t rx[4];
    sx1262_spi_cmd(cmd, 4, NULL, 0, rx, 4);
    return rx[3];
}

uint16_t sx1262_get_irq_reason(void) {
    uint8_t cmd[] = { SX1262_CMD_GET_IRQREASON, 0x00, 0x00 };
    uint8_t rx[3];
    sx1262_spi_cmd(cmd, 3, NULL, 0, rx, 3);
    return ((uint16_t)rx[1] << 8) | rx[2];
}

void sx1262_clear_irq(uint16_t irq_mask) {
    uint8_t cmd[] = {
        SX1262_CMD_SET_DIOIRQ,
        0x02,  /* IRQ clear page */
        (uint8_t)(irq_mask >> 8),
        (uint8_t)(irq_mask)
    };
    sx1262_spi_cmd(cmd, 4, NULL, 0, NULL, 0);
}

int16_t sx1262_get_rssi_inst(void) {
    uint8_t cmd[] = { SX1262_CMD_GET_RSSIINST, 0x00 };
    uint8_t rx[2];
    sx1262_spi_cmd(cmd, 2, NULL, 0, rx, 2);
    return -(int16_t)(rx[1] / 2);
}

void sx1262_get_packet_status(int16_t *rssi, int8_t *snr) {
    uint8_t cmd[] = { SX1262_CMD_GET_PACKETSTATUS, 0x00, 0x00, 0x00 };
    uint8_t rx[4];
    sx1262_spi_cmd(cmd, 4, NULL, 0, rx, 4);
    if (rssi) *rssi = -(int16_t)(rx[2] / 2);
    if (snr) *snr = (int8_t)(rx[3] / 4);
}

void sx1262_set_sync_word(uint16_t sync_word) {
    sx1262_write_register(SX1262_REG_SYNCWORD_MSB, (uint8_t)(sync_word >> 8));
    sx1262_write_register(SX1262_REG_SYNCWORD_LSB, (uint8_t)(sync_word));
}

void sx1262_set_payload_length(uint8_t len) {
    sx1262_write_register(SX1262_REG_PAYLOADLENGTH, len);
}

void sx1262_set_rx_gain(uint8_t gain) {
    sx1262_write_register(SX1262_REG_RX_GAIN, gain);
}

void sx1262_set_ocp(uint8_t ocp_ma) {
    /* OCP value: 0 = 45 mA, 1 = 50 mA, ... 15 = 120 mA, 16+ = 130 mA + (val-16)*10 */
    uint8_t reg;
    if (ocp_ma <= 45) reg = 0;
    else if (ocp_ma <= 120) reg = (ocp_ma - 40) / 5;
    else reg = 16 + (ocp_ma - 130) / 10;
    sx1262_write_register(SX1262_REG_OCP, reg);
}

void sx1262_calibrate(uint8_t calib_mask) {
    uint8_t cmd[] = { SX1262_CMD_CALIBRATE, calib_mask };
    sx1262_spi_cmd(cmd, 2, NULL, 0, NULL, 0);
    sx1262_wait_busy();
}

void sx1262_calibrate_image(uint32_t freq_hz) {
    /* Select calibration band based on frequency */
    uint8_t band;
    if (freq_hz < 430000000) band = 0x6B;  /* 430–440 MHz */
    else if (freq_hz < 480000000) band = 0x75;  /* 470–510 MHz */
    else if (freq_hz < 780000000) band = 0x81;  /* 779–787 MHz */
    else if (freq_hz < 900000000) band = 0x96;  /* 863–870 MHz */
    else if (freq_hz < 930000000) band = 0xA1;  /* 902–928 MHz */
    else band = 0xC5;  /* 960–1020 MHz */

    uint8_t cmd[] = { SX1262_CMD_CALIBRATEIMAGE, band };
    sx1262_spi_cmd(cmd, 2, NULL, 0, NULL, 0);
    sx1262_wait_busy();
}

void sx1262_set_tcxo_mode(uint8_t tcxo_voltage, uint32_t timeout_ms) {
    uint32_t timeout = (uint32_t)((float)timeout_ms * 1000.0f / 15.625f);
    uint8_t cmd[] = {
        SX1262_CMD_SET_TCXOMODE,
        tcxo_voltage,
        (uint8_t)(timeout >> 16),
        (uint8_t)(timeout >> 8),
        (uint8_t)(timeout)
    };
    sx1262_spi_cmd(cmd, 5, NULL, 0, NULL, 0);
}

bool sx1262_init_full(uint32_t freq_hz, uint8_t sf, uint8_t bw_khz,
                       uint8_t cr, int8_t power_dbm) {
    /* Reset the module */
    sx1262_reset();

    /* Set standby mode */
    sx1262_set_standby(0x00);  /* STDBY_RC */

    /* Calibrate */
    sx1262_calibrate(0x7F);  /* All blocks */

    /* Calibrate image for frequency band */
    sx1262_calibrate_image(freq_hz);

    /* Set TCXO mode if using external TCXO */
    /* (Not used in Terramesh — using internal RC) */

    /* Set packet type to LoRa */
    uint8_t cmd[] = { SX1262_CMD_SET_PACKETTYPE, 0x01 };  /* 0x01 = LoRa */
    sx1262_spi_cmd(cmd, 2, NULL, 0, NULL, 0);

    /* Set frequency */
    sx1262_set_frequency(freq_hz);

    /* Set TX parameters */
    sx1262_set_tx_params(power_dbm, 0x04);  /* 40 µs ramp */

    /* Set modulation parameters */
    sx1262_set_modulation_params(sf, bw_khz, cr, 0x00);

    /* Set packet parameters: variable length, CRC on */
    sx1262_set_packet_params(8, 0x00, 0xFF, 0x01, 0x00);

    /* Set sync word */
    sx1262_set_sync_word(0x2B);

    /* Set DIO1 to TX_DONE and RX_DONE */
    sx1262_set_dio_irq(
        SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_CRC_ERR,
        SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE,
        0, 0);

    /* Set RX gain to maximum */
    sx1262_set_rx_gain(0x94);  /* Power saving mode */

    /* Set OCP to 140 mA */
    sx1262_set_ocp(140);

    /* Verify communication */
    uint8_t status = sx1262_get_status();
    return ((status & SX1262_STATUS_MODE_MASK) == SX1262_STATUS_MODE_STBY_RC);
}
