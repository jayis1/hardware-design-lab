/*
 * main.c — StrataScan firmware main entry point
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Main firmware for the StrataScan SFCW ground penetrating radar.  Initializes
 * all hardware peripherals, runs the main state machine super-loop, and
 * coordinates the stepped-frequency sweep, I/Q capture, range-domain FFT
 * migration, B-scan assembly, and BLE/SD logging.
 *
 * Architecture: cooperative super-loop with interrupt-driven ADC capture.
 *  - PLL (SPI1): ADF4159 programmed per frequency step; LE latches register
 *  - ADC (SPI3/software): ADS131M08 reads I/Q at end of each dwell period
 *  - TIM3 quadrature encoder: survey wheel triggers trace acquisition
 *  - USART1: SAM-M10Q GNSS NMEA sentences (interrupt-driven)
 *  - USART3: nRF52840 BLE module (interrupt-driven)
 *  - I2C1: BMI270 IMU + SSD1309 OLED (polled)
 *  - SDMMC1: MicroSD FAT32 logging (SEG-Y format)
 *  - DAC1: VCO bias + LNA gain control voltages
 *  - ADC1 channel 10: battery voltage monitoring
 *  - EXTI10: ADS131M08 DRDY → triggers I/Q read
 *  - EXTI on GNSS PPS: 1-second time reference
 *
 * State machine:
 *   BOOT → IDLE → CALIBRATE → SURVEY ⇄ PAUSE → SHUTDOWN
 */

#include "board.h"
#include "registers.h"
#include "drivers/pll.h"
#include "drivers/receiver.h"
#include "drivers/radar.h"
#include "drivers/imu.h"
#include "drivers/wheel.h"
#include "drivers/gnss.h"
#include "drivers/sdlog.h"
#include "drivers/ble.h"
#include "drivers/display.h"
#include <string.h>

/* ===================================================================== */
/*  Global state                                                          */
/* ===================================================================== */

volatile uint32_t g_tick_ms = 0;
volatile sys_state_t g_state = STATE_BOOT;

/* Band presets: DEEP, LO, MED, HI, UHI */
const band_preset_t band_presets[5] = {
    /* name    f_start  f_stop  steps  dwell  vco_sel  lna_gain */
    { "DEEP",  1,       250,    512,   4000,  VCO_SEL_DIVIDER,  21.0f },
    { "LO",    50,      800,    512,   2000,  VCO_SEL_HMC585,   21.0f },
    { "MED",   200,     1500,   512,   1000,  VCO_SEL_HMC5841,  18.0f },
    { "HI",    500,     3000,   1024,  500,   VCO_SEL_HMC5841,  15.0f },
    { "UHI",   1000,    3000,   1024,  250,   VCO_SEL_HMC5841,  12.0f },
};

/* Active band index */
static volatile uint8_t g_band_idx = 2;  /* default MED */

/* B-scan buffer: traces × depth bins */
static float bscan_depth[MAX_SWEEP_STEPS];  /* depth-axis values (m) */
static float bscan_data[BSCAN_BUFFER_DEPTH][MAX_SWEEP_STEPS];
static volatile uint16_t g_bscan_traces = 0;
static volatile uint16_t g_bscan_width = 512;

/* Calibration data (shorted-load / noise floor) */
static float cal_I[MAX_SWEEP_STEPS];
static float cal_Q[MAX_SWEEP_STEPS];
static uint8_t g_calibrated = 0;

/* Current sweep I/Q buffers */
static float sweep_I[MAX_SWEEP_STEPS];
static float sweep_Q[MAX_SWEEP_STEPS];

/* Survey parameters */
static float g_eps_r = DEFAULT_EPS_R;
static float g_trace_spacing_mm = DEFAULT_TRACE_SPACING_MM;
static volatile uint32_t g_traces_acquired = 0;
static volatile uint8_t  g_survey_running = 0;
static volatile uint8_t  g_trace_trigger_pending = 0;

/* GNSS + IMU position data per trace */
static float g_last_lat = 0.0f;
static float g_last_lon = 0.0f;
static float g_imu_dx = 0.0f;  /* dead-reckoning delta X (m) */
static float g_imu_dy = 0.0f;  /* dead-reckoning delta Y (m) */

