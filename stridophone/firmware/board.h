/*
 * board.h — Stridophone pin map, clock tree, and peripheral assignments.
 *
 * Target : STM32H743VIT6 (LQFP100, Cortex-M7F @ 480 MHz)
 * Author : jayis1
 * License: MIT
 *
 * This file concentrates every hardware-level constant so that the rest of
 * the firmware stays board-agnostic. Pins are chosen so that no two
 * alternates collide on the same AF slot and so that the high-speed signals
 * (SAI, SDMMC, SPI) land on pads rated for the relevant max toggle rate.
 */
#ifndef STRIDOPHONE_BOARD_H
#define STRIDOPHONE_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Author / version metadata ----------------------------------- */
#define STRIDO_AUTHOR      "jayis1"
#define STRIDO_FW_VERSION  "1.0.0"
#define STRIDO_BUILD_DATE  __DATE__

/* ---- Clock tree --------------------------------------------------- */
/* HSE on board is a 16 MHz xtal. PLL1 builds SYS=480, CPU=480, AXI=240. */
#define BOARD_HSE_HZ       16000000UL
#define BOARD_SYS_HZ       480000000UL
#define BOARD_CPU_HZ       480000000UL
#define BOARD_AXI_HZ       240000000UL
#define BOARD_APB1_HZ      120000000UL
#define BOARD_APB2_HZ      240000000UL
#define BOARD_APB4_HZ      120000000UL

/* ---- SAI / I2S audio ---------------------------------------------- */
/* Four Knowles SPH0641LU4H-1 MEMS mics on a square array, 40 mm spacing.
 * We use SAI1 in TDM mode: FS = 48 kHz, 4 slots @ 24-bit (96-bit slot
 * total with padding), MCLK = 256*FS = 12.288 MHz from SAI PLL. */
#define AUDIO_FS_HZ        48000
#define AUDIO_BITS         24
#define AUDIO_NCH          4
#define AUDIO_BLOCK        512   /* samples per half DMA buffer per channel */
#define AUDIO_BLOCK_BYTES  (AUDIO_BLOCK * (AUDIO_BITS/8) * AUDIO_NCH)

/* SAI1 pin assignments (LQFP100) */
#define SAI1_SD1_PIN       99    /* PA0  — SAI1_SD_B */
#define SAI1_SD2_PIN       38    /* PB2  — SAI1_SD_A */
#define SAI1_FS_PIN        89    /* PA4  — SAI1_FS_A */
#define SAI1_SCK_PIN       90    /* PA5  — SAI1_SCK_A */
#define SAI1_MCLK_PIN      33    /* PC0  — SAI1_MCLK_A */
/* SAI1 port GPIO assignments */
#define SAI1_SD1_PORT      GPIOA
#define SAI1_SD1_PAD       0
#define SAI1_SD2_PORT      GPIOB
#define SAI1_SD2_PAD       2
#define SAI1_FS_PORT       GPIOA
#define SAI1_FS_PAD        4
#define SAI1_SCK_PORT      GPIOA
#define SAI1_SCK_PAD       5
#define SAI1_MCLK_PORT     GPIOC
#define SAI1_MCLK_PAD      0

/* ---- ADXL355 vibration accelerometer (SPI2) ---------------------- */
#define ADXL355_SPI          SPI2
#define ADXL355_CS_PORT      GPIOD
#define ADXL355_CS_PAD       8
#define ADXL355_SCK_PORT     GPIOD
#define ADXL355_SCK_PAD      3      /* SPI2_SCK on PD3 */
#define ADXL355_MISO_PORT    GPIOD
#define ADXL355_MISO_PAD     4      /* SPI2_MISO on PD4 */
#define ADXL355_MOSI_PORT    GPIOD
#define ADXL355_MOSI_PAD     5      /* SPI2_MOSI on PD5 */
#define ADXL355_DRDY_PORT    GPIOD
#define ADXL355_DRDY_PAD     6
#define ADXL355_SPI_BAUD     1000000   /* 1 MHz max for full res */

/* ---- SHT45 environmental sensor + BQ25895 charger (I2C4) ---------- */
#define I2C4_PERIPH          I2C4
#define I2C4_SCL_PORT        GPIOD
#define I2C4_SCL_PAD         12
#define I2C4_SDA_PORT        GPIOD
#define I2C4_SDA_PAD         13
#define I2C4_BAUD            100000

