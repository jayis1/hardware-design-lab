/*
 * main.c — SpectraPest main firmware entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * This is the main application for the SpectraPest multispectral + acoustic
 * pest identification device. It implements a cooperative super-loop
 * scheduler that:
 *
 *   1. Wakes every 60 seconds (configurable) from STOP mode
 *   2. Reads environmental sensors to check if conditions are suitable
 *   3. Captures 3 seconds of acoustic data and extracts wingbeat features
 *   4. Captures 6-band multispectral images and extracts spectral features
 *   5. Runs the edge AI model to classify the pest species
 *   6. Logs the detection to FRAM and SPI flash
 *   7. Broadcasts a pest alert via LoRa mesh
 *   8. Listens for mesh messages from other nodes
 *   9. Returns to STOP mode to conserve power
 *
 * The scheduler is tick-based (1 ms resolution) using TIM2 as the system
 * tick source. All blocking operations use timeouts to prevent deadlocks.
 *
 * Build: make
 * Flash: make flash (uses OpenOCD + ST-Link)
 */

#include "board.h"
#include "registers.h"
#include "drivers/imx519.h"
#include "drivers/acoustic.h"
#include "drivers/spectral.h"
#include "drivers/edgeai.h"
#include "drivers/environment.h"
#include "drivers/loramesh.h"
#include "drivers/gnss.h"
#include "drivers/storage.h"
#include "drivers/power.h"
#include <string.h>

/* ----------------------------------------------------------------- *
 *  Global state
 * ----------------------------------------------------------------- */
static volatile uint32_t g_system_tick = 0;
static uint8_t            g_node_id = 1;  /* Set during commissioning */
static uint32_t           g_detection_interval = DETECTION_INTERVAL_SEC * 1000;
static uint32_t           g_last_detection_tick = 0;
static power_state_t      g_power_state = PWR_SLEEP;

/* Buffers for capture data (placed in SRAM) */
static int16_t  audio_buffer[ACOUSTIC_BUF_SAMPLES];
static uint16_t band_images[BAND_COUNT][SPECTRAL_IMG_SIZE];

/* Detection statistics */
static uint32_t g_total_detections = 0;
static uint32_t g_pest_detections = 0;
static uint32_t g_failed_captures = 0;

/* ----------------------------------------------------------------- *
 *  System tick handler (TIM2 interrupt, 1 ms)
 * ----------------------------------------------------------------- */
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = ~TIM_SR_UIF;  /* Clear flag */
        g_system_tick++;
    }
}

/* ----------------------------------------------------------------- *
 *  Board-level functions
 * ----------------------------------------------------------------- */
