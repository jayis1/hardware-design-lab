/*
 * main.c — AeroCast firmware super-loop
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Entry point and 1 kHz tick orchestration for the AeroCast
 * real-time bioaerosol identification station.
 *
 * Design notes
 *  - No RTOS. A cooperative super-loop with a 1 ms SysTick and two
 *    ISRs (ADC window + UART RX) handles everything deterministically.
 *  - The hot path is: LTC2351-14 → DMA ring → event extractor →
 *    classifier → per-minute bin → storage + comms + display.
 *  - main() never exits.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "board.h"
#include "registers.h"
#include "drivers/optics.h"
#include "drivers/flow.h"
#include "drivers/afe.h"
#include "drivers/classify.h"
#include "drivers/sensors.h"
#include "drivers/comms.h"
#include "drivers/display.h"
#include "drivers/storage.h"

/* ---- SysTick counter (1 ms) ---- */
static volatile uint32_t g_tick_ms = 0;
static volatile uint32_t g_uptime_s = 0;

/* Called from SysTick_Handler at 1 kHz. Keep it tiny. */
void sys_tick_handler(void)
{
    g_tick_ms++;
    if ((g_tick_ms % 1000u) == 0u) g_uptime_s++;
}

uint32_t millis(void)        { return g_tick_ms; }
uint32_t uptime_seconds(void){ return g_uptime_s; }

/* ---- Per-minute sample bin ---- */
typedef struct {
    uint32_t epoch_min;       /* UNIX minute index */
    uint32_t counts[N_CLASSES];
    float    fsc_p50;         /* medians for logging */
    float    ssc_p50;
    float    flb_p50;
    float    fla_p50;
    float    temperature;     /* °C */
    float    humidity;        /* %RH */
    float    pressure;        /* hPa */
    float    flow_lpm;        /* measured sample flow */
    float    wind_dir;        /* ° (optional anemometer) */
    float    wind_speed;      /* m/s */
} minute_bin_t;

static minute_bin_t g_bin;
static minute_bin_t g_prev_bin;   /* last completed bin, for publish */

/* ---- Forward decls ---- */
static void clock_init(void);
static void gpio_board_init(void);
static void systick_init(void);
static void bin_reset(minute_bin_t *b);
static void bin_finalize(minute_bin_t *b);
static void publish_bin(const minute_bin_t *b);
static void update_display(const minute_bin_t *live, const minute_bin_t *prev);
static void handle_cmd(const char *line);

