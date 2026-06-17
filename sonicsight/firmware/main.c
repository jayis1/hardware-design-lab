/**
 * @file    main.c
 * @brief   SonicSight — Portable Acoustic Tomography Scanner Firmware.
 *          Main entry point, super-loop state machine, and acquisition
 *          orchestration for 32-channel ultrasonic pulse-echo tomography.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Hardware: STM32H743ZIT6 (480 MHz Cortex-M7), ADS5281 ×2 (8ch 50 MSPS ADC),
 *           ADG732 muxes (32:1), MD1210+TC6320 HV pulser, NRF52840 BLE module,
 *           GC9A01 240×240 LCD, ICM-20948 IMU, TMP117 temp sensor.
 *
 * Architecture: State-machine driven super-loop with interrupt-driven ADC
 * capture via DCMI (digital camera interface). Cross-correlation and
 * tomographic inversion run on-device using CMSIS-DSP and CORDIC.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "board.h"
#include "registers.h"
#include "acquisition.h"
#include "crosscorr.h"
#include "tomography.h"
#include "calibration.h"
#include "ble.h"
#include "display.h"
#include "sdlog.h"

/* ========================================================================== */
/*  Global State                                                              */
/* ========================================================================== */

static volatile enum sonicsight_state g_state = STATE_INIT;
static volatile int32_t g_error_code = ERR_OK;
static volatile uint32_t g_scan_count = 0;
static volatile uint32_t g_sys_tick_ms = 0;

/* Acquisition context — holds current scan parameters and results */
static acquisition_ctx_t g_acq_ctx;

/* Tomography context — solver state, grid, and output image */
static tomography_ctx_t g_tomo_ctx;

/* BLE context — connection handle, notification queues */
static ble_ctx_t g_ble_ctx;

/* Calibration data — per-channel gain, offset, timing delay */
static calibration_data_t g_cal_data;

/* Sensor positions (polar coordinates: angle in radians, radius in metres) */
static float g_sensor_theta[TOMO_MAX_SENSORS];
static float g_sensor_radius[TOMO_MAX_SENSORS];
static uint8_t g_sensor_active[TOMO_MAX_SENSORS]; /* bitmap of active channels */
static uint8_t g_num_active_sensors = 0;

/* Temperature reading from TMP117, updated each idle loop */
static float g_temperature_c = 25.0f;

/* ========================================================================== */
/*  Forward Declarations                                                      */
/* ========================================================================== */

static void system_clock_config(void);
static void gpio_init(void);
static void dma_init(void);
static void timer_init(void);
static void fmc_sdram_init(void);
static void state_machine_run(void);
static void enter_state(enum sonicsight_state new_state);
static void handle_error(int32_t err);
static void idle_sleep(void);
static void arm_sensors(void);
static void run_acquisition_cycle(void);
static void run_tomographic_inversion(void);
static void update_display(void);
static void stream_results(void);

/* ========================================================================== */
/*  Systick Handler (1 ms tick)                                               */
/* ========================================================================== */

void SysTick_Handler(void)
{
    g_sys_tick_ms++;
}

/* ========================================================================== */
/*  Main Entry Point                                                          */
/* ========================================================================== */

int main(void)
{
    /* --- HAL & System Initialisation --- */
    HAL_Init();
    system_clock_config();
    gpio_init();
    dma_init();
    timer_init();
    fmc_sdram_init();

    /* --- Peripheral Initialisation --- */
    display_init();            /* GC9A01 LCD — power-on splash */
    display_show_splash("SonicSight", FIRMWARE_VERSION_MAJOR,
                        FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);

    acquisition_init(&g_acq_ctx);
    tomography_init(&g_tomo_ctx);
    ble_init(&g_ble_ctx);
    sdlog_init();
    calibration_init(&g_cal_data);

    /* --- Sensor Geometry (default: 32 sensors equally spaced around 0.5 m circle) --- */
    for (uint8_t i = 0; i < TOMO_MAX_SENSORS; i++) {
        g_sensor_theta[i]  = (float)i * (2.0f * 3.14159265f) / (float)TOMO_MAX_SENSORS;
        g_sensor_radius[i] = 0.25f; /* 25 cm from centre (50 cm diameter object) */
        g_sensor_active[i] = 1;     /* all active by default */
    }
    g_num_active_sensors = TOMO_MAX_SENSORS;

    /* --- Load calibration (or use defaults) --- */
    if (calibration_load(&g_cal_data) != ERR_OK) {
        calibration_set_defaults(&g_cal_data);
        calibration_save(&g_cal_data);
    }

    /* --- Temperature compensation startup read --- */
    g_temperature_c = read_temperature(); /* TMP117 first read */

    /* --- Transition to IDLE --- */
    enter_state(STATE_IDLE);

    /* ====================================================================== */
    /*  Main Super-Loop                                                       */
    /* ====================================================================== */
    while (1) {
        state_machine_run();
    }

    return 0; /* unreachable */
}

