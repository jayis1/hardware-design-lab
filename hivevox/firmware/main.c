/*
 * main.c — HiveVox firmware main loop and state machine
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Bare-metal super-loop for the HiveVox acoustic beehive health monitor.
 * The RTC wakes the STM32L432 every DEFAULT_SAMPLE_INTERVAL_MIN minutes;
 * on each wake we: capture audio, compute FFT features, classify colony
 * state, read environmental sensors, read weight, build and TX a 12-byte
 * LoRaWAN uplink, then go back to STOP2 deep sleep.
 */
#include "board.h"
#include "registers.h"
#include "drivers/i2c.h"
#include "drivers/onewire.h"
#include "drivers/hx711.h"
#include "drivers/sx1276.h"
#include "drivers/acoustic.h"
#include <string.h>

/* ---- Global state -------------------------------------------------- */
static volatile uint32_t g_dwt_cycles_per_us = 80;

static uint8_t  g_sample_interval_min = DEFAULT_SAMPLE_INTERVAL_MIN;
static uint8_t  g_lora_sf = LORA_SF_DEFAULT;
static uint16_t g_boot_count = 0;
static uint8_t  g_baseline_frames_remaining = 144;  /* 24h / 10min = 144 cycles */

/* DS18B20 probe ROM IDs (discovered at boot via SEARCH ROM, simplified:
 * we assume up to 3 probes and read them by broadcast convert for simplicity
 * in this firmware; a production build caches ROM IDs.) */
static uint8_t  g_probe_count = 3;
static uint8_t  g_probe_fault = 0;

/* LoRaWAN device identity (provisioned at factory, stored in flash).
 * In a real deployment these come from the LoRaWAN stack's NVM. Here we
 * use placeholders — the SX1276 driver does raw LoRa, not the full MAC.
 * The app expects a 12-byte payload; we fill it with sensor data. */
static uint8_t g_deveui[8] = {0xAC,0xDE,0x48,0x00,0x00,0x00,0x00,0x01};

/* ---- Forward declarations ------------------------------------------ */
static void clock_init(void);
static void rtc_init(void);
static void rtc_set_wakeup(uint8_t minutes);
static void enter_stop2(void);
static void gpio_init_all(void);
static uint16_t read_battery_mv(void);
static void build_and_send_uplink(const acoustic_features_t *feat,
                                  int16_t brood_temp_centi,
                                  uint16_t rh_centi,
                                  int32_t weight_g,
                                  uint16_t bat_mv,
                                  colony_state_t state,
                                  uint8_t flags);
static colony_state_t colony_classify(const acoustic_features_t *feat,
                                      int16_t brood_temp_centi);

/* ---- Reset handler / entry point ----------------------------------- */
/* This is the bare-metal entry. In the full build, startup.s sets the
 * vector table and calls Reset_Handler → main(). Here we expose main()
 * and a minimal Reset_Handler stub the linker script references. */

void Reset_Handler(void)
{
    extern int main(void);
    (void)main();
    while (1) { }
}

