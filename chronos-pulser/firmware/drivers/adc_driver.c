// adc_driver.c — AD9230-250 ADC Capture Engine Driver
// Full DMA-based acquisition with SPI configuration and calibration
// 250 MSPS, 12-bit, LVDS DDR interface

#include "adc_driver.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

static volatile uint32_t *adc_base = (volatile uint32_t *)ADC_CAP_BASE;
static adc_acq_config_t current_config;
static bool driver_initialized = false;
static bool adc_powered = false;

// SPI transaction helper for AD9230 (3-wire SPI: CSB, SCLK, SDIO bidirectional)
static int adc_spi_transaction(uint8_t reg_addr, uint8_t *data, bool is_write) {
    uint32_t tx_val;
    int timeout = 10000;

    tx_val = (is_write ? ADC_SPI_WRITE : 0)
           | ((reg_addr & 0xFF) << 8)
           | (*data & 0xFF)
           | ADC_SPI_START;

    // Wait for previous transaction to clear
    while (REG_READ(ADC_SPI_RX_REG) & ADC_SPI_DONE) {
        // Previous transaction done — we can proceed
        break;
    }

    REG_WRITE(ADC_SPI_TX_REG, tx_val);

    while (timeout > 0) {
        uint32_t rx = REG_READ(ADC_SPI_RX_REG);
        if (rx & ADC_SPI_DONE) {
            if (!is_write) {
                *data = rx & 0xFF;
            }
            return 0;
        }
        timeout--;
    }

    return -1;
}

int adc_spi_read(uint8_t reg_addr, uint8_t *data) {
    uint8_t dummy = 0;
    return adc_spi_transaction(reg_addr, &dummy, false);
}

int adc_spi_write(uint8_t reg_addr, uint8_t data) {
    return adc_spi_transaction(reg_addr, &data, true);
}

int adc_driver_init(void) {
    if (driver_initialized) return 0;

    adc_power_up();

    // Reset capture engine
    REG_SET_BITS(SYS_RESET_REG, SYS_RESET_ADC_CAP);
    for (volatile int d = 0; d < 100; d++) NOP();
    REG_CLR_BITS(SYS_RESET_REG, SYS_RESET_ADC_CAP);

    // Clear state
    REG_WRITE(ADC_CTRL_REG, 0);
    REG_WRITE(ADC_SAMPLES_REG, 0);
    REG_WRITE(ADC_TRIG_DELAY_REG, 0);
    REG_WRITE(ADC_DMA_BASE_REG, 0);
    REG_WRITE(ADC_DMA_LEN_REG, 0);

    driver_initialized = true;
    return 0;
}

int adc_configure_defaults(void) {
    uint8_t data;
    int ret;

    // Verify chip ID
    ret = adc_spi_read(ADC_SPI_REG_CHIP_ID, &data);
    if (ret != 0) return -1;
    if (data != ADC_EXPECTED_CHIP_ID) return -2;

    // Output mode: LVDS, DDR, 12-bit, offset binary
    data = (1 << 5);  // DDR mode
    ret = adc_spi_write(ADC_SPI_REG_OUTPUT_MODE, data);
    if (ret != 0) return -3;

    // Output adjust: 3.5 mA LVDS drive
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_OUTPUT_ADJ, data);
    if (ret != 0) return -4;

    // Disable test mode
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_TEST_MODE, data);
    if (ret != 0) return -5;

    // Clock divider = 1
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_CLOCK_DIV, data);
    if (ret != 0) return -6;

    // Normal operation (not power-down)
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_PDWN_MODE, data);
    if (ret != 0) return -7;

    return 0;
}

int adc_verify_device(void) {
    uint8_t chip_id, chip_grade;
    int ret;

    ret = adc_spi_read(ADC_SPI_REG_CHIP_ID, &chip_id);
    if (ret != 0) return -1;

    ret = adc_spi_read(ADC_SPI_REG_CHIP_GRADE, &chip_grade);
    if (ret != 0) return -2;

    if (chip_id != ADC_EXPECTED_CHIP_ID) return -3;
    if ((chip_grade & 0x0F) != 0x00) return -4;  // Must be 250 MSPS grade

    return 0;
}

int adc_start_acquisition(const adc_acq_config_t *config) {
    if (!driver_initialized) return -1;
    if (!adc_powered) return -2;

    if (REG_READ(ADC_STATUS_REG) & ADC_STATUS_BUSY) {
        return -3;
    }

    memcpy(&current_config, config, sizeof(adc_acq_config_t));

    REG_WRITE(ADC_SAMPLES_REG, config->sample_count & 0xFFFFFF);
    REG_WRITE(ADC_TRIG_DELAY_REG, config->trigger_delay & 0xFFFF);

    uint32_t ctrl = ADC_CTRL_ENABLE;
    if (config->decimation_factor > 0) {
        ctrl |= ADC_CTRL_DECIMATION_EN;
        ctrl |= ((config->decimation_factor & 0xF) << 8);
    }
    if (config->averaging_depth > 0) {
        ctrl |= ADC_CTRL_AVERAGING_EN;
        ctrl |= ((config->averaging_depth & 0xFF) << 16);
    }
    if (config->triggered) ctrl |= ADC_CTRL_TRIGGERED;
    if (config->continuous) ctrl |= ADC_CTRL_CONTINUOUS;

    if (config->use_dma) {
        REG_WRITE(ADC_DMA_BASE_REG, config->dma_buffer_addr);
        REG_WRITE(ADC_DMA_LEN_REG, config->dma_buffer_size);
    }

    if (config->continuous) {
        ctrl |= ADC_CTRL_CONTINUOUS;
    } else {
        ctrl |= ADC_CTRL_SINGLE_SHOT;
    }
    REG_WRITE(ADC_CTRL_REG, ctrl);

    return 0;
}