/* ========================================================================== */
/*  State Machine                                                             */
/* ========================================================================== */

static void state_machine_run(void)
{
    switch (g_state) {

    case STATE_INIT:
        /* Already handled in main() — should not re-enter */
        enter_state(STATE_IDLE);
        break;

    case STATE_IDLE:
        /* Low-power sleep, wait for BLE command or button press */
        idle_sleep();
        break;

    case STATE_ARM_SENSORS:
        /* Check coupling, pre-charge high-voltage rail */
        arm_sensors();
        if (g_error_code == ERR_OK) {
            enter_state(STATE_TX_CHANNEL);
        } else {
            enter_state(STATE_ERROR);
        }
        break;

    case STATE_TX_CHANNEL:
        /* Fire one transmitter, capture all receivers — repeats for all TX */
        run_acquisition_cycle();
        if (g_acq_ctx.current_tx >= g_acq_ctx.total_tx) {
            enter_state(STATE_ALL_TX_DONE);
        } else {
            /* Stay in TX_CHANNEL for next transmitter */
            g_state = STATE_TX_CHANNEL;
        }
        break;

    case STATE_ALL_TX_DONE:
        /* All ray-path data collected; proceed to reconstruction */
        enter_state(STATE_RECONSTRUCT);
        break;

    case STATE_RECONSTRUCT:
        /* Run Tikhonov-regularised least-squares inversion */
        run_tomographic_inversion();
        if (g_error_code == ERR_OK) {
            enter_state(STATE_DISPLAY);
        } else {
            enter_state(STATE_ERROR);
        }
        break;

    case STATE_DISPLAY:
        /* Render tomogram to LCD */
        update_display();
        enter_state(STATE_STREAM);
        break;

    case STATE_STREAM:
        /* Send tomogram and ToF data over BLE */
        stream_results();
        g_scan_count++;
        /* Return to IDLE after streaming completes */
        enter_state(STATE_IDLE);
        break;

    case STATE_CALIBRATE:
        /* Run calibration phantom measurement */
        {
            int32_t cal_err = run_calibration_scan(&g_cal_data);
            if (cal_err == ERR_OK) {
                calibration_save(&g_cal_data);
                display_show_message("CALIBRATION OK");
            } else {
                display_show_message("CAL FAIL");
                g_error_code = cal_err;
            }
            enter_state(STATE_IDLE);
        }
        break;

    case STATE_ERROR:
        /* Display error and wait for reset */
        display_show_error(g_error_code);
        /* Blink error LED at 2 Hz */
        HAL_GPIO_TogglePin(LED_ERROR_PORT, LED_ERROR_PIN);
        HAL_Delay(500);
        break;

    default:
        break;
    }
}

/**
 * @brief Transition to a new state with proper entry/exit hooks.
 */
static void enter_state(enum sonicsight_state new_state)
{
    /* Exit actions for current state */
    switch (g_state) {

    case STATE_IDLE:
        /* Disable sleep; restore full clock */
        HAL_PWR_DisableSleepOnExit();
        HAL_PWR_EnableDomain3();  /* restore voltage domain */
        break;

    case STATE_STREAM:
        /* Stop BLE notifications if still active */
        ble_stop_notifications(&g_ble_ctx);
        break;

    case STATE_ERROR:
        /* Clear error LED */
        HAL_GPIO_WritePin(LED_ERROR_PORT, LED_ERROR_PIN, GPIO_PIN_RESET);
        break;

    default:
        break;
    }

    g_state = new_state;
    g_error_code = ERR_OK;

    /* Entry actions for new state */
    switch (new_state) {

    case STATE_IDLE:
        /* Set status LED heartbeat (dimly lit) */
        HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_PIN_SET);
        break;

    case STATE_ARM_SENSORS:
        /* Turn on status LED bright */
        HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_PIN_SET);
        break;

    case STATE_CALIBRATE:
        display_show_message("CALIBRATING...");
        break;

    default:
        break;
    }
}

/* ========================================================================== */
/*  IDLE — Low-power sleep, wake on BLE command or button press              */
/* ========================================================================== */

static void idle_sleep(void)
{
    /* Check for pending BLE commands */
    ble_process_commands(&g_ble_ctx);

    /* Check for button press (debounced) */
    if (HAL_GPIO_ReadPin(BTN_START_PORT, BTN_START_PIN) == GPIO_PIN_RESET) {
        /* Debounce delay */
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(BTN_START_PORT, BTN_START_PIN) == GPIO_PIN_RESET) {
            /* Wait for release */
            while (HAL_GPIO_ReadPin(BTN_START_PORT, BTN_START_PIN) == GPIO_PIN_RESET) {
                /* spin */
            }
            enter_state(STATE_ARM_SENSORS);
            return;
        }
    }

    /* Check for calibration request from BLE */
    if (g_ble_ctx.cmd_pending && g_ble_ctx.cmd == BLE_CMD_CALIBRATE) {
        g_ble_ctx.cmd_pending = 0;
        enter_state(STATE_CALIBRATE);
        return;
    }

    /* Check for scan request from BLE */
    if (g_ble_ctx.cmd_pending && g_ble_ctx.cmd == BLE_CMD_START_SCAN) {
        g_ble_ctx.cmd_pending = 0;
        enter_state(STATE_ARM_SENSORS);
        return;
    }

    /* Update temperature every 5 seconds */
    static uint32_t last_temp_update = 0;
    if (g_sys_tick_ms - last_temp_update > 5000) {
        g_temperature_c = read_temperature();
        last_temp_update = g_sys_tick_ms;
    }

    /* Update BLE status LED */
    if (g_ble_ctx.connected) {
        HAL_GPIO_WritePin(LED_BLE_PORT, LED_BLE_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_BLE_PORT, LED_BLE_PIN, GPIO_PIN_RESET);
    }

    /* Enter STOP mode (lowest power with SRAM retention) */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
}