/* BLE TX buffer */
static uint8_t  ble_tx_buf[256];
static uint16_t ble_tx_len = 0;

/* ===================================================================== */
/*  SysTick — 1 ms tick                                                   */
/* ===================================================================== */

void SysTick_Handler(void)
{
    g_tick_ms++;
}

uint32_t board_get_tick_ms(void)
{
    return g_tick_ms;
}

void board_delay_ms(uint32_t ms)
{
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms) {
        /* WFI would be fine here but keep simple for super-loop */
    }
}

void board_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    if (r) LED_R_ON(); else LED_R_OFF();
    if (g) LED_G_ON(); else LED_G_OFF();
    if (b) LED_B_ON(); else LED_B_OFF();
}

static void systick_init(void)
{
    /* 480 MHz / 1000 = 480000 reload for 1 ms */
    SysTick->LOAD = (BOARD_SYSCLK_FREQ_HZ / 1000) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}

/* ===================================================================== */
/*  Clock initialization — HSE → PLL1 → 480 MHz                           */
/* ===================================================================== */

static void clock_init(void)
{
    /* Enable HSE and wait for ready */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* Configure Flash latency for 480 MHz — need 4 wait states (VOS=1.2V) */
    FLASH_REG->ACR = FLASH_ACR_LATENCY_4WS;

    /* PLL1: source = HSE, M=1, N=120, P=2 → 480 MHz */
    RCC->PLLCKSELR = RCC_PLLCKSELR_PLLSRC_HSE | RCC_PLLCKSELR_DIVM1(PLL1_M);
    RCC->PLLCFGR   = RCC_PLLCFGR_PLL1VCOSEL_MID;  /* mid VCO range (192-836 MHz for P=2 → up to 432? we use HI */
    /* Actually for 960 MHz VCO need high-range: PLL1VCOSEL=1 */
    RCC->PLLCFGR   = RCC_PLLCFGR_PLL1VCOSEL_HI;
    /* DIVR = N=120, DIVP = P=2, DIVQ for USB not used here.
     * PLL1DIVR layout: N[8:0], P[14:12], Q[21:16], R[30:24]
     * P=2 → field value = 2-1=1 in bits 14:12
     * N=120 → field value = 120-1=119 in bits 8:0
     */
    RCC->PLL1DIVR = ((119 & 0x1FF) << 0) | ((1 & 0x7) << 12);

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY)) { }

    /* D1CFGR: HPRE, D1PPRE; configure bus dividers
     * HPRE = /1 → HCLK = 480 MHz? But we want HCLK=240 for safety.
     * HPRE /2 → 240 MHz.  D1PPRE /2 → 120 MHz D1 APB.
     * For H7 with VOS0 (high), HCLK can be 480 MHz. Let's use /1 → 480 MHz HCLK
     * to match our APB defines. Actually set HCLK=240 for lower power:
     *   HPRE = /2 (0x8), D1PPRE=/2 (8), D1HPRE etc.
     * Simplified: HPRE field at D1CFGR bits 0-3, 0x8 = /2.
     */
    RCC->D1CFGR = (0x8 << 0);   /* HPRE=/2 → HCLK 240 MHz */
    RCC->D2CFGR = 0;             /* APB1=/1 → 120 MHz, APB2=/1 → 120 MHz */
    RCC->D3CFGR = 0;

    /* Switch SYSCLK to PLL1 */
    RCC->CFGR = RCC_CFGR_SW_PLL1;
    while (((RCC->CFGR >> 3) & 0x3) != RCC_CFGR_SW_PLL1) { }

    /* Enable all needed peripheral clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                    RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN |
                    RCC_AHB2ENR_GPIOHEN;
    RCC->APB1LENR |= RCC_APB1LENR_TIM2EN | RCC_APB1LENR_TIM3EN |
                     RCC_APB1LENR_SPI3EN  | RCC_APB1LENR_USART2EN |
                     RCC_APB1LENR_USART3EN | RCC_APB1LENR_I2C1EN |
                     RCC_APB1LENR_DAC1EN;
    RCC->APB2ENR  |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USART1EN |
                     RCC_APB2ENR_TIM1EN;
    RCC->APB4ENR  |= RCC_APB4ENR_SYSCFGEN | RCC_APB4ENR_I2C1EN;
}

/* ===================================================================== */
/*  GPIO initialization                                                   */
/* ===================================================================== */

