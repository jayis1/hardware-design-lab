/*
 * NeuroLink — ADS1299 24-bit ADC Driver Implementation
 * Drives 8 ADS1299 devices in daisy-chain mode via SPI1+DMA
 */

#include "ads1299.h"
#include "board.h"
#include "registers.h"

static ads_state_t ads_state;

/* ============================================================
 * SPI1 Chip Select & Transfer Helpers
 * ============================================================ */

static inline void ads_cs_assert(void)
{
    GPIO_BSRR(GPIOA) |= (1 << (ADS_CS_PIN + 16));  /* PA4 low */
}

static inline void ads_cs_release(void)
{
    GPIO_BSRR(GPIOA) |= (1 << ADS_CS_PIN);  /* PA4 high */
}

static inline uint8_t ads_spi_xfer(uint8_t data)
{
    /* Wait for TX empty */
    while (!(SPI1_SR & (1 << 1)));   /* TXE bit */
    *((volatile uint8_t *)&SPI1_DR) = data;
    /* Wait for RX not empty */
    while (!(SPI1_SR & (1 << 0)));   /* RXNE bit */
    return *((volatile uint8_t *)&SPI1_DR);
}

/* ============================================================
 * Initialize ADS1299 Daisy Chain
 * ============================================================ */
void ads1299_init(void)
{
    /* Ensure START pin is low */
    GPIO_BSRR(GPIOA) |= (1 << (ADS_START_PIN + 16));  /* START low */

    /* Assert reset for 18 µs (tRESET) */
    GPIO_BSRR(GPIOE) |= (1 << (ADS_RESET_PIN + 16));  /* RESET low */
    for (volatile int i = 0; i < 200; i++);  /* ~18 µs at 480 MHz */
    GPIO_BSRR(GPIOE) |= (1 << ADS_RESET_PIN);  /* RESET high */

    /* Wait 2 ms for internal reference to settle */
    for (volatile int i = 0; i < 200000; i++);

    /* Read device IDs to verify daisy chain */
    for (int dev = 0; dev < ADS_NUM_DEVICES; dev++) {
        ads_state.id[dev] = ads1299_read_reg(dev, ADS_REG_ID);
    }
}

/* ============================================================
 * Configure All ADS1299 Devices
 * ============================================================ */
void ads1299_config_all(void)
{
    for (int dev = 0; dev < ADS_NUM_DEVICES; dev++) {
        /* Stop continuous data mode for configuration */
        ads_cs_assert();
        ads_spi_xfer(ADS_CMD_SDATAC);
        ads_cs_release();
        for (volatile int i = 0; i < 100; i++);

        /* CONFIG1: Daisy-chain mode, 250 SPS, internal clock */
        ads1299_write_reg(dev, ADS_REG_CONFIG1, 0x96);
        /* Bit 6: DAISY_EN = 1 (daisy chain)
         * Bit 5: CLK_EN = 0 (internal clock, external pin)
         * Bits 2-0: DR = 110 (250 SPS)
         */

        /* CONFIG2: Internal reference, 2.4V bias */
        ads1299_write_reg(dev, ADS_REG_CONFIG2, 0xC0);
        /* Bit 7: INT_TEST = 0
         * Bits 6-5: REF = 11 (internal reference enabled, 2.4V)
         */

        /* CONFIG3: Bias measurement enabled, BIASREF = internal */
        ads1299_write_reg(dev, ADS_REG_CONFIG3, 0xEC);
        /* Bit 7: PDB = 1 (bias amplifier on)
         * Bit 6: BIAS_MEAS = 1 (bias measurement enabled)
         * Bit 5: BIAS_REF_INT = 1 (internal bias reference)
         * Bits 4-2: BIAS_LOFF = 110
         * Bit 0: BIAS_STAT = 0
         */

        /* CONFIG4: Single-shot mode off, continuous mode */
        ads1299_write_reg(dev, ADS_REG_CONFIG4, 0x00);

        /* Configure all 8 channels: gain=6, normal electrode input */
        for (int ch = 0; ch < 8; ch++) {
            uint8_t ch_reg = ADS_REG_CH1SET + ch;
            ads1299_write_reg(dev, ch_reg, 0x60);
            /* Bit 7: PD = 0 (power on)
             * Bits 6-3: GAIN = 0110 (gain = 6)
             * Bits 2-0: MUX = 000 (normal electrode input)
             */
            ads_state.gain[dev][ch] = ADS_GAIN_6;
        }

        /* Enable bias for all channels (both positive and negative) */
        ads1299_write_reg(dev, ADS_REG_BIAS_SENSP, 0xFF);
        ads1299_write_reg(dev, ADS_REG_BIAS_SENSN, 0xFF);

        /* GPIO: outputs for impedance measurement MUX */
        ads1299_write_reg(dev, ADS_REG_GPIO, 0x00);
    }

    ads_state.sample_rate = 250;
    ads_state.is_running = 0;
}

