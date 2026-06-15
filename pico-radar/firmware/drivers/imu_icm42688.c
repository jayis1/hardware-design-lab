/*
 * drivers/imu_icm42688.c — ICM-42688-P 6-axis IMU driver for PicoRadar
 *
 * Uses SPI1 on STM32H743 with DMA for FIFO burst reads.
 * Manual NSS (PA4) — CS is managed in software.
 */

#include "imu_icm42688.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* DMA1 Stream 0 for SPI1_RX, Channel 3 (per STM32H7 DMAMUX) */
#define DMA_SPI1_RX_STREAM   0
#define DMA_SPI1_RX_CHANNEL  3
#define DMA_SPI1_RX_BASE     (DMA1_BASE + 0x10 + DMA_SPI1_RX_STREAM * 0x18)

/* RX/TX buffers (aligned for DMA) */
static __attribute__((aligned(32))) uint8_t spi1_tx_buf[32];
static __attribute__((aligned(32))) uint8_t spi1_rx_buf[32];

static volatile int dma_rx_complete = 0;
static imu_dma_cb_t user_dma_cb = NULL;

/* ---- Low-level SPI helpers ---- */

static void spi1_cs_low(void)
{
    GPIO_ODR(GPIOA_BASE) &= ~(1 << IMU_CS_PIN);
}

static void spi1_cs_high(void)
{
    GPIO_ODR(GPIOA_BASE) |= (1 << IMU_CS_PIN);
}

static void spi1_wait_busy(void)
{
    uint32_t timeout = 100000;
    while ((SPI_SR(SPI1_BASE) & (1 << 7)) && --timeout);
}

static void spi1_write_byte(uint8_t data)
{
    while (!(SPI_SR(SPI1_BASE) & (1 << 1)));
    *((volatile uint8_t *)&SPI_DR(SPI1_BASE)) = data;
    spi1_wait_busy();
}

static uint8_t spi1_read_byte(void)
{
    while (!(SPI_SR(SPI1_BASE) & (1 << 0)));
    return *((volatile uint8_t *)&SPI_DR(SPI1_BASE));
}

/* ---- Register access ---- */

int imu_read_register(uint8_t reg, uint8_t *val)
{
    spi1_cs_low();
    spi1_write_byte(reg | ICM42688_SPI_READ);
    spi1_write_byte(0x00);
    *val = spi1_read_byte();
    spi1_cs_high();
    return 0;
}

int imu_write_register(uint8_t reg, uint8_t val)
{
    spi1_cs_low();
    spi1_write_byte(reg & ~ICM42688_SPI_READ);
    spi1_write_byte(val);
    spi1_cs_high();
    return 0;
}

/* ---- DMA configuration for FIFO burst read ---- */

static void spi1_dma_config_rx(uint8_t *dst, uint16_t len)
{
    volatile uint32_t *scr  = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x00);
    volatile uint32_t *sndtr = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x0C);
    volatile uint32_t *spar = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x10);
    volatile uint32_t *sm0ar = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x14);

    *scr = 0;
    while (*scr & 1);

    *spar = (uint32_t)&SPI_DR(SPI1_BASE);
    *sm0ar = (uint32_t)dst;
    *sndtr = len;

    *scr = (1 << 0)    // EN
         | (1 << 1)    // TCIE
         | (0 << 6)    // DIR: periph-to-memory
         | (0 << 8)    // CIRC: not circular
         | (1 << 10)   // MINC
         | (0 << 11)   // PSIZE: 8-bit
         | (0 << 13)   // MSIZE: 8-bit
         | (2 << 16)   // PL: high priority
         | (DMA_SPI1_RX_CHANNEL << 25);

    dma_rx_complete = 0;
}

/* ---- DMA ISR ---- */

void imu_dma_irq_handler(void)
{
    volatile uint32_t *scr = (volatile uint32_t *)(DMA_SPI1_RX_BASE + 0x00);
    if (*scr & (1 << 1)) {
        *scr |= (1 << 2);
        dma_rx_complete = 1;
        spi1_cs_high();
        if (user_dma_cb) user_dma_cb();
    }
}

/* ---- Public API ---- */

int imu_init(void)
{
    uint8_t who_am_i;
    int retries = 3;

    /* Enable SPI1 clock */
    RCC_APB1LENR |= (1 << 12);

    /* SPI1 config: Master, CPOL=0, CPHA=0, 8-bit, MSB first */
    SPI_CR1(SPI1_BASE) = (1 << 2)   // MSTR
                       | (0 << 0)   // CPOL=0
                       | (0 << 1)   // CPHA=0
                       | (7 << 3)   // BR = /256 (safe for init)
                       | (0 << 7)   // MSB first
                       | (1 << 8)   // SSI=1
                       | (0 << 10); // Full duplex
    SPI_CR2(SPI1_BASE) = (0xF << 8) // DS=8-bit
                       | (1 << 12)  // FRXTH
                       | (1 << 1);  // SSOE
    SPI_CR1(SPI1_BASE) |= (1 << 6); // SPE

    /* Verify device ID */
    while (retries--) {
        imu_read_register(ICM42688_WHO_AM_I, &who_am_i);
        if (who_am_i == 0x47) break;
    }
    if (who_am_i != 0x47) return -1;

    /* Soft reset */
    imu_write_register(ICM42688_DEVICE_CONFIG, 0x01);
    for (volatile int i = 0; i < 100000; i++);

    /* Configure gyro: ±2000 DPS, ODR = 800 Hz */
    imu_write_register(ICM42688_GYRO_CONFIG0, (6 << 5) | (0 << 0));

    /* Configure accel: ±16 g, ODR = 800 Hz */
    imu_write_register(ICM42688_ACCEL_CONFIG0, (6 << 5) | (0 << 0));

    /* Power management: enable accel + gyro in low-noise mode */
    imu_write_register(ICM42688_PWR_MGMT0, 0x0F);
    for (volatile int i = 0; i < 500000; i++);

    /* Increase SPI speed: BR = 2 (/8 = 15 MHz) */
    SPI_CR1(SPI1_BASE) &= ~(1 << 6);
    SPI_CR1(SPI1_BASE) = (SPI_CR1(SPI1_BASE) & ~(7 << 3)) | (2 << 3);
    SPI_CR1(SPI1_BASE) |= (1 << 6);

    /* Enable DMA for SPI1 RX */
    SPI_CR2(SPI1_BASE) |= (1 << 0);

    /* Enable DMA1 clock */
    RCC_AHB1ENR |= (1 << 0);
    NVIC_ISER0 |= (1 << 11);

    return 0;
}

