/*
 * imu.c — ICM-42688-P 6-axis IMU driver (SPI, bare-metal)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Full bit-banged-over-SPI-peripheral driver for the ICM-42688-P.
 * The sensor is a 6-axis device (3-axis accelerometer + 3-axis gyroscope)
 * with a 2 kB FIFO. GlyphFlow samples it at 1 kHz during active writing and
 * uses the wake-on-motion (APEX) feature to wake the MCU from STOP2.
 */
#include "imu.h"
#include "../registers.h"
#include "../board.h"

/* ---- SPI low-level ------------------------------------------------ */
static void spi_init(void)
{
    /* Enable SPI1 + GPIOA clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA1 = CS (GPIO output, high idle) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_IMU_CS_BIT*2))) | (GPIO_MODE_OUT << (PIN_IMU_CS_BIT*2));
    GPIO_SET(PIN_IMU_CS_PORT, PIN_IMU_CS_BIT);

    /* PA5,6,7 = SCK,MISO,MOSI alt fn 5 */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (5*2))) | (GPIO_MODE_AF << (5*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (6*2))) | (GPIO_MODE_AF << (6*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (7*2))) | (GPIO_MODE_AF << (7*2));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFU << (5*4))) | (5U << (5*4));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFU << (6*4))) | (5U << (6*4));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFU << (7*4))) | (5U << (7*4));

    /* PA5 high speed to reduce SCK ringing */
    GPIOA->OSPEEDR |= (3U << (5*2));

    /* Configure SPI1: master, CPOL=0 CPHA=0, 8-bit, baud = PCLK/8 = 10 MHz */
    SPI1->CR1 = 0;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (2U << SPI_CR1_BR_SHIFT);
    SPI1->CR2 = (7U << SPI_CR2_DS_SHIFT) | SPI_CR2_FRXTH | SPI_CR2_NSSP;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static inline void spi_cs_low(void)
{
    GPIO_CLR(PIN_IMU_CS_PORT, PIN_IMU_CS_BIT);
}

static inline void spi_cs_high(void)
{
    GPIO_SET(PIN_IMU_CS_PORT, PIN_IMU_CS_BIT);
}

static uint8_t spi_xfer(uint8_t tx)
{
    /* TXE is set when TXFIFO has room; RXNE when a byte is available. */
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI1->DR;
}

static void spi_read_burst(uint8_t *buf, uint8_t n)
{
    for (uint8_t i = 0; i < n; i++) {
        while (!(SPI1->SR & SPI_SR_TXE)) { }
        *(volatile uint8_t *)&SPI1->DR = 0x00;
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
        buf[i] = *(volatile uint8_t *)&SPI1->DR;
    }
}

/* ---- Register access ---------------------------------------------- */
int imu_read_register(uint8_t reg, uint8_t *val)
{
    spi_cs_low();
    spi_xfer(reg | IMU_READ_BIT);
    *val = spi_xfer(0x00);
    spi_cs_high();
    return 0;
}

int imu_write_register(uint8_t reg, uint8_t val)
{
    spi_cs_high();
    spi_cs_low();
    spi_xfer(reg | IMU_WRITE_BIT);
    spi_xfer(val);
    spi_cs_high();
    return 0;
}

uint8_t imu_whoami(void)
{
    uint8_t v = 0;
    imu_read_register(IMU_WHO_AM_I, &v);
    return v;
}

int imu_clear_int1(void)
{
    uint8_t v;
    imu_read_register(IMU_INT_STATUS, &v);
    return 0;
}

int imu_disable_fifo(void)
{
    return imu_write_register(IMU_FIFO_CONFIG, 0x00);
}

int imu_enable_fifo(void)
{
    /* FIFO mode 1 (stream-to-full), accel+gyro packed. */
    imu_write_register(IMU_FIFO_CONFIG, 0x40);  /* FIFO mode stream */
    return 0;
}

/* ---- Initialization ----------------------------------------------- */
int imu_init(void)
{
    spi_init();
    /* Wait a few hundred µs for the IMU to come out of reset. */
    for (volatile int i = 0; i < 2000; i++) { }

    uint8_t who = imu_whoami();
    if (who != IMU_WHO_AM_I_VAL) {
        return -1;  /* not present or wrong device */
    }

    /* Software reset via PWR_MGMT0 bit[2:0] = 0b010 (software reset) */
    imu_write_register(IMU_PWR_MGMT0, 0x02);
    for (volatile int i = 0; i < 20000; i++) { }   /* ~1 ms */
    imu_write_register(IMU_PWR_MGMT0, 0x00);      /* back to off */
    for (volatile int i = 0; i < 20000; i++) { }

    return 0;
}