/* ============================================================
 * Read Single Register from Specific Device
 * In daisy chain, we shift through all devices
 * ============================================================ */
uint8_t ads1299_read_reg(uint8_t dev, uint8_t reg)
{
    uint8_t val;
    ads_cs_assert();

    /* Send RREG command: 001r rrrr where rrrr = register address */
    ads_spi_xfer(ADS_CMD_RREG | (reg & 0x1F));
    ads_spi_xfer(0);  /* Number of registers - 1 = 0 (1 register) */

    /* Clock through all 8 devices, read the one we want */
    for (int i = 0; i < ADS_NUM_DEVICES; i++) {
        val = ads_spi_xfer(0x00);
    }

    ads_cs_release();
    return val;
}

/* ============================================================
 * Write Register to All Devices (broadcast)
 * For per-device access, use SDATAC + individual WREG
 * ============================================================ */
void ads1299_write_reg(uint8_t dev, uint8_t reg, uint8_t val)
{
    ads_cs_assert();

    /* Send WREG command: 010r rrrr where rrrr = register address */
    ads_spi_xfer(ADS_CMD_WREG | (reg & 0x1F));
    ads_spi_xfer(0);  /* Number of registers - 1 */

    /* In daisy chain, we must shift data through all devices */
    for (int i = ADS_NUM_DEVICES - 1; i >= 0; i--) {
        if (i == dev) {
            ads_spi_xfer(val);
        } else {
            ads_spi_xfer(0x00);  /* NOP for other devices */
        }
    }

    ads_cs_release();
    for (volatile int i = 0; i < 50; i++);  /* tCSCH > 4 TCLK */
}

/* ============================================================
 * Set Channel Gain
 * ============================================================ */
void ads1299_set_gain(uint8_t dev, uint8_t ch, ads_gain_t gain)
{
    if (dev >= ADS_NUM_DEVICES || ch >= 8) return;
    uint8_t reg_val = (gain << 3) | 0x00;  /* MUX = normal input */
    ads1299_write_reg(dev, ADS_REG_CH1SET + ch, reg_val);
    ads_state.gain[dev][ch] = gain;
}

/* ============================================================
 * Set Channel Input Configuration
 * ============================================================ */
void ads1299_set_input(uint8_t dev, uint8_t ch, ads_input_t input)
{
    if (dev >= ADS_NUM_DEVICES || ch >= 8) return;
    uint8_t gain = ads_state.gain[dev][ch];
    uint8_t reg_val = (gain << 3) | (input & 0x07);
    ads1299_write_reg(dev, ADS_REG_CH1SET + ch, reg_val);
}

/* ============================================================
 * Set Sample Rate (all devices)
 * ============================================================ */
void ads1299_set_sample_rate(uint32_t sps)
{
    uint8_t dr_bits;
    switch (sps) {
        case 250:   dr_bits = 0x06; break;
        case 500:   dr_bits = 0x05; break;
        case 1000:  dr_bits = 0x04; break;
        case 2000:  dr_bits = 0x03; break;
        case 4000:  dr_bits = 0x02; break;
        case 8000:  dr_bits = 0x01; break;
        case 16000: dr_bits = 0x00; break;
        default:    dr_bits = 0x04; break;  /* Default 1 kSPS */
    }

    for (int dev = 0; dev < ADS_NUM_DEVICES; dev++) {
        uint8_t config1 = 0x90 | (dr_bits & 0x07);
        /* Bit 6: DAISY_EN = 1 */
        config1 |= 0x40;
        ads1299_write_reg(dev, ADS_REG_CONFIG1, config1);
    }
    ads_state.sample_rate = sps;
}

/* ============================================================
 * Start Continuous Conversion
 * ============================================================ */
