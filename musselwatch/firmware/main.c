/*
 * main.c — MusselWatch firmware main loop
 *
 * MusselWatch: Open Bivalve Valvometric Biosensor Network
 *
 * A solar-powered, LoRaWAN-capable biosensor node that monitors the
 * shell-gape activity of up to 8 freshwater bivalves (Unionidae) using
 * a Hall-effect sensor array, treating their valve behaviour as a
 * biological early-warning system for water contamination.  Mussels
 * are sentinel organisms: they filter-feed continuously and clamp
 * shut within minutes of detecting toxins, heavy metals, or pH
 * shifts — far faster than downstream chemical sensors.
 *
 * The firmware runs a low-duty-cycle state machine:
 *
 *   SLEEP (LPTIM1 wakes every 250 ms)
 *     -> SAMPLE: read all 8 Hall channels via the TMUX1108 mux + ADC
 *     -> ANALYSE: update rolling gape baselines & anomaly flags
 *     -> (every 600 s) UPLINK: send telemetry packet via LoRa
 *     -> (on anomaly) UPLINK: send alert packet immediately
 *     -> SLEEP
 *
 * Average current is ~ 250 uA at 4 Hz sampling, ~ 3 mA during the
 * ~ 200 ms LoRa TX, giving months of autonomy on a 1200 mAh Li-ion
 * cell + 1 W solar panel.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/adc.h"
#include "drivers/mux.h"
#include "drivers/sx1262.h"
#include "drivers/onewire.h"
#include "drivers/i2c_pmic.h"
#include "drivers/gape.h"

/* ---- Global state ------------------------------------------------- */

static channel_state_t channels[NUM_CHANNELS];
static telemetry_t     telemetry;
static gape_ring_t     activity_ring;

static volatile uint32_t system_tick_ms;
static volatile uint32_t uptime_seconds;
static uint32_t last_uplink_s;
static uint32_t last_alert_uplink_s;
static uint8_t  uplink_seq;
static uint8_t  node_id;

/* Calibration flag: if true, next sample for each channel sets baseline */
static bool     calibration_mode;

/* ---- Clock setup: MSI 4 MHz -> PLL -> 80 MHz ---------------------- */

static void clock_init(void)
{
    /* Enable MSI 4 MHz as system clock source during PLL setup */
    RCC_CR |= (1u << 0); /* MSION */
    while ((RCC_CR & (1u << 1)) == 0) { /* MSIRDY */ }
    RCC_CFGR = (RCC_CFGR & ~0x07u) | 0x00u;  /* SW = MSI */
    while ((RCC_CFGR >> 2) & 0x07u) { /* wait switch */ }

    /* Configure PLL: source = MSI, M=1, N=20, R=2  -> 4*20/2 = 80 MHz */
    RCC_PLLCFGR = (1u << 0)       /* PLLSRC = MSI */
                | (0u << 4)       /* PLLM = 1 */
                | (20u << 8)      /* PLLN = 20 */
                | (0u << 25);     /* PLLR = 2 (0b00) */
    RCC_CR |= (1u << 24);         /* PLLON */
    while ((RCC_CR & (1u << 25)) == 0) { /* PLLRDY */ }

    /* Flash latency 4 wait states for 80 MHz */
    FLASH_ACR = FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN;
    while ((FLASH_ACR & 0x07u) != FLASH_ACR_LATENCY_4WS) { /* wait */ }

    /* Select PLL as system clock */
    RCC_CFGR = (RCC_CFGR & ~0x07u) | 0x03u;  /* SW = PLL */
    while (((RCC_CFGR >> 2) & 0x07u) != 3u) { /* wait switch */ }

    /* Power range 1 (1.2V) for 80 MHz */
    PWR_CR1 = (PWR_CR1 & ~PWR_CR1_VOS_MASK) | PWR_CR1_VOS_RANGE1;
}

/* ---- SysTick: 1 ms tick ------------------------------------------- */

static void systick_init(void)
{
    SYST_RVR = 80000u - 1u;  /* 80 MHz / 1000 = 80k ticks per ms */
    SYST_CVR = 0u;
    SYST_CSR = SYSTICK_CLKSOURCE_Msk | SYSTICK_TICKINT_Msk | SYSTICK_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    system_tick_ms++;
    if (system_tick_ms % 1000u == 0u) {
        uptime_seconds++;
    }
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = system_tick_ms;
    while ((system_tick_ms - start) < ms) { __asm__("wfi"); }
}