int imu_configure_1khz(void)
{
    /* Put accel and gyro into low-noise mode (LN), 1 kHz ODR, ±8 g, ±2000 dps. */
    imu_write_register(IMU_ACCEL_CONFIG0,
                       (IMU_ACCEL_FS_8G << 4) | (IMU_ODR_1KHZ << 0));
    imu_write_register(IMU_GYRO_CONFIG0,
                       (IMU_GYRO_FS_2000DPS << 4) | (IMU_ODR_1KHZ << 0));

    /* PWR_MGMT0: accel LN (0b011), gyro LN (0b011), temp on. */
    imu_write_register(IMU_PWR_MGMT0, 0x0F);

    /* Configure INT1 to fire on FIFO watermark. */
    imu_write_register(IMU_INT_CONFIG0,
                       (1U << 2)  /* FIFO_FULL_INT1_EN = 1 */
                       | (1U << 6) /* FIFO_WM_INT1_EN = 1 */
                       );
    imu_write_register(IMU_INT_SOURCE0,
                       (1U << 2)  /* FIFO_FULL_INT1 = 1 */
                       | (1U << 6) /* FIFO_WM_INT1 = 1 */
                       );

    imu_enable_fifo();
    return 0;
}

int imu_configure_wom(const imu_wom_config_t *cfg)
{
    /* Put accel into low-power wake-on-motion mode. The ICM-42688-P exposes
     * wake-on-motion via the APEX features; here we use a minimal config that
     * triggers INT1 when the acceleration magnitude exceeds `threshold_mg`
     * for at least `duration_ms`. */
    imu_write_register(IMU_ACCEL_CONFIG0,
                       (IMU_ACCEL_FS_8G << 4) | (IMU_ODR_12_5HZ << 0));
    /* Set WOM threshold (1 LSB ≈ 4 mg in this mode). */
    uint8_t thr = (uint8_t)(cfg->threshold_mg / 4);
    imu_write_register(0x4A, thr);   /* ACCEL_WOM_X_THR */
    imu_write_register(0x4B, thr);   /* ACCEL_WOM_Y_THR */
    imu_write_register(0x4C, thr);   /* ACCEL_WOM_Z_THR */

    /* WOM logic: AND of axes, on-reference mode. */
    imu_write_register(IMU_INT_SOURCE1, 0x07);

    /* Put accel into WOM mode (PWR_MGMT0 accel bits = 0b010 LP, 0b110 wom). */
    imu_write_register(IMU_PWR_MGMT0, 0x02);

    /* INT1 active low, latched. */
    imu_write_register(IMU_INT_CONFIG, (1U << 1) | (1U << 0));
    return 0;
}

/* ---- FIFO burst read ---------------------------------------------- */
int imu_read_fifo(imu_sample_t *out, uint8_t max_count, uint8_t *read_count)
{
    uint8_t hi, lo;
    imu_read_register(IMU_FIFO_COUNTH, &hi);
    imu_read_register(IMU_FIFO_COUNTL, &lo);
    uint16_t count = ((uint16_t)hi << 8) | lo;

    /* Each FIFO record is 16 bytes (header + 6×2 axes + timestamp + temperature). */
    const uint8_t record_bytes = 16;
    uint16_t records = count / record_bytes;
    if (records > max_count) records = max_count;

    spi_cs_low();
    spi_xfer(IMU_FIFO_DATA | IMU_READ_BIT);
    for (uint16_t r = 0; r < records; r++) {
        uint8_t buf[16];
        spi_read_burst(buf, record_bytes);
        /* Layout: [0]=header, [1..2]=ax, [3..4]=ay, [5..6]=az,
         *         [7..8]=gx, [9..10]=gy, [11..12]=gz, [13..14]=temp, [15]=ts */
        out[r].ax = (int16_t)(((uint16_t)buf[1]  << 8) | buf[2]);
        out[r].ay = (int16_t)(((uint16_t)buf[3]  << 8) | buf[4]);
        out[r].az = (int16_t)(((uint16_t)buf[5]  << 8) | buf[6]);
        out[r].gx = (int16_t)(((uint16_t)buf[7]  << 8) | buf[8]);
        out[r].gy = (int16_t)(((uint16_t)buf[9]  << 8) | buf[10]);
        out[r].gz = (int16_t)(((uint16_t)buf[11] << 8) | buf[12]);
    }
    spi_cs_high();

    *read_count = (uint8_t)records;
    return 0;
}