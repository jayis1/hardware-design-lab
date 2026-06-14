/*
 * sx1262.h — Semtech SX1262 LoRa transceiver driver
 * SPI command interface, TX/RX, configuration for Chronos-RTK
 */

#ifndef SX1262_H
#define SX1262_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================== */
/* SX1262 SPI command opcodes                                                   */
/* ========================================================================== */
#define SX1262_CMD_SET_STANDBY          0x01
#define SX1262_CMD_SET_FS               0x02
#define SX1262_CMD_SET_TX               0x03
#define SX1262_CMD_SET_RX               0x04
#define SX1262_CMD_SET_RX_DUTY_CYCLE   0x05
#define SX1262_CMD_SET_LORA_PACKET_TYPE 0x07
#define SX1262_CMD_SET_MOD_PARAMS       0x08
#define SX1262_CMD_SET_PKT_PARAMS       0x09
#define SX1262_CMD_SET_CAD              0x0A
#define SX1262_CMD_SET_FREQ             0x0D
#define SX1262_CMD_SET_TX_PARAMS        0x0E
#define SX1262_CMD_WRITE_REG            0x0F
#define SX1262_CMD_READ_REG             0x10
#define SX1262_CMD_WRITE_BUF            0x12
#define SX1262_CMD_READ_BUF             0x13
#define SX1262_CMD_GET_STATUS            0x16
#define SX1262_CMD_GET_RSSI_INST        0x17
#define SX1262_CMD_GET_PKT_STATUS       0x1C
#define SX1262_CMD_CLR_IRQ              0x1D
#define SX1262_CMD_SET_IRQ_MASK          0x20
#define SX1262_CMD_SET_DIO_IRQ_PARAMS   0x08

/* ========================================================================== */
/* SX1262 register addresses                                                    */
/* ========================================================================== */
#define SX1262_REG_LORA_SYNC_WORD       0x0740
#define SX1262_REG_LORA_MOD_CFG         0x0741
#define SX1262_REG_LORA_PREAMBLE_MSB    0x0742
#define SX1262_REG_LORA_PREAMBLE_LSB    0x0743
#define SX1262_REG_PA_CONFIG            0x0889
#define SX1262_REG_PA_DUTY_CYCLE        0x088A
#define SX1262_REG_TX_CLAMP_CONFIG      0x088D
#define SX1262_REG_OCP_CONFIG           0x088E
#define SX1262_REG_TCXO_MODE            0x0891
#define SX1262_REG_TCXO_TRIM            0x0892
#define SX1262_REG_RX_GAIN              0x0899

/* ========================================================================== */
/* SX1262 IRQ flags                                                             */
/* ========================================================================== */
#define SX1262_IRQ_TX_DONE              (1U << 0)
#define SX1262_IRQ_RX_DONE              (1U << 1)
#define SX1262_IRQ_PREAMBLE_DETECTED    (1U << 2)
#define SX1262_IRQ_SYNC_WORD_VALID      (1U << 3)
#define SX1262_IRQ_HEADER_VALID         (1U << 4)
#define SX1262_IRQ_HEADER_ERR           (1U << 5)
#define SX1262_IRQ_CRC_ERR              (1U << 6)
#define SX1262_IRQ_CAD_DONE             (1U << 7)
#define SX1262_IRQ_CAD_DETECTED        (1U << 8)
#define SX1262_IRQ_TIMEOUT              (1U << 9)

/* ========================================================================== */
/* LoRa modulation parameters                                                   */
/* ========================================================================== */
typedef enum {
    SF5 = 5, SF6 = 6, SF7 = 7, SF8 = 8,
    SF9 = 9, SF10 = 10, SF11 = 11, SF12 = 12
} lora_spreading_factor_t;

typedef enum {
    BW_7KHZ = 0, BW_10KHZ = 1, BW_15KHZ = 2, BW_20KHZ = 3,
    BW_31KHZ = 4, BW_41KHZ = 5, BW_62KHZ = 6, BW_125KHZ = 7,
    BW_250KHZ = 8, BW_500KHZ = 9
} lora_bandwidth_t;

typedef enum {
    CR_4_5 = 1, CR_4_6 = 2, CR_4_7 = 3, CR_4_8 = 4
} lora_coding_rate_t;

