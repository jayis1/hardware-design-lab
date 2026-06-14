/*
 * SkyPilot — ICM-42688-P IMU Driver Implementation
 * SPI1 with DMA on STM32H743VIT6
 */

#include "imu_icm42688.h"
#include "../registers.h"
#include "../board.h"

/* Private variables */
static volatile int dma_complete_flag = 0;
static uint8_t spi_tx_buf[24];
static uint8_t spi_rx_buf[24];

/* Scale factors (depends on configured full-scale range) */
static float accel_scale = 16.0f / 32768.0f;  /* ±16g default */
static float gyro_scale = 2000.0f / 32768.0f;  /* ±2000dps default */

/* ==================== SPI Low-Level ==================== */

int icm42688_read_reg(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    ICM42688_CS_LOW();

    /* SPI1 CR1: set to 8-bit data, master, TX only first */
    SPI1->CR1 = SPI_CR1_SPE;

    /* Send register address with read bit (bit 7 = 0 for read on ICM-42688) */
    spi_tx_buf[0] = reg & 0x7F;
    SPI1->DR = spi_tx_buf[0];
    while (!(SPI1->SR & SPI_SR_TXE))
        ;

    /* Clock out dummy bytes while reading */
    for (i = 0; i < len; i++) {
        SPI1->DR = 0xFF;
        while (!(SPI1->SR & SPI_SR_RXNE))
            ;
        buf[i] = (uint8_t)(SPI1->DR & 0xFF);
        while (SPI1->SR & SPI_SR_BSY)
            ;
    }

    ICM42688_CS_HIGH();

    return 0;
}

int icm42688_write_reg(uint8_t reg, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    ICM42688_CS_LOW();

    /* Send register address with write bit (bit 7 = 1 for write on ICM-42688) */
    spi_tx_buf[0] = reg | 0x80;
    SPI1->DR = spi_tx_buf[0];
    while (!(SPI1->SR & SPI_SR_TXE))
        ;

    for (i = 0; i < len; i++) {
        SPI1->DR = buf[i];
        while (!(SPI1->SR & SPI_SR_TXE))
            ;
    }

    while (SPI1->SR & SPI_SR_BSY)
        ;

    ICM42688_CS_HIGH();

    return 0;
}

int icm42688_set_bank(uint8_t bank)
{
    uint8_t val = (bank & 0x03) << 2;
    return icm42688_write_reg(ICM42688_USER_BANK, &val, 1);
}

/* ==================== DMA ==================== */

int icm42688_start_dma_read(uint8_t *rx_buf, uint16_t len)
{
    /* Configure DMA1 Stream2 for SPI1 RX */
    DMA1_Stream2->CR &= ~DMA_SxCR_EN;         /* Disable stream */
    while (DMA1_Stream2->CR & DMA_SxCR_EN)
        ;

    DMA1_Stream2->PAR   = (uint32_t)&SPI1->DR;
    DMA1_Stream2->M0AR  = (uint32_t)rx_buf;
    DMA1_Stream2->NDTR  = len;
    DMA1_Stream2->CR    = DMA_SxCR_CHSEL_3 |   /* Channel 3 (SPI1_RX) */
                           DMA_SxCR_MINC |      /* Memory increment */
                           DMA_SxCR_TCIE |      /* Transfer complete interrupt */
                           DMA_SxCR_EN;

    dma_complete_flag = 0;

    /* Assert CS and start SPI transfer */
    ICM42688_CS_LOW();
    SPI1->CR2 |= SPI_CR2_RXDMAEN;

    return 0;
}

int icm42688_dma_complete(void)
{
    return dma_complete_flag;
}

/* DMA1 Stream2 interrupt handler */
void DMA1_Stream2_IRQHandler(void)
{
    if (DMA1->LISR & DMA_LISR_TCIF2) {
        DMA1->LIFCR = DMA_LIFCR_CTCIF2;
        dma_complete_flag = 1;
        ICM42688_CS_HIGH();
        SPI1->CR2 &= ~SPI_CR2_RXDMAEN;
    }
}

/* ==================== Initialization ==================== */