/* ========================================================================== */
/*  ARM_SENSORS — Verify coupling, pre-charge HV rail                        */
/* ========================================================================== */

static void arm_sensors(void)
{
    int32_t coupling_status;
    float hv_voltage;

    display_show_message("ARMING...");

    /* 1. Check high-voltage rail (target: 150 V) */
    hv_voltage = measure_hv_rail(); /* ADC on voltage divider */
    if (hv_voltage < 120.0f) {
        /* Attempt to boost */
        set_hv_rail_target(150.0f);
        HAL_Delay(100); /* Allow DC-DC to ramp */
        hv_voltage = measure_hv_rail();
        if (hv_voltage < 120.0f) {
            handle_error(ERR_HV_UNDERVOLTAGE);
            return;
        }
    }

    /* 2. Coupling quality check */
    coupling_status = acquisition_check_coupling(&g_acq_ctx, g_sensor_active,
                                                  g_num_active_sensors);
    if (coupling_status < 0) {
        handle_error(ERR_BAD_COUPLING);
        g_error_code = coupling_status;
        return;
    }

    /* 3. Deactivate poorly-coupled sensors (coupling returns bitmask of good ones) */
    for (uint8_t i = 0; i < TOMO_MAX_SENSORS; i++) {
        g_sensor_active[i] = (coupling_status >> i) & 0x01U;
    }

    /* 4. Configure acquisition parameters from calibration data */
    acquisition_set_gains(&g_acq_ctx, g_cal_data.channel_gain_db);
    acquisition_set_filter_fc(&g_acq_ctx, FILTER_FC_DEFAULT_HZ);

    /* 5. Prepare ADC (set sync, start clock) */
    acquisition_prepare_adc(&g_acq_ctx);

    /* 6. Reset acquisition context for new scan */
    g_acq_ctx.current_tx = 0;
    g_acq_ctx.total_tx   = g_num_active_sensors;
    g_acq_ctx.ray_count  = 0;

    display_show_message("READY");
}

/* ========================================================================== */
/*  ACQUISITION CYCLE — Fire one transmitter, capture all receivers          */
/* ========================================================================== */

static void run_acquisition_cycle(void)
{
    uint8_t tx_index = g_acq_ctx.current_tx;

    /* Skip inactive sensors */
    if (!g_sensor_active[tx_index]) {
        g_acq_ctx.current_tx++;
        return;
    }

    /* --- Transmit Phase --- */
    /* 1. Route HV pulse to selected transducer via TX mux */
    acquisition_select_tx_channel(tx_index);

    /* 2. Set receive mux bank for this transmitter (sequential mux switching) */
    acquisition_configure_rx_muxes(tx_index, g_sensor_active, g_num_active_sensors);

    /* 3. Wait for mux settling */
    delay_us(ACQ_SETTLE_TIME_US);

    /* 4. Fire burst: TIM1 outputs N cycles at TX_FREQ_HZ */
    acquisition_fire_burst(tx_index, TX_BURST_CYCLES_DEFAULT, TX_FREQ_HZ_DEFAULT);

    /* --- Receive Phase --- */
    /* 5. Start ADC capture via DCMI (DMA-based, 1024 samples × 8 channels) */
    acquisition_start_capture(&g_acq_ctx);

    /* 6. Wait for capture complete (handled by DCMI interrupt) */
    while (!g_acq_ctx.capture_done) {
        /* spin — or could use WFE for lower power */
    }
    g_acq_ctx.capture_done = 0;

    /* --- Time-of-Flight Extraction --- */
    /* 7. For each active receiver channel, compute ToF via cross-correlation */
    float arrival_times[ADC_NUM_CHANNELS];
    for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        uint8_t rx_sensor = g_acq_ctx.rx_mux_map[ch];
        if (!g_sensor_active[rx_sensor] || (rx_sensor == tx_index)) {
            /* Skip inactive receiver or self-transmit channel */
            arrival_times[ch] = -1.0f;
            continue;
        }

        /* Extract waveform from ADC buffer */
        int16_t *waveform = g_acq_ctx.adc_buffers[ch];
        uint32_t num_samples = ADC_SAMPLES_PER_TRACE;

        /* Cross-correlate against stored transmit pulse template */
        float peak_time_us;
        float snr_db;
        crosscorr_compute(waveform, num_samples,
                          g_acq_ctx.tx_template,
                          g_acq_ctx.tx_template_len,
                          &peak_time_us, &snr_db);

        /* Validate peak quality */
        if (snr_db < ACQ_CROSS_CORR_THRESH) {
            /* Fall back to AIC picker */
            peak_time_us = crosscorr_aic_pick(waveform, num_samples,
                                              ADC_SAMPLE_RATE);
        }

        /* Apply per-channel calibration offset (cable delay, transducer phase) */
        peak_time_us -= g_cal_data.channel_delay_us[ch];

        /* Temperature compensation: velocity varies with temp */
        float temp_factor = 1.0f + TOMO_TEMP_ALPHA * (g_temperature_c - TOMO_TEMP_REF);
        peak_time_us /= temp_factor;

        arrival_times[ch] = peak_time_us;
    }

    /* 8. Store extracted ToFs for this transmitter */
    acquisition_store_tofs(&g_acq_ctx, tx_index, arrival_times, ADC_NUM_CHANNELS);

    /* 9. Increment transmitter index */
    g_acq_ctx.current_tx++;
    g_acq_ctx.ray_count += (g_num_active_sensors - 1); /* N−1 rays per TX */

    /* 10. Update progress on LCD */
    display_show_progress(g_acq_ctx.current_tx, g_acq_ctx.total_tx);

    /* 11. Log raw waveform to SD card (optional, one per scan) */
    if (g_acq_ctx.current_tx == 1) {
        sdlog_write_waveforms(tx_index, g_acq_ctx.adc_buffers,
                              ADC_NUM_CHANNELS, ADC_SAMPLES_PER_TRACE);
    }
}

