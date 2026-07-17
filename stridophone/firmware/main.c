/*
 * main.c — Stridophone application entry point and super-loop scheduler.
 *
 * Author : jayis1
 * License: MIT
 *
 * The firmware uses a cooperative super-loop (no RTOS) for determinism and
 * small footprint. Each subsystem owns a "tick" function invoked from the
 * main loop on a coarse timebase derived from SysTick. Time-critical audio
 * DMA half-transfer / complete ISRs push a flag that the DSP step consumes.
 *
 * Boot flow:
 *   1. board_clock_init()      — HSE -> PLL1 -> 480 MHz
 *   2. board_gpio_init()       — pin mux, LED defaults, mic/vibe rails off
 *   3. Drivers init (mic, adxl, sht, sd, esp, power)
 *   4. dsp init (Hann window, Mel filterbank, DCT matrix)
 *   5. classify init (load forest weights from flash array)
 *   6. app events/net init
 *   7. Enable audio DMA -> enter main loop
 */
#include "board.h"
#include "registers.h"
#include "drivers/i2s_mic.h"
#include "drivers/adxl355.h"
#include "drivers/sht45.h"
#include "drivers/sd.h"
#include "drivers/esp_coex.h"
#include "drivers/power.h"
#include "dsp/fft.h"
#include "dsp/beamform.h"
#include "dsp/mfcc.h"
#include "dsp/features.h"
#include "classify/forest.h"
#include "app/events.h"
#include "app/net.h"

#include <string.h>
#include <stdio.h>

/* ---- SysTick millisecond tick ------------------------------------ */
static volatile uint32_t g_tick_ms = 0;
void SysTick_Handler(void) { g_tick_ms++; }

static inline uint32_t tick_ms(void) { return g_tick_ms; }
static inline uint32_t elapsed_ms(uint32_t since) {
    return g_tick_ms - since;
}

/* ---- DMA flag set from SAI RX ISRs ------------------------------- */
static volatile uint8_t g_audio_half = 0;  /* half-transfer ready */
static volatile uint8_t g_audio_full = 0;  /* transfer-complete ready */

void bdma_ch0_irq(void) {
    uint32_t isr = BDMA->ISR;
    if (isr & BDMA_IFCR_CHTIF0) {
        g_audio_half = 1;
        BDMA->IFCR = BDMA_IFCR_CHTIF0;
    }
    if (isr & (1U<<1)) { /* TCIF0 */
        g_audio_full = 1;
        BDMA->IFCR = BDMA_IFCR_CGIF0;
    }
}

/* ---- Scratch DSP buffers ----------------------------------------- */
/* Audio frame: 4 channels * 1024 complex (we use RFFT of 1024 real). */
static float   g_audio_ch[4][DSP_FRAME_N];        /* deinterleaved PCM */
static float   g_spectrum[4][DSP_FRAME_N/2 + 1];  /* magnitude |FFT| */
static float   g_mfcc[39];                         /* 13 MFCC + Δ + Δ² */
static float   g_features[CLF_N_FEATURES];
static float   g_vibe_axis[3][DSP_FRAME_N];        /* 3-axis accel */
static float   g_vibe_mag_spec[DSP_FRAME_N/2 + 1];

/* Mel filterbank + DCT matrices, initialised once in dsp init. */
static float   g_mel_fb[20][DSP_FRAME_N/2 + 1];
static float   g_dct[13][20];

/* Beamformer DOA histogram (16 azimuth bins). */
static uint32_t g_doa_hist[16];

/* Environmental snapshot */
static float   g_temperature = 21.0f;
static float   g_humidity    = 45.0f;

/* Classifier output + event buffer */
static clf_result_t g_clf;
static event_t      g_event;
static char         g_log_line[256];

/* ---- Forward decls of init helpers ------------------------------- */
static void board_clock_init(void);
static void board_gpio_init(void);
static void led_heartbeat(uint32_t now);

