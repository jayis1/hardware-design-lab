/*
 * main.c — SpeckleFlow main firmware entry point
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * SpeckleFlow is a portable Laser Speckle Contrast Imaging (LSCI)
 * blood-flow imager. This firmware runs on the STM32H733 and
 * coordinates the camera, FPGA contrast pipeline, laser, display,
 * BLE bridge, SD logging, IMU, and power management.
 *
 * Architecture: cooperative super-loop with prioritized ISRs.
 * No RTOS — the frame pipeline requires deterministic timing that
 * an RTOS scheduler would compromise.
 */

#include "board.h"
#include "registers.h"
#include "drivers/camera.h"
#include "drivers/fpga.h"
#include "drivers/laser.h"
#include "drivers/display.h"
#include "drivers/ble.h"
#include "drivers/sdcard.h"
#include "drivers/imu.h"
#include "drivers/power.h"
#include <string.h>

/* ---- Global state ------------------------------------------------------ */

volatile uint32_t system_ms = 0;       /* 1 ms system tick */
static enum device_state state = STATE_BOOT;
static enum colormap_id current_colormap = CMAP_JET;
static uint8_t current_window = 1;      /* 0=5×5, 1=7×7, 2=9×9 */
static uint8_t current_fps_mode = 1;    /* 0=30, 1=60, 2=120 */
static uint8_t laser_power_pct = 100;
static uint32_t boot_ms = 0;
static uint32_t last_status_tx_ms = 0;
static uint32_t last_power_update_ms = 0;
static uint32_t last_ble_tile_ms = 0;
static uint32_t frames_processed = 0;
static uint8_t roi_x = 80, roi_y = 60, roi_w = 160, roi_h = 120;
static uint8_t sd_logging_enabled = 0;
static uint8_t ble_streaming_enabled = 1;
static uint16_t static_k_reference = 128;  /* calibration value */

/* Button debounce state */
typedef struct {
    uint8_t  last_state;
    uint8_t  stable_state;
    uint32_t last_change_ms;
} btn_state_t;
static btn_state_t btn_trigger, btn_mode, btn_up, btn_down;

/* ---- SysTick 1 ms interrupt -------------------------------------------- */

void SysTick_Handler(void) {
    system_ms++;
}

/* ---- System clock initialization ---------------------------------------- */

void clock_init(void) {
    /* 1. Enable HSE (25 MHz external crystal) */
    RCC->CR |= (1u << 16);  /* HSEON */
    while (!(RCC->CR & (1u << 17))) { }  /* wait HSERDY */

    /* 2. Configure power: enable VOS0 for 480 MHz */
    PWR->CR3 |= (1u << 4);  /* SDEXIST, enable VOS0 */
    for (volatile int i = 0; i < 1000; i++) { }
    PWR->CR3 |= (1u << 1);  /* VOS0 bit */

    /* 3. Configure PLL1: M=5, N=192, P=2 → 25/5*192/2 = 480 MHz */
    RCC->PLLCFGR = (5u << 4)   |   /* M */
                   (192u << 8) |  /* N */
                   (0u << 16)  |  /* P = /2 */
                   (1u << 24)  |  /* PLL1_QEN */
                   (1u << 0);     /* PLL1 source = HSE */

    RCC->CR |= (1u << 24);  /* PLL1ON */
    while (!(RCC->CR & (1u << 25))) { }  /* wait PLL1RDY */

    /* 4. Configure bus dividers:
     *    SYS = 480 MHz, HCLK = 240 MHz (DIV2),
     *    APB1 = 120 MHz (DIV2), APB2 = 120 MHz (DIV2),
     *    APB4 = 120 MHz (DIV2) */
    RCC->CFGR = (1u << 0)   |   /* HPRE = DIV2 */
                (4u << 8)  |   /* PPRE1 = DIV4 → wait, let me recalc */
                (4u << 11) |   /* PPRE2 = DIV4 */
                (4u << 15);   /* PPRE4 = DIV4 (actually in D1CCIPR) */

    /* Actually, for STM32H7:
     *   CFGR: HPRE (bits 7-4), PPRE1 (bits 10-8), PPRE2 (bits 13-11)
     *   PPRE3/PPRE4 are in D1CFGR/D3CFGR.
     * Let's use the simplified approach: set all dividers to /2. */
    RCC->CFGR = 0;
    RCC->CFGR |= (4u << 4);   /* HPRE = /2 → 240 MHz */
    RCC->CFGR |= (4u << 8);   /* PPRE1 = /2 → 120 MHz */
    RCC->CFGR |= (4u << 11);  /* PPRE2 = /2 → 120 MHz */

    /* 5. Flash latency: 4 wait states for 240 MHz @ 3.3V */
    FLASH->ACR = (4u << 0) | FLASH_ACR_PRFTEN | (1u << 8);

    /* 6. Switch system clock to PLL1 */
    RCC->CFGR |= (3u << 0);  /* SW = PLL1 */
    while ((RCC->CFGR & (7u << 3)) != (3u << 3)) { }  /* wait SWS */

    /* 7. Enable peripheral clocks */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                   RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                   RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN;
    RCC->AHB3ENR |= RCC_AHB3ENR_SDMMC1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_USB1EN | RCC_AHB1ENR_DMA1EN |
                   RCC_AHB1ENR_DMA2EN;
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN | RCC_APB1LENR_I2C1EN |
                     RCC_APB1LENR_DAC1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SPI4EN |
                     RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM8EN |
                     RCC_APB2ENR_USART1EN;
    RCC->APB4ENR |= RCC_APB4ENR_I2C4EN | RCC_APB4ENR_SYSCFGEN;

    /* 8. SysTick: 1 ms at 240 MHz HCLK */
    SysTick->LOAD = 240000u - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT |
                    SysTick_CTRL_ENABLE;
}

