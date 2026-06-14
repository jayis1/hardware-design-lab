/*
 * ad9645.h — AD9645 14-bit 61.44 MSPS ADC driver
 * SPI configuration interface, LVDS data output to FPGA
 */

#ifndef VORTEX_SDR_AD9645_H
#define VORTEX_SDR_AD9645_H

#include <stdint.h>

/* AD9645 register map */
#define AD9645_REG_CHIP_ID        0x00    /* Chip ID (read-only) */
#define AD9645_REG_CHIP_GRADE     0x01    /* Chip grade (read-only) */
#define AD9645_REG_POWER_MODE    0x02    /* Power modes */
#define AD9645_REG_CLOCK_DIV     0x03    /* Clock divide ratio */
#define AD9645_REG_DECIMATION   0x04    /* Decimation ratio */
#define AD9645_REG_DEC_FILTER    0x05    /* Decimation filter select */
#define AD9645_REG_DATA_FORMAT   0x08    /* Data format select */
#define AD9645_REG_TEST_MODE     0x09    /* Test mode */
#define AD9645_REG_ADC_BUF_CFG   0x0A    /* ADC input buffer config */
#define AD9645_REG_ADC_REF_CFG   0x0B    /* ADC reference config */
#define AD9645_REG_OUTPUT_MODE   0x0C    /* Output mode (LVDS/CMOS) */
#define AD9645_REG_OUTPUT_DRIVE   0x0D    /* Output drive config */
#define AD9645_REG_ADC_INPUT_CFG  0x0E    /* ADC input configuration */
#define AD9645_REG_VREF_CFG       0x0F    /* Voltage reference config */
#define AD9645_REG_TRANSFER       0xFF    /* Transfer register */

/* Power mode bits */
#define AD9645_PM_NORMAL          0x00    /* Normal operation */
#define AD9645_PM_FULL_PD         0x01    /* Full power-down */
#define AD9645_PM_STANDBY         0x02    /* Standby */
#define AD9645_PM_DIG_PD          0x04    /* Digital power-down */

/* Data format bits */
#define AD9645_FMT_OFFSET_BIN     0x00    /* Offset binary */
#define AD9645_FMT_TWOS_COMP      0x01    /* Two's complement */

/* Output mode bits */
#define AD9645_OUT_LVDS           0x00    /* LVDS output */
#define AD9645_OUT_CMOS           0x01    /* CMOS output */

/* Test mode bits */
#define AD9645_TEST_OFF           0x00    /* Normal operation */
#define AD9645_TEST_MIDSCALE      0x01    /* Midscale output */
#define AD9645_TEST_POS_FS        0x02    /* Positive full-scale */
#define AD9645_TEST_NEG_FS        0x03    /* Negative full-scale */
#define AD9645_TEST_ALT_CHECKER   0x04    /* Alternating checkerboard */
#define AD9645_TEST_PN_SEQ        0x05    /* PN sequence */
#define AD9645_TEST_RAMP           0x06    /* Ramp output */
#define AD9645_TEST_ALT_1_0        0x07    /* Alternating 1/0 */

/* Decimation values */
#define AD9645_DEC_1              0x00    /* No decimation */
#define AD9645_DEC_2              0x01    /* Decimate by 2 */
#define AD9645_DEC_4              0x03    /* Decimate by 4 */
#define AD9645_DEC_8              0x07    /* Decimate by 8 */
#define AD9645_DEC_16             0x0F    /* Decimate by 16 */

/* Function prototypes */
void ad9645_init(void);
void ad9645_set_decimation(uint8_t dec_ratio);
void ad9645_set_data_format(uint8_t format);
void ad9645_set_test_mode(uint8_t mode);
void ad9645_power_down(void);
void ad9645_power_up(void);
uint8_t ad9645_read_reg(uint8_t addr);
void ad9645_write_reg(uint8_t addr, uint8_t data);

#endif /* VORTEX_SDR_AD9645_H */