int icm42688_init(void)
{
    uint8_t val;
    uint8_t who_am_i;

    /* Reset device */
    val = 0x01;
    icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, &val, 1);

    /* Wait for reset to complete */
    for (volatile uint32_t i = 0; i < 1000000; i++)
        ;

    /* Verify WHO_AM_I */
    who_am_i = 0;
    icm42688_read_reg(ICM42688_WHO_AM_I, &who_am_i, 1);
    if (who_am_i != 0x47) {  /* ICM-42688-P WHO_AM_I = 0x47 */
        return -1;
    }

    /* Bank 0: Configure gyro */
    icm42688_set_bank(0);

    /* Gyro: ±2000dps, 8kHz ODR, low-noise mode */
    val = (ICM42688_GYRO_FS_2000DPS << 5) | ICM42688_ODR_8KHZ;
    icm42688_write_reg(ICM42688_GYRO_CONFIG0, &val, 1);

    /* Accelerometer: ±16g, 8kHz ODR, low-noise mode */
    val = (ICM42688_ACCEL_FS_16G << 5) | ICM42688_ODR_8KHZ;
    icm42688_write_reg(ICM42688_ACCEL_CONFIG0, &val, 1);

    /* Enable gyro and accel in low-noise mode */
    val = (ICM42688_GYRO_MODE_LN << 2) | ICM42688_ACCEL_MODE_LN;
    icm42688_write_reg(ICM42688_PWR_MGMT0, &val, 1);

    /* Set scale factors */
    accel_scale = 16.0f / 32768.0f;
    gyro_scale  = 2000.0f / 32768.0f;

    /* Configure data ready interrupt on INT1 (PB3) */
    val = 0x03;  /* UI DRDY int on INT1 */
    icm42688_write_reg(ICM42688_INT_SOURCE0, &val, 1);

    return 0;
}

int icm42688_who_am_i(void)
{
    uint8_t id = 0;
    icm42688_read_reg(ICM42688_WHO_AM_I, &id, 1);
    return id;
}

int icm42688_reset(void)
{
    uint8_t val = 0x01;
    icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, &val, 1);
    for (volatile uint32_t i = 0; i < 1000000; i++)
        ;
    return 0;
}

int icm42688_config_gyro(uint8_t fs, uint8_t odr)
{
    uint8_t val = (fs << 5) | odr;
    icm42688_write_reg(ICM42688_GYRO_CONFIG0, &val, 1);

    /* Update scale factor */
    switch (fs) {
        case ICM42688_GYRO_FS_2000DPS: gyro_scale = 2000.0f / 32768.0f; break;
        case ICM42688_GYRO_FS_1000DPS: gyro_scale = 1000.0f / 32768.0f; break;
        case ICM42688_GYRO_FS_500DPS:  gyro_scale = 500.0f  / 32768.0f; break;
        case ICM42688_GYRO_FS_250DPS:  gyro_scale = 250.0f  / 32768.0f; break;
        case ICM42688_GYRO_FS_125DPS:  gyro_scale = 125.0f  / 32768.0f; break;
        default: gyro_scale = 2000.0f / 32768.0f; break;
    }
    return 0;
}

int icm42688_config_accel(uint8_t fs, uint8_t odr)
{
    uint8_t val = (fs << 5) | odr;
    icm42688_write_reg(ICM42688_ACCEL_CONFIG0, &val, 1);

    /* Update scale factor */
    switch (fs) {
        case ICM42688_ACCEL_FS_16G: accel_scale = 16.0f / 32768.0f; break;
        case ICM42688_ACCEL_FS_8G:  accel_scale =  8.0f / 32768.0f; break;
        case ICM42688_ACCEL_FS_4G:  accel_scale =  4.0f / 32768.0f; break;
        case ICM42688_ACCEL_FS_2G:  accel_scale =  2.0f / 32768.0f; break;
        default: accel_scale = 16.0f / 32768.0f; break;
    }
    return 0;
}

/* ==================== Data Read ==================== */

int icm42688_read_raw(imu_data_t *data)
{
    uint8_t buf[14];

    /* Burst read from TEMP_DATA1 (0x1D) through GYRO_DATA_Z0 (0x2A) */
    icm42688_read_reg(ICM42688_TEMP_DATA1, buf, 14);

    data->temperature = (int16_t)((buf[0] << 8) | buf[1]);
    data->accel_x     = (int16_t)((buf[2] << 8) | buf[3]);
    data->accel_y     = (int16_t)((buf[4] << 8) | buf[5]);
    data->accel_z     = (int16_t)((buf[6] << 8) | buf[7]);
    data->gyro_x      = (int16_t)((buf[8] << 8) | buf[9]);
    data->gyro_y      = (int16_t)((buf[10] << 8) | buf[11]);
    data->gyro_z      = (int16_t)((buf[12] << 8) | buf[13]);
    data->timestamp   = 0;  /* Filled by caller from DWT->CYCCNT */

    return 0;
}

void icm42688_scale_data(const imu_data_t *raw, imu_scaled_t *scaled)
{
    scaled->accel_x = (float)raw->accel_x * accel_scale;
    scaled->accel_y = (float)raw->accel_y * accel_scale;
    scaled->accel_z = (float)raw->accel_z * accel_scale;
    scaled->gyro_x  = (float)raw->gyro_x  * gyro_scale;
    scaled->gyro_y  = (float)raw->gyro_y  * gyro_scale;
    scaled->gyro_z  = (float)raw->gyro_z  * gyro_scale;
    scaled->temperature = ((float)raw->temperature / 132.48f) + 25.0f;
}

int icm42688_read_scaled(imu_scaled_t *data)
{
    imu_data_t raw;
    int ret = icm42688_read_raw(&raw);
    if (ret != 0) return ret;
    icm42688_scale_data(&raw, data);
    return 0;
}