#define SHT45_ADDR           0x44
#define BQ25895_ADDR         0x6A

/* ---- SDMMC1 (4-bit) ---------------------------------------------- */
#define SDMMC1_CK_PORT       GPIOC
#define SDMMC1_CK_PAD        12
#define SDMMC1_CMD_PORT      GPIOD
#define SDMMC1_CMD_PAD       2
#define SDMMC1_D0_PORT       GPIOC
#define SDMMC1_D0_PAD        8
#define SDMMC1_D1_PORT       GPIOC
#define SDMMC1_D1_PAD        9
#define SDMMC1_D2_PORT       GPIOC
#define SDMMC1_D2_PAD        10
#define SDMMC1_D3_PORT       GPIOC
#define SDMMC1_D3_PAD        11
#define SDMMC_DET_PORT       GPIOC
#define SDMMC_DET_PAD        13

/* ---- ESP32-C6 co-processor (USART3) ------------------------------ */
#define ESP_UART             USART3
#define ESP_UART_BAUD        1000000
#define ESP_TX_PORT          GPIOB
#define ESP_TX_PAD           10
#define ESP_RX_PORT          GPIOB
#define ESP_RX_PAD           11
#define ESP_BOOT_PORT        GPIOB
#define ESP_BOOT_PAD         12     /* ESP BOOT0 strap (reset-to-flash) */
#define ESP_EN_PORT          GPIOB
#define ESP_EN_PAD           13     /* ESP CHIP_EN (reset) */
#define ESP_HANDSHAKE_PORT   GPIOB
#define ESP_HANDSHAKE_PAD    14     /* ESP -> STM32 "data ready" IRQ */

/* ---- Status LEDs + button ---------------------------------------- */
#define LED_STATUS_PORT      GPIOE
#define LED_STATUS_PAD       0      /* solid = running, blink = booting */
#define LED_ACTIVITY_PORT    GPIOE
#define LED_ACTIVITY_PAD     1      /* pulse on DSP frame */
#define LED_ALERT_PORT       GPIOE
#define LED_ALERT_PAD        2      /* lit when an alert is armed */
#define LED_CHARGE_PORT      GPIOE
#define LED_CHARGE_PAD       3      /* reflects BQ25895 STAT */

#define BTN_PORT             GPIOC
#define BTN_PAD              5      /* tactile: long-press = factory reset */

/* ---- Misc power / enable ----------------------------------------- */
#define PMIC_VBUS_PORT       GPIOA
#define PMIC_VBUS_PAD        9      /* USB-C VBUS detect */
#define MIC_EN_PORT          GPIOA
#define MIC_EN_PAD           15     /* gate the mic analog rail */
#define VIBE_EN_PORT         GPIOB
#define VIBE_EN_PAD          15     /* gate ADXL355 rail (idle power-down) */

/* ---- DMA channels ------------------------------------------------- */
#define DMA_AUDIO_RX_STREAM  0      /* BDMA stream 0 for SAI1 RX */
#define DMA_AUDIO_RX_CHAN    BDMA_REQUEST_SAI1_A
#define DMA_SDMMC_STREAM     0      /* SDMMC1 uses its own DMA controller */

/* ---- Timing ------------------------------------------------------- */
#define DSP_FRAME_HOP        512
#define DSP_FRAME_N          1024
#define DSP_FRAME_MS         (1000 * DSP_FRAME_HOP / AUDIO_FS_HZ)  /* ~10.7 */
#define CLASSIFY_INTERVAL_MS 1000   /* run classifier at 1 Hz */
#define HEARTBEAT_MS         250
#define ENV_SAMPLE_MS        5000
#define SD_FLUSH_MS          30000

/* ---- Classifier --------------------------------------------------- */
#define CLF_N_FEATURES       44
#define CLF_N_CLASSES        26
#define CLF_N_TREES          32
#define CLF_MAX_DEPTH        8
#define CLF_CONF_THRESHOLD   55     /* percent; below => "unknown" */

/* ---- Event / network --------------------------------------------- */
#define EVENT_QUEUE_DEPTH    64
#define WAV_SNIPPET_MS       2000
#define NET_TX_BURST_MAX     8

/* ---- Forward decls of driver init functions ---------------------- */
void board_init(void);
void board_clock_init(void);
void board_gpio_init(void);

#endif /* STRIDOPHONE_BOARD_H */