/* ---- CRC32 (IEEE 802.3 polynomial 0xEDB88320) -------------------- */

static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 1u) crc = (crc >> 1) ^ 0xEDB88320u;
            else          crc = (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

/* ---- Unique node ID (low byte of 64-bit UID) --------------------- */

static uint8_t read_node_id(void)
{
    uint32_t uid = *(volatile uint32_t *)UID64_BASE;
    return (uint8_t)(uid ^ (uid >> 8) ^ (uid >> 16) ^ (uid >> 24));
}

/* ---- Status LED --------------------------------------------------- */

static void led_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    GPIO_MODER(GPIOC_BASE) &= ~(3u << (LED_PIN * 2u));
    GPIO_MODER(GPIOC_BASE) |=  (1u << (LED_PIN * 2u));
    /* Off (active-low: high = off) */
    GPIO_BSRR(GPIOC_BASE) = (1u << LED_PIN);
}

static void led_on(void)
{
    GPIO_BSRR(GPIOC_BASE) = (1u << (LED_PIN + 16u));  /* low = on */
}

static void led_off(void)
{
    GPIO_BSRR(GPIOC_BASE) = (1u << LED_PIN);          /* high = off */
}

static void led_pulse(uint32_t ms)
{
    led_on();
    delay_ms(ms);
    led_off();
}

/* ---- Heater (anti-condensation) ---------------------------------- */

static void heater_init(void)
{
    GPIO_MODER(GPIOC_BASE) &= ~(3u << (HEATER_PIN * 2u));
    GPIO_MODER(GPIOC_BASE) |=  (1u << (HEATER_PIN * 2u));
    GPIO_BSRR(GPIOC_BASE) = (1u << HEATER_PIN);  /* off */
}

static void heater_set(bool on)
{
    if (on) GPIO_BSRR(GPIOC_BASE) = (1u << (HEATER_PIN + 16u));
    else    GPIO_BSRR(GPIOC_BASE) = (1u << HEATER_PIN);
    if (on) telemetry.flags |=  0x01u;
    else    telemetry.flags &= ~0x01u;
}

/* ---- Debug UART (USART1, 115200 8N1) ----------------------------- */

static void uart_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* PA9 = TX (AF7), PA10 = RX (AF7) */
    GPIO_MODER(GPIOA_BASE) &= ~((3u << (9u * 2u)) | (3u << (10u * 2u)));
    GPIO_MODER(GPIOA_BASE) |=  ((2u << (9u * 2u)) | (2u << (10u * 2u)));
    GPIO_AFRH(GPIOA_BASE) &= ~(0xFu << ((9u - 8u) * 4u));
    GPIO_AFRH(GPIOA_BASE) &= ~(0xFu << ((10u - 8u) * 4u));
    GPIO_AFRH(GPIOA_BASE) |=  (7u << ((9u - 8u) * 4u));
    GPIO_AFRH(GPIOA_BASE) |=  (7u << ((10u - 8u) * 4u));

    USART1_BRR = 80000000u / 115200u;  /* 80 MHz / 115200 */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart_putc(char c)
{
    while ((USART1_ISR & USART_ISR_TXE) == 0u) { /* wait TX empty */ }
    USART1_TDR = (uint32_t)c;
}

static void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

/* ---- Sample all 8 channels --------------------------------------- */

static void sample_all_channels(void)
{
    for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
        mux_select(c);
        /* Oversample 4x to reduce Hall sensor noise */
        uint32_t acc = 0u;
        for (uint8_t k = 0; k < 4u; k++) {
            acc += adc_read(HALL_ADC_CH);
        }
        uint16_t raw = (uint16_t)(acc / 4u);

        if (calibration_mode) {
            gape_calibrate(&channels[c], raw);
        } else {
            gape_update(&channels[c], raw);
        }
    }
    mux_disable();
}

/* ---- Build telemetry packet -------------------------------------- */

