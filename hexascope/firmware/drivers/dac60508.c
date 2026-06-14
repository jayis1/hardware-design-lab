/*
 * dac60508.c — DAC60508 8-Channel 12-bit DAC Driver for HexaScope
 * Controls comparator thresholds and analog calibration via SPI2
 *
 * Channel assignments:
 *   CH0: CMP1_THRESH_P — Comparator 1 positive threshold
 *   CH1: CMP1_THRESH_N — Comparator 1 negative threshold
 *   CH2: CMP2_THRESH_P — Comparator 2 positive threshold
 *   CH3: CMP2_THRESH_N — Comparator 2 negative threshold
 *   CH4: ANA_CAL_REF   — Analog calibration reference
 *   CH5: ANA_OFFSET_CH1 — Channel 1 offset adjust
 *   CH6: ANA_OFFSET_CH2 — Channel 2 offset adjust
 *   CH7: ANA_GAIN_ADJ   — Gain adjustment reference
 */

#include "board.h"
#include "registers.h"

/* ============================================================
 * SPI2 Transfer (with CS_N manual control via PB2)
 * SPI2 is configured as Mode 1 (CPOL=0, CPHA=1)
 * ============================================================ */

static void dac_cs_assert(void)
{
    GPIOB->ODR &= ~(1U << SPI2_NSS_PIN);  /* CS_N = low */
    /* Small delay for CS setup time (50 ns min) */
    for (volatile uint32_t i = 0; i < 5; i++)
        ;
}

static void dac_cs_deassert(void)
{
    /* Small delay for CS hold time (10 ns min) */
    for (volatile uint32_t i = 0; i < 5; i++)
        ;
    GPIOB->ODR |= (1U << SPI2_NSS_PIN);   /* CS_N = high */
}

static uint8_t spi2_transfer(uint8_t tx_data)
{
    /* Wait until TXE flag is set */
    uint32_t timeout = 10000;
    while (!(SPI2->SR & SPI_SR_TXE) && timeout--)
        ;

    /* Send data */
    SPI2->DR = tx_data;

    /* Wait until RXNE flag is set */
    timeout = 10000;
    while (!(SPI2->SR & SPI_SR_RXNE) && timeout--)
        ;

    /* Read received data */
    uint8_t rx_data = (uint8_t)SPI2->DR;

    /* Wait until BSY flag is reset */
    timeout = 10000;
    while ((SPI2->SR & SPI_SR_BSY) && timeout--)
        ;

    return rx_data;
}

/* ============================================================
 * DAC60508 24-bit SPI Frame Format
 *
 * Frame: [8-bit command] [8-bit MSB data] [8-bit LSB data]
 *
 * Command byte format:
 *   Bits [7:6]: Channel address (for single-channel writes)
 *     00 = CH0, 01 = CH1, 10 = CH2, 11 = CH3
 *     (For register writes, these are part of the register address)
 *   Bit [5]: 0 = Write, 1 = Read
 *   Bits [4:0]: Register address (for broadcast, sync, etc.)
 *
 * DAC data format:
 *   Bits [15:4]: 12-bit DAC value
 *   Bits [3:0]:  Don't care (set to 0)
 * ============================================================ */

/* Write a DAC channel register */
static void dac_write_reg(uint8_t reg, uint16_t value)
{
    dac_cs_assert();

    /* 24-bit frame: [reg_addr + command] [data_msb] [data_lsb] */
    spi2_transfer(reg & 0x7F);           /* Command: write, register address */
    spi2_transfer((value >> 8) & 0xFF);  /* Data MSB */
    spi2_transfer(value & 0xFF);          /* Data LSB */

    dac_cs_deassert();
}

/* Read a DAC channel register */
static uint16_t dac_read_reg(uint8_t reg)
{
    uint16_t value;
    uint8_t msb, lsb;

    dac_cs_assert();

    /* Send read command */
    spi2_transfer(reg | 0x80);  /* Read bit set */
    spi2_transfer(0x00);          /* Dummy TX for RX */
    msb = spi2_transfer(0x00);   /* Dummy TX for RX */
    lsb = spi2_transfer(0x00);

    dac_cs_deassert();

    value = ((uint16_t)msb << 8) | lsb;
    return value;
}

/* ============================================================
 * DAC60508 Public API
 * ============================================================ */

