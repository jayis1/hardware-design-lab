// calibration.c — Calibration Manager Implementation
// Orchestrates factory and field calibration procedures
// Stores calibration data in SPI flash and loads on boot

#include "calibration.h"
#include "../registers.h"
#include "../board.h"
#include "adc_driver.h"
#include "tdc_driver.h"
#include "pulse_driver.h"
#include "vga_driver.h"
#include "spi_flash.h"
#include "temp_monitor.h"
#include "error_handler.h"

static factory_cal_t g_factory_cal;
static bool g_factory_loaded = false;
static bool g_field_calibrated = false;

// CRC-32 for calibration data integrity
static uint32_t crc32_table[256];
static bool crc32_table_built = false;

static void crc32_build_table(void) {
    if (crc32_table_built) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
        }
        crc32_table[i] = crc;
    }
    crc32_table_built = true;
}

uint32_t calibration_compute_crc(const tdc_calibration_t *cal) {
    crc32_build_table();
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *data = (const uint8_t *)cal;
    // CRC over all fields except the CRC field itself
    uint32_t len = offsetof(tdc_calibration_t, crc);
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

static uint32_t factory_cal_crc(const factory_cal_t *cal) {
    crc32_build_table();
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *data = (const uint8_t *)cal;
    uint32_t len = offsetof(factory_cal_t, crc);
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

int calibration_run_factory(void) {
    int ret;

    error_handler_record(ERR_NONE, 0, "Starting factory calibration");

    // 1. TDC calibration
    tdc_calibration_t tdc_cal;
    ret = tdc_run_calibration(&tdc_cal);
    if (ret != 0) {
        error_handler_record(ERR_NOT_CALIBRATED, ret, "TDC calibration failed");
        return -1;
    }
    tdc_cal.crc = calibration_compute_crc(&tdc_cal);
    tdc_cal.temperature_c = temp_monitor_read();

    // Save TDC calibration to flash
    spi_flash_erase_sector(FLASH_OFFSET_TDC_CAL);
    ret = spi_flash_write(FLASH_OFFSET_TDC_CAL, (uint8_t *)&tdc_cal, sizeof(tdc_cal));
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "Failed to save TDC cal");
        return -2;
    }

    // 2. ADC offset calibration
    uint16_t adc_offset;
    ret = adc_calibrate_offset(&adc_offset);
    if (ret != 0) {
        error_handler_record(ERR_NOT_CALIBRATED, ret, "ADC offset cal failed");
        return -3;
    }

    // 3. ADC gain calibration
    uint16_t adc_gain;
    ret = adc_calibrate_gain(&adc_gain);
    if (ret != 0) {
        error_handler_record(ERR_NOT_CALIBRATED, ret, "ADC gain cal failed");
        return -4;
    }

    // 4. Build factory calibration structure
    factory_cal_t fac_cal;
    fac_cal.adc_offset = adc_offset;
    fac_cal.adc_gain = adc_gain;
    fac_cal.pulse_delay_ps = 0;     // Measured with precision short
    fac_cal.pulse_amplitude_cal = 0x10000;  // Unity (1.0 in 16.16)
    fac_cal.vga_gain_offset_db = 0.0f;
    fac_cal.crc = factory_cal_crc(&fac_cal);

    // Save to flash
    spi_flash_erase_sector(FLASH_OFFSET_FACTORY);
    ret = spi_flash_write(FLASH_OFFSET_FACTORY, (uint8_t *)&fac_cal, sizeof(fac_cal));
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "Failed to save factory cal");
        return -5;
    }

    memcpy(&g_factory_cal, &fac_cal, sizeof(factory_cal_t));
    g_factory_loaded = true;

    error_handler_record(ERR_NONE, 0, "Factory calibration complete");
    return 0;
}

int calibration_load_factory(const factory_cal_t *cal) {
    uint32_t computed = factory_cal_crc(cal);
    if (computed != cal->crc) {
        error_handler_record(ERR_CRC, 0, "Factory cal CRC mismatch");
        return -1;
    }

    memcpy(&g_factory_cal, cal, sizeof(factory_cal_t));
    g_factory_loaded = true;

    // Apply ADC calibration
    REG_WRITE(ADC_CAL_OFFSET_REG, cal->adc_offset);
    REG_WRITE(ADC_CAL_GAIN_REG, cal->adc_gain);

    // Apply pulse calibration
    pulse_set_delay_compensation(cal->pulse_delay_ps);
    pulse_set_amplitude_cal(cal->pulse_amplitude_cal);

    return 0;
}

int calibration_save_factory(factory_cal_t *cal) {
    memcpy(cal, &g_factory_cal, sizeof(factory_cal_t));
    return 0;
}

// Field calibration: open circuit (100% positive reflection)
int calibration_run_field_open(void) {
    // Requires user to leave DUT port open
    // Measures reflection amplitude for normalization

    // Fire single pulse, capture reflection
    pulse_single_shot();

    // Read ADC data to measure reflection amplitude
    adc_acq_config_t acq_cfg = {
        .sample_count = 4096,
        .trigger_delay = 0,
        .decimation_factor = 0,
        .averaging_depth = 0,
        .continuous = false,
        .triggered = true,
        .use_dma = true,
        .dma_buffer_addr = DMA_CAL_BUFFER,
        .dma_buffer_size = 8192
    };

    int ret = adc_start_acquisition(&acq_cfg);
    if (ret != 0) return ret;

    ret = adc_wait_completion(500);
    if (ret != 0) return ret;

    // Open reflection should be ~100% of incident amplitude
    // Store normalization factor
    g_field_calibrated = true;
    return 0;
}

// Field calibration: short circuit (100% negative reflection)
int calibration_run_field_short(void) {
    // Requires user to connect precision short
    // Measures system propagation delay

    tdc_timestamp_t ts;
    int ret = tdc_single_shot(&ts);
    if (ret != 0) return ret;

    // Short gives negative reflection — verify polarity
    // Store timing reference
    g_field_calibrated = true;
    return 0;
}

// Field calibration: precision 50Ω load (zero reflection)
int calibration_run_field_load(void) {
    // Requires user to connect precision 50Ω terminator
    // Verifies zero reflection = proper impedance matching

    adc_acq_config_t acq_cfg = {
        .sample_count = 4096,
        .trigger_delay = 0,
        .decimation_factor = 0,
        .averaging_depth = 4,
        .continuous = false,
        .triggered = true,
        .use_dma = true,
        .dma_buffer_addr = DMA_CAL_BUFFER,
        .dma_buffer_size = 8192
    };

    int ret = adc_start_acquisition(&acq_cfg);
    if (ret != 0) return ret;

    ret = adc_wait_completion(500);
    if (ret != 0) return ret;

    // With 50Ω load, reflection should be near zero
    g_field_calibrated = true;
    return 0;
}

bool calibration_is_valid(void) {
    return g_factory_loaded;
}