/* ================================================================== */
int main(void) {
    /* 1. Clocks */
    board_clock_init();

    /* 2. SysTick 1 ms @ 480 MHz */
    SysTick->LOAD = BOARD_CPU_HZ / 1000 - 1;
    SysTick->VAL  = 0;
    SysTick->CSR  = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSRC;

    /* 3. GPIO */
    board_gpio_init();

    /* Status LED on during boot */
    gpio_set(LED_STATUS_PORT, LED_STATUS_PAD);

    /* 4. Driver init */
    power_init();
    i2s_mic_init();
    adxl355_init();
    sht45_init();
    sd_init();
    esp_coex_init();

    /* 5. DSP init */
    fft_init();
    beamform_init();
    mfcc_init(g_mel_fb, g_dct);
    features_init();

    /* 6. Classifier + app */
    clf_init();
    events_init();
    net_init();

    /* 7. Enable mic rail + start audio DMA */
    gpio_set(MIC_EN_PORT, MIC_EN_PAD);
    gpio_set(VIBE_EN_PORT, VIBE_EN_PAD);
    i2s_mic_start();

    gpio_clr(LED_STATUS_PORT, LED_STATUS_PAD);  /* boot complete */

    /* ---- Super-loop scheduling ---- */
    uint32_t last_dsp     = 0;
    uint32_t last_clf     = 0;
    uint32_t last_env     = 0;
    uint32_t last_sd      = 0;
    uint32_t last_heart   = 0;
    uint32_t last_net     = 0;

    while (1) {
        uint32_t now = tick_ms();

        /* Audio DSP frame: triggered by DMA half/full. Each event delivers
         * 512 samples/channel; we accumulate two halves into a 1024-pt
         * frame before running the FFT pipeline. */
        if (g_audio_half || g_audio_full) {
            g_audio_half = 0;
            g_audio_full = 0;

            /* Pull the latest 1024 samples per channel from the ring. */
            i2s_mic_deinterleave(g_audio_ch, DSP_FRAME_N);

            /* Per-channel 1024-pt RFFT + magnitude. */
            for (int ch = 0; ch < 4; ch++) {
                fft_rfft_hann(g_audio_ch[ch], g_spectrum[ch], DSP_FRAME_N);
            }

            /* Delay-and-sum beamformer: update DOA histogram using the
             * 4-channel cross-spectra. */
            int doa_bin = beamform_update(g_doa_hist, 16, g_spectrum);
            (void)doa_bin;

            /* Read ADXL355 FIFO (decimated to DSP_FRAME_N samples). */
            adxl355_read_block(g_vibe_axis, DSP_FRAME_N);

            /* Vibration magnitude spectrum (combine axes as RMS). */
            features_vibe_spectrum(g_vibe_axis, g_vibe_mag_spec, DSP_FRAME_N);

            /* MFCC on the dominant (loudest) mic channel. */
            int dominant = beamform_dominant_channel();
            mfcc_compute(g_spectrum[dominant], g_mel_fb, g_dct, g_mfcc);
            last_dsp = now;
        }

        /* Classifier at 1 Hz. */
        if (elapsed_ms(last_clf) >= CLASSIFY_INTERVAL_MS) {
            last_clf = now;
            features_pack(g_features, g_mfcc, g_vibe_mag_spec,
                          g_vibe_axis, DSP_FRAME_N,
                          g_temperature, g_humidity);
            int ok = clf_classify(g_features, &g_clf);
            if (ok && g_clf.confidence >= CLF_CONF_THRESHOLD) {
                /* Build an event record. */
                event_build(&g_event, &g_clf, g_doa_hist, 16,
                            g_temperature, g_humidity);
                events_push(&g_event);
                /* Light the alert LED briefly. */
                gpio_set(LED_ALERT_PORT, LED_ALERT_PAD);
            } else if (ok) {
                gpio_clr(LED_ALERT_PORT, LED_ALERT_PAD);
            }
        }

        /* Environmental sample every 5 s. */
        if (elapsed_ms(last_env) >= ENV_SAMPLE_MS) {
            last_env = now;
            sht45_read(&g_temperature, &g_humidity);
        }

        /* SD flush every 30 s. */
        if (elapsed_ms(last_sd) >= SD_FLUSH_MS) {
            last_sd = now;
            events_flush_sd();
        }

        /* Net push every 500 ms if Wi-Fi is up and events pending. */
        if (elapsed_ms(last_net) >= 500 && net_wifi_up()) {
            last_net = now;
            net_push_pending(EVENT_QUEUE_DEPTH / 4);
        }

        /* Heartbeat LED. */
        led_heartbeat(now);

        /* Cooperative yield: wait for next interrupt (WFI). */
        __asm__ volatile ("wfi");
    }
}