/* ---- GPIO initialization ----------------------------------------------- */

static void gpio_config_alt(GPIO_TypeDef *port, uint8_t pin, uint8_t af) {
    uint32_t mask = 0x3u << (pin * 2);
    port->MODER = (port->MODER & ~mask) | (2u << (pin * 2));  /* AF mode */
    port->OSPEEDR |= (3u << (pin * 2));  /* very high speed */
    if (pin < 8) {
        port->AFRL = (port->AFRL & ~(0xFu << (pin * 4))) |
                     ((uint32_t)af << (pin * 4));
    } else {
        port->AFRH = (port->AFRH & ~(0xFu << ((pin - 8) * 4))) |
                     ((uint32_t)af << ((pin - 8) * 4));
    }
}

static void gpio_config_output(GPIO_TypeDef *port, uint8_t pin,
                               uint8_t opendrain) {
    port->MODER = (port->MODER & ~(3u << (pin * 2))) | (1u << (pin * 2));
    port->OTYPER = (port->OTYPER & ~(1u << pin)) | ((uint32_t)opendrain << pin);
    port->OSPEEDR |= (3u << (pin * 2));
}

static void gpio_config_input(GPIO_TypeDef *port, uint8_t pin,
                              uint8_t pullup) {
    port->MODER &= ~(3u << (pin * 2));  /* input */
    if (pullup) {
        port->PUPDR = (port->PUPDR & ~(3u << (pin * 2))) | (1u << (pin * 2));
    }
}

static void gpio_config_analog(GPIO_TypeDef *port, uint8_t pin) {
    port->MODER |= (3u << (pin * 2));  /* analog */
}