int main(void)
{
    /* One-time boot init */
    clock_init();
    gpio_init_all();
    i2c3_init();
    hx711_init();
    sx1276_init();
    acoustic_init();
    veml7700_init();

    /* RTC: configure for periodic wake alarm */
    rtc_init();

    g_boot_count++;

    /* Main super-loop: each iteration is one sample cycle. */
    while (1) {
        /* ---- 1. Acoustic capture ---- */
        mic_enable();
        acoustic_features_t feat;
        memset(&feat, 0, sizeof(feat));
        acoustic_capture_features(&feat, MIC_FRAMES_PER_CYCLE);
        mic_disable();

        /* ---- 2. Environmental sensors ---- */
        int16_t brood_temp_centi = -27000;  /* sentinel */
        int16_t t0 = 0, t1 = 0, t2 = 0;
        g_probe_fault = 0;

        if (ds18b20_start_all() == 0) {
            if (ds18b20_wait_done(800) == 0) {
                if (ds18b20_read_scratch(&t0) == 0) brood_temp_centi = t0;
                else g_probe_fault = 1;
                /* For multi-probe we would iterate ROM IDs; here we use
                 * the broadcast read as the mid-brood probe. */
            } else {
                g_probe_fault = 1;
            }
        } else {
            g_probe_fault = 1;
        }
        (void)t1; (void)t2;

        int16_t entry_temp_centi = 0;
        uint16_t rh_centi = 0;
        if (sht45_read(&entry_temp_centi, &rh_centi) < 0) {
            rh_centi = 0;  /* mark as invalid */
        }

        uint16_t lux = 0;
        veml7700_read_lux(&lux);
        (void)lux;

        /* ---- 3. Weight ---- */
        int32_t weight_g = hx711_read_grams();
        if (weight_g == HX711_READ_TIMEOUT) {
            weight_g = 0;
            g_probe_fault = 1;
        }

        /* ---- 4. Battery ---- */
        uint16_t bat_mv = read_battery_mv();

        /* ---- 5. Classify ---- */
        colony_state_t state;
        if (g_baseline_frames_remaining > 0) {
            state = STATE_BASELINE;
            g_baseline_frames_remaining--;
        } else {
            state = colony_classify(&feat, brood_temp_centi);
        }

        /* ---- 6. Build flags + uplink + TX ---- */
        uint8_t flags = 0;
        if (state == STATE_QUEENLESS || state == STATE_SWARM_PREP ||
            state == STATE_DEAD) {
            flags |= ALERT_FLAG_ALERT;
        }
        if (bat_mv > SOLAR_GOOD_THRESHOLD_MV) flags |= ALERT_FLAG_SOLAR_GOOD;
        if (g_probe_fault) flags |= ALERT_FLAG_PROBE_FAULT;

        build_and_send_uplink(&feat, brood_temp_centi, rh_centi,
                              weight_g, bat_mv, state, flags);

        /* ---- 7. Optional downlink (short RX window after TX) ---- */
        uint8_t dl[DOWNLINK_PAYLOAD_LEN];
        int dl_len = sx1276_rx(dl, DOWNLINK_PAYLOAD_LEN, 2000);
        if (dl_len == DOWNLINK_PAYLOAD_LEN) {
            if (dl[0] >= MIN_SAMPLE_INTERVAL_MIN &&
                dl[0] <= MAX_SAMPLE_INTERVAL_MIN) {
                g_sample_interval_min = dl[0];
            }
            if (dl[1] >= 7 && dl[1] <= 12) {
                g_lora_sf = dl[1];
                sx1276_set_sf(g_lora_sf);
            }
            /* dl[2..3] reserved */
        }

        /* ---- 8. Sleep ---- */
        sx1276_sleep();
        hx711_power_down();
        rtc_set_wakeup(g_sample_interval_min);
        enter_stop2();
        /* Wakes here after RTC alarm. Loop back to top. */
    }
}

/* ---- Clock initialization (PLL to 80 MHz from HSI16) --------------- */
static void clock_init(void)
{
    /* Enable HSI16 (16 MHz internal RC) */
    RCC_CR |= (1U << 8);  /* HSION */
    while (!(RCC_CR & (1U << 10))) { }  /* HSIRDY */

    /* Set flash latency to 4 wait states for 80 MHz */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | 4 | FLASH_ACR_PRFTEN;

    /* PLL config: PLLSRC=HSI16, M=1, N=20, R=2 → 16/1*20/2 = 160 MHz (SYS 80) */
    RCC_PLLCFGR = (2U << 0)     /* PLLSRC = HSI16 */
                | (1U << 4)     /* PLLM = 1 */
                | (20U << 8)    /* PLLN = 20 */
                | (0U << 25);   /* PLLR = 2 */
    RCC_CR |= (1U << 24);  /* PLLON */
    while (!(RCC_CR & (1U << 25))) { }  /* PLLRDY */

    /* Select PLL as system clock */
    RCC_CFGR = (RCC_CFGR & ~0x3U) | 3U;  /* SW = PLL */
    while (((RCC_CFGR >> 2) & 0x3) != 3) { }  /* SWS = PLL */

    /* Enable LSE for RTC (32.768 kHz crystal) */
    RCC_BDCR |= 1U;  /* LSEON — access via BDCR (backup domain) */
    while (!(RCC_BDCR & 2U)) { }  /* LSERDY */

    /* Select LSE as RTC clock source and enable RTC */
    RCC_BDCR = (RCC_BDCR & ~(3U << 16)) | (1U << 16);  /* RTCSEL = LSE */
    RCC_BDCR |= (1U << 15);  /* RTCEN */
}

/* ---- RTC initialization ------------------------------------------- */
static void rtc_init(void)
{
    /* Unlock write protection */
    RTC_WPR = 0xCA;
    RTC_WPR = 0x53;

    /* Enter init mode */
    RTC_ISR |= (1U << 6);  /* INIT */
    while (!(RTC_ISR & (1U << 6))) { }  /* INITF */

    /* Prescaler: ck_spre = 32768 / (PREDIV_A+1) / (PREDIV_S+1) = 1 Hz
     * PREDIV_A = 127 (7-bit), PREDIV_S = 255 (15-bit) */
    RTC_PRER = (127U << 16) | 255U;

    /* Exit init mode */
    RTC_ISR &= ~(1U << 6);

    /* Enable alarm A interrupt */
    RTC_CR |= (1U << 12);  /* ALRAIE */

    /* Configure EXTI line 18 (RTC alarm) to wake from STOP */
    EXTI_IMR1 |= (1U << 18);

    /* Enable RTC alarm NVIC */
    NVIC_ISER1 |= (1U << (NVIC_RTC_ALARM_VEC - 32));

    /* Lock writes */
    RTC_WPR = 0x00;
}