static void build_telemetry_pkt(uplink_pkt_t *pkt)
{
    pkt->type           = PKT_TYPE_TELEMETRY;
    pkt->node_id        = node_id;
    pkt->seq            = uplink_seq++;
    pkt->flags          = telemetry.flags;
    pkt->battery_mv     = telemetry.battery_mv;
    pkt->solar_mv       = telemetry.solar_mv;
    pkt->water_temp_c10 = telemetry.water_temp_c10;
    pkt->active_channels = telemetry.active_channels;
    pkt->max_anomaly    = telemetry.max_anomaly;
    pkt->channel        = 0xFFu;  /* not applicable */
    pkt->anomaly_flag   = 0u;
    pkt->gape_um        = 0u;
    pkt->activity_score = 0u;
    pkt->reserved       = 0u;
    pkt->uptime_s       = telemetry.uptime_s;
    pkt->crc32          = 0u;  /* filled below */
    pkt->crc32          = crc32_calc((const uint8_t *)pkt, sizeof(*pkt) - 4u);
}

static void build_alert_pkt(uplink_pkt_t *pkt, uint8_t channel)
{
    pkt->type           = PKT_TYPE_ALERT;
    pkt->node_id        = node_id;
    pkt->seq            = uplink_seq++;
    pkt->flags          = telemetry.flags | 0x04u;  /* alert flag */
    pkt->battery_mv     = telemetry.battery_mv;
    pkt->solar_mv       = telemetry.solar_mv;
    pkt->water_temp_c10 = telemetry.water_temp_c10;
    pkt->active_channels = telemetry.active_channels;
    pkt->max_anomaly    = gape_anomaly_score(&channels[channel]);
    pkt->channel        = channel;
    pkt->anomaly_flag   = channels[channel].anomaly_flag;
    pkt->gape_um        = channels[channel].gape_um;
    pkt->activity_score = channels[channel].activity_score;
    pkt->reserved       = 0u;
    pkt->uptime_s       = telemetry.uptime_s;
    pkt->crc32          = 0u;
    pkt->crc32          = crc32_calc((const uint8_t *)pkt, sizeof(*pkt) - 4u);
}

/* ---- Send uplink ------------------------------------------------- */

static void send_uplink(const uplink_pkt_t *pkt)
{
    led_pulse(5);
    if (sx1262_init()) {
        sx1262_send((const uint8_t *)pkt, sizeof(*pkt));
        sx1262_sleep();
    }
}

/* ---- Update telemetry from sensors -------------------------------- */

static void update_telemetry(void)
{
    telemetry.battery_mv = adc_read_vbat_mv();
    telemetry.solar_mv   = adc_read_solar_mv();
    telemetry.uptime_s   = uptime_seconds;

    /* Water temperature from DS18B20 (on PB6 1-Wire) */
    int16_t t = 0;
    if (ds18b20_read_temp_c10(&t)) {
        telemetry.water_temp_c10 = t;
    }

    /* Charger state from BQ25870 */
    uint8_t chg = 0;
    if (pmic_get_charger_state(&chg)) {
        telemetry.charger_state = chg;
    }

    /* Active channels mask */
    uint8_t mask = 0u;
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (channels[i].raw_baseline != 2048u || channels[i].raw_hall != 0u)
            mask |= (1u << i);
    }
    telemetry.active_channels = mask;

    /* Max anomaly across channels */
    uint8_t maxa = 0u;
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        uint8_t s = gape_anomaly_score(&channels[i]);
        if (s > maxa) maxa = s;
    }
    telemetry.max_anomaly = maxa;

    /* Low battery flag */
    if (telemetry.battery_mv < BATT_MV_LOW)
        telemetry.flags |=  0x02u;
    else
        telemetry.flags &= ~0x02u;

    /* Heater control: enable if water < 0°C and battery > 3.7 V */
    if (telemetry.water_temp_c10 < 0 && telemetry.battery_mv > 3700u)
        heater_set(true);
    else
        heater_set(false);
}

/* ---- Low-power sleep via WFI (until next SysTick) ---------------- */

static void enter_sleep(void)
{
    /*
     * In a production build, STOP2 with LPTIM1 as wake source is used.
     * Here we use WFI for simplicity; SysTick keeps running.
     */
    __asm__("wfi");
}

/* ---- Calibration command (via UART single 'C' character) -------- */

