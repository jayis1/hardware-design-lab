/*
 * main.c — FrostSentinel firmware main entry point and scheduler
 *
 * Implements the boot sequence, the 4-state cooperative scheduler
 * (Sleep / Sample / Compute / Transmit), the RFRI fusion logic,
 * the frost-watch state machine, and the alert dispatch.
 *
 * There is no RTOS.  A single 1 kHz tick (from the RV-3028-C7 RTC
 * 1 kHz CLKOUT, captured by TIM6) drives the scheduler.  Between
 * sample cycles the MCU enters Stop2 low-power mode, woken by the
 * RTC alarm or a LoRa RX interrupt.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "registers.h"

#include "drivers/skyir.h"
#include "drivers/psychro.h"
#include "drivers/leafwet.h"
#include "drivers/acoustic.h"
#include "drivers/radio.h"
#include "drivers/ble.h"
#include "drivers/flashio.h"
#include "drivers/power.h"
#include "drivers/rtc.h"
#include "drivers/thermo.h"
#include "drivers/model.h"

/* ------------------------------------------------------------------ */
/*  Global state                                                       */
/* ------------------------------------------------------------------ */
volatile uint32_t g_tick_ms = 0;
volatile uint32_t g_rtc_seconds = 0;
sys_state_block_t g_sys;

/* Default network key (in production, provisioned via BLE) */
static const uint8_t k_default_net_key[16] = {
    0xF5, 0x05, 0x12, 0x83, 0xA1, 0x9E, 0x4B, 0xC7,
    0xD2, 0x3F, 0x68, 0x11, 0x7A, 0xB4, 0xE6, 0x9D
};

/* ------------------------------------------------------------------ */
/*  TIM6 1 kHz system tick (driven by RTC CLKOUT on PA8/TIM6_ETR)      */
/*  For simplicity in this bare-metal build, we use TIM6 as a         */
/*  free-running 1 kHz timer from the APB1 clock.                     */
/* ------------------------------------------------------------------ */
static void sys_tick_init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;
    TIM6->PSC = (APB1_HZ / 1000) - 1;  /* 160 MHz / 1000 = 1 kHz */
    TIM6->ARR = 1000;
    TIM6->DIER = TIM_DIER_UIE;
    TIM6->CR1 = TIM_CR1_CEN | TIM_CR1_ARPE;

    /* Enable TIM6 interrupt in NVIC */
    extern void __NVIC_EnableIRQ(uint32_t);
    __NVIC_EnableIRQ(IRQ_TIM6_DAC);
}

/* TIM6 global interrupt handler — 1 kHz tick */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR = 0;  /* clear flag */
        g_tick_ms++;
    }
}