void gpio_init(void) {
    /* SPI1: SCK=PB3, MISO=PB4, MOSI=PA7, NSS=PA4 */
    gpio_config_alt(FPGA_SCK_PORT, FPGA_SCK_PIN, FPGA_SCK_AF);
    gpio_config_alt(FPGA_MISO_PORT, FPGA_MISO_PIN, FPGA_MISO_AF);
    gpio_config_alt(FPGA_MOSI_PORT, FPGA_MOSI_PIN, FPGA_MOSI_AF);
    gpio_config_alt(FPGA_NSS_PORT, FPGA_NSS_PIN, FPGA_NSS_AF);
    gpio_config_output(FPGA_NSS_PORT, FPGA_NSS_PIN, 0);
    FPGA_NSS_PORT->BSRR = (1u << FPGA_NSS_PIN);  /* NSS high (idle) */
    gpio_config_input(FPGA_IRQ_PORT, FPGA_IRQ_PIN, 1);
    gpio_config_input(FPGA_CDONE_PORT, FPGA_CDONE_PIN, 0);
    gpio_config_output(FPGA_CRST_PORT, FPGA_CRST_PIN, 0);

    /* SPI4: SCK=PE2, MISO=PE5, MOSI=PE6, CS=PE4, DC=PE3, RST=PE1, BL=PE0 */
    gpio_config_alt(DISP_SCK_PORT, DISP_SCK_PIN, DISP_SCK_AF);
    gpio_config_alt(DISP_MISO_PORT, DISP_MISO_PIN, DISP_MISO_AF);
    gpio_config_alt(DISP_MOSI_PORT, DISP_MOSI_PIN, DISP_MOSI_AF);
    gpio_config_output(DISP_CS_PORT, DISP_CS_PIN, 0);
    gpio_config_output(DISP_DC_PORT, DISP_DC_PIN, 0);
    gpio_config_output(DISP_RST_PORT, DISP_RST_PIN, 0);
    gpio_config_output(DISP_BL_PORT, DISP_BL_PIN, 0);
    DISP_CS_PORT->BSRR = (1u << DISP_CS_PIN);

    /* USART3: TX=PB10, RX=PB11, CTS=PB13, RTS=PB14, RST=PB15 */
    gpio_config_alt(BLE_TX_PORT, BLE_TX_PIN, BLE_TX_AF);
    gpio_config_alt(BLE_RX_PORT, BLE_RX_PIN, BLE_RX_AF);
    gpio_config_alt(BLE_CTS_PORT, BLE_CTS_PIN, BLE_CTS_AF);
    gpio_config_alt(BLE_RTS_PORT, BLE_RTS_PIN, BLE_RTS_AF);
    gpio_config_output(BLE_RST_PORT, BLE_RST_PIN, 0);

    /* I2C1: SCL=PB8, SDA=PB9 */
    gpio_config_alt(I2C1_SCL_PORT, I2C1_SCL_PIN, I2C1_SCL_AF);
    gpio_config_alt(I2C1_SDA_PORT, I2C1_SDA_PIN, I2C1_SDA_AF);
    I2C1->TIMINGR = I2C1_TIMING;
    I2C1->CR1 = I2C_CR1_PE;

    /* I2C4: SCL=PD12, SDA=PD13 */
    gpio_config_alt(I2C4_SCL_PORT, I2C4_SCL_PIN, I2C4_SCL_AF);
    gpio_config_alt(I2C4_SDA_PORT, I2C4_SDA_PIN, I2C4_SDA_AF);
    I2C4->TIMINGR = I2C4_TIMING;
    I2C4->CR1 = I2C_CR1_PE;

    /* DAC1: PA4 (analog) */
    gpio_config_analog(LASER_DAC_PORT, LASER_DAC_PIN);

    /* TIM1 CH1: PA8 (laser PWM) */
    gpio_config_alt(LASER_PWM_PORT, LASER_PWM_PIN, LASER_PWM_AF);

    /* TIM8 CH1: PC6 (TEC PWM) */
    gpio_config_alt(TEC_PWM_PORT, TEC_PWM_PIN, TEC_PWM_AF);

    /* ADC: PA0, PA1, PA2 (analog) */
    gpio_config_analog(GPIOA, 0);
    gpio_config_analog(GPIOA, 1);
    gpio_config_analog(GPIOA, 2);

    /* Buttons: PC9, PC10, PC11, PC12 (input with pull-up) */
    gpio_config_input(BTN_TRIGGER_PORT, BTN_TRIGGER_PIN, 1);
    gpio_config_input(BTN_MODE_PORT, BTN_MODE_PIN, 1);
    gpio_config_input(BTN_UP_PORT, BTN_UP_PIN, 1);
    gpio_config_input(BTN_DOWN_PORT, BTN_DOWN_PIN, 1);

    /* Interlock: PC14 (input with pull-up, active-low) */
    gpio_config_input(INTERLOCK_PORT, INTERLOCK_PIN, 1);

    /* Key switch: PB2 (input with pull-up) */
    gpio_config_input(KEY_SW_PORT, KEY_SW_PIN, 1);

    /* LEDs: PB0 (status), PB1 (laser warning) */
    gpio_config_output(LED_STATUS_PORT, LED_STATUS_PIN, 0);
    gpio_config_output(LED_LASER_PORT, LED_LASER_PIN, 0);

    /* Camera PWDN=PC6 wait, PC6 is TEC PWM. Use PC7 for XSHUT, PC3 for PWDN */
    gpio_config_output(GPIOC, 3, 0);  /* PWDN */
    gpio_config_output(GPIOC, 7, 0);  /* XSHUT */

    /* SDMMC1: PC8-PC12, PD2 (alt function 12) */
    gpio_config_alt(SD_CK_PORT, SD_CK_PIN, SD_CK_AF);
    gpio_config_alt(SD_CMD_PORT, SD_CMD_PIN, SD_CMD_AF);
    gpio_config_alt(SD_D0_PORT, SD_D0_PIN, SD_D0_AF);
    gpio_config_alt(SD_D1_PORT, SD_D1_PIN, SD_D1_AF);
    gpio_config_alt(SD_D2_PORT, SD_D2_PIN, SD_D2_AF);
    gpio_config_alt(SD_D3_PORT, SD_D3_PIN, SD_D3_AF);
    gpio_config_input(SD_CD_PORT, SD_CD_PIN, 1);

    /* USB-C VBUS detect: PB5 (input) */
    gpio_config_input(GPIOB, 5, 0);
}

