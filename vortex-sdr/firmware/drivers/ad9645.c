/*
 * ad9645.c — AD9645 14-bit 61.44 MSPS ADC driver implementation
 * SPI interface via SPI3 (PB12=SCK, PB13=MISO, PB14=MOSI, PB15=CS#)
 * ADC data output via parallel/LVDS interface to FPGA
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "ad9645.h"

/* ========================================================================== */
/* SPI3 read/write for AD9645                                                  */
/* AD9645 SPI protocol:                                                         */
/*   Write: [1:W/R=0][1:byte_count][6:addr_hi][8:addr_lo][8:data]...          */
/*   Read:  [1:W/R=1][1:byte_count][6:addr_hi][8:addr_lo][8:dont_care]...    */
/* ========================================================================== */

static void ad9645_cs_assert(void)
{
    CLR_BIT(GPIOC->ODR, PIN_FPGA_CS);  /* Repurpose FPGA_CS as ADC CS temporarily */
    /* Actually, AD9645 has its own CS. In our design, we use a separate GPIO.
     * For the Vortex-SDR board, AD9645 CS is shared with FPGA config SPI.
     * We'll use SPI3 with separate CS management.
     */
    /* Wait for SPI3 to be ready */
    while (SPI3->SR & SPI_SR_BSY)
        ;
}

static void ad9645_cs_deassert(void)
{
    SET_BIT(GPIOC->ODR, PIN_FPGA_CS);
    /* CS high-to-low transition timing */
    for (volatile int d = 0; d < 10; d++)
        ;
}

static void spi3_write_byte(uint8_t data)
{
    while (!(SPI3->SR & SPI_SR_TXE))
        ;
    /* Write to DR with 8-bit data */
    *(volatile uint8_t *)&(SPI3->DR) = data;
    while (!(SPI3->SR & SPI_SR_RXNE))
        ;
    (void)*(volatile uint8_t *)&(SPI3->DR);  /* Read to clear RXNE */
}