/* ------------------------------------------------------------------ */
/*  Clock setup: HSI16 → PLL1 → 160 MHz SYSCLK                         */
/* ------------------------------------------------------------------ */
static void clock_init(void)
{
    /* Enable HSI16 */
    RCC->CR |= (1u << 8);  /* HSION */
    while (!(RCC->CR & (1u << 10))) ;  /* wait HSIRDY */

    /* Configure PLL1: input = HSI16, M=1, N=20, R=2 → 160 MHz */
    RCC->PLL1CKSELR = 0x00;     /* DIVM1=1, input = HSI16 */
    RCC->PLL1CFGR = (1u << 0);  /* PLL1RGE = 00 (HSI16 range) */
    /* Set N=20, R=2 via the PLL1CFGR fields (simplified) */
    SET_FIELD(RCC->PLL1CFGR, 0x7F00u, 20u << 8);   /* DIVN1 = 20 */
    SET_FIELD(RCC->PLL1CFGR, 0x70000u, 2u << 16);  /* DIVR1 = 2 */

    /* Enable PLL1 */
    RCC->CR |= (1u << 24);  /* PLL1ON */
    while (!(RCC->CR & (1u << 25))) ;  /* wait PLL1RDY */

    /* Switch SYSCLK to PLL1 */
    SET_FIELD(RCC->CFGR, 0x07u, 0x03u);  /* SW = PLL1R */
    while (((RCC->CFGR >> 3) & 0x07) != 0x03) ;  /* wait SWS */

    /* Enable all needed peripheral clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_FLASHEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                    RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOHEN |
                    RCC_AHB2ENR_AES1EN | RCC_AHB2ENR_ADC12EN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_TIM2EN |
                     RCC_APB1ENR1_TIM3EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
}

/* ------------------------------------------------------------------ */
/*  GPIO initialization for board pins                                 */
/* ------------------------------------------------------------------ */
static void gpio_init(void)
{
    /* PB5 = fan enable (output, default off) */
    GPIO_CONFIG(GPIOB, 5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_LOW,
                GPIO_PUPD_NONE, 0);
    FAN_OFF();

    /* PB12 = DS18B20 1-wire (will be configured by ds18b20 driver) */
    /* PB0, PB1 = flash CS, SX1262 CS (configured by their drivers) */
}

/* ------------------------------------------------------------------ */
/*  I2C1 init (shared by MLX90632, SHT45, BMP390, RV3028, LC709203F)   */
/* ------------------------------------------------------------------ */
static void i2c1_init(void)
{
    /* PB6 = I2C1_SCL (AF4), PB7 = I2C1_SDA (AF4) */
    GPIO_CONFIG(GPIOB, 6, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_HIGH,
                GPIO_PUPD_UP, 4);
    GPIO_CONFIG(GPIOB, 7, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_HIGH,
                GPIO_PUPD_UP, 4);

    /* I2C1 timing for 100 kHz at 160 MHz: TIMINGR = 0x30A09E6E (approx) */
    I2C1->TIMINGR = 0x30A09E6Eu;
    I2C1->CR1 = I2C_CR1_PE;  /* Enable peripheral */
}

/* ------------------------------------------------------------------ */
/*  BLE command callback                                               */
/* ------------------------------------------------------------------ */
static void ble_command_handler(const uint8_t *payload, uint8_t len)
{
    if (len < 1) return;

    uint8_t cmd = payload[0];
    switch (cmd) {
    case 0x01:  /* Set time */
        if (len >= 5) {
            uint32_t epoch = ((uint32_t)payload[1] << 24) |
                             ((uint32_t)payload[2] << 16) |
                             ((uint32_t)payload[3] << 8)  |
                             (uint32_t)payload[4];
            rtc_set_seconds(epoch);
            g_rtc_seconds = epoch;
        }
        break;

    case 0x02:  /* Set sample interval (minutes) */
        if (len >= 2) {
            g_sys.sample_interval = payload[1];
        }
        break;

    case 0x03:  /* Start frost watch (force armed AE) */
        g_sys.state = SYS_STATE_FROST_WATCH;
        g_sys.flags |= SYS_FLAG_AE_ARMED;
        break;

    case 0x04:  /* Stop frost watch */
        g_sys.state = SYS_STATE_SLEEP;
        g_sys.flags &= ~SYS_FLAG_AE_ARMED;
        acoustic_reset();
        break;

    case 0x05:  /* Request log dump */
        /* In production, stream records via BLE_FRAME_LOG_RECORD */
        break;

    case 0x10:  /* Provision: set node_id, mesh_role, network_key */
        if (len >= 19) {
            g_sys.node_id = payload[1];
            g_sys.mesh_role = payload[2];
            ble_set_network_key(&payload[3]);
        }
        break;

    case 0x20:  /* Calibrate leaf wetness threshold */
        if (len >= 3) {
            /* payload[1..2] = uint16 threshold */
            /* Store in flash config (production) */
        }
        break;

    case 0x30:  /* Reset AE baseline */
        acoustic_reset();
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Compute RFRI (Radiative Frost Risk Index)                          */
/*                                                                    */
/*  Fuses the TinyML model output with a deterministic heuristic      */
/*  based on wet-bulb, radiative deficit, and leaf wetness.           */
/*  Returns Q8.8 RFRI (0..256 = 0.0..1.0).                            */
/* ------------------------------------------------------------------ */
static int16_t compute_rfri(void)
{
    /* Model probability (Q8.8) */
    int16_t p_model = model_compute_frost_probability();

    /* Deterministic heuristic (Q8.8) */
    int16_t p_heuristic = 0;

    /* Factor 1: wet-bulb proximity to 0 °C */
    /* p_wb = max(0, 1 - twet / critical) ; twet in 0.01 °C */
    int32_t twet = g_sys.twet_cx100;
    int32_t p_wb;
    if (twet <= 0) {
        p_wb = 256;  /* wet-bulb at or below freezing → max risk */
    } else if (twet > 500) {  /* > 5 °C → no risk from wet-bulb */
        p_wb = 0;
    } else {
        p_wb = 256 - (twet * 256) / 500;
    }

    /* Factor 2: radiative deficit */
    int32_t drad = g_sys.delta_rad_cx100;  /* 0.01 K units */
    int32_t p_rad;
    if (drad >= 2000) {  /* ≥ 20 K deficit → max radiative risk */
        p_rad = 256;
    } else if (drad < 500) {  /* < 5 K → no radiative risk */
        p_rad = 0;
    } else {
        p_rad = ((drad - 500) * 256) / 1500;
    }

    /* Factor 3: leaf wetness */
    int32_t p_wet;
    if (g_sys.leaf_wet >= 300) {
        p_wet = 256;
    } else if (g_sys.leaf_wet < 50) {
        p_wet = 0;
    } else {
        p_wet = (g_sys.leaf_wet - 50) * 256 / 250;
    }

    /* Heuristic = 0.4*p_wb + 0.3*p_rad + 0.3*p_wet */
    p_heuristic = (int16_t)((p_wb * 102 + p_rad * 77 + p_wet * 77) / 256);

    /* Fuse model and heuristic: 60% model, 40% heuristic
     * (model is trained, heuristic is the safety net) */
    int16_t rfri = (int16_t)((p_model * 153 + p_heuristic * 102) / 256);

    /* If AE nucleation detected, force RFRI to maximum */
    if (g_sys.ae_status == AE_STATUS_NUCLEATION) {
        rfri = 256;
    }

    /* Clamp */
    if (rfri < 0) rfri = 0;
    if (rfri > 256) rfri = 256;

    return rfri;
}

/* ------------------------------------------------------------------ */
/*  Compute time-to-critical-freeze (hours)                            */
/*                                                                    */
/*  Simple linear extrapolation from the last 2 wet-bulb samples.      */
/*  Returns hours until T_wet reaches 0 °C, or 255 if > 12 h.          */
/* ------------------------------------------------------------------ */
static uint8_t compute_time_to_critical(void)
{
    /* If wet-bulb is already at or below 0, return 0 */
    if (g_sys.twet_cx100 <= 0) return 0;

    /* If wet-bulb is above 5 °C, not critical */
    if (g_sys.twet_cx100 > 500) return 255;

    /* Without stored history, use a simple rate estimate:
     * assume cooling rate from radiative deficit.
     * Rate ≈ delta_rad / 20 * 1 K/hr  (empirical) */
    int32_t rate_cx100_per_hr = (g_sys.delta_rad_cx100 * 100) / 2000;
    if (rate_cx100_per_hr < 10) return 255;  /* cooling too slowly */

    int32_t hours = g_sys.twet_cx100 / rate_cx100_per_hr;
    if (hours > 12) return 255;
    return (uint8_t)hours;
}

/* ------------------------------------------------------------------ */
/*  Sample state: run all sensors                                      */
/* ------------------------------------------------------------------ */
static void do_sample(void)
{
    int32_t sky_t = 0, tdry = 0, twet = 0, dep = 0, air_t = 0, rh = 0;
    int32_t pressure = 0, bmp_temp = 0, leaf_t = 0;
    uint16_t wetness = 0;
    uint8_t wick_dry = 0;

    /* Sky IR */
    if (skyir_read_sky_temp_cx100(&sky_t) == 0) {
        g_sys.sky_t_cx100 = (int16_t)sky_t;
    }

    /* Psychrometer (wet-bulb) */
    if (psychro_measure(&tdry, &twet, &dep, &wick_dry) == 0) {
        g_sys.air_t_cx100 = (int16_t)tdry;
        g_sys.twet_cx100  = (int16_t)twet;
        g_sys.wick_dry = wick_dry;
        if (wick_dry) {
            g_sys.flags |= SYS_FLAG_WICK_DRY;
        } else {
            g_sys.flags &= ~SYS_FLAG_WICK_DRY;
        }
    }

    /* Leaf wetness */
    if (leafwet_read(&wetness) == 0) {
        g_sys.leaf_wet = wetness;
    }

    /* Auxiliary: SHT45 (air T/RH cross-check) */
    if (sht45_measure(&air_t, &rh) == 0) {
        /* Use SHT45 air T if psychrometer failed */
        if (tdry == 0) g_sys.air_t_cx100 = (int16_t)air_t;
    }

    /* Auxiliary: BMP390 (pressure for psychrometric calc) */
    bmp390_read(&pressure, &bmp_temp);
    if (pressure == 0) pressure = 1013;  /* default sea-level */

    /* Auxiliary: DS18B20 (leaf-surface temperature) */
    ds18b20_read_cx100(&leaf_t);

    /* Compute radiative deficit */
    g_sys.delta_rad_cx100 = (int16_t)(tdry - sky_t);

    /* Power status */
    power_update_status();
}

/* ------------------------------------------------------------------ */
/*  Compute state: RFRI, AE check, alert logic                         */
/* ------------------------------------------------------------------ */
static void do_compute(void)
{
    /* Compute RFRI */
    g_sys.rfri_q8 = compute_rfri();

    /* Check if AE should be armed:
     * armed when leaf wetness > threshold AND T_wet ≤ +1 °C */
    int dew = leafwet_is_dew_present(g_sys.leaf_wet);
    if (dew && g_sys.twet_cx100 <= 100) {
        g_sys.flags |= SYS_FLAG_AE_ARMED;
        g_sys.ae_status = AE_STATUS_ARMED;
    } else {
        g_sys.flags &= ~SYS_FLAG_AE_ARMED;
        if (g_sys.ae_status != AE_STATUS_NUCLEATION) {
            g_sys.ae_status = AE_STATUS_IDLE;
        }
    }

    /* If AE is armed, check for nucleation */
    if (g_sys.flags & SYS_FLAG_AE_ARMED) {
        uint8_t ae_result = acoustic_check();
        g_sys.ae_status = ae_result;
        if (ae_result == AE_STATUS_NUCLEATION) {
            g_sys.ae_energy = (uint16_t)(acoustic_get_cumulative_energy() & 0xFFFF);
        }
    }

    /* Alert logic: alert if RFRI > 0.85 (Q8.8 = 218) or AE nucleation */
    if (g_sys.rfri_q8 >= 218 || g_sys.ae_status == AE_STATUS_NUCLEATION) {
        g_sys.flags |= SYS_FLAG_ALERT_ACTIVE;
        radio_set_alert_pending();
    } else if (g_sys.rfri_q8 < 180) {
        /* Clear alert when risk drops */
        g_sys.flags &= ~SYS_FLAG_ALERT_ACTIVE;
    }

    /* Update RTC seconds cache */
    g_rtc_seconds = rtc_get_seconds();
}

/* ------------------------------------------------------------------ */
/*  Transmit state: mesh TX + BLE notify + flash journal               */
/* ------------------------------------------------------------------ */
static void do_transmit(void)
{
    uint8_t record[FLASH_RECORD_BYTES];

    /* Write to flash journal */
    flashio_build_record(record, g_rtc_seconds);
    flashio_write_record(record);

    /* Transmit on mesh */
    if (radio_get_alert_pending()) {
        radio_tx_alert();
        radio_clear_alert_pending();
    } else {
        radio_tx(MESH_MSG_DATA);
    }

    /* BLE notify (if connected) */
    ble_send_live_data();

    g_sys.last_tx_ms = time_ms();
}

/* ------------------------------------------------------------------ */
/*  Main scheduler loop                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    /* Boot: initialize hardware */
    clock_init();
    gpio_init();
    i2c1_init();
    sys_tick_init();

    /* Initialize state block */
    memset(&g_sys, 0, sizeof(g_sys));
    g_sys.state = SYS_STATE_SAMPLE;
    g_sys.node_id = 0x01;
    g_sys.mesh_role = 0;  /* leaf */
    g_sys.sample_interval = 5;  /* 5 minutes */
    g_sys.flags = 0;

    /* Initialize drivers */
    rtc_init();
    g_rtc_seconds = rtc_get_seconds();

    power_gauge_init();
    bmp390_init();
    acoustic_init();
    flashio_init();

    /* Initialize radio with default key and node ID */
    radio_init(k_default_net_key, g_sys.node_id, 2);

    /* Initialize BLE */
    ble_init(ble_command_handler);

    /* First sample immediately */
    g_sys.last_sample_ms = 0;

    /* Main loop */
    while (1) {
        uint32_t now = time_ms();
        uint32_t interval_ms = (uint32_t)g_sys.sample_interval * 60 * 1000;

        /* Check for BLE commands (non-blocking) */
        ble_poll();

        /* Check if it's time to sample */
        uint32_t since_sample = now - g_sys.last_sample_ms;

        if (g_sys.state == SYS_STATE_FROST_WATCH) {
            /* In frost watch: sample every 1 minute, AE every 30 s */
            if (since_sample >= 60000) {
                g_sys.state = SYS_STATE_SAMPLE;
            } else if ((g_sys.flags & SYS_FLAG_AE_ARMED) &&
                       (now - g_sys.last_ae_ms >= 30000)) {
                g_sys.ae_status = acoustic_check();
                g_sys.last_ae_ms = now;
            }
        } else if (since_sample >= interval_ms) {
            g_sys.state = SYS_STATE_SAMPLE;
        }

        /* State machine */
        switch (g_sys.state) {

        case SYS_STATE_SLEEP:
            /* Enter low-power mode until next sample or alarm */
            {
                uint32_t wake_in = interval_ms / 1000;
                if (wake_in > 2) wake_in -= 1;  /* wake 1 s early */
                if (wake_in < 1) wake_in = 1;
                rtc_set_alarm(wake_in);
                power_enter_stop2(wake_in);
                /* On wake: check if alarm triggered */
                if (rtc_alarm_triggered()) {
                    g_sys.state = SYS_STATE_SAMPLE;
                }
            }
            break;

        case SYS_STATE_SAMPLE:
            do_sample();
            g_sys.last_sample_ms = time_ms();
            g_sys.state = SYS_STATE_COMPUTE;
            break;

        case SYS_STATE_COMPUTE:
            do_compute();
            g_sys.state = SYS_STATE_TRANSMIT;
            break;

        case SYS_STATE_TRANSMIT:
            do_transmit();
            g_sys.last_sample_ms = time_ms();
            /* Transition: if alert active, enter frost watch; else sleep */
            if (g_sys.flags & SYS_FLAG_ALERT_ACTIVE) {
                g_sys.state = SYS_STATE_FROST_WATCH;
            } else {
                g_sys.state = SYS_STATE_SLEEP;
            }
            break;

        case SYS_STATE_FROST_WATCH:
            /* In frost watch: sample every 1 minute instead of 5 min.
             * The AE check is done in the periodic section above. */
            if (since_sample >= 60000) {
                g_sys.state = SYS_STATE_SAMPLE;
            }
            /* Stay awake during frost watch (don't enter Stop2) */
            /* Poll BLE more frequently for app connections */
            ble_poll();
            delay_ms(100);
            break;

        default:
            g_sys.state = SYS_STATE_SLEEP;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  NVIC enable helper (called from sys_tick_init)                     */
/* ------------------------------------------------------------------ */
void __NVIC_EnableIRQ(uint32_t irqn)
{
    /* NVIC_ISER0 is at 0xE000E100; each bit enables one interrupt */
    volatile uint32_t *iser = (volatile uint32_t *)0xE000E100;
    if (irqn < 32) {
        iser[0] = (1u << irqn);
    } else if (irqn < 64) {
        iser[1] = (1u << (irqn - 32));
    }
}

/* ------------------------------------------------------------------ */
/*  Hard fault handler                                                 */
/* ------------------------------------------------------------------ */
void HardFault_Handler(void)
{
    while (1) {
        /* Blink an LED or toggle a pin if available, then hang.
         * In production, log fault info to flash and reboot. */
        delay_ms(200);
    }
}

/* ------------------------------------------------------------------ */
/*  NMI handler                                                        */
/* ------------------------------------------------------------------ */
void NMI_Handler(void)
{
    /* Non-maskable interrupt: typically clock security system. */
    while (1) ;
}