static void gpio_init_one(GPIO_TypeDef *port, uint8_t pin,
                           uint8_t mode, uint8_t otype,
                           uint8_t speed, uint8_t pupd, uint8_t af)
{
    uint32_t moder_val = (uint32_t)mode << (pin * 2);
    uint32_t moder_msk = 0x3UL << (pin * 2);
    port->MODER = (port->MODER & ~moder_msk) | moder_val;

    port->OTYPER = (port->OTYPER & ~(1UL << pin)) | ((uint32_t)otype << pin);

    uint32_t ospeedr_msk = 0x3UL << (pin * 2);
    port->OSPEEDR = (port->OSPEEDR & ~ospeedr_msk) | ((uint32_t)speed << (pin * 2));

    uint32_t pupdr_msk = 0x3UL << (pin * 2);
    port->PUPDR = (port->PUPDR & ~pupdr_msk) | ((uint32_t)pupd << (pin * 2));

    if (af != 0xFF) {
        if (pin < 8) {
            uint32_t afr_msk = 0xFUL << (pin * 4);
            port->AFRL = (port->AFRL & ~afr_msk) | ((uint32_t)af << (pin * 4));
        } else {
            uint32_t afr_msk = 0xFUL << ((pin - 8) * 4);
            port->AFRH = (port->AFRH & ~afr_msk) | ((uint32_t)af << ((pin - 8) * 4));
        }
    }
}

