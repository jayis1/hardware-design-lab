// tdc_driver.c — TDC Core Driver Implementation
// Full calibration, DMA timestamp capture, and precision timing
// 256-tap carry-chain TDC with 10 ps effective resolution

#include "tdc_driver.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

static volatile uint32_t *tdc_base = (volatile uint32_t *)TDC_BASE;
static tdc_calibration_t current_calibration;
static bool driver_initialized = false;
static bool calibration_loaded = false;

uint64_t tdc_timestamp_to_ps(const tdc_timestamp_t *ts) {
    if (!calibration_loaded) {
        return ((uint64_t)ts->coarse * TDC_COARSE_LSB_PS)
             + ((uint64_t)ts->fine * TDC_NOMINAL_BIN_PS);
    }

    uint64_t coarse_ps = (uint64_t)ts->coarse * TDC_COARSE_LSB_PS;
    uint64_t fine_ps = 0;
    uint16_t remaining = ts->fine;

    for (int bin = 0; bin < TDC_FINE_BINS && remaining > 0; bin++) {
        uint16_t bin_width = current_calibration.bin_width_fs[bin];
        if (remaining >= bin_width) {
            fine_ps += bin_width;
            remaining -= bin_width;
        } else {
            fine_ps += remaining;
            remaining = 0;
        }
    }

    return coarse_ps + fine_ps;
}

int tdc_driver_init(void) {
    if (driver_initialized) return 0;

    REG_SET_BITS(SYS_RESET_REG, SYS_RESET_TDC);
    for (volatile int d = 0; d < 100; d++) NOP();
    REG_CLR_BITS(SYS_RESET_REG, SYS_RESET_TDC);

    REG_WRITE(TDC_CTRL_REG, 0);
    tdc_clear_fifo();

    uint32_t version = REG_READ(TDC_VERSION_REG);
    uint16_t core_id = (version >> 16) & 0xFFFF;
    if (core_id != TDC_CORE_ID) return -1;

    driver_initialized = true;
    return 0;
}

int tdc_load_calibration(const tdc_calibration_t *cal) {
    if (!driver_initialized) return -1;

    for (int bin = 0; bin < TDC_FINE_BINS; bin++) {
        REG_WRITE(TDC_CAL_ADDR_REG, (bin & 0xFF) | (1 << 31));
        REG_WRITE(TDC_CAL_DATA_REG, cal->bin_width_fs[bin] & 0xFFFF);
    }

    memcpy(&current_calibration, cal, sizeof(tdc_calibration_t));
    calibration_loaded = true;
    return 0;
}

int tdc_save_calibration(tdc_calibration_t *cal) {
    if (!calibration_loaded) return -1;
    memcpy(cal, &current_calibration, sizeof(tdc_calibration_t));
    return 0;
}

int tdc_run_calibration(tdc_calibration_t *cal_out) {
    if (!driver_initialized) return -1;

    tdc_config_t cal_config = {
        .continuous_mode = false,
        .single_shot = false,
        .trigger_source = 0,
        .averaging_depth = 0,
        .rising_edge = true,
        .falling_edge = false,
        .hit_threshold = 1,
        .dead_time = 0,
        .calibration_mode = true
    };

    int ret = tdc_configure(&cal_config);
    if (ret != 0) return ret;

    REG_SET_BITS(TDC_CTRL_REG, TDC_CTRL_ENABLE | TDC_CTRL_CAL_MODE);

    uint32_t timeout = 500000000;
    while (timeout > 0) {
        if (REG_READ(TDC_STATUS_REG) & TDC_STATUS_CAL_DONE) break;
        timeout--;
    }

    if (timeout == 0) {
        REG_CLR_BITS(TDC_CTRL_REG, TDC_CTRL_ENABLE);
        return -2;
    }

    for (int bin = 0; bin < TDC_FINE_BINS; bin++) {
        REG_WRITE(TDC_CAL_ADDR_REG, bin & 0xFF);
        cal_out->bin_width_fs[bin] = REG_READ(TDC_CAL_DATA_REG) & 0xFFFF;
    }

    cal_out->cal_timestamp = REG_READ(SYS_UPTIME_LO_REG);
    cal_out->temperature_c = 25.0f;
    cal_out->crc = 0;

    REG_CLR_BITS(TDC_CTRL_REG, TDC_CTRL_ENABLE | TDC_CTRL_CAL_MODE);
    tdc_load_calibration(cal_out);

    return 0;
}

int tdc_configure(const tdc_config_t *config) {
    if (!driver_initialized) return -1;

    tdc_stop();

    uint32_t cfg = 0;
    if (config->rising_edge)  cfg |= TDC_CFG_RISING;
    if (config->falling_edge) cfg |= TDC_CFG_FALLING;
    if (config->rising_edge && config->falling_edge) cfg |= TDC_CFG_BOTH;
    cfg |= ((config->hit_threshold & 0xFF) << 8);
    cfg |= ((config->dead_time & 0xFF) << 16);
    REG_WRITE(TDC_CFG_REG, cfg);

    uint32_t ctrl = 0;
    if (config->continuous_mode) ctrl |= TDC_CTRL_CONTINUOUS;
    if (config->single_shot)     ctrl |= TDC_CTRL_SINGLE_SHOT;
    if (config->calibration_mode) ctrl |= TDC_CTRL_CAL_MODE;
    ctrl |= ((config->trigger_source & 0xF) << 8);
    ctrl |= ((config->averaging_depth & 0xFF) << 16);
    REG_WRITE(TDC_CTRL_REG, ctrl);

    return 0;
}