void ads1299_start(void)
{
    /* Enable RDATAC mode */
    ads_cs_assert();
    ads_spi_xfer(ADS_CMD_RDATAC);
    ads_cs_release();

    /* Assert START pin */
    GPIO_BSRR(GPIOA) |= (1 << ADS_START_PIN);  /* START high */
    ads_state.is_running = 1;
}

/* ============================================================
 * Stop Continuous Conversion
 * ============================================================ */
void ads1299_stop(void)
{
    /* De-assert START pin */
    GPIO_BSRR(GPIOA) |= (1 << (ADS_START_PIN + 16));  /* START low */

    /* Send STOP command */
    ads_cs_assert();
    ads_spi_xfer(ADS_CMD_STOP);
    ads_cs_release();

    /* Send SDATAC to exit continuous mode */
    ads_cs_assert();
    ads_spi_xfer(ADS_CMD_SDATAC);
    ads_cs_release();

    ads_state.is_running = 0;
}

/* ============================================================
 * Hardware Reset
 * ============================================================ */
void ads1299_reset(void)
{
    ads1299_stop();
    GPIO_BSRR(GPIOE) |= (1 << (ADS_RESET_PIN + 16));  /* RESET low */
    for (volatile int i = 0; i < 200; i++);  /* 18 µs */
    GPIO_BSRR(GPIOE) |= (1 << ADS_RESET_PIN);  /* RESET high */
    for (volatile int i = 0; i < 200000; i++);  /* 2 ms */
    ads1299_config_all();
}

/* ============================================================
 * Read Full Frame (216 bytes) via SPI with DMA
 * ============================================================ */
void ads1299_read_frame(uint8_t *buf)
{
    /* Wait for DRDY low (data ready) — already handled by EXTI */
    /* Read 216 bytes from daisy chain */

    ads_cs_assert();

    /* In RDATAC mode, clock out 216 bytes (8 devices × 27 bytes each) */
    for (int i = 0; i < ADS_TOTAL_FRAME; i++) {
        buf[i] = ads_spi_xfer(0x00);
    }

    ads_cs_release();
}

/* ============================================================
 * Parse Raw Frame into Sample Structures
 * ============================================================ */
void ads1299_parse_frame(const uint8_t *buf, ads_sample_t *samples)
{
    for (int dev = 0; dev < ADS_NUM_DEVICES; dev++) {
        const uint8_t *frame = buf + (dev * ADS_FRAME_PER_DEV);
        ads_sample_t *s = &samples[dev];

        /* 24-bit status word (3 bytes) */
        s->status[0] = frame[0];
        s->status[1] = frame[1];
        s->status[2] = frame[2];

        /* 8 channels, each 24-bit signed (3 bytes, MSB first) */
        for (int ch = 0; ch < 8; ch++) {
            int offset = 3 + (ch * 3);
            int32_t raw = ((int32_t)frame[offset] << 16) |
                          ((int32_t)frame[offset + 1] << 8) |
                          ((int32_t)frame[offset + 2]);
            /* Sign-extend from 24-bit to 32-bit */
            if (raw & 0x800000) {
                raw |= 0xFF000000;
            }
            s->channels[ch] = raw;
        }
    }
}

/* ============================================================
 * Impedance Measurement Mode
 * ============================================================ */
void ads1299_impedance_measure(uint8_t dev, uint8_t ch)
{
    if (dev >= ADS_NUM_DEVICES || ch >= 8) return;

    /* Switch channel to test signal input for calibration */
    ads1299_set_input(dev, ch, ADS_INPUT_TEST);

    /* Configure internal test signal */
    ads1299_write_reg(dev, ADS_REG_CONFIG2, 0xD0);
    /* INT_TEST = 1, REF = 10 (2.4V), test signal = 1×, 1 Hz square */

    /* After measurement, restore */
    /* (In real implementation, we'd read data, compute impedance,
     * then restore normal operation) */
}

/* ============================================================
 * Enable Bias for Lead-Off Detection
 * ============================================================ */
void ads1299_enable_bias(uint8_t dev_mask, uint8_t ch_mask_p, uint8_t ch_mask_n)
{
    for (int dev = 0; dev < ADS_NUM_DEVICES; dev++) {
        if (dev_mask & (1 << dev)) {
            ads1299_write_reg(dev, ADS_REG_BIAS_SENSP, ch_mask_p);
            ads1299_write_reg(dev, ADS_REG_BIAS_SENSN, ch_mask_n);
        }
    }
}