/* ---- SPI initialization ------------------------------------------------- */

static void spi1_init(void) {
    /* SPI1: master, 8-bit, mode 0, baud rate = APB2/4 = 30 MHz */
    SPI1->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_DSIZE_8 | SPI_CFG1_MBR_DIV4;
    SPI1->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSOE | SPI_CFG2_AFCNTR;
    SPI1->CR1 = SPI_CR1_SPE;
}

static void spi4_init(void) {
    /* SPI4: master, 16-bit, mode 0, baud rate = APB2/4 = 30 MHz */
    SPI4->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_DSIZE_16 | SPI_CFG1_MBR_DIV4;
    SPI4->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSOE | SPI_CFG2_AFCNTR;
    SPI4->CR1 = SPI_CR1_SPE;
}

/* ---- USART initialization ----------------------------------------------- */

static void usart3_init(void) {
    /* USART3: 3 Mbps, 8N1, hardware flow control (CTS/RTS) */
    USART3->CR1 = 0;
    USART3->CR2 = 0;
    USART3->CR3 = USART_CR3_CTSE | USART_CR3_RTSE;  /* hardware flow control */
    /* BRR = APB1 / baud = 120 MHz / 3 Mbps = 40 */
    USART3->BRR = 40;
    USART3->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    nvic_set_priority(USART3_IRQn, 5);
    nvic_enable(USART3_IRQn);
}

/* ---- LED management ----------------------------------------------------- */

void led_set(enum led_state s) {
    static uint32_t last_toggle = 0;
    static uint8_t toggle_state = 0;
    static uint8_t double_pulse_phase = 0;

    switch (s) {
    case LED_OFF:
        LED_STATUS_PORT->BSRR = (1u << (LED_STATUS_PIN + 16));
        break;
    case LED_ON:
        LED_STATUS_PORT->BSRR = (1u << LED_STATUS_PIN);
        break;
    case LED_BLINK_SLOW:
        if (system_ms - last_toggle >= 500) {
            last_toggle = system_ms;
            toggle_state ^= 1;
            if (toggle_state)
                LED_STATUS_PORT->BSRR = (1u << LED_STATUS_PIN);
            else
                LED_STATUS_PORT->BSRR = (1u << (LED_STATUS_PIN + 16));
        }
        break;
    case LED_BLINK_FAST:
        if (system_ms - last_toggle >= 125) {
            last_toggle = system_ms;
            toggle_state ^= 1;
            if (toggle_state)
                LED_STATUS_PORT->BSRR = (1u << LED_STATUS_PIN);
            else
                LED_STATUS_PORT->BSRR = (1u << (LED_STATUS_PIN + 16));
        }
        break;
    case LED_BLINK_DOUBLE:
        /* Double-pulse every 1 s to indicate low battery */
        if (system_ms - last_toggle >= 1000) {
            last_toggle = system_ms;
            double_pulse_phase = 0;
        }
        if (system_ms - last_toggle < 100) {
            LED_STATUS_PORT->BSRR = (1u << LED_STATUS_PIN);
        } else if (system_ms - last_toggle < 200) {
            LED_STATUS_PORT->BSRR = (1u << (LED_STATUS_PIN + 16));
        } else if (system_ms - last_toggle < 300) {
            LED_STATUS_PORT->BSRR = (1u << LED_STATUS_PIN);
        } else {
            LED_STATUS_PORT->BSRR = (1u << (LED_STATUS_PIN + 16));
        }
        break;
    }
}