static void check_calibration_command(void)
{
    if (USART1_ISR & USART_ISR_RXNE) {
        char cmd = (char)USART1_RDR;
        if (cmd == 'C' || cmd == 'c') {
            calibration_mode = true;
            uart_puts("CAL: close all shells now, sampling baseline...\r\n");
        } else if (cmd == 'X' || cmd == 'x') {
            calibration_mode = false;
            uart_puts("CAL: done.\r\n");
        } else if (cmd == 'S' || cmd == 's') {
            uart_puts("STA: sending uplink now.\r\n");
            update_telemetry();
            uplink_pkt_t pkt;
            build_telemetry_pkt(&pkt);
            send_uplink(&pkt);
            last_uplink_s = uptime_seconds;
        }
    }
}

/* ---- Main loop state machine ------------------------------------- */

static void run_state_machine(void)
{
    static uint32_t last_sample_ms = 0u;

    /* Sample at 4 Hz */
    if ((system_tick_ms - last_sample_ms) >= SAMPLE_PERIOD_MS) {
        last_sample_ms = system_tick_ms;

        /* Power up ADC, sample, power down */
        adc_init();
        sample_all_channels();
        adc_enter_lowpower();

        if (calibration_mode) {
            /* baseline is captured; exit cal mode after one full sweep */
            calibration_mode = false;
            uart_puts("CAL: baseline captured.\r\n");
        }
    }

    /* Update telemetry & check for alerts every 15 seconds */
    static uint32_t last_telemetry_s = 0u;
    if ((uptime_seconds - last_telemetry_s) >= 15u) {
        last_telemetry_s = uptime_seconds;
        adc_init();
        update_telemetry();
        adc_enter_lowpower();
    }

    /* Alert uplink: if any channel has anomaly_flag set, send alert */
    for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
        if (channels[c].anomaly_flag &&
            (uptime_seconds - last_alert_uplink_s) >= ALERT_UPLINK_HOLD_S) {
            uplink_pkt_t pkt;
            build_alert_pkt(&pkt, c);
            send_uplink(&pkt);
            last_alert_uplink_s = uptime_seconds;
            uart_puts("ALR: anomaly on channel\r\n");
            break;  /* one alert per epoch */
        }
    }

    /* Periodic telemetry uplink */
    if ((uptime_seconds - last_uplink_s) >= UPLINK_INTERVAL_S) {
        uplink_pkt_t pkt;
        build_telemetry_pkt(&pkt);
        send_uplink(&pkt);
        last_uplink_s = uptime_seconds;
    }
}

/* ---- Main -------------------------------------------------------- */

int main(void)
{
    /* --- Hardware init --- */
    clock_init();
    systick_init();
    led_init();
    heater_init();
    uart_init();
    i2c1_init();
    ow_init();
    mux_init();

    node_id = read_node_id();

    /* --- Announce --- */
    uart_puts("\r\n=== MusselWatch v1.0 (c) jayis1 ===\r\n");
    uart_puts("Bivalve valvometric biosensor node\r\n");
    uart_puts("Node ID: 0x");
    /* Hex print node_id */
    char hex[3] = { "00" };
    hex[0] = (node_id >> 4) < 10 ? '0' + (node_id >> 4) : 'A' + (node_id >> 4) - 10;
    hex[1] = (node_id & 0xF) < 10 ? '0' + (node_id & 0xF) : 'A' + (node_id & 0xF) - 10;
    hex[2] = '\0';
    uart_puts(hex);
    uart_puts("\r\n");

    /* --- Initialise channel & telemetry state --- */
    gape_init(channels);
    memset(&telemetry, 0, sizeof(telemetry));
    ring_init(&activity_ring);
    uplink_seq = 0u;
    last_uplink_s = 0u;
    last_alert_uplink_s = 0u;
    calibration_mode = false;

    /* --- Init LoRa radio --- */
    uart_puts("INIT: LoRa SX1262...\r\n");
    if (sx1262_init()) {
        uart_puts("INIT: LoRa OK\r\n");
        sx1262_sleep();
    } else {
        uart_puts("INIT: LoRa FAIL\r\n");
    }

    /* --- PMIC: set 500 mA charge current --- */
    pmic_set_charge_current_ma(500u);

    /* --- Startup LED sequence --- */
    for (uint8_t i = 0; i < 3; i++) {
        led_pulse(100);
        delay_ms(100);
    }

    uart_puts("RUN: entering main loop\r\n");

    /* --- Main super-loop --- */
    while (1) {
        check_calibration_command();
        run_state_machine();
        enter_sleep();
    }

    return 0;  /* unreachable */
}