void dac60508_init(void)
{
    /* Make sure CS_N is initially high */
    GPIOB->ODR |= (1U << SPI2_NSS_PIN);

    /* Software reset the DAC */
    dac_write_reg(DAC60508_REG_SYNC, 0x0000);
    for (volatile uint32_t i = 0; i < 1000; i++)
        ;

    /* Configure DAC:
     * - Internal 2.5V reference
     * - Power-up all channels
     * - No software trigger (transparent mode)
     * - 2x gain disabled (output range 0–2.5V with 2.5V ref)
     *    With 2x gain: 0–5V output (matches LT1715 comparator range)
     *    Enable 2x gain for 0–5V output range
     */
    dac_write_reg(DAC60508_REG_CONFIG, 0x0000);  /* Reset config */
    dac_write_reg(DAC60508_REG_GAIN, 0x0002);      /* Enable 2x gain on all channels */
    dac_write_reg(DAC60508_REG_TRIGGER, 0x0001);   /* Software trigger: load all */

    /* Set all channels to mid-scale (2048 = ~1.25V → ~2.5V with 2x gain) */
    for (int ch = 0; ch < 8; ch++) {
        dac_set_channel(ch, 2048);
    }

    /* Set LDAC (PB2) low momentarily to update outputs */
    /* Note: LDAC is shared with SPI2_NSS; we use the trigger register instead */
    dac_write_reg(DAC60508_REG_TRIGGER, 0x0001);
}

void dac_set_channel(uint8_t ch, uint16_t value)
{
    /* Clamp value to 12-bit range */
    if (value > 4095)
        value = 4095;

    /* Shift 12-bit value to 16-bit format (left-aligned) */
    uint16_t dac_val = value << 4;

    /* Write to the appropriate channel register */
    /* Channel registers: CH0=0x08, CH1=0x09, ..., CH7=0x0F */
    uint8_t reg = DAC60508_REG_DAC_CH0 + ch;
    dac_write_reg(reg, dac_val);

    /* Trigger update */
    dac_write_reg(DAC60508_REG_TRIGGER, 0x0001);
}

uint16_t dac_get_channel(uint8_t ch)
{
    if (ch > 7)
        return 0;

    uint8_t reg = DAC60508_REG_DAC_CH0 + ch;
    uint16_t raw = dac_read_reg(reg);
    return raw >> 4;  /* Right-align to get 12-bit value */
}

/* Set comparator threshold voltage for a digital channel
 * voltage: desired threshold in millivolts (0–5000 mV)
 * channel: 0 for D1, 1 for D2
 * polarity: 0 for positive threshold, 1 for negative threshold
 */
void dac_set_threshold(uint8_t channel, uint8_t polarity, uint16_t voltage_mv)
{
    /* DAC output = voltage_mv / 5000 * 4095 (with 2x gain, VREF=2.5V) */
    uint16_t dac_val;
    if (voltage_mv > 5000)
        voltage_mv = 5000;
    dac_val = (uint16_t)((uint32_t)voltage_mv * 4095 / 5000);

    uint8_t ch;
    if (channel == 0) {
        ch = polarity ? 1 : 0;  /* CH0: D1+, CH1: D1- */
    } else {
        ch = polarity ? 3 : 2;  /* CH2: D2+, CH3: D2- */
    }

    dac_set_channel(ch, dac_val);
}

/* Set analog offset for a specific analog channel
 * offset_mv: offset voltage in millivolts (-2500 to +2500 mV)
 * channel: analog channel 1-4
 */
void dac_set_analog_offset(uint8_t channel, int16_t offset_mv)
{
    uint8_t ch;

    /* Offset channels: CH4=cal_ref, CH5=offset_CH1, CH6=offset_CH2, CH7=gain */
    switch (channel) {
    case 1:  ch = 5; break;  /* ANA_OFFSET_CH1 */
    case 2:  ch = 6; break;  /* ANA_OFFSET_CH2 */
    default: return;
    }

    /* Convert offset to DAC value (mid-scale = 0 offset) */
    uint16_t dac_val = 2048 + (uint16_t)((int32_t)offset_mv * 2048 / 2500);
    if (dac_val > 4095) dac_val = 4095;

    dac_set_channel(ch, dac_val);
}

/* Set gain adjustment for analog channels */
void dac_set_gain_adjust(uint16_t gain_code)
{
    /* gain_code: 0-4095, where 2048 = 1.0x gain */
    if (gain_code > 4095)
        gain_code = 4095;
    dac_set_channel(7, gain_code);  /* CH7: ANA_GAIN_ADJ */
}

/* Get device ID (should return 0x084 for DAC60508) */
uint16_t dac_get_device_id(void)
{
    return dac_read_reg(DAC60508_REG_DEVICE_ID);
}

/* Get status register */
uint16_t dac_get_status(void)
{
    return dac_read_reg(DAC60508_REG_STATUS);
}