/* Set alarm to fire `minutes` from now. Uses alarm sub-second support to
 * generate a precise interval. For simplicity we set a relative alarm
 * by writing the alarm register to current seconds + minutes*60.
 */
static void rtc_set_wakeup(uint8_t minutes)
{
    if (minutes == 0) minutes = 1;

    /* Unlock */
    RTC_WPR = 0xCA;
    RTC_WPR = 0x53;

    /* Disable alarm to configure */
    RTC_CR &= ~(1U << 8);  /* ALRAE */
    while (!(RTC_ISR & (1U << 8))) { }  /* ALRAWF */

    /* Read current time (BCD). RTC_TR has SS:MM:HH in BCD, we only need SS. */
    uint32_t tr = RTC_TR;
    uint32_t cur_ss = (tr & 0x7F);        /* seconds BCD (low byte) */
    uint32_t add_ss = (uint32_t)minutes * 60;

    /* Set alarm mask to match only seconds-unit (mask all higher fields)
     * so alarm fires when seconds reach target. We use a coarse approach:
     * set alarm for "in N minutes" by masking everything but the seconds
     * count and relying on the 1 Hz clock. For a robust implementation we
     * use the wakeup timer instead. */

    /* Use the wakeup timer (simpler and designed for this) */
    RTC_CR &= ~(1U << 10);  /* WUTE */
    while (!(RTC_ISR & (1U << 2))) { }  /* WUTWF */

    /* WUTR is in units of ck_spre (1 Hz) when WUCKSEL=110 (bits 2:0 of CR). */
    RTC_WUTR = (uint32_t)minutes * 60 - 1;
    RTC_CR |= (1U << 14);   /* WUTIE */
    RTC_CR |= (1U << 10);   /* WUTE */

    /* Lock */
    RTC_WPR = 0x00;

    (void)cur_ss; (void)add_ss;
}

/* ---- STOP2 deep-sleep entry ---------------------------------------- */
static void enter_stop2(void)
{
    /* Set LPMS = STOP2 in PWR_CR1 */
    PWR_CR1 = (PWR_CR1 & ~0x7U) | PWR_CR1_LPMS_STOP2;

    /* Set SLEEPDEEP */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    /* WFI enters STOP2 */
    __asm volatile ("wfi");

    /* On wake: clear SLEEPDEEP, re-init clocks (STOP2 kills PLL) */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    clock_init();

    /* Clear wake flag */
    RTC_SCR = 0;  /* clear RTC alarm/wakeup flags */
}

/* ---- GPIO pin initialization --------------------------------------- */
static void gpio_init_all(void)
{
    /* Enable GPIOA, GPIOB, GPIOC clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

    /* ADC pin PA0 as analog */
    GPIOA->MODER |= (GPIO_MODE_ANALOG << (0*2));

    /* HX711 pins (PA1 out, PA4 in) handled in hx711_init() */
    /* I2C pins (PA8/PA9) handled in i2c3_init() */
    /* I2S pins (PA10/11/12, PB3) handled in acoustic_init() */
    /* SPI2 pins (PB4/5/13/14/15) handled in sx1276_init() */
    /* 1-Wire PB0 handled in ow_reset() on first use */

    /* Lora DIO0/DIO1 as inputs (PB6, PB7) */
    GPIOB->MODER &= ~((3U << (6*2)) | (3U << (7*2)));
    GPIOB->PUPDR |= (1U << (6*2)) | (1U << (7*2));  /* pull-up */
}

/* ---- Battery voltage read (ADC on PA0) ----------------------------- */
static uint16_t read_battery_mv(void)
{
    /* Enable ADC clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_ADCEN;

    /* Enable ADC */
    ADC_CR |= ADC_CR_ADEN;
    while (!(ADC_ISR & ADC_ISR_ADRDY)) { }

    /* Channel 5 (PA0), sample time 92.5 cycles */
    ADC_SMPR1 = (5U << 0);  /* SMP0 = 010 = 12.5 ADC cycles */

    /* Single conversion, software trigger, 12-bit */
    ADC_CFGR = 0;

    /* Start conversion */
    ADC_CR |= ADC_CR_ADSTART;
    while (!(ADC_ISR & ADC_ISR_EOC)) { }

    uint16_t raw = (uint16_t)ADC_DR;

    /* Convert: Vref ~3.0V, divider ratio 2 → Vbat = raw/4095*3.0*2 */
    uint32_t mv = (uint32_t)raw * ADC_REF_MV * 2 / 4095;
    return (uint16_t)mv;
}