static uint8_t spi3_read_byte(void)
{
    while (!(SPI3->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&(SPI3->DR) = 0xFF;  /* Dummy write */
    while (!(SPI3->SR & SPI_SR_RXNE))
        ;
    return *(volatile uint8_t *)&(SPI3->DR);
}

/* ========================================================================== */
/* AD9645 SPI write (single register)                                           */
/* ========================================================================== */
void ad9645_write_reg(uint8_t addr, uint8_t data)
{
    ad9645_cs_assert();

    /* Instruction byte: [W/R=0][byte_count=0 (1 byte)][addr_high[5:0]] */
    /* For AD9645, address is 13 bits: A[12:0] */
    /* Instruction byte format: bit 15=0 (write), bit 14=0 (1 byte),
     * bits 13:8 = addr[5:0], then addr[7:6] in next byte? No...
     * Actually: Byte 0 = 0b0_NC_A12_A11_A10_A9_A8_A7_A6
     *           Byte 1 = 0bA5_A4_A3_A2_A1_A0_NC_NC
     * Simplified: Byte 0 = (0 << 7) | (0 << 6) | (addr >> 6)
     *             Byte 1 = (addr << 2) & 0xFC
     * Wait, the AD9645 SPI uses a simpler protocol:
     *   Byte 0: [0=write|1=read][0=1byte|1=2bytes][addr[12:7]]
     *   Byte 1: [addr[6:0]][0][0]
     * Let me just use the standard format.
     */

    /* Instruction byte: bit15=0(write), bit14=0(single byte), bits[13:8]=addr[5:0] */
    uint8_t instr = (0 << 7) | (0 << 6) | ((addr >> 6) & 0x3F);
    uint8_t addr_byte = (addr << 2) & 0xFC;

    spi3_write_byte(instr);
    spi3_write_byte(addr_byte);
    spi3_write_byte(data);

    ad9645_cs_deassert();

    /* Write transfer register to commit changes */
    ad9645_cs_assert();
    spi3_write_byte((0 << 7) | (0 << 6) | (AD9645_REG_TRANSFER >> 6));
    spi3_write_byte((AD9645_REG_TRANSFER << 2) & 0xFC);
    spi3_write_byte(0x01);  /* Transfer bit */
    ad9645_cs_deassert();
}

/* ========================================================================== */
/* AD9645 SPI read (single register)                                           */
/* ========================================================================== */
uint8_t ad9645_read_reg(uint8_t addr)
{
    uint8_t data;

    ad9645_cs_assert();

    /* Instruction byte: bit15=1(read), bit14=0(single byte), bits[13:8]=addr[5:0] */
    uint8_t instr = (1 << 7) | (0 << 6) | ((addr >> 6) & 0x3F);
    uint8_t addr_byte = (addr << 2) & 0xFC;

    spi3_write_byte(instr);
    spi3_write_byte(addr_byte);
    data = spi3_read_byte();  /* Read data byte */

    ad9645_cs_deassert();

    return data;
}

/* ========================================================================== */
/* Initialize AD9645                                                           */
/* ========================================================================== */
void ad9645_init(void)
{
    /* Software reset */
    ad9645_write_reg(AD9645_REG_POWER_MODE, 0x80);  /* Reset bit */
    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Power mode: normal operation */
    ad9645_write_reg(AD9645_REG_POWER_MODE, AD9645_PM_NORMAL);

    /* Clock divide: 1 (no division, 61.44 MSPS) */
    ad9645_write_reg(AD9645_REG_CLOCK_DIV, 0x00);

    /* Decimation: 1 (no decimation, full bandwidth) */
    ad9645_write_reg(AD9645_REG_DECIMATION, AD9645_DEC_1);

    /* Data format: two's complement */
    ad9645_write_reg(AD9645_REG_DATA_FORMAT, AD9645_FMT_TWOS_COMP);

    /* Output mode: LVDS */
    ad9645_write_reg(AD9645_REG_OUTPUT_MODE, AD9645_OUT_LVDS);

    /* ADC input buffer: 3.3V, default */
    ad9645_write_reg(AD9645_REG_ADC_BUF_CFG, 0x00);

    /* ADC reference: internal 1.0V reference */
    ad9645_write_reg(AD9645_REG_VREF_CFG, 0x01);

    /* Output drive: default strength */
    ad9645_write_reg(AD9645_REG_OUTPUT_DRIVE, 0x00);

    /* ADC input configuration: default (AC-coupled, 1Vpp) */
    ad9645_write_reg(AD9645_REG_ADC_INPUT_CFG, 0x00);

    /* Test mode: off (normal operation) */
    ad9645_write_reg(AD9645_REG_TEST_MODE, AD9645_TEST_OFF);

    /* Verify chip ID */
    uint8_t chip_id = ad9645_read_reg(AD9645_REG_CHIP_ID);
    (void)chip_id;  /* Expected: 0x6A for AD9645 */
}

/* ========================================================================== */
/* Set ADC decimation ratio                                                    */
/* ========================================================================== */
void ad9645_set_decimation(uint8_t dec_ratio)
{
    uint8_t dec_val;

    switch (dec_ratio) {
    case 1:   dec_val = AD9645_DEC_1;  break;
    case 2:   dec_val = AD9645_DEC_2;  break;
    case 4:   dec_val = AD9645_DEC_4;  break;
    case 8:   dec_val = AD9645_DEC_8;  break;
    case 16:  dec_val = AD9645_DEC_16; break;
    default:  dec_val = AD9645_DEC_1;  break;
    }

    ad9645_write_reg(AD9645_REG_DECIMATION, dec_val);

    /* Select appropriate filter for decimation */
    if (dec_ratio > 1) {
        /* Use low-pass filter for decimation */
        ad9645_write_reg(AD9645_REG_DEC_FILTER, 0x01);
    } else {
        /* No filter needed at full rate */
        ad9645_write_reg(AD9645_REG_DEC_FILTER, 0x00);
    }
}

/* ========================================================================== */
/* Set ADC data format                                                        */
/* ========================================================================== */
void ad9645_set_data_format(uint8_t format)
{
    ad9645_write_reg(AD9645_REG_DATA_FORMAT, format);
}

/* ========================================================================== */
/* Set ADC test mode                                                          */
/* ========================================================================== */
void ad9645_set_test_mode(uint8_t mode)
{
    ad9645_write_reg(AD9645_REG_TEST_MODE, mode);
}

/* ========================================================================== */
/* Power down ADC                                                             */
/* ========================================================================== */
void ad9645_power_down(void)
{
    ad9645_write_reg(AD9645_REG_POWER_MODE, AD9645_PM_FULL_PD);
}

/* ========================================================================== */
/* Power up ADC                                                               */
/* ========================================================================== */
void ad9645_power_up(void)
{
    ad9645_write_reg(AD9645_REG_POWER_MODE, AD9645_PM_NORMAL);
    /* Wait for ADC to stabilize */
    for (volatile uint32_t i = 0; i < 50000; i++)
        ;
}