/* ========================================================================== */
/*  TOMOGRAPHIC INVERION — Solve: (S^T S + λL^T L) m = S^T t                */
/* ========================================================================== */

static void run_tomographic_inversion(void)
{
    uint32_t grid_size = TOMO_GRID_DEFAULT;
    float lambda = TOMO_LAMBDA_DEFAULT;

    /* Use custom λ if set via BLE command */
    if (g_ble_ctx.custom_lambda > 0.0f) {
        lambda = g_ble_ctx.custom_lambda;
    }

    /* 1. Setup tomography context with current sensor geometry and ToF data */
    tomography_setup(&g_tomo_ctx,
                     g_sensor_theta,
                     g_sensor_radius,
                     g_sensor_active,
                     g_num_active_sensors,
                     g_acq_ctx.tof_matrix,
                     g_acq_ctx.ray_count,
                     grid_size);

    /* 2. Set regularization parameter */
    g_tomo_ctx.lambda = lambda;

    /* 3. Run reconstruction */
    int32_t solver_status = tomography_solve(&g_tomo_ctx);

    if (solver_status != ERR_OK) {
        handle_error(ERR_TOMO_SOLVER);
        return;
    }

    /* 4. Apply post-processing: 3×3 median filter on the image */
    tomography_median_filter(g_tomo_ctx.image, grid_size, grid_size, 3);

    /* 5. Compute statistics */
    tomography_compute_stats(&g_tomo_ctx);

    /* 6. Log ToF data and image to SD */
    sdlog_write_tof_matrix(&g_acq_ctx);
    sdlog_write_image(g_tomo_ctx.image, grid_size, grid_size);
}

/* ========================================================================== */
/*  DISPLAY — Render tomogram to 240×240 GC9A01 LCD                          */
/* ========================================================================== */