int adc_abort_acquisition(void) {
    if (!driver_initialized) return -1;

    REG_CLR_BITS(ADC_CTRL_REG, ADC_CTRL_ENABLE);
    REG_SET_BITS(ADC_CTRL_REG, ADC_CTRL_CLEAR_FIFO);
    for (volatile int d = 0; d < 10; d++) NOP();
    REG_CLR_BITS(ADC_CTRL_REG, ADC_CTRL_CLEAR_FIFO);

    return 0;
}

int adc_wait_completion(uint32_t timeout_ms) {
    if (!driver_initialized) return -1;

    uint32_t max_loops = timeout_ms * 100000;
    uint32_t loops = 0;

    while (loops < max_loops) {
        uint32_t status = REG_READ(ADC_STATUS_REG);
        if (!(status & ADC_STATUS_BUSY)) {
            if (status & ADC_STATUS_FIFO_OVERFLOW) return -2;
            return 0;
        }
        loops++;
    }

    return -3;
}

int adc_get_result(adc_acq_result_t *result) {
    if (!driver_initialized) return -1;

    uint32_t status = REG_READ(ADC_STATUS_REG);
    uint32_t dma_status = REG_READ(ADC_DMA_STATUS_REG);

    result->samples_captured = (status >> 8) & 0xFFFF;
    result->overrange_events = 0;
    result->completed = !(status & ADC_STATUS_BUSY);
    result->dma_error = (dma_status & ADC_DMA_ERROR) ? true : false;
    result->fifo_overflow = (status & ADC_STATUS_FIFO_OVERFLOW) ? true : false;

    return 0;
}

int adc_read_samples(adc_sample_t *buffer, uint32_t offset, uint32_t count) {
    if (!driver_initialized) return -1;

    uint32_t dma_status = REG_READ(ADC_DMA_STATUS_REG);
    if (dma_status & ADC_DMA_BUSY) return -2;
    if (dma_status & ADC_DMA_ERROR) return -3;

    uint32_t src_addr = current_config.dma_buffer_addr + (offset * sizeof(adc_sample_t));
    uint32_t byte_count = count * sizeof(adc_sample_t);

    volatile uint32_t *src = (volatile uint32_t *)src_addr;
    uint32_t word_count = (byte_count + 3) / 4;
    uint32_t *dst = (uint32_t *)buffer;

    for (uint32_t i = 0; i < word_count; i++) {
        dst[i] = src[i];
    }

    return 0;
}

void adc_power_down(void) {
    gpio_set(GPIO_ADC_PDWN, 1);
    uint8_t data = 0x03;
    adc_spi_write(ADC_SPI_REG_PDWN_MODE, data);
    adc_powered = false;
}

void adc_power_up(void) {
    if (adc_powered) return;
    gpio_set(GPIO_ADC_PDWN, 0);
    uint8_t data = 0x00;
    adc_spi_write(ADC_SPI_REG_PDWN_MODE, data);
    for (volatile int d = 0; d < 50000; d++) NOP();
    adc_powered = true;
}

int adc_calibrate_offset(uint16_t *offset_out) {
    adc_acq_config_t cal_cfg = {
        .sample_count = 10000,
        .trigger_delay = 0,
        .decimation_factor = 0,
        .averaging_depth = 0,
        .continuous = false,
        .triggered = false,
        .use_dma = true,
        .dma_buffer_addr = DMA_CAL_BUFFER,
        .dma_buffer_size = 20000
    };

    int ret = adc_start_acquisition(&cal_cfg);
    if (ret != 0) return ret;

    ret = adc_wait_completion(100);
    if (ret != 0) return ret;

    adc_sample_t samples[10000];
    ret = adc_read_samples(samples, 0, 10000);
    if (ret != 0) return ret;

    uint64_t sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += (samples[i] & 0xFFF);
    }

    *offset_out = (uint16_t)(sum / 10000);
    uint8_t offset_byte = (uint8_t)((*offset_out - 2048) / 2);
    adc_spi_write(ADC_SPI_REG_OFFSET, offset_byte);

    return 0;
}

int adc_calibrate_gain(uint16_t *gain_out) {
    *gain_out = 0x8000;  // Unity gain in 16.16 fixed point
    REG_WRITE(ADC_CAL_GAIN_REG, *gain_out);
    return 0;
}
