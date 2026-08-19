/*
 * board.h — Pin assignments, constants, and configuration for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * All hardware-specific pin assignments, timing constants, and tuning
 * parameters live here so the rest of the firmware stays portable.
 */

#ifndef SPECKLEFLOW_BOARD_H
#define SPECKLEFLOW_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ---- Clock configuration ------------------------------------------------ */
/* HSE = 25 MHz external crystal on board.
 * SYSCLK = 480 MHz via PLL1 (P=2, N=192, M=4 → 25/4*192/2 = 600 MHz? no)
 * Actual: VCO = HSE/M * N = 25/5 * 120 = 600 MHz; SYS = VCO/P = 600/2 = 300?
 * STM32H733 max is 480 MHz: PLL1: M=5, N=192, P=2 → VCO=25/5*192=960, SYS=480.
 */
#define HSE_VALUE_HZ        25000000u
#define SYSCLK_HZ           480000000u
#define HCLK_HZ             240000000u   /* DIV2 for AHB (168 MHz max for H733? */
                                         /* Actually H733 supports 480/240 = 240 MHz AHB */
#define APB1_HZ             120000000u    /* DIV4 */
#define APB2_HZ             120000000u    /* DIV4 */
#define APB4_HZ             120000000u    /* DIV4 */

/* ---- Pin assignments (LQFP100) ------------------------------------------ */
/* We use GPIO alt-function numbers per STM32H7 datasheet. */

/* SPI1 — FPGA contrast data (RX from FPGA, 50 MHz) */
#define FPGA_SPI            SPI1
#define FPGA_SCK_PORT       GPIOB
#define FPGA_SCK_PIN        3
#define FPGA_SCK_AF         5
#define FPGA_MISO_PORT      GPIOB
#define FPGA_MISO_PIN       4
#define FPGA_MISO_AF        6
#define FPGA_MOSI_PORT      GPIOA
#define FPGA_MOSI_PIN       7     /* used for FPGA config in master mode */
#define FPGA_MOSI_AF        5
#define FPGA_NSS_PORT       GPIOA
#define FPGA_NSS_PIN        4
#define FPGA_NSS_AF         6
#define FPGA_IRQ_PORT       GPIOC
#define FPGA_IRQ_PIN        4     /* ext int: frame ready */
#define FPGA_CDONE_PORT     GPIOC
#define FPGA_CDONE_PIN      5     /* config done signal */
#define FPGA_CRST_PORT      GPIOC
#define FPGA_CRST_PIN       13    /* config reset (open-drain) */

/* SPI4 — ILI9341 TFT display (TX to display, 40 MHz) */
#define DISP_SPI            SPI4
#define DISP_SCK_PORT       GPIOE
#define DISP_SCK_PIN        2
#define DISP_SCK_AF         5
#define DISP_MISO_PORT      GPIOE
#define DISP_MISO_PIN       5
#define DISP_MISO_AF        5
#define DISP_MOSI_PORT      GPIOE
#define DISP_MOSI_PIN       6
#define DISP_MOSI_AF        5
#define DISP_CS_PORT        GPIOE
#define DISP_CS_PIN         4
#define DISP_DC_PORT        GPIOE
#define DISP_DC_PIN         3     /* data/command */
#define DISP_RST_PORT       GPIOE
#define DISP_RST_PIN        1
#define DISP_BL_PORT        GPIOE
#define DISP_BL_PIN         0     /* backlight enable */

/* USART3 — nRF52840 BLE bridge (3 Mbps) */
#define BLE_UART            USART3
#define BLE_TX_PORT         GPIOB
#define BLE_TX_PIN          10
#define BLE_TX_AF           7
#define BLE_RX_PORT         GPIOB
#define BLE_RX_PIN          11
#define BLE_RX_AF           7
#define BLE_CTS_PORT        GPIOB
#define BLE_CTS_PIN         13
#define BLE_CTS_AF          8
#define BLE_RTS_PORT        GPIOB
#define BLE_RTS_PIN         14
#define BLE_RTS_AF          8
#define BLE_RST_PORT        GPIOB
#define BLE_RST_PIN         15
#define BLE_BAUD            3000000u