static void update_display(void)
{
    uint32_t grid_size = g_tomo_ctx.grid_size;

    /* 1. Draw colour-mapped tomogram (scale to fit 240×240) */
    float scale = (float)LCD_WIDTH / (float)grid_size;
    for (uint32_t y = 0; y < LCD_HEIGHT; y++) {
        for (uint32_t x = 0; x < LCD_WIDTH; x++) {
            uint32_t gx = (uint32_t)((float)x / scale);
            uint32_t gy = (uint32_t)((float)y / scale);
            if (gx >= grid_size) gx = grid_size - 1;
            if (gy >= grid_size) gy = grid_size - 1;

            float slowness = g_tomo_ctx.image[gy * grid_size + gx];
            float velocity = (slowness > 0.001f) ? (1.0f / slowness) : 0.0f;

            /* Map velocity 0–4000 m/s to 8-bit colour index (0=red, 255=blue) */
            uint8_t idx;
            if (velocity > 4000.0f)      idx = 255;
            else if (velocity < 500.0f)  idx = 0;
            else                         idx = (uint8_t)((velocity - 500.0f) / 13.73f);

            uint16_t colour = tomo_colormap[idx];
            display_draw_pixel(x, y, colour);
        }
    }

    /* 2. Overlay colour bar legend on right edge */
    for (uint32_t y = 0; y < LCD_HEIGHT; y++) {
        uint8_t idx = (uint8_t)(y * 255 / LCD_HEIGHT);
        uint16_t colour = tomo_colormap[255 - idx];
        for (uint32_t x = 220; x < 230; x++) {
            display_draw_pixel(x, y, colour);
        }
    }

    /* 3. Overlay transducer positions as small circles */
    for (uint8_t i = 0; i < g_num_active_sensors; i++) {
        if (!g_sensor_active[i]) continue;
        /* Convert polar to screen coordinates (centre of LCD, radius scaled) */
        float sx = 120.0f + g_sensor_radius[i] * 240.0f * cosf(g_sensor_theta[i]);
        float sy = 120.0f + g_sensor_radius[i] * 240.0f * sinf(g_sensor_theta[i]);
        /* Draw small + marker */
        int32_t ix = (int32_t)sx;
        int32_t iy = (int32_t)sy;
        for (int32_t dx = -2; dx <= 2; dx++) {
            display_draw_pixel(ix + dx, iy, 0xFFFF); /* white */
            display_draw_pixel(ix, iy + dx, 0xFFFF);
        }
    }

    /* 4. Show scan info overlay */
    char info_buf[64];
    snprintf(info_buf, sizeof(info_buf), "Vmin:%.0f Vmax:%.0f m/s",
             g_tomo_ctx.vel_min, g_tomo_ctx.vel_max);
    display_draw_text(2, 220, info_buf, 0xFFFF, 0x0000);

    snprintf(info_buf, sizeof(info_buf), "Scan #%lu  %d°C",
             g_scan_count, (int32_t)g_temperature_c);
    display_draw_text(2, 2, info_buf, 0xFFFF, 0x0000);
}

/* ========================================================================== */
/*  BLE STREAM — Send tomogram and ToF data over BLE notifications           */
/* ========================================================================== */

static void stream_results(void)
{
    if (!g_ble_ctx.connected) {
        return; /* No one listening */
    }

    /* 1. Send status update */
    ble_send_status(&g_ble_ctx, g_scan_count, g_temperature_c,
                    g_tomo_ctx.vel_min, g_tomo_ctx.vel_max, g_error_code);

    /* 2. Send compressed tomogram (64×64 float → 4096 bytes via BLE packetisation) */
    ble_send_tomogram(&g_ble_ctx, g_tomo_ctx.image, g_tomo_ctx.grid_size);

    /* 3. Send ToF matrix (ray count × 2 bytes per ToF) */
    ble_send_tof_raw(&g_ble_ctx, g_acq_ctx.tof_matrix,
                     g_acq_ctx.ray_count);

    /* 4. Send diagnostic data (signal quality per channel) */
    ble_send_gain_report(&g_ble_ctx, g_acq_ctx.channel_snr,
                         g_acq_ctx.channel_gain, ADC_NUM_CHANNELS);
}

/* ========================================================================== */
/*  ERROR HANDLING                                                            */
/* ========================================================================== */

static void handle_error(int32_t err)
{
    g_error_code = err;
    enter_state(STATE_ERROR);
}

/* ========================================================================== */
/*  SYSTEM — Clock, GPIO, DMA, Timer, SDRAM Initialisation                   */
/* ========================================================================== */

static void system_clock_config(void)
{
    /* Configure HSE → PLL1 → System Clock = 480 MHz
     * HSE = 25 MHz, PLL1M = 5, PLL1N = 96, PLL1P = 1 (÷1)
     * VCO = 25/5 × 96 = 480 MHz
     * SysClk = 480/1 = 480 MHz
     * APB1 = 480/4 = 120 MHz
     * APB2 = 480/2 = 240 MHz
     */
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

    HSE_Enable();
    HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM = 5;
    RCC_OscInit.PLL.PLLN = 96;
    RCC_OscInit.PLL.PLLP = 1;
    RCC_OscInit.PLL.PLLQ = 20;  /* 480/20 = 24 MHz for SDMMC */
    RCC_OscInit.PLL.PLLR = 2;   /* 480/2 = 240 MHz for D1 domain */
    RCC_OscInit.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
    RCC_OscInit.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInit.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                          | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                          | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInit.AHBCLKD ivider = RCC_HCLK_DIV1;
    RCC_ClkInit.APB3CLKDivider = RCC_APB3_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_APB1_DIV4;   /* 120 MHz */
    RCC_ClkInit.APB2CLKDivider = RCC_APB2_DIV2;   /* 240 MHz */
    RCC_ClkInit.APB4CLKDivider = RCC_APB4_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_4);
}