static void gpio_init_all(void)
{
    /* PLL SPI1: PB3=SCK(AF5), PB4=MISO(AF5), PB5=MOSI(AF5) */
    gpio_init_one(PLL_SCK_PORT,  PLL_SCK_PIN,  GPIO_MODE_AF,  GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, AF_SPI1_SCK);
    gpio_init_one(PLL_MISO_PORT, PLL_MISO_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, AF_SPI1_MISO);
    gpio_init_one(PLL_MOSI_PORT, PLL_MOSI_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, AF_SPI1_MOSI);

    /* PLL control GPIOs: PA8=LE, PB0=CE, PC5=RF_EN */
    gpio_init_one(PLL_LE_PORT,   PLL_LE_PIN,   GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(PLL_CE_PORT,   PLL_CE_PIN,   GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(PLL_RF_EN_PORT, PLL_RF_EN_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);

    /* ADC control: software SPI + nCS + nRST + DRDY(input) */
    gpio_init_one(ADC_SCK_PORT, ADC_SCK_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(ADC_NCS_PORT,  ADC_NCS_PIN,  GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(ADC_NRST_PORT, ADC_NRST_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(ADC_DRDY_PORT, ADC_DRDY_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_UP, 0xFF);

    /* ADC MISO/MOSI via software SPI on PA6/PA7 (we use software SPI, set as input/output) */
    gpio_init_one(ADC_MISO_PORT, ADC_MISO_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(ADC_MOSI_PORT, ADC_MOSI_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 0xFF);

    /* I2C1: PB6=SCL(AF4), PB7=SDA(AF4) */
    gpio_init_one(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_MODE_AF, GPIO_OTYPE_OD,
                  GPIO_SPEED_HIGH, GPIO_PUPD_UP, AF_I2C1_SCL);
    gpio_init_one(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_MODE_AF, GPIO_OTYPE_OD,
                  GPIO_SPEED_HIGH, GPIO_PUPD_UP, AF_I2C1_SDA);

    /* USART1 GNSS: PA9=TX(AF7), PA10=RX(AF7) */
    gpio_init_one(GNSS_TX_PORT, GNSS_TX_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_USART1_TX);
    gpio_init_one(GNSS_RX_PORT, GNSS_RX_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_USART1_RX);
    gpio_init_one(GNSS_PPS_PORT, GNSS_PPS_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_DOWN, 0xFF);
    gpio_init_one(GNSS_NRST_PORT, GNSS_NRST_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);

    /* USART2 RS-485: PA2=TX, PA3=RX, PD5=DE */
    gpio_init_one(RS485_TX_PORT, RS485_TX_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_USART2_TX);
    gpio_init_one(RS485_RX_PORT, RS485_RX_PIN, GPIO_MODE_AF, 0,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_USART2_RX);
    gpio_init_one(RS485_DE_PORT, RS485_DE_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);

    /* USART3 BLE: PD8=TX, PD9=RX */
    gpio_init_one(BLE_TX_PORT, BLE_TX_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_USART3_TX);
    gpio_init_one(BLE_RX_PORT, BLE_RX_PIN, GPIO_MODE_AF, 0,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_USART3_RX);
    gpio_init_one(BLE_NRST_PORT, BLE_NRST_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(BLE_INT_PORT, BLE_INT_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_UP, 0xFF);

    /* TIM3 survey wheel: PC6=CH1(AF2), PC7=CH2(AF2) */
    gpio_init_one(WHEEL_CHA_PORT, WHEEL_CHA_PIN, GPIO_MODE_AF, 0,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_TIM3_CH1);
    gpio_init_one(WHEEL_CHB_PORT, WHEEL_CHB_PIN, GPIO_MODE_AF, 0,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, AF_TIM3_CH2);

    /* SDMMC1: PC8-12 data/clk, PD2 CMD (AF12) */
    gpio_init_one(SD_D0_PORT,  SD_D0_PIN,  GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_UP, AF_SDMMC1_D);
    gpio_init_one(SD_D1_PORT,  SD_D1_PIN,  GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_UP, AF_SDMMC1_D);
    gpio_init_one(SD_D2_PORT,  SD_D2_PIN,  GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_UP, AF_SDMMC1_D);
    gpio_init_one(SD_D3_PORT,  SD_D3_PIN,  GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_UP, AF_SDMMC1_D);
    gpio_init_one(SD_CLK_PORT, SD_CLK_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, AF_SDMMC1_CK);
    gpio_init_one(SD_CMD_PORT, SD_CMD_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_UP, AF_SDMMC1_CMD);

    /* USB OTG FS: PA11=DM(AF10), PA12=DP(AF10) */
    gpio_init_one(USB_DM_PORT, USB_DM_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, AF_USB_FS);
    gpio_init_one(USB_DP_PORT, USB_DP_PIN, GPIO_MODE_AF, GPIO_OTYPE_PP,
                  GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, AF_USB_FS);

    /* Status LEDs: PC13, PC14, PC15 — output */
    gpio_init_one(LED_R_PORT, LED_R_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(LED_G_PORT, LED_G_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(LED_B_PORT, LED_B_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0xFF);

    /* Power: PD3=PWR_EN, PD4=CHG_STAT(input) */
    gpio_init_one(PWR_EN_PORT, PWR_EN_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(CHG_STAT_PORT, CHG_STAT_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_UP, 0xFF);

    /* RF control: PB1=VCO_SEL0, PB2=VCO_SEL1, PC4=LNA_EN */
    gpio_init_one(VCO_SEL0_PORT, VCO_SEL0_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(VCO_SEL1_PORT, VCO_SEL1_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(LNA_EN_PORT, LNA_EN_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_HIGH, GPIO_PUPD_NONE, 0xFF);

    /* DAC: PA4, PA5 — analog mode */
    gpio_init_one(DAC_VCO_BIAS_PORT, DAC_VCO_BIAS_PIN, GPIO_MODE_ANALOG, 0,
                  0, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(DAC_LNA_GAIN_PORT, DAC_LNA_GAIN_PIN, GPIO_MODE_ANALOG, 0,
                  0, GPIO_PUPD_NONE, 0xFF);

    /* Buttons + buzzer: PD12-14 */
    gpio_init_one(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP,
                  GPIO_SPEED_LOW, GPIO_PUPD_NONE, 0xFF);
    gpio_init_one(NRST_BTN_PORT, NRST_BTN_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_UP, 0xFF);
    gpio_init_one(USER_BTN_PORT, USER_BTN_PIN, GPIO_MODE_INPUT, 0,
                  0, GPIO_PUPD_UP, 0xFF);

    /* Enable main 3V3 regulator */
    PWR_EN_PORT->BSRR = (1UL << PWR_EN_PIN);   /* PWR_EN high = enable */
}

/* ===================================================================== */
/*  DAC initialization for VCO bias + LNA gain control                    */
/* ===================================================================== */

static void dac_init(void)
{
    /* Enable DAC1 channel 1 (PA4) and channel 2 (PA5) */
    DAC1->CR |= DAC_CR_EN1 | DAC_CR_EN2;
    board_delay_ms(1);  /* DAC settle */
}

static void dac_set_vco_bias(uint16_t val)
{
    DAC1->DHR12R1 = val & 0xFFF;
}

static void dac_set_lna_gain(uint16_t val)
{
    DAC1->DHR12R2 = val & 0xFFF;
}

/* ===================================================================== */
/*  VCO selection                                                         */
/* ===================================================================== */

static void vco_select(uint8_t sel)
{
    /* VCO_SEL0 = PB1, VCO_SEL1 = PB2 — 2-bit code selects VCO + matching */
    if (sel & 1) VCO_SEL0_PORT->BSRR = (1UL << VCO_SEL0_PIN);
    else        VCO_SEL0_PORT->BRR  = (1UL << VCO_SEL0_PIN);
    if (sel & 2) VCO_SEL1_PORT->BSRR = (1UL << VCO_SEL1_PIN);
    else        VCO_SEL1_PORT->BRR  = (1UL << VCO_SEL1_PIN);
}

/* ===================================================================== */
/*  Stepped-frequency sweep execution                                     */
/* ===================================================================== */

/*
 * Perform one complete stepped-frequency sweep, populating sweep_I[] and
 * sweep_Q[] with the coherent I/Q samples for each frequency step.
 * Returns 0 on success.
 */
static int perform_sweep(const band_preset_t *bp)
{
    uint16_t n = bp->num_steps;
    if (n > MAX_SWEEP_STEPS) n = MAX_SWEEP_STEPS;

    g_bscan_width = n;

    /* Select VCO + matching network */
    vco_select(bp->vco_sel);

    /* Set LNA gain via DAC (0-4095 → 0-3.3V → LNA gain control) */
    uint16_t lna_dac = (uint16_t)((bp->lna_gain_db / 30.0f) * 4095.0f);
    if (lna_dac > 4095) lna_dac = 4095;
    dac_set_lna_gain(lna_dac);

    /* Enable LNA */
    LNA_EN_PORT->BSRR = (1UL << LNA_EN_PIN);

    /* Enable PLL RF output */
    PLL_RF_EN_PORT->BSRR = (1UL << PLL_RF_EN_PIN);
    board_delay_ms(5);  /* LNA + PLL settle */

    /* Compute frequency step size */
    uint32_t f_step_hz = 0;
    if (n > 1) {
        f_step_hz = ((uint64_t)(bp->f_stop_mhz - bp->f_start_mhz) * 1000000ULL) / (n - 1);
    }
    uint32_t f_current = (uint32_t)bp->f_start_mhz * 1000000ULL;

    /* Step through frequencies */
    for (uint16_t k = 0; k < n; k++) {
        /* Program PLL to f_current */
        pll_set_frequency(f_current);

        /* Dwell for the configured time (PLL + reflections settle) */
        if (bp->dwell_us >= 1000) {
            board_delay_ms(bp->dwell_us / 1000);
        } else {
            /* Microsecond delay: busy-wait */
            volatile uint32_t cyc = (bp->dwell_us * (BOARD_HCLK_FREQ_HZ / 1000000ULL));
            while (cyc--) { __asm volatile("nop"); }
        }

        /* Trigger ADC I/Q read — wait for DRDY, then read 2 channels */
        receiver_read_iq(&sweep_I[k], &sweep_Q[k]);

        f_current += f_step_hz;
    }

    /* Disable RF output + LNA after sweep */
    PLL_RF_EN_PORT->BRR = (1UL << PLL_RF_EN_PIN);
    LNA_EN_PORT->BRR     = (1UL << LNA_EN_PIN);

    return 0;
}

/* ===================================================================== */
/*  Calibration — capture system noise floor + DC offsets                   */
/* ===================================================================== */

static void run_calibration(const band_preset_t *bp)
{
    /* TX disabled: capture only system noise + DC offsets */
    PLL_RF_EN_PORT->BRR = (1UL << PLL_RF_EN_PIN);
    LNA_EN_PORT->BRR    = (1UL << LNA_EN_PIN);

    uint16_t n = bp->num_steps;
    if (n > MAX_SWEEP_STEPS) n = MAX_SWEEP_STEPS;

    vco_select(bp->vco_sel);
    board_delay_ms(10);

    uint32_t f_step_hz = 0;
    if (n > 1)
        f_step_hz = ((uint64_t)(bp->f_stop_mhz - bp->f_start_mhz) * 1000000ULL) / (n - 1);
    uint32_t f_current = (uint32_t)bp->f_start_mhz * 1000000ULL;

    for (uint16_t k = 0; k < n; k++) {
        pll_set_frequency(f_current);
        board_delay_ms(bp->dwell_us >= 1000 ? bp->dwell_us / 1000 : 1);
        receiver_read_iq(&cal_I[k], &cal_Q[k]);
        f_current += f_step_hz;
    }

    g_calibrated = 1;
}

/* ===================================================================== */
/*  Trace acquisition — one A-scan → B-scan row                           */
/* ===================================================================== */

static void acquire_trace(void)
{
    const band_preset_t *bp = &band_presets[g_band_idx];

    /* Perform the frequency sweep */
    perform_sweep(bp);

    /* Apply calibration subtraction (if available) */
    if (g_calibrated) {
        for (uint16_t k = 0; k < g_bscan_width; k++) {
            sweep_I[k] -= cal_I[k];
            sweep_Q[k] -= cal_Q[k];
        }
    }

    /* Run range-domain IFFT to produce A-scan (real-valued envelope) */
    float *ascan = bscan_data[g_bscan_traces];
    radar_range_fft(sweep_I, sweep_Q, ascan, g_bscan_width, bp);

    /* Compute depth axis */
    radar_compute_depth_axis(bscan_depth, g_bscan_width, bp, g_eps_r);

    /* Update position from IMU + GNSS */
    float dx, dy;
    imu_get_delta(&dx, &dy);
    g_imu_dx += dx;
    g_imu_dy += dy;
    gnss_get_position(&g_last_lat, &g_last_lon, NULL);

    g_bscan_traces++;
    g_traces_acquired++;
    if (g_bscan_traces >= BSCAN_BUFFER_DEPTH) g_bscan_traces = 0;  /* wrap */

    /* Update OLED with latest A-scan preview */
    display_draw_ascan_preview(ascan, g_bscan_width);

    /* LED pulse to indicate trace captured */
    LED_G_PORT->BSRR = (1UL << LED_G_PIN);
    board_delay_ms(2);
    LED_G_PORT->BRR  = (1UL << LED_G_PIN);
}

/* ===================================================================== */
/*  Survey wheel trigger — called from TIM3 ISR                          */
/* ===================================================================== */

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_CC1IF) {
        TIM3->SR = ~TIM_SR_CC1IF;
        /* Check if enough wheel pulses for one trace spacing */
        uint32_t pulses_per_trace = (uint32_t)((g_trace_spacing_mm /
                                WHEEL_CIRCUMFERENCE_MM) * WHEEL_PPR);
        static uint32_t pulse_count = 0;
        pulse_count++;
        if (pulse_count >= pulses_per_trace && g_survey_running) {
            pulse_count = 0;
            g_trace_trigger_pending = 1;
        }
    }
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR = ~TIM_SR_UIF;
    }
}

/* ===================================================================== */
/*  GNSS PPS EXTI handler — time reference pulse                          */
/* ===================================================================== */

/* EXTI line 10 (PB10 ADC DRDY) and line 10 for GNSS PPS on PD10.
 * On STM32H7, EXTI10 is shared. We handle ADC DRDY here and use
 * a separate EXTI line for PPS (PD10 = EXTI10).
 * For simplicity, PPS handling is in gnss.c via polling.
 */

/* ===================================================================== */
/*  Main state machine                                                    */
/* ===================================================================== */

int main(void)
{
    /* 1. Hardware initialization */
    clock_init();
    systick_init();
    gpio_init_all();
    dac_init();

    board_led_set(1, 0, 0);  /* red = booting */

    /* 2. Initialize subsystems */
    pll_init();
    receiver_init();
    radar_init();
    imu_init();
    wheel_init();
    gnss_init();
    sdlog_init();
    ble_init();
    display_init();

    /* 3. Show boot banner on OLED */
    display_clear();
    display_draw_text(0, 0, "StrataScan");
    display_draw_text(0, 16, "by jayis1");
    display_draw_text(0, 32, "v1.0  GPR SFCW");
    display_draw_text(0, 48, "Initializing...");
    board_delay_ms(1000);

    /* 4. Auto-calibrate current band */
    g_state = STATE_CALIBRATE;
    display_draw_text(0, 48, "Calibrating...  ");
    run_calibration(&band_presets[g_band_idx]);
    display_draw_text(0, 48, "Cal done       ");

    /* 5. Enter IDLE state — wait for BLE command or wheel trigger */
    g_state = STATE_IDLE;
    board_led_set(0, 1, 0);  /* green = ready */

    uint32_t last_display_update = 0;
    uint32_t last_battery_check  = 0;
    uint8_t  battery_pct = 100;

    /* 6. Main super-loop */
    while (1) {
        uint32_t now = g_tick_ms;

        /* --- BLE command processing --- */
        ble_tx_len = 0;
        uint8_t ble_cmd = ble_get_command();
        if (ble_cmd != 0) {
            switch (ble_cmd) {
            case 0x01:  /* START SURVEY */
                if (g_state == STATE_IDLE || g_state == STATE_PAUSE) {
                    g_state = STATE_SURVEY;
                    g_survey_running = 1;
                    g_traces_acquired = 0;
                    g_bscan_traces = 0;
                    sdlog_start_survey(g_band_idx, g_eps_r);
                    board_led_set(0, 0, 1);  /* blue = surveying */
                }
                break;
            case 0x02:  /* PAUSE SURVEY */
                g_state = STATE_PAUSE;
                g_survey_running = 0;
                board_led_set(1, 1, 0);  /* yellow = paused */
                break;
            case 0x03:  /* STOP SURVEY */
                g_state = STATE_IDLE;
                g_survey_running = 0;
                sdlog_end_survey();
                board_led_set(0, 1, 0);  /* green = ready */
                break;
            case 0x04:  /* RECALIBRATE */
                g_state = STATE_CALIBRATE;
                run_calibration(&band_presets[g_band_idx]);
                g_state = STATE_IDLE;
                break;
            case 0x05: { /* SET BAND */
                uint8_t new_band = ble_get_param_byte();
                if (new_band < 5) {
                    g_band_idx = new_band;
                    g_calibrated = 0;
                    run_calibration(&band_presets[g_band_idx]);
                }
                break;
            }
            case 0x06: { /* SET EPS_R */
                uint32_t raw = ble_get_param_word();
                g_eps_r = (float)raw / 100.0f;
                if (g_eps_r < 1.0f) g_eps_r = 1.0f;
                if (g_eps_r > 81.0f) g_eps_r = 81.0f;
                break;
            }
            case 0x07: { /* SET TRACE SPACING */
                uint32_t raw = ble_get_param_word();
                g_trace_spacing_mm = (float)raw / 10.0f;
                if (g_trace_spacing_mm < 5.0f) g_trace_spacing_mm = 5.0f;
                break;
            }
            case 0x08:  /* SEND STATUS */
                ble_tx_buf[0] = 0x08;  /* status response */
                ble_tx_buf[1] = (uint8_t)g_state;
                ble_tx_buf[2] = g_band_idx;
                ble_tx_buf[3] = (uint8_t)(g_traces_acquired >> 0);
                ble_tx_buf[4] = (uint8_t)(g_traces_acquired >> 8);
                ble_tx_buf[5] = (uint8_t)(g_traces_acquired >> 16);
                ble_tx_buf[6] = (uint8_t)(battery_pct);
                ble_tx_buf[7] = g_calibrated;
                ble_tx_buf[8] = (uint8_t)(g_bscan_traces >> 0);
                ble_tx_buf[9] = (uint8_t)(g_bscan_traces >> 8);
                /* GNSS fix */
                int32_t lat_i = (int32_t)(g_last_lat * 1e5f);
                int32_t lon_i = (int32_t)(g_last_lon * 1e5f);
                memcpy(&ble_tx_buf[10], &lat_i, 4);
                memcpy(&ble_tx_buf[14], &lon_i, 4);
                ble_tx_len = 18;
                ble_send_packet(ble_tx_buf, ble_tx_len);
                break;
            case 0x09:  /* SEND BSCAN ROW */
                if (g_bscan_traces > 0) {
                    uint16_t row = g_bscan_traces - 1;
                    if (row >= BSCAN_BUFFER_DEPTH) row = 0;
                    ble_send_bscan_row(bscan_data[row], bscan_depth,
                                       g_bscan_width, row);
                }
                break;
            case 0x0A:  /* SHUTDOWN */
                g_state = STATE_SHUTDOWN;
                sdlog_end_survey();
                display_clear();
                display_draw_text(0, 0, "Shutdown");
                board_delay_ms(500);
                PWR_EN_PORT->BRR = (1UL << PWR_EN_PIN);  /* power off */
                while (1) { }  /* should not reach here */
                break;
            default:
                break;
            }
        }

        /* --- Survey trace acquisition --- */
        if (g_state == STATE_SURVEY && g_trace_trigger_pending) {
            g_trace_trigger_pending = 0;
            acquire_trace();

            /* Log trace to SD card (SEG-Y format) */
            sdlog_write_trace(bscan_data[g_bscan_traces == 0 ?
                                        BSCAN_BUFFER_DEPTH - 1 : g_bscan_traces - 1],
                              g_bscan_width, g_traces_acquired,
                              g_last_lat, g_last_lon);

            /* Stream trace header to BLE for live radargram */
            ble_notify_trace_ready(g_traces_acquired);
        }

        /* --- Periodic display update (every 500 ms) --- */
        if ((now - last_display_update) >= 500) {
            last_display_update = now;
            if (g_state == STATE_SURVEY) {
                display_draw_status(g_traces_acquired, battery_pct,
                                    g_band_idx, g_state);
            } else if (g_state == STATE_IDLE) {
                display_draw_text(0, 48, "Ready - Start via app");
            }
        }

        /* --- Periodic battery check (every 5 s) --- */
        if ((now - last_battery_check) >= 5000) {
            last_battery_check = now;
            /* Read battery voltage via ADC1 channel 10 (PC0)
             * Simplified: use a polling read of the internal ADC.
             * The actual implementation configures ADC1 channel 10 with
             * a 3.3V reference and divides by a 2:1 resistor divider.
             */
            /* battery_pct = adc_read_battery(); */
            /* placeholder: decrement slowly */
            if (battery_pct > 0 && g_survey_running) battery_pct--;
            if (battery_pct < 15) {
                board_led_set(1, 0, 0);  /* red = low battery */
                display_draw_text(0, 56, "LOW BATTERY!");
            }
        }

        /* --- Watchdog: reset button check --- */
        if (!(NRST_BTN_PORT->IDR & (1UL << NRST_BTN_PIN))) {
            /* Reset button pressed (active low) */
            g_state = STATE_SHUTDOWN;
            sdlog_end_survey();
            board_delay_ms(100);
            NVIC->ICER[0] = 0xFFFFFFFF;  /* disable all interrupts */
            PWR_EN_PORT->BRR = (1UL << PWR_EN_PIN);
            while (1) { }
        }
    }
}

/* ===================================================================== */
/*  HardFault handler — blink red LED rapidly                            */
/* ===================================================================== */

void HardFault_Handler(void)
{
    while (1) {
        LED_R_PORT->BSRR = (1UL << LED_R_PIN);
        board_delay_ms(100);
        LED_R_PORT->BRR  = (1UL << LED_R_PIN);
        board_delay_ms(100);
    }
}

/* ===================================================================== */
/*  Startup / vector table (minimal)                                      */
/* ===================================================================== */

/* This is a simplified view; the actual startup vector table is provided
 * by the linker script's startup code (startup_stm32h743.s).  The key
 * interrupt handlers are:
 *   Reset_Handler      → calls SystemInit + main
 *   SysTick_Handler    → g_tick_ms++
 *   TIM3_IRQHandler    → survey wheel trace trigger
 *   USART1_IRQHandler  → GNSS NMEA RX
 *   USART3_IRQHandler  → BLE UART RX
 *   DMA1_Stream0_IRQHandler → ADC DMA transfer complete
 *   HardFault_Handler  → error blink
 */

/* End of main.c */