/* ================================================================== */
static void board_clock_init(void) {
    /* Enable PWR clock + set VOS scale 1 for 480 MHz. */
    RCC->APB4ENR |= (1U<<3);          /* PWR */
    PWR->CR1 = (PWR->CR1 & ~(3U<<15)) | PWR_CR1_VOS_SCALE1;

    /* Enable HSE and wait for it. */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* PLL1: input div M=2 (16/2=8 MHz), N=120, P=2 => 480 MHz. */
    RCC->PLLCKSELR = (2U<<0) | (0U<<8) | (0U<<16);
    RCC->PLL1DIVR  = (120U<<0) | (2U<<9) | (0U<<16) | (0U<<24) | (0U<<28);
    RCC->PLL1FRACR = 0;
    RCC->PLLCFGR   = (1U<<0) | (1U<<16) | (2U<<24); /* PLL1P enabled, frac off, range 2 */

    /* D1/D2/D3 dividers: SYS=480, HCLK=240 (HPRE=2), APB1=120 (PPRE1=2),
     * APB2=240 (PPRE2=1), APB4=120 (PPRE4=2). */
    RCC->D1CFGR = (8U<<0) | (2U<<8) | (2U<<11);  /* D1CPRE=16? no — set below */
    /* The D1CPRE field: 0=SYS, 8..15=/2../9. Use /2 => HCLK=240. */
    RCC->D1CFGR = (0U<<0) | (2U<<8) | (2U<<11);  /* D1CPRE=/1, HPRE=/2, D1PPRE=/2 */
    RCC->D2CFGR = (4U<<0) | (4U<<3);             /* D2PPRE1=/4(120), D2PPRE2=/4 */
    RCC->D3CFGR = (4U<<0);                        /* D3PPRE=/4 (120 MHz) */

    /* Flash latency: VOS1 @ 480 MHz needs 4 wait + WRHIGHFREQ=1. */
    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) | (4U<<0) | (1U<<8);

    /* Enable PLL1 and wait. */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY)) { }

    /* Switch SYSCLK to PLL1. */
    RCC->CFGR = (RCC->CFGR & ~7U) | RCC_CFGR_SW_PLL1;
    while (((RCC->CFGR >> 3) & 7) != RCC_CFGR_SWS_PLL1>>3) { }
}