/* ============================================================ */
int main(void)
{
    /* 1. Hardware bring-up */
    clock_init();
    gpio_board_init();
    systick_init();

    /* Status LED red while booting */
    gpio_set(PORTB, 2);
    gpio_clr(PORTB, 1);

    /* 2. Subsystem init */
    display_init();
    display_boot_splash("AeroCast  v" FW_VERSION_STR "  — jayis1");

    storage_init();
    storage_append_log("# AeroCast boot, author jayis1, " FW_VERSION_STR "\n");

    sensors_init();        /* SHT45, BMP390 */
    flow_init();           /* SDP810 + blower PI */
    optics_init();         /* laser, UV, PMT bias */
    afe_init();            /* LTC2351-14 + event extractor */
    classify_init();       /* load calib.bin from eMMC */
    comms_init();          /* ESP32-C6 AT bridge */

    /* 3. Start sampling */
    flow_set_target(TARGET_FLOW_LPM);
    optics_laser_on(0x600);       /* ~75 % drive */
    optics_pmt_bias_set(0x980);   /* ~780 V */

    bin_reset(&g_bin);
    g_bin.epoch_min = uptime_seconds() / 60u;

    /* 4. Super-loop */
    uint32_t last_10ms = 0, last_100ms = 0, last_1s = 0, last_60s = 0;
    char cmd_buf[128];
    uint8_t cmd_len = 0;

    gpio_clr(PORTB, 2);    /* red off */
    gpio_set(PORTB, 1);    /* green on — running */

    while (1) {
        uint32_t now = millis();

        /* ---- 10 ms: pull particle events from AFE ring ---- */
        if ((now - last_10ms) >= 10u) {
            last_10ms = now;
            particle_event_t ev;
            while (afe_pop_event(&ev)) {
                int cls = classify_event(&ev);
                if (cls >= 0 && cls < N_CLASSES) {
                    g_bin.counts[cls]++;
                    /* running median accumulators (approx p50) */
                    g_bin.fsc_p50 += 0.01f * ((float)ev.fsc_area - g_bin.fsc_p50);
                    g_bin.ssc_p50 += 0.01f * ((float)ev.ssc_area - g_bin.ssc_p50);
                    g_bin.flb_p50 += 0.01f * ((float)ev.fl_blue   - g_bin.flb_p50);
                    g_bin.fla_p50 += 0.01f * ((float)ev.fl_amber  - g_bin.fla_p50);
                }
            }
        }

        /* ---- 100 ms: flow PI + environment poll ---- */
        if ((now - last_100ms) >= 100u) {
            last_100ms = now;
            flow_pi_step();                 /* update blower PWM */
            g_bin.flow_lpm = flow_measured();
            sensors_read_env(&g_bin.temperature, &g_bin.humidity, &g_bin.pressure);
            sensors_read_wind(&g_bin.wind_dir, &g_bin.wind_speed);
        }

        /* ---- 1 s: comms poll + display refresh + watchdog ---- */
        if ((now - last_1s) >= 1000u) {
            last_1s = now;
            comms_poll();                   /* service ESP32-C6 */

            /* drain any downlink commands */
            uint8_t c;
            while (comms_getc(&c)) {
                if (c == '\n' || cmd_len >= sizeof(cmd_buf) - 1u) {
                    cmd_buf[cmd_len] = '\0';
                    if (cmd_len) handle_cmd(cmd_buf);
                    cmd_len = 0;
                } else {
                    cmd_buf[cmd_len++] = (char)c;
                }
            }

            update_display(&g_bin, &g_prev_bin);
        }

        /* ---- 60 s: finalize bin, log, publish ---- */
        if ((now - last_60s) >= 60000u) {
            last_60s = now;
            bin_finalize(&g_bin);
            g_prev_bin = g_bin;
            publish_bin(&g_prev_bin);
            storage_write_bin_csv(&g_prev_bin);
            bin_reset(&g_bin);
            g_bin.epoch_min = uptime_seconds() / 60u;
        }

        /* ---- Lid interlock safety ---- */
        if (gpio_read(PORTC, 0) == 0u) {  /* lid open (active-low) */
            optics_laser_off();
            optics_uv_off();
            gpio_set(PORTB, 2);            /* red */
            gpio_clr(PORTB, 1);            /* green */
            display_alert("LID OPEN — laser disabled");
            while (gpio_read(PORTC, 0) == 0u) delay_ms(50);
            optics_laser_on(0x600);
            gpio_clr(PORTB, 2);
            gpio_set(PORTB, 1);
        }
    }
    /* never reached */
}

/* ============================================================ */
static void bin_reset(minute_bin_t *b)
{
    memset(b, 0, sizeof(*b));
    b->fsc_p50 = b->ssc_p50 = b->flb_p50 = b->fla_p50 = 0.0f;
    b->temperature = b->humidity = b->pressure = 0.0f;
    b->flow_lpm = 0.0f;
    b->wind_dir = b->wind_speed = 0.0f;
}

static void bin_finalize(minute_bin_t *b)
{
    if (b->flow_lpm > 0.1f) {
        /* Convert raw counts to concentration (grains/m³).
         * Sample volume per minute = flow(L/min) * 1e-3 m³/L.
         * counts / volume = grains/m³. */
        float vol_m3 = b->flow_lpm * 1e-3f * 60.0f; /* 1 min */
        for (int i = 0; i < N_CLASSES; ++i) {
            /* store concentration in upper bits? keep raw counts for log;
             * the app does the division. We just guard divide-by-zero. */
            (void)vol_m3;
        }
    }
}

static void publish_bin(const minute_bin_t *b)
{
    /* JSON over MQTT via ESP32-C6 AT bridge */
    char json[512];
    int n = snprintf(json, sizeof(json),
        "{\"t\":%lu,\"flow\":%.2f,\"T\":%.1f,\"RH\":%.1f,\"P\":%.1f,"
        "\"wd\":%.0f,\"ws\":%.1f,\"c\":[",
        (unsigned long)b->epoch_min, b->flow_lpm, b->temperature,
        b->humidity, b->pressure, b->wind_dir, b->wind_speed);
    for (int i = 0; i < N_CLASSES; ++i) {
        n += snprintf(json + n, sizeof(json) - n, "%s%lu",
                      i ? "," : "", (unsigned long)b->counts[i]);
        if (n >= (int)sizeof(json) - 32) break;
    }
    snprintf(json + n, sizeof(json) - n, "]}");
    comms_mqtt_publish("aerocast/events", json);
}

