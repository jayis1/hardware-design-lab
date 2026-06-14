/*
 * NeuroLink — ADS1299 24-bit ADC Driver
 * 8-device daisy chain, SPI1 interface
 */

#ifndef ADS1299_H
#define ADS1299_H

#include <stdint.h>

/* ADS1299 SPI Commands */
#define ADS_CMD_WAKEUP      0x02
#define ADS_CMD_STANDBY     0x04
#define ADS_CMD_RESET       0x06
#define ADS_CMD_START       0x08
#define ADS_CMD_STOP        0x0A
#define ADS_CMD_RDATAC      0x10
#define ADS_CMD_SDATAC      0x11
#define ADS_CMD_RDATA       0x12
#define ADS_CMD_RREG        0x20
#define ADS_CMD_WREG        0x40

/* ADS1299 Register Map */
#define ADS_REG_ID          0x00
#define ADS_REG_CONFIG1     0x01
#define ADS_REG_CONFIG2     0x02
#define ADS_REG_CONFIG3     0x03
#define ADS_REG_LOFF        0x04
#define ADS_REG_CH1SET      0x05
#define ADS_REG_CH2SET      0x06
#define ADS_REG_CH3SET      0x07
#define ADS_REG_CH4SET      0x08
#define ADS_REG_CH5SET      0x09
#define ADS_REG_CH6SET      0x0A
#define ADS_REG_CH7SET      0x0B
#define ADS_REG_CH8SET      0x0C
#define ADS_REG_BIAS_SENSP  0x0D
#define ADS_REG_BIAS_SENSN  0x0E
#define ADS_REG_LOFF_SENSP  0x0F
#define ADS_REG_LOFF_SENSN  0x10
#define ADS_REG_LOFF_FLIP   0x11
#define ADS_REG_LOFF_STATP  0x12
#define ADS_REG_LOFF_STATN  0x13
#define ADS_REG_GPIO         0x14
#define ADS_REG_MISC1       0x15
#define ADS_REG_MISC2       0x16
#define ADS_REG_CONFIG4      0x17

/* Number of devices in daisy chain */
#define ADS_NUM_DEVICES     8

/* Frame size per device: 3 bytes status + 8 channels × 3 bytes = 27 bytes */
#define ADS_FRAME_PER_DEV   27
#define ADS_TOTAL_FRAME     (ADS_FRAME_PER_DEV * ADS_NUM_DEVICES)  /* 216 bytes */

/* Channel gain settings */
typedef enum {
    ADS_GAIN_1 = 0x00,
    ADS_GAIN_2 = 0x10,
    ADS_GAIN_4 = 0x20,
    ADS_GAIN_6 = 0x30,
    ADS_GAIN_8 = 0x40,
    ADS_GAIN_12 = 0x50,
    ADS_GAIN_24 = 0x60,
} ads_gain_t;

/* Channel input selection */
typedef enum {
    ADS_INPUT_NORMAL    = 0x00,  /* Normal electrode input */
    ADS_INPUT_SHORTED   = 0x01,  /* Input shorted (for offset calibration) */
    ADS_INPUT_RBIAS_DR  = 0x02,  /* Rbias + DC block */
    ADS_INPUT_RBIAS     = 0x03,  /* Rbias only */
    ADS_INPUT_MVDD      = 0x04,  /* Measure (AVDD-AVSS)/2 */
    ADS_INPUT_TEMP       = 0x05,  /* Temperature sensor */
    ADS_INPUT_TEST      = 0x06,  /* Test signal */
    ADS_INPUT_RBIAS_DR2 = 0x07,  /* Rbias + DC block (alt) */
} ads_input_t;

/* Data sample structure (per channel) */
typedef struct {
    uint8_t  status[3];    /* 24-bit status word */
    int32_t  channels[8];  /* 24-bit signed channel data (sign-extended) */
} ads_sample_t;

/* Device state */
typedef struct {
    uint8_t  id[ADS_NUM_DEVICES];
    uint8_t  gain[ADS_NUM_DEVICES][8];  /* Current gain per channel */
    uint32_t sample_rate;   /* Current SPS */
    uint8_t  is_running;
} ads_state_t;

/* Public API */
void     ads1299_init(void);
void     ads1299_config_all(void);
void     ads1299_start(void);
void     ads1299_stop(void);
void     ads1299_reset(void);
uint8_t  ads1299_read_reg(uint8_t dev, uint8_t reg);
void     ads1299_write_reg(uint8_t dev, uint8_t reg, uint8_t val);
void     ads1299_set_gain(uint8_t dev, uint8_t ch, ads_gain_t gain);
void     ads1299_set_input(uint8_t dev, uint8_t ch, ads_input_t input);
void     ads1299_set_sample_rate(uint32_t sps);
void     ads1299_read_frame(uint8_t *buf);
void     ads1299_parse_frame(const uint8_t *buf, ads_sample_t *samples);
void     ads1299_impedance_measure(uint8_t dev, uint8_t ch);
void     ads1299_enable_bias(uint8_t dev_mask, uint8_t ch_mask_p, uint8_t ch_mask_n);

/* Low-level SPI helpers */
static inline void ads_cs_assert(void);
static inline void ads_cs_release(void);
static inline uint8_t ads_spi_xfer(uint8_t data);

#endif /* ADS1299_H */