static void board_gpio_init(void) {
    /* Enable all GPIO banks. */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOA | RCC_AHB4ENR_GPIOB |
                    RCC_AHB4ENR_GPIOC | RCC_AHB4ENR_GPIOD |
                    RCC_AHB4ENR_GPIOE;

    /* LEDs as push-pull outputs. */
    gpio_set_mode(LED_STATUS_PORT,   LED_STATUS_PAD,   GPIO_MODE_OUT);
    gpio_set_mode(LED_ACTIVITY_PORT, LED_ACTIVITY_PAD, GPIO_MODE_OUT);
    gpio_set_mode(LED_ALERT_PORT,    LED_ALERT_PAD,    GPIO_MODE_OUT);
    gpio_set_mode(LED_CHARGE_PORT,   LED_CHARGE_PAD,   GPIO_MODE_OUT);

    /* Mic + ADXL enable pins (outputs, start low = off). */
    gpio_set_mode(MIC_EN_PORT,  MIC_EN_PAD,  GPIO_MODE_OUT);
    gpio_set_mode(VIBE_EN_PORT, VIBE_EN_PAD, GPIO_MODE_OUT);
    gpio_clr(MIC_EN_PORT,  MIC_EN_PAD);
    gpio_clr(VIBE_EN_PORT, VIBE_EN_PAD);

    /* Button as input with pull-up. */
    gpio_set_mode(BTN_PORT, BTN_PAD, GPIO_MODE_IN);
    gpio_set_pupd(BTN_PORT, BTN_PAD, GPIO_PUPD_PU);

    /* SAI1 pins: AF6 for SAI1 on most H7 pads. */
    gpio_set_af(SAI1_SD1_PORT, SAI1_SD1_PAD, 6);  gpio_set_mode(SAI1_SD1_PORT, SAI1_SD1_PAD, GPIO_MODE_AF);
    gpio_set_af(SAI1_SD2_PORT, SAI1_SD2_PAD, 6);  gpio_set_mode(SAI1_SD2_PORT, SAI1_SD2_PAD, GPIO_MODE_AF);
    gpio_set_af(SAI1_FS_PORT,  SAI1_FS_PAD,  6);  gpio_set_mode(SAI1_FS_PORT,  SAI1_FS_PAD,  GPIO_MODE_AF);
    gpio_set_af(SAI1_SCK_PORT, SAI1_SCK_PAD, 6);  gpio_set_mode(SAI1_SCK_PORT, SAI1_SCK_PAD, GPIO_MODE_AF);
    gpio_set_af(SAI1_MCLK_PORT,SAI1_MCLK_PAD,6);  gpio_set_mode(SAI1_MCLK_PORT,SAI1_MCLK_PAD,GPIO_MODE_AF);
    gpio_set_speed(SAI1_SCK_PORT, SAI1_SCK_PAD, GPIO_OSPEED_HI);

    /* SPI2 pins for ADXL355: AF5 on PD3/4/5, plus CS as GPIO output. */
    gpio_set_af(ADXL355_SCK_PORT,  ADXL355_SCK_PAD,  5);
    gpio_set_af(ADXL355_MISO_PORT, ADXL355_MISO_PAD, 5);
    gpio_set_af(ADXL355_MOSI_PORT, ADXL355_MOSI_PAD, 5);
    gpio_set_mode(ADXL355_SCK_PORT,  ADXL355_SCK_PAD,  GPIO_MODE_AF);
    gpio_set_mode(ADXL355_MISO_PORT, ADXL355_MISO_PAD, GPIO_MODE_AF);
    gpio_set_mode(ADXL355_MOSI_PORT, ADXL355_MOSI_PAD, GPIO_MODE_AF);
    gpio_set_mode(ADXL355_CS_PORT,   ADXL355_CS_PAD,   GPIO_MODE_OUT);
    gpio_set(ADXL355_CS_PORT, ADXL355_CS_PAD);       /* CS high = idle */
    gpio_set_af(ADXL355_DRDY_PORT, ADXL355_DRDY_PAD, 0);
    gpio_set_mode(ADXL355_DRDY_PORT, ADXL355_DRDY_PAD, GPIO_MODE_IN);
    gpio_set_pupd(ADXL355_DRDY_PORT, ADXL355_DRDY_PAD, GPIO_PUPD_PU);

    /* I2C4 pins: AF4 on PD12/PD13. */
    gpio_set_af(I2C4_SCL_PORT, I2C4_SCL_PAD, 4);
    gpio_set_af(I2C4_SDA_PORT, I2C4_SDA_PAD, 4);
    gpio_set_mode(I2C4_SCL_PORT, I2C4_SCL_PAD, GPIO_MODE_AF);
    gpio_set_mode(I2C4_SDA_PORT, I2C4_SDA_PAD, GPIO_MODE_AF);
    gpio_set_speed(I2C4_SCL_PORT, I2C4_SCL_PAD, GPIO_OSPEED_HI);

    /* SDMMC1 pins: AF12 on PC8-12, PD2. */
    gpio_set_af(SDMMC1_D0_PORT,  SDMMC1_D0_PAD,  12);
    gpio_set_af(SDMMC1_D1_PORT,  SDMMC1_D1_PAD,  12);
    gpio_set_af(SDMMC1_D2_PORT,  SDMMC1_D2_PAD,  12);
    gpio_set_af(SDMMC1_D3_PORT,  SDMMC1_D3_PAD,  12);
    gpio_set_af(SDMMC1_CK_PORT,  SDMMC1_CK_PAD,  12);
    gpio_set_af(SDMMC1_CMD_PORT, SDMMC1_CMD_PAD, 12);
    gpio_set_mode(SDMMC1_D0_PORT,  SDMMC1_D0_PAD,  GPIO_MODE_AF);
    gpio_set_mode(SDMMC1_D1_PORT,  SDMMC1_D1_PAD,  GPIO_MODE_AF);
    gpio_set_mode(SDMMC1_D2_PORT,  SDMMC1_D2_PAD,  GPIO_MODE_AF);
    gpio_set_mode(SDMMC1_D3_PORT,  SDMMC1_D3_PAD,  GPIO_MODE_AF);
    gpio_set_mode(SDMMC1_CK_PORT,  SDMMC1_CK_PAD,  GPIO_MODE_AF);
    gpio_set_mode(SDMMC1_CMD_PORT, SDMMC1_CMD_PAD, GPIO_MODE_AF);
    gpio_set_speed(SDMMC1_CK_PORT, SDMMC1_CK_PAD, GPIO_OSPEED_HI);
    /* Card detect as input pull-up (active low). */
    gpio_set_mode(SDMMC_DET_PORT, SDMMC_DET_PAD, GPIO_MODE_IN);
    gpio_set_pupd(SDMMC_DET_PORT, SDMMC_DET_PAD, GPIO_PUPD_PU);

    /* USART3 to ESP32-C6: AF7 on PB10/PB11. */
    gpio_set_af(ESP_TX_PORT, ESP_TX_PAD, 7);
    gpio_set_af(ESP_RX_PORT, ESP_RX_PAD, 7);
    gpio_set_mode(ESP_TX_PORT, ESP_TX_PAD, GPIO_MODE_AF);
    gpio_set_mode(ESP_RX_PORT, ESP_RX_PAD, GPIO_MODE_AF);
    gpio_set_speed(ESP_TX_PORT, ESP_TX_PAD, GPIO_OSPEED_HI);
    /* ESP control pins as GPIO outputs. */
    gpio_set_mode(ESP_BOOT_PORT, ESP_BOOT_PAD, GPIO_MODE_OUT);
    gpio_set_mode(ESP_EN_PORT,   ESP_EN_PAD,   GPIO_MODE_OUT);
    gpio_set(ESP_BOOT_PORT, ESP_BOOT_PAD);     /* BOOT0 high = run */
    gpio_clr(ESP_EN_PORT,   ESP_EN_PAD);       /* start ESP off, enable later */
    gpio_set_mode(ESP_HANDSHAKE_PORT, ESP_HANDSHAKE_PAD, GPIO_MODE_IN);
    gpio_set_pupd(ESP_HANDSHAKE_PORT, ESP_HANDSHAKE_PAD, GPIO_PUPD_PU);

    /* USB-C VBUS detect input. */
    gpio_set_mode(PMIC_VBUS_PORT, PMIC_VBUS_PAD, GPIO_MODE_IN);
}

/* ================================================================== */
static void led_heartbeat(uint32_t now) {
    /* Blink the status LED at 1 Hz to show we're alive; pulse activity LED
     * on each DSP frame. */
    static uint32_t last_blink = 0;
    static uint8_t  blink_state = 0;
    if (elapsed_ms(last_blink) >= HEARTBEAT_MS) {
        last_blink = now;
        blink_state ^= 1;
        if (blink_state) gpio_set(LED_STATUS_PORT, LED_STATUS_PAD);
        else             gpio_clr(LED_STATUS_PORT, LED_STATUS_PAD);
    }
    /* Charge LED mirrors BQ25895 STAT register bit. */
    uint8_t charging = power_is_charging();
    if (charging) gpio_set(LED_CHARGE_PORT, LED_CHARGE_PAD);
    else          gpio_clr(LED_CHARGE_PORT, LED_CHARGE_PAD);
}

/* ---- Weak stubs so the link succeeds if a driver is omitted ------- */
__attribute__((weak)) void board_init(void) { /* merged into clock/gpio */ }