/* USART1 — USB-C CDC (via internal PHY, but we route via GPIO for debug) */
#define USB_UART            USART1
#define USB_TX_PORT         GPIOB
#define USB_TX_PIN          6
#define USB_TX_AF           7
#define USB_RX_PORT         GPIOB
#define USB_RX_PIN          7
#define USB_RX_AF           7
#define USB_BAUD            115200u

/* I2C1 — Camera SCCB + DAC + TEC thermistor */
#define CAM_I2C             I2C1
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SCL_PIN        8
#define I2C1_SCL_AF         4
#define I2C1_SDA_PORT       GPIOB
#define I2C1_SDA_PIN        9
#define I2C1_SDA_AF         4
#define I2C1_TIMING         0x10C0ECFFu  /* 400 kHz @ 120 MHz PCLK */

/* I2C4 — IMU (ICM-42688-P) + Fuel Gauge (MAX17048) */
#define IMU_I2C             I2C4
#define I2C4_SCL_PORT       GPIOD
#define I2C4_SCL_PIN        12
#define I2C4_SCL_AF         4
#define I2C4_SDA_PORT       GPIOD
#define I2C4_SDA_PIN       13
#define I2C4_SDA_AF         4
#define I2C4_TIMING         0x10C0ECFFu

/* DAC1 — Laser current control (channel 1, 12-bit) */
#define LASER_DAC            DAC1
#define LASER_DAC_PORT       GPIOA
#define LASER_DAC_PIN        4      /* DAC1_OUT1 = PA4 */
#define LASER_DAC_ANALOG     1

/* TIM1 CH1 — Laser PWM dimming / trigger gate (PA8) */
#define LASER_PWM_TIM        TIM1
#define LASER_PWM_PORT       GPIOA
#define LASER_PWM_PIN        8
#define LASER_PWM_AF         1

/* TIM8 CH1 — TEC PWM (PC6) */
#define TEC_PWM_TIM          TIM8
#define TEC_PWM_PORT        GPIOC
#define TEC_PWM_PIN         6
#define TEC_PWM_AF          3

/* ADC1 — Battery voltage (PA0), TEC thermistor (PA1), Laser current sense (PA2) */
#define ADC_BATT_CH          0   /* ADC_IN0 = PA0 */
#define ADC_THERM_CH         1   /* ADC_IN1 = PA1 */
#define ADC_LASER_ISENSE_CH  2   /* ADC_IN2 = PA2 */

/* GPIO — Buttons, interlock, LED */
#define BTN_TRIGGER_PORT     GPIOC
#define BTN_TRIGGER_PIN      9
#define BTN_MODE_PORT        GPIOC
#define BTN_MODE_PIN         10
#define BTN_UP_PORT          GPIOC
#define BTN_UP_PIN           11
#define BTN_DOWN_PORT        GPIOC
#define BTN_DOWN_PIN         12
#define INTERLOCK_PORT       GPIOC
#define INTERLOCK_PIN        14    /* active-low hardware interlock */
#define LED_STATUS_PORT      GPIOB
#define LED_STATUS_PIN       0     /* status LED (blue) */
#define LED_LASER_PORT       GPIOB
#define LED_LASER_PIN        1     /* laser warning LED (red) */
#define KEY_SW_PORT          GPIOB
#define KEY_SW_PIN           2     /* key switch input */

/* SDMMC1 — microSD (4-bit UHS-I) */
#define SD_SDMMC            SDMMC1
#define SD_CK_PORT          GPIOC
#define SD_CK_PIN           12
#define SD_CK_AF            12
#define SD_CMD_PORT         GPIOD
#define SD_CMD_PIN          2
#define SD_CMD_AF           12
#define SD_D0_PORT          GPIOC
#define SD_D0_PIN           8
#define SD_D0_AF            12
#define SD_D1_PORT          GPIOC
#define SD_D1_PIN           9
#define SD_D1_AF            12
#define SD_D2_PORT          GPIOC
#define SD_D2_PIN           10
#define SD_D2_AF            12
#define SD_D3_PORT          GPIOC
#define SD_D3_PIN           11
#define SD_D3_AF            12
#define SD_CD_PORT          GPIOC
#define SD_CD_PIN           15    /* card detect */