/* ---- Button debounce --------------------------------------------------- */

static uint8_t debounce_button(btn_state_t *btn, uint8_t reading) {
    if (reading != btn->last_state) {
        btn->last_change_ms = system_ms;
        btn->last_state = reading;
    }
    if (system_ms - btn->last_change_ms >= 20) {
        btn->stable_state = reading;
    }
    return btn->stable_state;
}

static uint8_t read_button_triggered = 0;
static uint8_t read_button_mode_pressed = 0;
static uint8_t read_button_up_pressed = 0;
static uint8_t read_button_down_pressed = 0;

static void buttons_scan(void) {
    /* Buttons are active-low (pull-up, short to GND when pressed) */
    uint8_t trig = debounce_button(&btn_trigger,
        (BTN_TRIGGER_PORT->IDR & (1u << BTN_TRIGGER_PIN)) ? 0 : 1);
    uint8_t mode = debounce_button(&btn_mode,
        (BTN_MODE_PORT->IDR & (1u << BTN_MODE_PIN)) ? 0 : 1);
    uint8_t up = debounce_button(&btn_up,
        (BTN_UP_PORT->IDR & (1u << BTN_UP_PIN)) ? 0 : 1);
    uint8_t down = debounce_button(&btn_down,
        (BTN_DOWN_PORT->IDR & (1u << BTN_DOWN_PIN)) ? 0 : 1);

    /* Edge detection */
    static uint8_t prev_trig = 0, prev_mode = 0, prev_up = 0, prev_down = 0;
    read_button_triggered = (trig && !prev_trig);
    read_button_mode_pressed = (mode && !prev_mode);
    read_button_up_pressed = (up && !prev_up);
    read_button_down_pressed = (down && !prev_down);
    prev_trig = trig;
    prev_mode = mode;
    prev_up = up;
    prev_down = down;
}

/* ---- State machine ------------------------------------------------------ */

void enter_state(enum device_state s) {
    switch (s) {
    case STATE_BOOT:
        break;
    case STATE_STANDBY:
        laser_enable(0);
        fpga_enable(0);
        camera_standby();
        display_clear(display_rgb565(0, 0, 0));
        display_draw_text(40, 100, "SPECKLEFLOW READY", 0xFFFF, 0x0000);
        display_draw_text(60, 120, "Press trigger", 0x07FF, 0x0000);
        display_present();
        break;
    case STATE_WARMUP:
        laser_enable(1);
        camera_init();
        fpga_enable(1);
        display_clear(display_rgb565(0, 0, 40));
        display_draw_text(40, 100, "LASER WARMING UP", 0xFFE0, 0x0000);
        display_draw_text(50, 120, "Please wait 5s...", 0x07FF, 0x0000);
        display_present();
        break;
    case STATE_IMAGING:
        display_clear(display_rgb565(0, 0, 0));
        break;
    case STATE_CALIBRATE:
        fpga_start_calibration();
        display_clear(display_rgb565(0, 0, 0));
        display_draw_text(40, 100, "CALIBRATING...", 0xF800, 0x0000);
        display_draw_text(50, 120, "Hold steady", 0xFFFF, 0x0000);
        display_present();
        break;
    case STATE_SHUTDOWN:
        laser_enable(0);
        fpga_enable(0);
        camera_standby();
        display_clear(0x0000);
        display_draw_text(60, 100, "SHUTDOWN", 0xF800, 0x0000);
        display_present();
        break;
    }
    state = s;
}

/* ---- Frame processing --------------------------------------------------- */

