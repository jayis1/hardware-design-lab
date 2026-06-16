// tdc_driver.h — TDC Core Driver Header
// Sub-nanosecond precision time-to-digital converter

#ifndef TDC_DRIVER_H
#define TDC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t coarse;
    uint16_t fine;
} tdc_timestamp_t;

typedef struct {
    uint16_t bin_width_fs[256];
    uint32_t cal_timestamp;
    float    temperature_c;
    uint32_t crc;
} tdc_calibration_t;

typedef struct {
    bool     continuous_mode;
    bool     single_shot;
    uint8_t  trigger_source;
    uint8_t  averaging_depth;
    bool     rising_edge;
    bool     falling_edge;
    uint8_t  hit_threshold;
    uint8_t  dead_time;
    bool     calibration_mode;
} tdc_config_t;

typedef struct {
    uint32_t timestamps_captured;
    uint32_t triggers_received;
    bool     fifo_overflow;
    bool     completed;
} tdc_result_t;

uint64_t tdc_timestamp_to_ps(const tdc_timestamp_t *ts);
int  tdc_driver_init(void);
int  tdc_load_calibration(const tdc_calibration_t *cal);
int  tdc_save_calibration(tdc_calibration_t *cal);
int  tdc_run_calibration(tdc_calibration_t *cal_out);
int  tdc_configure(const tdc_config_t *config);
int  tdc_start(void);
int  tdc_stop(void);
int  tdc_single_shot(tdc_timestamp_t *ts_out);
int  tdc_read_timestamps(tdc_timestamp_t *buffer, uint32_t count, uint32_t *actual_count);
int  tdc_read_timestamps_dma(tdc_timestamp_t *buffer, uint32_t max_count,
                             uint32_t dma_buffer_addr, uint32_t *actual_count);
int  tdc_get_result(tdc_result_t *result);
void tdc_clear_fifo(void);
int  tdc_self_test(void);

#endif