typedef enum {
    HEADER_EXPLICIT = 0, HEADER_IMPLICIT = 1
} lora_header_t;

typedef enum {
    CRC_OFF = 0, CRC_ON = 1
} lora_crc_t;

/* ========================================================================== */
/* Default configuration                                                        */
/* ========================================================================== */
#define LORA_SYNC_WORD       0x12

/* ========================================================================== */
/* RTCM frame structure (for LoRa relay)                                       */
/* ========================================================================== */
#define RTCM_FRAME_MAX_SIZE  512

typedef struct {
    uint8_t  preamble;          /* 0xD3 */
    uint16_t length;            /* Frame length (after preamble + length) */
    uint8_t  data[RTCM_FRAME_MAX_SIZE];
    uint32_t crc;              /* CRC-24Q */
} rtcm_frame_t;

/* ========================================================================== */
/* Public API                                                                   */
/* ========================================================================== */

/**
 * Initialize SX1262 transceiver
 * - Reset the chip
 * - Configure SPI interface
 * - Set standby mode
 * - Calibrate RF
 * - Configure default parameters
 */
int sx1262_init(void);

/**
 * Set carrier frequency in Hz
 * Valid range: 150-960 MHz (check SX1262 datasheet for specific bands)
 */
int sx1262_set_frequency(uint32_t freq_hz);

/**
 * Set LoRa modulation parameters
 */
int sx1262_set_modulation_params(lora_spreading_factor_t sf,
                                  lora_bandwidth_t bw,
                                  lora_coding_rate_t cr);

/**
 * Set LoRa packet parameters
 */
int sx1262_set_packet_params(uint16_t preamble_len,
                               lora_header_t header_mode,
                               uint8_t payload_len,
                               lora_crc_t crc);

/**
 * Set TX output power (dBm)
 * Valid range: -17 to +22 dBm
 */
int sx1262_set_tx_power(int8_t power_dbm);

/**
 * Set LoRa sync word
 */
int sx1262_set_sync_word(uint8_t sync_word);

/**
 * Enter transmit mode
 * @param timeout_ms 0 = no timeout, else timeout in ms
 */
int sx1262_set_tx(uint32_t timeout_ms);

/**
 * Enter receive mode
 * @param timeout_ms 0 = continuous RX, else timeout in ms
 */
int sx1262_set_rx(uint32_t timeout_ms);

/**
 * Enter standby mode
 * @param rc_clock 0 = RC clock, 1 = XOSC (TCXO)
 */
int sx1262_set_standby(uint8_t rc_clock);

/**
 * Write data to SX1262 TX buffer
 * @param offset  Buffer offset (0-255)
 * @param data    Pointer to data
 * @param len     Data length
 */
int sx1262_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len);

/**
 * Read data from SX1262 RX buffer
 * @param offset  Buffer offset (0-255)
 * @param data    Pointer to output buffer
 * @param len     Data length to read
 */
int sx1262_read_buffer(uint8_t offset, uint8_t *data, uint8_t len);

/**
 * Clear IRQ status flags
 * @param irq_mask  Bitmask of IRQ flags to clear
 */
int sx1262_clear_irq(uint16_t irq_mask);

/**
 * Get current status byte
 * @return Status byte (chip mode + command status)
 */
uint8_t sx1262_get_status(void);

/**
 * Get instantaneous RSSI during RX
 * @return RSSI in dBm (negative)
 */
int16_t sx1262_get_rssi_inst(void);

/**
 * Handle DIO1 interrupt (called from EXTI ISR)
 * Processes TX_DONE, RX_DONE, CRC_ERR, TIMEOUT events
 */
void sx1262_handle_irq(void);

/**
 * Send RTCM frame over LoRa
 * @param frame  Pointer to RTCM frame
 * @return 0 on success, negative on error
 */
int sx1262_send_rtcm(const rtcm_frame_t *frame);

/**
 * Set callback for received RTCM frames
 * @param callback  Function pointer called when RTCM frame received
 */
typedef void (*rtcm_rx_callback_t)(const rtcm_frame_t *frame);
void sx1262_set_rtcm_callback(rtcm_rx_callback_t callback);

#endif /* SX1262_H */