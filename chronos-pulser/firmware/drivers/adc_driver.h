// adc_driver.h — AD9230-250 ADC Capture Engine Driver Header
// See adc_driver.c for full implementation

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t adc_sample_t;

typedef struct {
    uint32_t sample_count;
    uint32_t trigger_delay;
    uint8_t  decimation_factor;
    uint8_t  averaging_depth;
    bool     continuous;
    bool     triggered;
    bool     use_dma;
    uint32_t dma_buffer_addr;
    uint32_t dma_buffer_size;
} adc_acq_config_t;

typedef struct {
    uint32_t samples_captured;
    uint32_t overrange_events;
    bool     completed;
    bool     dma_error;
    bool     fifo_overflow;
} adc_acq_result_t;

#define ADC_SPI_REG_CHIP_ID      0x00
#define ADC_SPI_REG_CHIP_GRADE   0x01
#define ADC_SPI_REG_DEV_CFG      0x02
#define ADC_SPI_REG_TEST_MODE    0x0D
#define ADC_SPI_REG_OFFSET       0x10
#define ADC_SPI_REG_GAIN         0x11
#define ADC_SPI_REG_OUTPUT_MODE  0x14
#define ADC_SPI_REG_OUTPUT_ADJ   0x15
#define ADC_SPI_REG_CLOCK_DIV    0x16
#define ADC_SPI_REG_PDWN_MODE    0x18
#define ADC_EXPECTED_CHIP_ID     0x64

int  adc_driver_init(void);
int  adc_spi_read(uint8_t reg_addr, uint8_t *data);
int  adc_spi_write(uint8_t reg_addr, uint8_t data);
int  adc_configure_defaults(void);
int  adc_verify_device(void);
int  adc_start_acquisition(const adc_acq_config_t *config);
int  adc_abort_acquisition(void);
int  adc_wait_completion(uint32_t timeout_ms);
int  adc_get_result(adc_acq_result_t *result);
int  adc_read_samples(adc_sample_t *buffer, uint32_t offset, uint32_t count);
void adc_power_down(void);
void adc_power_up(void);
int  adc_calibrate_offset(uint16_t *offset_out);
int  adc_calibrate_gain(uint16_t *gain_out);

#endif