static void update_display(const minute_bin_t *live, const minute_bin_t *prev)
{
    uint32_t total = 0;
    for (int i = 0; i < N_CLASSES; ++i) total += live->counts[i];
    display_status(total, live->flow_lpm, live->temperature, live->humidity,
                   prev ? prev->counts : NULL);
}

static void handle_cmd(const char *line)
{
    /* Minimal command set over BLE/Wi-Fi downlink */
    if (strncmp(line, "set flow ", 9) == 0) {
        float f = (float)atof(line + 9);
        if (f > 0.5f && f < 25.0f) flow_set_target(f);
    } else if (strncmp(line, "laser off", 9) == 0) {
        optics_laser_off();
    } else if (strncmp(line, "laser on", 8) == 0) {
        optics_laser_on(0x600);
    } else if (strncmp(line, "blank", 5) == 0) {
        /* clean-air blank run — flush for 30 s and re-zero baselines */
        afe_run_blank(30u);
    } else if (strncmp(line, "calib upload", 12) == 0) {
        comms_begin_ota_calib();
    } else if (strncmp(line, "reboot", 6) == 0) {
        NVIC_SystemReset();
    } else if (strncmp(line, "version", 7) == 0) {
        comms_send_line("AeroCast " FW_VERSION_STR " by jayis1");
    }
}

/* ============================================================ */
/* Clock & GPIO bring-up                                         */
/* ============================================================ */
static void clock_init(void)
{
    /* Enable clocks for all peripherals we use. On STM32H7A3 the exact
     * RCC bit layout is peripheral-specific; we set the common enables
     * here and let drivers handle the finer bits. */
    RCC_AHB4ENR  |= (1u << 0u) | (1u << 1u) | (1u << 2u) | (1u << 3u) | (1u << 7u); /* GPIOA-D,H */
    RCC_AHB2ENR  |= (1u << 0u) | (1u << 5u);  /* ADC1, DAC1 */
    RCC_APB1ENR  |= (1u << 0u) | (1u << 1u)   /* TIM2, TIM3 */
                  | (1u << 14u) | (1u << 15u) /* USART2, USART3 */
                  | (1u << 21u) | (1u << 23u) | (1u << 24u) /* I2C1, SPI2, SPI3 */
                  | (1u << 24u);               /* I2C4 (bit varies; safe OR) */
    RCC_APB2ENR  |= (1u << 12u) | (1u << 14u); /* SPI1, USART1 */
}