static void process_frame(uint8_t *frame) {
    /* 1. Render to display with colormap */
    display_render_flow(frame, IMG_WIDTH, IMG_HEIGHT);

    /* 2. Draw HUD overlay */
    uint16_t white = display_rgb565(255, 255, 255);
    uint16_t green = display_rgb565(0, 255, 0);
    uint16_t red   = display_rgb565(255, 0, 0);

    /* ROI box (scaled from 640×480 to 320×240) */
    uint16_t roi_sx = roi_x / 2;
    uint16_t roi_sy = roi_y / 2;
    uint16_t roi_sw = roi_w / 2;
    uint16_t roi_sh = roi_h / 2;
    display_draw_rect(roi_sx, roi_sy, roi_sw, roi_sh, green);

    /* Compute mean flow in ROI */
    uint32_t flow_sum = 0;
    uint32_t flow_count = 0;
    for (uint16_t y = roi_y; y < roi_y + roi_h && y < IMG_HEIGHT; y++) {
        for (uint16_t x = roi_x; x < roi_x + roi_w && x < IMG_WIDTH; x++) {
            flow_sum += frame[y * IMG_WIDTH + x];
            flow_count++;
        }
    }
    uint8_t mean_flow = (uint8_t)(flow_count ? flow_sum / flow_count : 0);

    /* HUD text */
    char hud[32];
    /* Simple itoa for flow value */
    hud[0] = 'F'; hud[1] = ':'; hud[2] = ' ';
    hud[3] = '0' + (mean_flow / 100);
    hud[4] = '0' + ((mean_flow / 10) % 10);
    hud[5] = '0' + (mean_flow % 10);
    hud[6] = 0;
    display_draw_text(5, 5, hud, white, 0x0000);

    /* Battery indicator */
    uint8_t batt = power_get_battery_pct();
    hud[0] = 'B'; hud[1] = ':'; hud[2] = ' ';
    hud[3] = '0' + (batt / 100);
    hud[4] = '0' + ((batt / 10) % 10);
    hud[5] = '0' + (batt % 10);
    hud[6] = '%';
    hud[7] = 0;
    display_draw_text(250, 5, hud, power_is_low() ? red : white, 0x0000);

    /* Laser status */
    if (laser_is_enabled()) {
        if (laser_is_ramping())
            display_draw_text(5, 220, "LASER:RAMP", 0xFFE0, 0x0000);
        else
            display_draw_text(5, 220, "LASER:ON", red, 0x0000);
    }

    /* Frame counter */
    uint32_t fc = fpga_get_frame_count();
    hud[0] = '#'; hud[1] = ':';
    hud[2] = '0' + ((fc / 100000) % 10);
    hud[3] = '0' + ((fc / 10000) % 10);
    hud[4] = '0' + ((fc / 1000) % 10);
    hud[5] = '0' + ((fc / 100) % 10);
    hud[6] = '0' + ((fc / 10) % 10);
    hud[7] = '0' + (fc % 10);
    hud[8] = 0;
    display_draw_text(250, 220, hud, white, 0x0000);

    /* 3. Push framebuffer to display */
    display_present();

    /* 4. Stream to BLE (downsampled tiles) */
    if (ble_streaming_enabled && !ble_is_tx_busy()) {
        /* Send a subset of tiles to stay within BLE bandwidth.
         * At 60 fps with 2400 tiles/frame, we send every 10th tile
         * per frame, cycling through all tiles over 10 frames. */
        static uint16_t tile_offset = 0;
        for (int t = 0; t < 240; t++) {  /* 240 tiles per frame burst */
            uint16_t idx = (tile_offset + t) % BLE_TILES_PER_FRAME;
            uint16_t tx = idx % BLE_TILES_X;
            uint16_t ty = idx / BLE_TILES_X;
            /* Extract 16×8 tile from the 640×480 frame, downsampled 4×4 */
            uint8_t tile[BLE_TILE_BYTES];
            for (int dy = 0; dy < BLE_TILE_H; dy++) {
                for (int dx = 0; dx < BLE_TILE_W; dx++) {
                    uint16_t sx = (tx * BLE_TILE_W + dx) * 4;
                    uint16_t sy = (ty * BLE_TILE_H + dy) * 4;
                    if (sx < IMG_WIDTH && sy < IMG_HEIGHT)
                        tile[dy * BLE_TILE_W + dx] = frame[sy * IMG_WIDTH + sx];
                    else
                        tile[dy * BLE_TILE_W + dx] = 0;
                }
            }
            ble_send_flow_tile(tile, idx);
        }
        tile_offset = (tile_offset + 240) % BLE_TILES_PER_FRAME;
    }

    /* 5. Log to SD card */
    if (sd_logging_enabled && sdcard_is_present()) {
        sdcard_log_frame(frame, FRAME_BYTES, system_ms - boot_ms);
    }

    frames_processed++;
}

/* ---- Command handling --------------------------------------------------- */