/* ---- Display constants -------------------------------------------------- */
#define DISP_WIDTH          320
#define DISP_HEIGHT         240
#define DISP_COLORMODE      0x55    /* 16-bit RGB565 */

/* ---- Imaging constants -------------------------------------------------- */
#define IMG_WIDTH           640
#define IMG_HEIGHT          480
#define IMG_FPS             60
#define CONTRAST_WINDOW     7       /* 7×7 sliding window (odd) */
#define CONTRAST_WINDOW_ALT_5  5
#define CONTRAST_WINDOW_ALT_9  9
#define CAM_WIDTH           1280
#define CAM_HEIGHT          800
#define CAM_FPS             120
#define CAM_EXPOSURE_US     5000    /* 5 ms default exposure */

/* ---- Laser constants ---------------------------------------------------- */
#define LASER_WAVELENGTH_NM 785
#define LASER_MAX_POWER_MW  30
#define LASER_RAMP_MS       5000    /* 5 s ramp-up per IEC 60825-1 */
#define LASER_AUTO_OFF_MS   30000   /* auto-shutoff after 30 s idle */
#define LASER_DAC_MAX       4095    /* 12-bit DAC full-scale */
#define LASER_DAC_30MW      3300    /* DAC value for 30 mW (calibrated) */
#define TEC_SETPOINT_RAW    2048    /* thermistor ADC setpoint (~25 °C) */
#define TEC_KP              120
#define TEC_KI              8
#define TEC_KD              2
#define TEC_PWM_MAX         1000

/* ---- BLE protocol ------------------------------------------------------- */
#define BLE_TILE_W          16
#define BLE_TILE_H          8
#define BLE_TILES_X         (IMG_WIDTH / BLE_TILE_W)    /* 40 */
#define BLE_TILES_Y         (IMG_HEIGHT / BLE_TILE_H)   /* 60 */
#define BLE_TILES_PER_FRAME (BLE_TILES_X * BLE_TILES_Y) /* 2400 */
#define BLE_TILE_BYTES      (BLE_TILE_W * BLE_TILE_H)    /* 128 */
#define BLE_STATUS_SIZE     8
#define BLE_CMD_SIZE        4

/* ---- Frame buffers ------------------------------------------------------ */
/* The FPGA writes flow-map frames into this buffer via SPI DMA.
 * We use a double-buffer scheme: while one frame is being processed
 * (colormap + display + BLE + SD), the next is being received. */
#define FRAME_BYTES        (IMG_WIDTH * IMG_HEIGHT) /* 307200 bytes (8-bit K) */
#define FRAME_BUF_COUNT    2

/* Colormap LUT: 256 entries × 2 bytes (RGB565) = 512 bytes */
#define COLORMAP_ENTRIES   256

/* ---- Battery / power ---------------------------------------------------- */
#define BATT_FULL_MV       4200
#define BATT_EMPTY_MV      3200
#define BATT_WARN_PCT       15

/* ---- LED blink patterns ------------------------------------------------- */
enum led_state {
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,    /* 1 Hz — standby */
    LED_BLINK_FAST,    /* 4 Hz — active imaging */
    LED_BLINK_DOUBLE,  /* double-pulse — low battery */
};

/* ---- Device states ------------------------------------------------------ */
enum device_state {
    STATE_BOOT,
    STATE_STANDBY,
    STATE_WARMUP,      /* laser ramp-up */
    STATE_IMAGING,
    STATE_CALIBRATE,
    STATE_SHUTDOWN,
};

/* ---- Colormap identifiers ----------------------------------------------- */
enum colormap_id {
    CMAP_JET = 0,
    CMAP_THERMAL,
    CMAP_GRAYSCALE,
    CMAP_VIRIDIS,
    CMAP_INFERNO,
    CMAP_COUNT,
};

/* ---- Function prototypes (implemented in main.c) ------------------------ */
void board_init(void);
void clock_init(void);
void gpio_init(void);
void led_set(enum led_state s);
void enter_state(enum device_state s);

#endif /* SPECKLEFLOW_BOARD_H */