int imu_read_raw(imu_sample_t *sample)
{
    uint8_t buf[14];

    spi1_cs_low();
    spi1_write_byte(0x1F | ICM42688_SPI_READ);
    for (int i = 0; i < 14; i++) {
        spi1_write_byte(0x00);
        buf[i] = spi1_read_byte();
    }
    spi1_cs_high();

    sample->accel_x = (int16_t)((buf[0] << 8) | buf[1]);
    sample->accel_y = (int16_t)((buf[2] << 8) | buf[3]);
    sample->accel_z = (int16_t)((buf[4] << 8) | buf[5]);
    sample->gyro_x  = (int16_t)((buf[8] << 8) | buf[9]);
    sample->gyro_y  = (int16_t)((buf[10] << 8) | buf[11]);
    sample->gyro_z  = (int16_t)((buf[12] << 8) | buf[13]);
    sample->temperature = (int16_t)((buf[6] << 8) | buf[7]);

    return 0;
}

void imu_scale(const imu_sample_t *raw, imu_scaled_t *scaled)
{
    scaled->accel_x = (float)raw->accel_x * IMU_ACCEL_SCALE;
    scaled->accel_y = (float)raw->accel_y * IMU_ACCEL_SCALE;
    scaled->accel_z = (float)raw->accel_z * IMU_ACCEL_SCALE;
    scaled->gyro_x  = (float)raw->gyro_x * IMU_GYRO_SCALE;
    scaled->gyro_y  = (float)raw->gyro_y * IMU_GYRO_SCALE;
    scaled->gyro_z  = (float)raw->gyro_z * IMU_GYRO_SCALE;
    scaled->temperature = ((float)raw->temperature / 132.48f) + 25.0f;
}

int imu_fifo_read(imu_sample_t *samples, uint16_t max_count, uint16_t *out_count)
{
    uint8_t fifo_count_hi, fifo_count_lo;
    imu_read_register(ICM42688_FIFO_COUNTH, &fifo_count_hi);
    imu_read_register(ICM42688_FIFO_COUNTH + 1, &fifo_count_lo);
    uint16_t fifo_count = ((fifo_count_hi & 0x07) << 8) | fifo_count_lo;

    uint16_t frame_count = fifo_count / 16;
    if (frame_count > max_count) frame_count = max_count;
    *out_count = frame_count;

    if (frame_count == 0) return 0;

    uint16_t total_bytes = frame_count * 16;

    spi1_cs_low();
    spi1_tx_buf[0] = ICM42688_FIFO_DATA | ICM42688_SPI_READ;

    spi1_dma_config_rx(spi1_rx_buf, total_bytes);

    for (uint16_t i = 0; i < total_bytes + 1; i++) {
        spi1_write_byte(i == 0 ? spi1_tx_buf[0] : 0x00);
    }

    uint32_t timeout = 10000;
    while (!dma_rx_complete && --timeout);

    spi1_cs_high();

    if (!dma_rx_complete) return -2;

    for (uint16_t f = 0; f < frame_count; f++) {
        uint8_t *frame = &spi1_rx_buf[f * 16];
        samples[f].accel_x = (int16_t)((frame[1] << 8) | frame[2]);
        samples[f].accel_y = (int16_t)((frame[3] << 8) | frame[4]);
        samples[f].accel_z = (int16_t)((frame[5] << 8) | frame[6]);
        samples[f].gyro_x  = (int16_t)((frame[7] << 8) | frame[8]);
        samples[f].gyro_y  = (int16_t)((frame[9] << 8) | frame[10]);
        samples[f].gyro_z  = (int16_t)((frame[11] << 8) | frame[12]);
        samples[f].temperature = (int16_t)((frame[13] << 8) | frame[14]);
        samples[f].timestamp = frame[15];
    }

    return 0;
}

int imu_self_test(void)
{
    uint8_t who_am_i;
    imu_read_register(ICM42688_WHO_AM_I, &who_am_i);
    return (who_am_i == 0x47) ? 0 : -1;
}

void imu_power_down(void)
{
    imu_write_register(ICM42688_PWR_MGMT0, 0x00);
}

int imu_configure_fifo(uint16_t watermark)
{
    imu_write_register(ICM42688_FIFO_CONFIG, 0x02);
    imu_write_register(0x18, (watermark >> 8) & 0x1F);
    imu_write_register(0x19, watermark & 0xFF);
    return 0;
}

void imu_set_dma_callback(imu_dma_cb_t cb)
{
    user_dma_cb = cb;
}