static void handle_command(const uint8_t *cmd) {
    /* Command format: [cmd_id] [param0] [param1] [param2] */
    switch (cmd[0]) {
    case 0x01:  /* Start imaging */
        if (state == STATE_STANDBY) enter_state(STATE_WARMUP);
        break;
    case 0x02:  /* Stop imaging */
        if (state == STATE_IMAGING || state == STATE_WARMUP)
            enter_state(STATE_STANDBY);
        break;
    case 0x03:  /* Calibrate */
        enter_state(STATE_CALIBRATE);
        break;
    case 0x04:  /* Set colormap */
        if (cmd[1] < CMAP_COUNT) {
            current_colormap = (enum colormap_id)cmd[1];
            display_set_colormap(current_colormap);
        }
        break;
    case 0x05:  /* Set window size */
        if (cmd[1] <= 2) {
            current_window = cmd[1];
            fpga_set_window(current_window);
        }
        break;
    case 0x06:  /* Set frame rate */
        if (cmd[1] <= 2) {
            current_fps_mode = cmd[1];
            fpga_set_frame_rate(current_fps_mode);
        }
        break;
    case 0x07:  /* Set laser power */
        laser_power_pct = cmd[1];
        laser_set_power(laser_power_pct);
        break;
    case 0x08:  /* Set exposure */
        camera_set_exposure((uint32_t)cmd[1] * 1000);
        break;
    case 0x09:  /* Enable SD logging */
        sd_logging_enabled = cmd[1] ? 1 : 0;
        if (sd_logging_enabled) sdcard_start_session();
        break;
    case 0x0A:  /* Enable BLE streaming */
        ble_streaming_enabled = cmd[1] ? 1 : 0;
        break;
    case 0x0B:  /* Set ROI */
        roi_x = cmd[1]; roi_y = cmd[2];
        roi_w = cmd[3] ? cmd[3] : 160;
        roi_h = cmd[3] ? 120 : 120;
        break;
    default:
        break;
    }
}

/* ---- Board init --------------------------------------------------------- */

void board_init(void) {
    clock_init();
    gpio_init();
    spi1_init();
    spi4_init();
    usart3_init();
}

/* ---- Main super-loop ---------------------------------------------------- */

