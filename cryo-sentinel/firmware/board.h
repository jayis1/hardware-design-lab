/*
 * board.h — Pin map and board constants for Cryo-Sentinel.
 *
 * Target: Nordic nRF5340 PDK + custom Cryo-Sentinel carrier.
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_BOARD_H
#define CRYO_SENTINEL_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- nRF5340 P1 (app core) pin map ----------------------------------- */

/* I2C bus 0: FDC2214 (level), MAX31865 control regs, BME280, IMU, FRAM.
 * I2C bus 1: TCA9548A mux channel selection only (same physical bus 0
 *            routed through the mux to the three MAX31865s). */
#define CS_I2C_SDA_P0           26
#define CS_I2C_SCL_P0           27

/* SPI bus 0: MAX31865 RTD chips (3x), selected via TCA9548A on I2C.
 * The MAX31865 itself is SPI; we use a slow SPI @ 1 MHz. */
#define CS_SPI_MOSI             23
#define CS_SPI_MISO             24
#define CS_SPI_SCK              25
#define CS_SPI_CS_MAX31865_A    6   /* top RTD */
#define CS_SPI_CS_MAX31865_B    7   /* mid RTD */
#define CS_SPI_CS_MAX31865_C    8   /* bottom RTD */

/* GPIOs */
#define CS_GPIO_LID_REED        11  /* reed switch on Dewar lid hinge */
#define CS_GPIO_ENC_HALL        12  /* hall switch on enclosure lid (tamper) */
#define CS_GPIO_MAINS_OPTO      13  /* opto output, low = mains present */
#define CS_GPIO_CELL_PWR_EN     14  /* load-switch gate for BG770A VINT */
#define CS_GPIO_CELL_PWRGOOD    15  /* BG770A power-good / status */
#define CS_GPIO_LED_WARN        28  /* amber warning LED */
#define CS_GPIO_LED_CRIT        29  /* red critical LED */
#define CS_GPIO_BUZZER          30  /* piezo buzzer (local annunciation) */
#define CS_GPIO_PROBE_PRESENT   31  /* pulldown on probe harness detect */

/* UART to BG770A cellular modem */
#define CS_UART_CELL_TX         32
#define CS_UART_CELL_RX         33
#define CS_UART_CELL_RTS        34
#define CS_UART_CELL_CTS        35

/* IPC / radio to network core via nRF5340 IPC peripheral */
#define CS_IPC_TX_CH            0
#define CS_IPC_RX_CH            1

/* ---- Timing constants ------------------------------------------------ */
#define CS_TICK_MS              5000   /* main sensor tick */
#define CS_HEARTBEAT_MIN        15     /* cellular heartbeat interval */
#define CS_ALARM_REPEAT_MIN     5      /* unacked critical repeat interval */
#define CS_DEBOUNCE_MS          60     /* GPIO debounce window */

/* ---- Thresholds (defaults; per-Dewar override via BLE) ---------------- */
#define CS_LEVEL_WARN_PCT       35.0f
#define CS_LEVEL_CRIT_PCT       20.0f
#define CS_LEVEL_RATE_CRIT_PH   2.0f   /* %/hour drop -> critical */
#define CS_TILT_WARN_DEG        8.0f
#define CS_TILT_CRIT_DEG        15.0f
#define CS_LID_OPEN_WARN_MIN    10
#define CS_AMBIENT_WATCH_C      26.0f
#define CS_BATT_CRIT_PCT        15

/* ---- FRAM layout (MB85RC04, 512 x 8) --------------------------------- */
#define CS_FRAM_I2C_ADDR        0x50
#define CS_FRAM_MAGIC           0xC5A1
#define CS_FRAM_OFF_MAGIC       0x00   /* u16 */
#define CS_FRAM_OFF_SEQ         0x02   /* u32 monotonic */
#define CS_FRAM_OFF_CAL_BASE    0x10   /* 4-point calibration, 64 bytes */
#define CS_FRAM_OFF_LOG_BASE    0x80   /* ring log: 64 x 16-byte records */
#define CS_FRAM_LOG_CAP         64
#define CS_FRAM_LOG_REC_LEN     16

/* ---- Net-core IPC opcodes ------------------------------------------- */
#define CS_IPC_OP_HEARTBEAT     0x01
#define CS_IPC_OP_ALARM_T1      0x02
#define CS_IPC_OP_ALARM_T2      0x03
#define CS_IPC_OP_ALARM_ACK     0x04
#define CS_IPC_OP_BLE_PAIR      0x05
#define CS_IPC_OP_BLE_CAL       0x06
#define CS_IPC_OP_LOG_SYNC      0x07
#define CS_IPC_OP_SHUTDOWN      0x08

/* ---- Misc ----------------------------------------------------------- */
#define CS_LEVEL_SAMPLES        16    /* per-tick median filter size */
#define CS_GRAD_WINDOW_MIN      15    /* boil-off gradient window */
#define CS_RTD_COUNT            3
#define CS_RTD_TOP              0
#define CS_RTD_MID              1
#define CS_RTD_BOT              2

#endif /* CRYO_SENTINEL_BOARD_H */