static void gpio_board_init(void)
{
    /* ADC SPI pins */
    gpio_set_af(PORTA, 5, 5); gpio_set_mode(PORTA, 5, GPIO_MODE_AF);
    gpio_set_af(PORTB, 4, 5); gpio_set_mode(PORTB, 4, GPIO_MODE_AF);
    gpio_set_af(PORTB, 5, 5); gpio_set_mode(PORTB, 5, GPIO_MODE_AF);
    gpio_set_mode(PORTC, 8, GPIO_MODE_OUTPUT);   /* CS */
    gpio_set_mode(PORTC, 9, GPIO_MODE_OUTPUT);   /* CONVST */
    gpio_set_mode(PORTC, 10, GPIO_MODE_INPUT);   /* BUSY */

    /* Display SPI3 */
    gpio_set_af(PORTC, 10, 6); gpio_set_mode(PORTC, 10, GPIO_MODE_AF);
    gpio_set_af(PORTC, 12, 6); gpio_set_mode(PORTC, 12, GPIO_MODE_AF);
    gpio_set_mode(PORTC, 13, GPIO_MODE_OUTPUT);
    gpio_set_mode(PORTC, 14, GPIO_MODE_OUTPUT);
    gpio_set_mode(PORTC, 15, GPIO_MODE_OUTPUT);
    gpio_set_mode(PORTD, 0, GPIO_MODE_OUTPUT);

    /* ESP32-C6 USART2 */
    gpio_set_af(PORTA, 2, 7); gpio_set_mode(PORTA, 2, GPIO_MODE_AF);
    gpio_set_af(PORTA, 3, 7); gpio_set_mode(PORTA, 3, GPIO_MODE_AF);
    gpio_set_mode(PORTA, 6, GPIO_MODE_OUTPUT);  /* RST */
    gpio_set_mode(PORTA, 7, GPIO_MODE_OUTPUT);  /* BOOT */
    gpio_clr(PORTA, 7);  /* BOOT = 0 normal */
    gpio_clr(PORTA, 6);  /* hold ESP in reset briefly */
    delay_ms(50);
    gpio_set(PORTA, 6);  /* release ESP reset */

    /* Wind USART3 */
    gpio_set_af(PORTB, 10, 7); gpio_set_mode(PORTB, 10, GPIO_MODE_AF);
    gpio_set_af(PORTB, 11, 7); gpio_set_mode(PORTB, 11, GPIO_MODE_AF);
    gpio_set_mode(PORTB, 12, GPIO_MODE_OUTPUT);

    /* I2C1 (env) */
    gpio_set_af(PORTB, 6, 4); gpio_set_mode(PORTB, 6, GPIO_MODE_AF);
    gpio_set_af(PORTB, 7, 4); gpio_set_mode(PORTB, 7, GPIO_MODE_AF);
    gpio_set_pupd(PORTB, 6, 1); gpio_set_pupd(PORTB, 7, 1);

    /* I2C4 (flow) */
    gpio_set_af(PORTB, 8, 2); gpio_set_mode(PORTB, 8, GPIO_MODE_AF);
    gpio_set_af(PORTB, 9, 2); gpio_set_mode(PORTB, 9, GPIO_MODE_AF);
    gpio_set_pupd(PORTB, 8, 1); gpio_set_pupd(PORTB, 9, 1);

    /* Blower PWM TIM2_CH1 on PA15 */
    gpio_set_af(PORTA, 15, 1); gpio_set_mode(PORTA, 15, GPIO_MODE_AF);

    /* UV PWM TIM3_CH1 on PB0 */
    gpio_set_af(PORTB, 0, 2); gpio_set_mode(PORTB, 0, GPIO_MODE_AF);

    /* DAC1 channels */
    gpio_set_mode(PORTA, 4, GPIO_MODE_ANALOG);  /* PMT bias */
    gpio_set_mode(PORTA, 5, GPIO_MODE_ANALOG);  /* laser Iset (note: shared w/ SPI1 SCK
                                                   on some revisions — see errata) */

    /* Lid interlock + button */
    gpio_set_mode(PORTC, 0, GPIO_MODE_INPUT);
    gpio_set_pupd(PORTC, 0, 1);  /* pull-up */
    gpio_set_mode(PORTC, 1, GPIO_MODE_INPUT);
    gpio_set_pupd(PORTC, 1, 1);

    /* LEDs */
    gpio_set_mode(PORTB, 1, GPIO_MODE_OUTPUT);  /* green */
    gpio_set_mode(PORTB, 2, GPIO_MODE_OUTPUT);  /* red */
}

static void systick_init(void)
{
    /* SysTick: 280 MHz / 1000 → reload 280000-1 */
    volatile uint32_t *syst = (volatile uint32_t *)0xE000E010UL;
    syst[0] = 0;            /* CTRL */
    syst[1] = 280000u - 1u; /* LOAD */
    syst[2] = 0;            /* VAL */
    syst[0] = 0x7u;         /* CLKSOURCE=processor, TICKINT=1, ENABLE=1 */
}

/* NVIC system reset (SCB AIRCR) */
void NVIC_SystemReset(void)
{
    volatile uint32_t *aircr = (volatile uint32_t *)0xE000ED0CUL;
    __asm volatile("cpsid i");
    *aircr = (0x5FAu << 16u) | (1u << 2u); /* SYSRESETREQ */
    while (1) ;
}

/* Helper used by Makefile version string */
#define FW_VERSION_STR "1.0.0"

/* Linker provides these via startup_stm32h7a3.s (not included in this
 * source review bundle but present in the build). */
void Reset_Handler(void) { main(); }