static void gpio_init(void)
{
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* --- Status LED: PE14 (green) --- */
    gpio.Pin = LED_STATUS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(LED_STATUS_PORT, &gpio);

    /* --- Error LED: PE15 (red) --- */
    gpio.Pin = LED_ERROR_PIN;
    HAL_GPIO_Init(LED_ERROR_PORT, &gpio);

    /* --- BLE LED: PE0 (blue) --- */
    gpio.Pin = LED_BLE_PIN;
    HAL_GPIO_Init(LED_BLE_PORT, &gpio);

    /* --- Start Button: PC13 (pull-up, falling-edge interrupt) --- */
    gpio.Pin = BTN_START_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BTN_START_PORT, &gpio);

    /* --- TX Mux control: PA0–PA5 --- */
    gpio.Pin = TX_MUX_SEL0_PIN | TX_MUX_SEL1_PIN | TX_MUX_SEL2_PIN
             | TX_MUX_SEL3_PIN | TX_MUX_SEL4_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(TX_MUX_SEL_PORT, &gpio);

    gpio.Pin = TX_MUX_EN_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(TX_MUX_EN_PORT, &gpio);

    /* Enable TX mux (active low) */
    HAL_GPIO_WritePin(TX_MUX_EN_PORT, TX_MUX_EN_PIN, GPIO_PIN_RESET);

    /* --- RX Mux control: PB0–PB5, PB8–PB12 --- */
    gpio.Pin = RX_MUX0_SEL0_PIN | RX_MUX0_SEL1_PIN | RX_MUX0_SEL2_PIN
             | RX_MUX0_SEL3_PIN | RX_MUX0_SEL4_PIN | RX_MUX0_EN_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(RX_MUX0_SEL_PORT, &gpio);

    /* Enable RX mux 0 */
    HAL_GPIO_WritePin(RX_MUX0_EN_PORT, RX_MUX0_EN_PIN, GPIO_PIN_RESET);

    /* ADC CS: PF12 */
    gpio.Pin = ADC_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(ADC_CS_PORT, &gpio);
    HAL_GPIO_WritePin(ADC_CS_PORT, ADC_CS_PIN, GPIO_PIN_SET); /* deselect */

    /* LCD DC/RST: PC2, PC3 */
    gpio.Pin = LCD_DC_PIN | LCD_RST_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_DC_PORT, &gpio);

    /* LCD CS: PA4 */
    gpio.Pin = LCD_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_CS_PORT, &gpio);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET); /* deselect */

    /* BLE Reset: PD13 */
    gpio.Pin = BLE_RESET_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(BLE_RESET_PORT, &gpio);
    /* Hold NRF52 in reset during init */
    HAL_GPIO_WritePin(BLE_RESET_PORT, BLE_RESET_PIN, GPIO_PIN_RESET);

    /* BLE SPI CS: PD1 */
    gpio.Pin = BLE_SPI_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(BLE_SPI_CS_PORT, &gpio);
    HAL_GPIO_WritePin(BLE_SPI_CS_PORT, BLE_SPI_CS_PIN, GPIO_PIN_SET);

    /* SD CS: PG8 */
    gpio.Pin = SD_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(SD_CS_PORT, &gpio);
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
}

static void dma_init(void)
{
    /* Enable DMA1/DMA2 clocks */
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* DCMI DMA stream (ADC capture): DMA2 Stream 1, Channel 1 */
    /* Configured in acquisition_init() */
}

static void timer_init(void)
{
    /* TIM1 — TX Burst Generation (advanced timer, PWM mode) */
    /* Clock = APB2 = 240 MHz. Prescaler for 50 kHz: 240,000,000 / (2400 × 2) = 50 kHz */
    /* ARR = 2400 − 1, PSC = 1 − 1 (÷1). CCR1 = 1200 (50% duty) */
    __HAL_RCC_TIM1_CLK_ENABLE();

    TIM1->PSC = 0;
    TIM1->ARR = 4799;  /* 240 MHz / 4800 = 50 kHz */
    TIM1->CCR1 = 2400; /* 50% duty cycle */
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE; /* CH1 and CH1N */
    TIM1->BDTR |= TIM_BDTR_MOE;  /* Main output enable */
    TIM1->CR1 |= TIM_CR1_CEN;    /* Start counter (paused by gate control) */
    TIM1->CR1 &= ~TIM_CR1_CEN;   /* Stop — started by acquisition_fire_burst */

    /* TIM3 — LCD backlight PWM (240 MHz / (2400 × 2) = 50 kHz) */
    __HAL_RCC_TIM3_CLK_ENABLE();
    TIM3->PSC = 0;
    TIM3->ARR = 2399;
    TIM3->CCR3 = 1200;  /* 50% brightness default */
    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CCMR2 |= (6 << TIM_CCMR2_OC3M_Pos); /* PWM mode 1 */
    TIM3->CR1 |= TIM_CR1_CEN;

    /* TIM8 — Acquisition trigger timing */
    __HAL_RCC_TIM8_CLK_ENABLE();
    TIM8->PSC = 239;   /* 240 MHz / 240 = 1 MHz → 1 µs resolution */
    TIM8->ARR = 999;    /* 1000 µs period = 1 kHz trigger rate */
    TIM8->CCR1 = 1;     /* Trigger pulse width: 1 µs */
    TIM8->CCER |= TIM_CCER_CC1E;
    TIM8->CR1 |= TIM_CR1_CEN;
    TIM8->CR1 &= ~TIM_CR1_CEN; /* Stopped — enabled during acquisition */
}