int tdc_start(void) {
    if (!driver_initialized) return -1;
    REG_SET_BITS(TDC_CTRL_REG, TDC_CTRL_ENABLE);
    return 0;
}

int tdc_stop(void) {
    if (!driver_initialized) return -1;
    REG_CLR_BITS(TDC_CTRL_REG, TDC_CTRL_ENABLE);
    return 0;
}

int tdc_single_shot(tdc_timestamp_t *ts_out) {
    if (!driver_initialized) return -1;

    tdc_config_t ss_config = {
        .single_shot = true,
        .trigger_source = 0,
        .rising_edge = true,
        .falling_edge = false,
        .hit_threshold = 1,
        .dead_time = 0,
        .averaging_depth = 0,
        .continuous_mode = false,
        .calibration_mode = false
    };
    tdc_configure(&ss_config);
    tdc_clear_fifo();
    tdc_start();

    uint32_t timeout = 1000000;
    while (timeout > 0) {
        uint32_t status = REG_READ(TDC_STATUS_REG);
        uint32_t fifo_count = (status >> 8) & 0xFF;
        if (fifo_count > 0) break;
        timeout--;
    }

    if (timeout == 0) {
        tdc_stop();
        return -2;
    }

    uint32_t raw = REG_READ(TDC_FIFO_RD_REG);
    ts_out->coarse = (raw >> 16) & 0xFFFF;
    ts_out->fine = raw & 0xFFFF;

    tdc_stop();
    return 0;
}

int tdc_read_timestamps(tdc_timestamp_t *buffer, uint32_t count, uint32_t *actual_count) {
    if (!driver_initialized) return -1;

    uint32_t read = 0;
    uint32_t timeout = count * 100;

    while (read < count && timeout > 0) {
        uint32_t status = REG_READ(TDC_STATUS_REG);
        uint32_t fifo_count = (status >> 8) & 0xFF;
        if (fifo_count > 0) {
            uint32_t raw = REG_READ(TDC_FIFO_RD_REG);
            buffer[read].coarse = (raw >> 16) & 0xFFFF;
            buffer[read].fine = raw & 0xFFFF;
            read++;
        }
        timeout--;
    }

    *actual_count = read;
    if (read < count && timeout == 0) return -2;
    return 0;
}

int tdc_read_timestamps_dma(tdc_timestamp_t *buffer, uint32_t max_count,
                             uint32_t dma_buffer_addr, uint32_t *actual_count) {
    if (!driver_initialized) return -1;

    volatile uint32_t *acq_base = (volatile uint32_t *)ACQ_BASE;
    REG_WRITE(ACQ_TDC_BASE_REG, dma_buffer_addr);

    uint32_t timeout = 5000000;
    while (timeout > 0) {
        if (REG_READ(ACQ_STATUS_REG) & ACQ_STATUS_COMPLETE) break;
        timeout--;
    }

    if (timeout == 0) return -2;

    uint32_t fifo_count = (REG_READ(TDC_STATUS_REG) >> 8) & 0xFF;
    uint32_t copy_count = (fifo_count < max_count) ? fifo_count : max_count;

    volatile uint32_t *src = (volatile uint32_t *)dma_buffer_addr;
    for (uint32_t i = 0; i < copy_count; i++) {
        uint32_t raw = src[i];
        buffer[i].coarse = (raw >> 16) & 0xFFFF;
        buffer[i].fine = raw & 0xFFFF;
    }

    *actual_count = copy_count;
    return 0;
}

int tdc_get_result(tdc_result_t *result) {
    if (!driver_initialized) return -1;

    uint32_t status = REG_READ(TDC_STATUS_REG);
    result->timestamps_captured = (status >> 8) & 0xFF;
    result->triggers_received = REG_READ(TDC_TRIG_COUNT_REG);
    result->fifo_overflow = (status & TDC_STATUS_FIFO_OVERFLOW) ? true : false;
    result->completed = !(status & TDC_STATUS_BUSY);

    return 0;
}

void tdc_clear_fifo(void) {
    REG_SET_BITS(TDC_CTRL_REG, TDC_CTRL_CLEAR_FIFO);
    for (volatile int d = 0; d < 10; d++) NOP();
    REG_CLR_BITS(TDC_CTRL_REG, TDC_CTRL_CLEAR_FIFO);
}

int tdc_self_test(void) {
    if (!driver_initialized) return -1;

    uint32_t version = REG_READ(TDC_VERSION_REG);
    if ((version >> 16) != TDC_CORE_ID) return -10;

    tdc_clear_fifo();
    uint32_t status = REG_READ(TDC_STATUS_REG);
    if (!(status & TDC_STATUS_FIFO_EMPTY)) return -11;

    tdc_timestamp_t ts;
    int ret = tdc_single_shot(&ts);
    if (ret != 0) return -12;
    if (ts.coarse == 0 && ts.fine == 0) return -13;
    if (ts.fine > 65535) return -14;

    tdc_timestamp_t ts2;
    ret = tdc_single_shot(&ts2);
    if (ret != 0) return -15;
    if (ts.coarse == ts2.coarse && ts.fine == ts2.fine) return -16;

    return 0;
}