int main(void) {
    /* 1. Hardware initialization */
    board_init();
    boot_ms = system_ms;

    /* 2. Initialize subsystems */
    display_init();
    display_clear(display_rgb565(0, 0, 50));
    display_draw_text(40, 100, "SPECKLEFLOW", 0xFFFF, 0x0000);
    display_draw_text(50, 120, "BOOTING...", 0x07FF, 0x0000);
    display_present();

    power_init();
    ble_init();
    imu_init();
    laser_init();

    /* 3. Initialize camera and FPGA */
    int cam_ok = camera_init();
    int fpga_ok = fpga_init();

    /* 4. Initialize SD card if present */
    if (sdcard_is_present()) {
        sdcard_init();
    }

    /* 5. Enter standby state */
    enter_state(STATE_STANDBY);

    /* 6. Main super-loop */
    while (1) {
        /* --- 1 ms tick handlers --- */
        static uint32_t last_tick_ms = 0;
        if (system_ms != last_tick_ms) {
            uint32_t dt = system_ms - last_tick_ms;
            last_tick_ms = system_ms;

            /* Laser safety + TEC PID */
            laser_tick();

            /* LED status indicator */
            switch (state) {
            case STATE_BOOT:      led_set(LED_BLINK_FAST); break;
            case STATE_STANDBY:   led_set(LED_BLINK_SLOW); break;
            case STATE_WARMUP:    led_set(LED_BLINK_FAST); break;
            case STATE_IMAGING:   led_set(LED_ON); break;
            case STATE_CALIBRATE: led_set(LED_BLINK_DOUBLE); break;
            case STATE_SHUTDOWN:  led_set(LED_OFF); break;
            }

            /* Power update (1 Hz) */
            if (system_ms - last_power_update_ms >= 1000) {
                last_power_update_ms = system_ms;
                power_update();
            }

            /* BLE status update (2 Hz) */
            if (system_ms - last_status_tx_ms >= 500) {
                last_status_tx_ms = system_ms;
                int8_t temp = 25;
                imu_read_temp(&temp);
                ble_send_status(
                    power_get_battery_pct(),
                    laser_is_enabled(),
                    (current_fps_mode == 0) ? 30 :
                    (current_fps_mode == 1) ? 60 : 120,
                    temp,
                    fpga_get_frame_count()
                );
            }
        }

        /* --- Button scan (every loop iteration) --- */
        buttons_scan();

        /* --- State machine --- */
        switch (state) {
        case STATE_BOOT:
            /* Should not stay here, but just in case */
            enter_state(STATE_STANDBY);
            break;

        case STATE_STANDBY:
            if (read_button_triggered) {
                enter_state(STATE_WARMUP);
            }
            if (read_button_mode_pressed) {
                /* Cycle colormap */
                current_colormap = (enum colormap_id)(
                    (current_colormap + 1) % CMAP_COUNT);
                display_set_colormap(current_colormap);
            }
            if (read_button_up_pressed) {
                current_window = (current_window + 1) % 3;
                fpga_set_window(current_window);
            }
            if (read_button_down_pressed) {
                current_fps_mode = (current_fps_mode + 1) % 3;
                fpga_set_frame_rate(current_fps_mode);
            }
            break;

        case STATE_WARMUP:
            /* Wait for laser ramp to complete (5 s) */
            if (!laser_is_ramping()) {
                enter_state(STATE_IMAGING);
            }
            if (read_button_triggered) {
                /* Cancel warmup */
                enter_state(STATE_STANDBY);
            }
            break;

        case STATE_IMAGING:
            /* Check for new frame from FPGA */
            if (fpga_is_frame_ready()) {
                uint8_t *frame = fpga_get_frame();
                if (frame) {
                    process_frame(frame);
                }
            }

            /* Trigger button registers laser activity */
            if (read_button_triggered) {
                laser_trigger();
            }

            /* Mode button: cycle colormap */
            if (read_button_mode_pressed) {
                current_colormap = (enum colormap_id)(
                    (current_colormap + 1) % CMAP_COUNT);
                display_set_colormap(current_colormap);
            }

            /* Up/Down: adjust ROI */
            if (read_button_up_pressed && roi_h > 20) {
                roi_h -= 10; roi_w -= 10;
            }
            if (read_button_down_pressed && roi_h < 200) {
                roi_h += 10; roi_w += 10;
            }

            /* Release trigger for 30 s → auto-shutdown to standby */
            /* (handled in laser_tick, but we also check here) */
            if (!laser_is_enabled()) {
                enter_state(STATE_STANDBY);
            }
            break;

        case STATE_CALIBRATE:
            /* Wait for calibration to complete */
            {
                uint8_t status = fpga_get_status();
                if (!(status & 0x08)) {  /* calibration done */
                    enter_state(STATE_IMAGING);
                }
            }
            break;

        case STATE_SHUTDOWN:
            /* Stay in shutdown until power cycle */
            break;
        }

        /* --- Handle BLE commands --- */
        uint8_t cmd[4];
        if (ble_get_command(cmd) == 0) {
            handle_command(cmd);
        }

        /* --- Low battery warning --- */
        if (power_is_low() && state != STATE_SHUTDOWN) {
            /* Flash low-battery warning on display */
            static uint32_t last_warn = 0;
            if (system_ms - last_warn >= 2000) {
                last_warn = system_ms;
                display_fill_rect(80, 100, 160, 40, 0xF800);
                display_draw_text(90, 110, "LOW BATTERY", 0xFFFF, 0xF800);
                display_present();
            }
        }
    }

    return 0;
}

/* ---- Interrupt handlers ------------------------------------------------- */

/* FPGA frame-ready interrupt (EXTI line 4) */
void EXTI4_IRQHandler(void) {
    fpga_isr_frame_ready();
}

/* DMA2 Stream0 (SPI1 RX) transfer complete */
void DMA2_Stream0_IRQHandler(void) {
    if (DMA2->Stream[0].CR & DMA_CR_TCIE) {
        DMA2->Common.IFCR = (1u << 5);  /* clear TC flag for stream 0 */
        fpga_isr_dma_complete();
    }
}

/* DMA2 Stream1 (SPI4 TX) transfer complete */
void DMA2_Stream1_IRQHandler(void) {
    if (DMA2->Stream[1].CR & DMA_CR_TCIE) {
        DMA2->Common.IFCR = (1u << 7);  /* clear TC flag for stream 1 */
    }
}