/* ---- Colony state classifier (decision tree) ---------------------- */
static colony_state_t colony_classify(const acoustic_features_t *feat,
                                      int16_t brood_temp_centi)
{
    /* Dead/empty: extremely low energy for sustained period.
     * We approximate: if total energy below threshold and we're not in
     * winter, it's dead. In winter, low energy is expected. */
    int16_t temp_c = brood_temp_centi / 100;

    if (feat->total_energy < 500) {
        if (temp_c < 10) {
            /* Could be normal winter cluster; need dominant freq to confirm */
            if (feat->dominant_hz >= 60 && feat->dominant_hz <= 90) {
                return STATE_WINTER;
            }
        }
        return STATE_DEAD;
    }

    /* Winter cluster: low freq, low energy, cold */
    if (temp_c < 10 && feat->dominant_hz >= 50 && feat->dominant_hz <= 95) {
        return STATE_WINTER;
    }

    /* Queenless: high dominant frequency, lots of high-band energy */
    if (feat->dominant_hz > 380 && feat->r_hi > 89) {  /* r_hi > 0.35 */
        return STATE_QUEENLESS;
    }

    /* Swarming prep: pulsing 110 Hz tone */
    if (feat->dominant_hz >= 95 && feat->dominant_hz <= 125 &&
        feat->cv_loud > 63) {  /* cv > 0.25 */
        return STATE_SWARM_PREP;
    }

    /* Healthy queenright: 240–320 Hz, low high-band ratio, stable loudness */
    if (feat->dominant_hz >= 240 && feat->dominant_hz <= 320 &&
        feat->r_hi < 51 && feat->cv_loud < 38) {  /* r_hi<0.20, cv<0.15 */
        return STATE_QUEENRIGHT;
    }

    return STATE_UNKNOWN;
}

/* ---- Uplink packet builder + LoRa TX ------------------------------- */
static void build_and_send_uplink(const acoustic_features_t *feat,
                                  int16_t brood_temp_centi,
                                  uint16_t rh_centi,
                                  int32_t weight_g,
                                  uint16_t bat_mv,
                                  colony_state_t state,
                                  uint8_t flags)
{
    uint8_t pkt[UPLINK_PAYLOAD_LEN];
    memset(pkt, 0, sizeof(pkt));

    /* Clip values to packet ranges */
    if (feat->dominant_hz > 65534) feat->dominant_hz = 65534;
    int16_t temp_pkt = brood_temp_centi;
    if (temp_pkt < -32767) temp_pkt = -32767;
    if (rh_centi > 10000) rh_centi = 10000;
    int32_t w = weight_g / 10;  /* decigrams for range */
    if (w < 0) w = 0;
    if (w > 65534) w = 65534;
    uint16_t bat_v = bat_mv / 20;
    if (bat_v > 255) bat_v = 255;

    /* Pack little-endian */
    pkt[0]  = (uint8_t)state;
    pkt[1]  = (uint8_t)(feat->dominant_hz & 0xFF);
    pkt[2]  = (uint8_t)((feat->dominant_hz >> 8) & 0xFF);
    pkt[3]  = feat->r_hi;
    pkt[4]  = feat->cv_loud;
    pkt[5]  = (uint8_t)(temp_pkt & 0xFF);
    pkt[6]  = (uint8_t)((temp_pkt >> 8) & 0xFF);
    pkt[7]  = (uint8_t)(rh_centi / 100);  /* %RH integer */
    pkt[8]  = (uint8_t)(w & 0xFF);
    pkt[9]  = (uint8_t)((w >> 8) & 0xFF);
    pkt[10] = (uint8_t)bat_v;
    pkt[11] = flags;

    /* Transmit (the full LoRaWAN stack would handle MIC, DevNonce, etc.
     * Here we send raw LoRa bytes; the gateway/app must match our format.) */
    sx1276_tx(pkt, UPLINK_PAYLOAD_LEN);
}

/* ---- RTC alarm ISR (clears flag so wake succeeds) ------------------ */
void RTC_Alarm_IRQHandler(void)
{
    /* Clear alarm and wakeup timer flags */
    RTC_SCR = 0x3F;  /* clear all RTC flags */
    EXTI_RPR1 |= (1U << 18);  /* clear rising edge pending */
    EXTI_FPR1 |= (1U << 18);  /* clear falling edge pending */
}

/* ---- Simple delay (SysTick-based) ---------------------------------- */
void delay_ms(uint32_t ms)
{
    /* Use SysTick in polling mode for delays. */
    SYSTICK_LOAD = 80000 - 1;  /* 1 ms at 80 MHz */
    SYSTICK_VAL = 0;
    SYSTICK_CSR = SYSTICK_CSR_CLKSOURCE | SYSTICK_CSR_ENABLE;
    for (uint32_t i = 0; i < ms; i++) {
        while (!(SYSTICK_CSR & (1U << 16))) { }  /* COUNTFLAG */
    }
    SYSTICK_CSR = 0;
}