void board_init(void) {
    /* Enable HSE and configure PLL for 480 MHz system clock */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Configure PLL1: HSE / PLLM(4) × PLLN(120) / PLLP(2) = 480 MHz */
    RCC->PLL1CFGR = 0;  /* Clear first */
    RCC->PLL1CFGR = (PLLM << 0) | (PLLN << 8) | (0 << 24) | (PLLP << 17);

    /* Enable PLL1 */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    /* Configure flash latency for 480 MHz (5 wait states) */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_MASK) | 5 |
                 FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* Switch system clock to PLL1 */
    RCC->CFGR = (RCC->CFGR & ~0x3) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL);

    /* Enable all GPIO ports */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN;

    /* Configure status LEDs */
    gpio_set_mode(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(LED_ACTIVITY_PORT, LED_ACTIVITY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(LED_ERROR_PORT, LED_ERROR_PIN, GPIO_MODE_OUTPUT);

    /* Configure user button */
    gpio_set_mode(BTN_USER_PORT, BTN_USER_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BTN_USER_PORT, BTN_USER_PIN, GPIO_PUPD_UP);

    /* Configure TIM2 for 1 ms system tick */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    TIM2->PSC = (PCLK1_FREQ / 1000) - 1;  /* 120 MHz / 1000 = 120 kHz → 1 kHz */
    TIM2->ARR = 1000 - 1;  /* 1 ms period */
    TIM2->DIER = TIM_DIER_UIE;
    TIM2->CR1 = TIM_CR1_CEN;

    /* Enable TIM2 interrupt */
    NVIC_ISER0 |= (1 << IRQ_TIM2);

    /* Indicate boot complete */
    board_led_set(0, 1);  /* Status LED on */
}

void board_led_set(uint8_t led, uint8_t on) {
    switch (led) {
        case 0: gpio_write(LED_STATUS_PORT, LED_STATUS_PIN, on); break;
        case 1: gpio_write(LED_ACTIVITY_PORT, LED_ACTIVITY_PIN, on); break;
        case 2: gpio_write(LED_ERROR_PORT, LED_ERROR_PIN, on); break;
    }
}

uint32_t board_get_tick_ms(void) {
    return g_system_tick;
}

void board_delay_ms(uint32_t ms) {
    uint32_t start = g_system_tick;
    while (g_system_tick - start < ms) {
        __asm__("wfi");  /* Wait for interrupt (tick) */
    }
}

void board_enter_sleep(void) {
    __asm__("wfi");
}

void board_wakeup(void) {
    /* Nothing needed — waking from WFI is automatic */
}

uint16_t board_read_battery_mv(void) {
    return power_read_battery_mv();
}

uint16_t board_read_solar_mv(void) {
    return power_read_solar_mv();
}

/* ----------------------------------------------------------------- *
 *  Run a complete detection cycle
 *  Returns 0 on success, negative on error
 * ----------------------------------------------------------------- */
static int run_detection_cycle(void) {
    board_led_set(1, 1);  /* Activity LED on */

    /* Step 1: Read environmental conditions */
    env_data_t env;
    if (env_read_all(&env) < 0) {
        board_led_set(2, 1);  /* Error LED */
        board_delay_ms(100);
        board_led_set(2, 0);
        g_failed_captures++;
        board_led_set(1, 0);
        return -1;
    }

    /* Check if conditions are suitable for insect activity */
    if (env.temperature_c < TEMP_MIN_INSECT_C) {
        /* Too cold for insects — skip detection, save power */
        board_led_set(1, 0);
        return 1;  /* Skipped (too cold) */
    }

    /* Step 2: Read GNSS position */
    gnss_fix_t fix;
    int gps_result = gnss_read_fix(&fix, 5000);  /* 5 second timeout */

    /* Step 3: Acoustic capture and feature extraction */
    acoustic_power_up();
    int samples = acoustic_capture(audio_buffer, ACOUSTIC_BUF_SAMPLES);
    if (samples < 0) {
        g_failed_captures++;
        acoustic_power_down();
        board_led_set(1, 0);
        return -1;
    }

    acoustic_features_t acoustic_features;
    if (acoustic_extract_features(audio_buffer, (uint32_t)samples,
                                    &acoustic_features) < 0) {
        g_failed_captures++;
        acoustic_power_down();
        board_led_set(1, 0);
        return -1;
    }
    acoustic_power_down();

    /* Step 4: Multispectral capture */
    spectral_features_t spectral_features;
    int spectral_ok = 1;

    /* Capture all 6 bands */
    for (int b = 0; b < BAND_COUNT; b++) {
        if (spectral_capture_band((filter_band_t)b, band_images[b]) < 0) {
            spectral_ok = 0;
            g_failed_captures++;
            break;
        }
    }

    if (spectral_ok) {
        /* Extract spectral features */
        const uint16_t *band_ptrs[BAND_COUNT] = {
            band_images[0], band_images[1], band_images[2],
            band_images[3], band_images[4], band_images[5]
        };
        if (spectral_extract_features(band_ptrs, &spectral_features) < 0) {
            spectral_ok = 0;
        }
    }

    /* If spectral capture failed, use zeroed features (acoustic-only mode) */
    if (!spectral_ok) {
        memset(&spectral_features, 0, sizeof(spectral_features_t));
    }

    /* Step 5: Edge AI inference */
    ai_result_t ai_result;
    if (ai_infer(&acoustic_features, &spectral_features, &ai_result) < 0) {
        board_led_set(2, 1);
        board_delay_ms(200);
        board_led_set(2, 0);
        board_led_set(1, 0);
        return -1;
    }

    /* Step 6: Classify severity */
    uint8_t severity = ai_classify_severity(ai_result.top_class,
                                             ai_result.top_confidence,
                                             spectral_features.damage_area_pct);

    /* Step 7: Build detection event */
    detection_event_t event;
    memset(&event, 0, sizeof(event));
    event.timestamp = (gps_result == 0) ? fix.timestamp : g_system_tick / 1000;
    event.latitude  = (gps_result == 0) ? fix.latitude : 0;
    event.longitude = (gps_result == 0) ? fix.longitude : 0;
    event.species_id = ai_result.top_class;
    event.confidence = ai_result.top_confidence;
    event.severity   = severity;
    event.wingbeat_hz = acoustic_features.fundamental_hz;
    event.ndvi       = spectral_features.ndvi;
    event.ndre       = spectral_features.ndre;
    event.damage_area_pct = spectral_features.damage_area_pct;
    event.temp_c      = env.temperature_c;
    event.humidity_pct = env.humidity_pct;
    event.co2_ppm     = env.co2_ppm;
    event.node_id    = g_node_id;

    /* Step 8: Log to FRAM */
    fram_write_event(&event);

    /* Step 9: Store spectral thumbnail to SPI flash (if confidence > 0.5) */
    if (ai_result.top_confidence > 0.5f && ai_result.top_class != SPECIES_NONE) {
        /* Store a downscaled 128×96 image from the NIR band as a thumbnail.
         * Address: (event_count * SPECTRAL_IMG_SIZE * 2) % 14MB */
        uint32_t flash_addr = (g_total_detections * SPECTRAL_IMG_SIZE * 2)
                              % (14 * 1024 * 1024);
        /* Erase sector if needed (4 KB boundaries) */
        if ((flash_addr & 0xFFF) == 0) {
            flash_erase_sector(flash_addr);
        }
        flash_write_image(flash_addr, (uint8_t *)band_images[BAND_810],
                          SPECTRAL_IMG_SIZE * 2);
    }

    /* Step 10: Send LoRa mesh alert */
    if (ai_result.top_class != SPECIES_NONE && ai_result.top_confidence > 0.5f) {
        pest_alert_t alert;
        alert.species_id     = ai_result.top_class;
        alert.severity       = severity;
        alert.confidence_pct  = (uint8_t)(ai_result.top_confidence * 100);
        alert.node_id        = g_node_id;
        alert.lat_e7         = (int32_t)(event.latitude * 1e7f);
        alert.lon_e7         = (int32_t)(event.longitude * 1e7f);
        alert.timestamp      = event.timestamp;

        lora_send_pest_alert(&alert, MESH_BROADCAST_ADDR);
        g_pest_detections++;
    }

    g_total_detections++;

    board_led_set(1, 0);  /* Activity LED off */
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Process incoming mesh messages
 * ----------------------------------------------------------------- */
static void process_mesh_messages(void) {
    mesh_message_t msg;
    /* Listen for 200 ms */
    int result = lora_receive(&msg, 200);
    if (result == 0) {
        mesh_process_rx(&msg);

        /* If this is a pest alert from another node, log it too */
        if (msg.msg_type == MESH_MSG_PEST && msg.payload_len >= 12) {
            detection_event_t event;
            memset(&event, 0, sizeof(event));
            event.node_id = msg.payload[3];
            event.species_id = msg.payload[0];
            event.severity = msg.payload[1];
            event.confidence = (float)msg.payload[2] / 100.0f;

            int32_t lat_e7, lon_e7;
            memcpy(&lat_e7, &msg.payload[4], 4);
            memcpy(&lon_e7, &msg.payload[8], 4);
            event.latitude = (float)lat_e7 / 1e7f;
            event.longitude = (float)lon_e7 / 1e7f;
            event.timestamp = board_get_tick_ms() / 1000;

            fram_write_event(&event);
        }
    }
}

/* ----------------------------------------------------------------- *
 *  Check battery and solar status
 * ----------------------------------------------------------------- */
static int check_power_status(void) {
    power_status_t pwr;
    if (power_read_status(&pwr) < 0) return -1;

    /* If battery is critical, enter deep sleep until solar charges it */
    if (pwr.battery_mv < BATTERY_CRIT_MV && !pwr.charging) {
        /* Emergency: enter STOP mode and wait for solar */
        board_led_set(2, 1);  /* Error LED on */
        board_delay_ms(1000);
        board_led_set(2, 0);

        power_set_state(PWR_SLEEP);
        power_enter_stop_mode();
        power_wakeup();
        return -1;
    }

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Main loop
 * ----------------------------------------------------------------- */
int main(void) {
    /* Initialize board (clocks, GPIO, tick timer) */
    board_init();

    /* Initialize all subsystems */
    power_init();
    env_init();
    storage_init();
    ai_init();
    spectral_init();
    acoustic_init();
    gnss_init();

    /* Initialize LoRa mesh with node ID */
    mesh_init(g_node_id);

    /* Main super-loop */
    uint32_t last_mesh_tick = 0;
    g_last_detection_tick = board_get_tick_ms();

    while (1) {
        uint32_t now = board_get_tick_ms();
        uint32_t elapsed = now - g_last_detection_tick;

        /* Check if it's time for a detection cycle */
        if (elapsed >= g_detection_interval) {
            /* Check power status first */
            if (check_power_status() >= 0) {
                /* Wake up for detection */
                power_set_state(PWR_ACTIVE_CAPTURE);
                board_led_set(0, 1);  /* Status LED on */

                int result = run_detection_cycle();

                if (result == 0) {
                    /* Detection completed successfully */
                    g_last_detection_tick = now;
                } else if (result == 1) {
                    /* Skipped (too cold) — still count as a cycle */
                    g_last_detection_tick = now;
                } else {
                    /* Error — retry sooner (10 seconds) */
                    g_last_detection_tick = now - g_detection_interval + 10000;
                }

                board_led_set(0, 1);  /* Status LED back on */
            } else {
                g_last_detection_tick = now;
            }
        }

        /* Mesh maintenance every 5 seconds */
        if (now - last_mesh_tick > 5000) {
            mesh_tick();
            last_mesh_tick = now;
        }

        /* Process any incoming mesh messages (non-blocking) */
        if (now - last_mesh_tick > 1000) {
            power_set_state(PWR_MESH_RX);
            process_mesh_messages();
            power_set_state(PWR_SLEEP);
        }

        /* Enter sleep until next event (tick interrupt or LoRa DIO1) */
        power_set_state(PWR_SLEEP);
        board_led_set(0, 0);  /* Status LED off in sleep */
        board_enter_sleep();
    }

    return 0;  /* Never reached */
}