static void fmc_sdram_init(void)
{
    /* FMC SDRAM Interface: IS42S32800J (8 MB, 32-bit, 2 banks) */
    __HAL_RCC_FMC_CLK_ENABLE();

    SDRAM_HandleTypeDef hsdram;
    hsdram.Instance = FMC_SDRAM_DEVICE;
    hsdram.Init.SDBank = FMC_SDRAM_BANK1;
    hsdram.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_8;
    hsdram.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_11;
    hsdram.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_32;
    hsdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_2;
    hsdram.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;
    hsdram.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    hsdram.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2; /* HCLK/2 = 120 MHz */
    hsdram.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
    hsdram.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;

    HAL_SDRAM_Init(&hsdram, NULL);

    /* SDRAM initialization sequence */
    FMC_SDRAM_CommandTypeDef cmd;
    cmd.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    cmd.AutoRefreshNumber = 1;
    cmd.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(&hsdram, &cmd, 10);

    HAL_Delay(1); /* > 100 µs wait */

    cmd.CommandMode = FMC_SDRAM_CMD_PALL;
    HAL_SDRAM_SendCommand(&hsdram, &cmd, 10);

    cmd.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    cmd.AutoRefreshNumber = 8;
    HAL_SDRAM_SendCommand(&hsdram, &cmd, 10);

    /* Mode register: CAS=2, burst length=1, sequential, standard operation */
    uint32_t mode_reg = (uint32_t)SDRAM_CAS_LATENCY_2 | 0x0000;
    cmd.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    cmd.ModeRegisterDefinition = mode_reg;
    HAL_SDRAM_SendCommand(&hsdram, &cmd, 10);

    /* Set auto-refresh rate (64 ms / 4096 rows ≈ 15.625 µs) */
    uint32_t refresh_count = (uint32_t)(64 * 120000000 / 4096); /* ~1875 */
    HAL_SDRAM_ProgramRefreshRate(&hsdram, refresh_count);
}

/* ========================================================================== */
/*  UTILITY: Microsecond Delay (busy-wait loop)                               */
/* ========================================================================== */

void delay_us(uint32_t us)
{
    /* DWT cycle counter for precise microsecond delays */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SYSCLK_FREQ / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) {
        /* spin */
    }
}

/* ========================================================================== */
/*  UTILITY: Read TMP117 Temperature (±0.1 °C) — I2C                         */
/* ========================================================================== */

float read_temperature(void)
{
    uint8_t buf[2];
    HAL_I2C_Master_Transmit(&hi2c1, TMP117_ADDR << 1,
                            (uint8_t[]) { TMP117_REG_TEMP }, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, TMP117_ADDR << 1, buf, 2, 100);

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    return (float)raw * TMP117_LSB_DEGC;
}

/* ========================================================================== */
/*  UTILITY: Measure HV Rail Voltage (ADC or voltage divider reading)        */
/* ========================================================================== */

float measure_hv_rail(void)
{
    /* HV rail measured through a 1000:1 resistive divider (1 MΩ / 1 kΩ)
     * into a single-shot ADC conversion on PA6 (ADC3_INP3).
     * ADC3 is configured in acquisition_init().
     */
    ADC3->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC3->SR & ADC_SR_EOC)) { /* spin */ }
    uint16_t raw = (uint16_t)ADC3->DR;
    /* ADC ref = 3.3 V, 12-bit. V_divider = raw × 3.3 / 4096.
     * HV = V_divider × 1001 ≈ raw × 0.8056 */
    return (float)raw * 0.806f;
}

/* ========================================================================== */
/*  UTILITY: Set HV Rail Target Voltage (PWM to LT3905 feedback)             */
/* ========================================================================== */

void set_hv_rail_target(float voltage)
{
    /* LT3905 feedback is controlled via PWM duty cycle on TIM1_CH3.
     * Duty = 0–100% maps to 100–200 V output.
     * voltage = 100 + duty * 1.0
     */
    uint32_t duty = (uint32_t)(voltage - 100.0f);
    if (duty > 100) duty = 100;
    uint32_t ccr = duty * 48; /* TIM1 ARR = 4800, so 1% = 48 ticks */
    TIM1->CCR3 = ccr;
}

/* ========================================================================== */
/*  HAL Overrides (weak symbols)                                              */
/* ========================================================================== */

void HAL_MspInit(void)
{
    /* Set priority grouping */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* Enable HSI48 for USB */
    __HAL_RCC_HSI48_ENABLE();

    /* SysTick interrupt priority */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

void HAL_DCMI_ConvCpltCallback(DCMI_HandleTypeDef *hdcmi)
{
    g_acq_ctx.capture_done = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_START_PIN) {
        /* Button pressed — handled in idle sleep loop */
    }
}

