/*
 * NeuroLink — iCE40UP5K FPGA SPI Driver
 * SPI2 interface for bitstream loading and DSP data exchange
 */

#ifndef ICE40_SPI_H
#define ICE40_SPI_H

#include <stdint.h>

/* FPGA SPI Commands (sent as first byte after CS assert) */
#define ICE40_CMD_NOP          0x00
#define ICE40_CMD_READ_ID      0x01
#define ICE40_CMD_READ_STATUS  0x02
#define ICE40_CMD_WRITE_CONFIG 0x03
#define ICE40_CMD_READ_DATA    0x04
#define ICE40_CMD_WRITE_DATA   0x05
#define ICE40_CMD_SET_FILTER   0x06
#define ICE40_CMD_GET_FEATURES 0x07
#define ICE40_CMD_RESET_PIPE   0x08
#define ICE40_CMD_SET_GAIN     0x09
#define ICE40_CMD_IMPEDANCE    0x0A

/* FPGA Status Register Bits */
#define ICE40_STATUS_CFG_DONE   (1 << 0)
#define ICE40_STATUS_DSP_ACTIVE (1 << 1)
#define ICE40_STATUS_DATA_READY (1 << 2)
#define ICE40_STATUS_OVERRUN    (1 << 3)
#define ICE40_STATUS_ERROR      (1 << 7)

/* Filter configuration */
typedef struct {
    uint8_t  filter_type;    /* 0=bandpass, 1=notch, 2=lowpass, 3=highpass */
    uint8_t  order;          /* 1–4 (IIR filter order) */
    uint16_t cutoff_low;     /* Low cutoff frequency (0.01 Hz units) */
    uint16_t cutoff_high;    /* High cutoff frequency (0.01 Hz units) */
    uint16_t sample_rate;    /* Sample rate in Hz */
} ice40_filter_t;

/* Feature extraction mask */
typedef struct {
    uint8_t  channel_mask;    /* Which channels to extract features from (32-bit for 32 ch) */
    uint8_t  feature_mask;    /* Which features to compute */
    uint16_t window_size;     /* Window size in samples */
} ice40_feature_config_t;

/* Feature mask bits */
#define FEAT_RMS    (1 << 0)
#define FEAT_MAV    (1 << 1)
#define FEAT_WL     (1 << 2)
#define FEAT_SSC    (1 << 3)
#define FEAT_ZC     (1 << 4)
#define FEAT_VAR    (1 << 5)

/* Processed sample structure from FPGA */
typedef struct {
    uint32_t timestamp;        /* Sample timestamp (ms) */
    int16_t  filtered[32];     /* Filtered channel data */
    uint16_t features[32];     /* Feature values (RMS, etc.) */
    uint8_t  trigger_mask;     /* Which channels triggered */
    uint8_t  impedance[32];    /* Impedance status flags */
} ice40_processed_t;

/* Public API */
int      ice40_load_bitstream(void);
void     ice40_reset_assert(void);
void     ice40_reset_release(void);
uint8_t  ice40_is_config_done(void);
uint8_t  ice40_read_id(void);
uint8_t  ice40_read_status(void);
void     ice40_send_samples(const uint8_t *data, uint16_t len);
void     ice40_read_processed(uint8_t *buf, uint16_t len);
void     ice40_set_filter(const ice40_filter_t *filt);
void     ice40_set_feature_config(const ice40_feature_config_t *cfg);
void     ice40_reset_pipeline(void);
uint16_t ice40_crc16(const uint8_t *data, uint16_t len);

/* Low-level SPI helpers */
static inline void ice40_cs_assert(void);
static inline void ice40_cs_release(void);
static inline uint8_t ice40_spi_xfer(uint8_t data);

#endif /* ICE40_SPI_H */