/* HAL I2C handle — defined here as it's a singleton */
I2C_HandleTypeDef hi2c1 = {0};
void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00201D2B; /* 400 kHz @ 240 MHz */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* Weak aliases for CubeMX-generated init functions (stubs) */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) { (void)hspi; }
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram) { (void)hsdram; }
void HAL_DCMI_MspInit(DCMI_HandleTypeDef *hdcmi) { (void)hdcmi; }

/**
 * Colour map — 256-entry "turbo" style: blue→cyan→green→yellow→orange→red.
 * Generated for SonicSight velocity visualisation.
 * Index 0 = slowest (red), Index 255 = fastest (blue).
 */
const uint16_t tomo_colormap[256] = {
    0xF800, 0xF804, 0xF808, 0xF80C, 0xF810, 0xF814, 0xF818, 0xF81C,
    0xF820, 0xF824, 0xF828, 0xF82C, 0xF830, 0xF834, 0xF838, 0xF83C,
    0xF840, 0xF844, 0xF848, 0xF84C, 0xF850, 0xF854, 0xF858, 0xF85C,
    0xF860, 0xF864, 0xF868, 0xF86C, 0xF870, 0xF874, 0xF878, 0xF87C,
    0xFC00, 0xFC20, 0xFC40, 0xFC60, 0xFC80, 0xFCA0, 0xFCC0, 0xFCE0,
    0xFD00, 0xFD20, 0xFD40, 0xFD60, 0xFD80, 0xFDA0, 0xFDC0, 0xFDE0,
    0xFE00, 0xFE20, 0xFE40, 0xFE60, 0xFE80, 0xFEA0, 0xFEC0, 0xFEE0,
    0xFF00, 0xFF20, 0xFF40, 0xFF60, 0xFF80, 0xFFA0, 0xFFC0, 0xFFE0,
    0xFFE0, 0xFFE2, 0xFFE4, 0xFFE6, 0xFFE8, 0xFFEA, 0xFFEC, 0xFFEE,
    0xFFF0, 0xFFF2, 0xFFF4, 0xFFF6, 0xFFF8, 0xFFFA, 0xFFFC, 0xFFFE,
    0xFFFF, 0xEFF7, 0xDFEF, 0xCFE7, 0xBFDF, 0xAFD7, 0x9FCF, 0x8FC7,
    0x7FBF, 0x6FB7, 0x5FAF, 0x4FA7, 0x3F9F, 0x2F97, 0x1F8F, 0x0F87,
    0x079D, 0x0799, 0x0795, 0x0791, 0x078D, 0x0789, 0x0785, 0x0781,
    0x077D, 0x0779, 0x0775, 0x0771, 0x076D, 0x0769, 0x0765, 0x0761,
    0x075D, 0x0759, 0x0755, 0x0751, 0x074D, 0x0749, 0x0745, 0x0741,
    0x073D, 0x0739, 0x0735, 0x0731, 0x072D, 0x0729, 0x0725, 0x0721,
    0x071D, 0x0719, 0x0715, 0x0711, 0x070D, 0x0709, 0x0705, 0x0701,
    0x06FD, 0x06F9, 0x06F5, 0x06F1, 0x06ED, 0x06E9, 0x06E5, 0x06E1,
    0x06DD, 0x06D9, 0x06D5, 0x06D1, 0x06CD, 0x06C9, 0x06C5, 0x06C1,
    0x06BD, 0x06B9, 0x06B5, 0x06B1, 0x06AD, 0x06A9, 0x06A5, 0x06A1,
    0x069D, 0x0699, 0x0695, 0x0691, 0x068D, 0x0689, 0x0685, 0x0681,
    0x067D, 0x0679, 0x0675, 0x0671, 0x066D, 0x0669, 0x0665, 0x0661,
    0x065D, 0x0659, 0x0655, 0x0651, 0x064D, 0x0649, 0x0645, 0x0641,
    0x063D, 0x0639, 0x0635, 0x0631, 0x062D, 0x0629, 0x0625, 0x0621,
    0x061D, 0x0619, 0x0615, 0x0611, 0x060D, 0x0609, 0x0605, 0x0601,
    0x05FD, 0x05F9, 0x05F5, 0x05F1, 0x05ED, 0x05E9, 0x05E5, 0x05E1,
    0x05DD, 0x05D9, 0x05D5, 0x05D1, 0x05CD, 0x05C9, 0x05C5, 0x05C1,
    0x05BD, 0x05B9, 0x05B5, 0x05B1, 0x05AD, 0x05A9, 0x05A5, 0x05A1,
    0x059D, 0x0599, 0x0595, 0x0591, 0x058D, 0x0589, 0x0585, 0x0581,
    0x057D, 0x0579, 0x0575, 0x0571, 0x056D, 0x0569, 0x0565, 0x0561,
    0x055D, 0x0559, 0x0555, 0x0551, 0x054D, 0x0549, 0x0545, 0x0541,
    0x053D, 0x0539, 0x0535, 0x0531, 0x052D, 0x0529, 0x0525, 0x0521
};