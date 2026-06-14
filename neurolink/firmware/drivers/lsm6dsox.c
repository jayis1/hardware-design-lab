/*
 * NeuroLink — LSM6DSOX 6-axis IMU Driver Implementation
 * I2C1 interface at 400 kHz (FM+)
 */

#include "lsm6dsox.h"
#include "board.h"
#include "registers.h"

/* ============================================================
 * I2C1 Helper Functions
 * ============================================================ */

static void i2c1_wait_flag(uint32_t flag)
{
    uint32_t timeout = 100000;
    while (!(I2C1_ISR & flag) && timeout--) ;
}

static int i2c1_write_byte(uint8_t addr, uint8_t reg, uint8_t data)
{
    I2C1_CR2 = ((addr & 0x7F) << 1) |
               (2 << 16) |           /* Nbytes = 2 */
               (1 << 25) |           /* Auto-end */
               (1 << 13) |           /* START */
               (0 << 10);            /* Write */

    i2c1_wait_flag(1 << 1);  /* TXIS */
    I2C1_TXDR = reg;

    i2c1_wait_flag(1 << 1);  /* TXIS */
    I2C1_TXDR = data;

    i2c1_wait_flag(1 << 5);  /* STOPF */

    if (I2C1_ISR & (1 << 2)) {  /* NACK */
        I2C1_ICR |= (1 << 4);
        return -1;
    }
    return 0;
}

static int i2c1_read_bytes(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    /* Phase 1: Write register address */
    I2C1_CR2 = ((addr & 0x7F) << 1) |
               (1 << 16) |       /* 1 byte */
               (0 << 25) |       /* Software end */
               (1 << 13) |       /* START */
               (0 << 10);        /* Write */

    i2c1_wait_flag(1 << 1);  /* TXIS */
    I2C1_TXDR = reg;
    i2c1_wait_flag(1 << 6);  /* TC */

    /* Phase 2: Read data */
    I2C1_CR2 = ((addr & 0x7F) << 1) |
               (len << 16) |       /* Nbytes */
               (1 << 25) |          /* Auto-end */
               (1 << 13) |          /* Repeated START */
               (1 << 10);           /* Read */

    for (uint8_t i = 0; i < len; i++) {
        i2c1_wait_flag(1 << 2);  /* RXNE */
        buf[i] = (uint8_t)(I2C1_RXDR & 0xFF);
    }

    i2c1_wait_flag(1 << 5);  /* STOPF */
    return 0;
}

static uint8_t i2c1_read_byte(uint8_t addr, uint8_t reg)
{
    uint8_t val;
    i2c1_read_bytes(addr, reg, &val, 1);
    return val;
}

/* ============================================================
 * LSM6DSOX Initialization
 * ============================================================ */
void lsm6dsox_init(void)
{
    /* Soft reset */
    lsm6dsox_soft_reset();

    /* Wait for reset to complete */
    for (volatile int i = 0; i < 100000; i++);

    /* Verify device ID */
    uint8_t who_am_i = lsm6dsox_who_am_i();
    if (who_am_i != LSM6DSOX_WHO_AM_I_VAL) {
        /* Communication error — handle */
        return;
    }

    /* CTRL3_C: BDU = 1 (block data update), IF_INC = 1 (auto-increment) */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL3_C, 0x44);
    /* Bit 7: BOOT = 0
     * Bit 6: BDU = 1 (block data update)
     * Bit 5: H_LACTIVE = 0 (active high)
     * Bit 4: PP_OD = 0 (push-pull)
     * Bit 3: SIM = 0 (SPI 4-wire)
     * Bit 2: IF_INC = 1 (auto-increment address)
     * Bits 1:0: SW_RESET = 0
     */

    /* CTRL1_XL: Accelerometer ODR = 104 Hz, FS = ±4g */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL1_XL, 0x42);
    /* Bits 7:4: ODR_XL = 0100 (104 Hz)
     * Bits 3:2: FS_XL = 10 (±4g)
     * Bits 1:0: LPF2_XL = 00 (no LPF2)
     */

    /* CTRL2_G: Gyroscope ODR = 104 Hz, FS = ±500 dps */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL2_G, 0x52);
    /* Bits 7:4: ODR_G = 0101 (104 Hz)
     * Bits 3:0: FS_G = 0010 (±500 dps)
     */

    /* CTRL6_C: Gyro LPF1 bandwidth, XL_HM_MODE */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL6_C, 0x00);

    /* CTRL8_XL: Accelerometer low-pass filter */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL8_XL, 0x00);

    /* Configure FIFO in bypass mode (no FIFO) */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_FIFO_CTRL3, 0x00);
}

/* ============================================================
 * WHO_AM_I Check
 * ============================================================ */
uint8_t lsm6dsox_who_am_i(void)
{
    return i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_WHO_AM_I);
}

/* ============================================================
 * Soft Reset
 * ============================================================ */
void lsm6dsox_soft_reset(void)
{
    uint8_t reg = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL3_C);
    reg |= (1 << 0);  /* SW_RESET = 1 */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL3_C, reg);
}

/* ============================================================
 * Set Accelerometer ODR
 * ============================================================ */
void lsm6dsox_set_xl_odr(lsm6dsox_xl_odr_t odr)
{
    uint8_t reg = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL1_XL);
    reg = (reg & 0x0F) | ((odr & 0x0F) << 4);
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL1_XL, reg);
}

/* ============================================================
 * Set Accelerometer Full Scale
 * ============================================================ */
void lsm6dsox_set_xl_fs(lsm6dsox_xl_fs_t fs)
{
    uint8_t reg = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL1_XL);
    reg = (reg & 0xF3) | ((fs & 0x03) << 2);
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL1_XL, reg);
}

/* ============================================================
 * Set Gyroscope ODR
 * ============================================================ */
void lsm6dsox_set_g_odr(lsm6dsox_g_odr_t odr)
{
    uint8_t reg = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL2_G);
    reg = (reg & 0x0F) | ((odr & 0x0F) << 4);
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL2_G, reg);
}

/* ============================================================
 * Set Gyroscope Full Scale
 * ============================================================ */
void lsm6dsox_set_g_fs(lsm6dsox_g_fs_t fs)
{
    uint8_t reg = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL2_G);
    /* FS_G is bits 3:0 for values 1-4, bit 0 for 125dps */
    reg = (reg & 0xF0) | (fs & 0x0F);
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_CTRL2_G, reg);
}

/* ============================================================
 * Read Accelerometer Data
 * ============================================================ */
void lsm6dsox_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    i2c1_read_bytes(LSM6DSOX_I2C_ADDR, LSM6DSOX_OUTX_L_XL, buf, 6);
    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);
}

/* ============================================================
 * Read Gyroscope Data
 * ============================================================ */
void lsm6dsox_read_gyro(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    i2c1_read_bytes(LSM6DSOX_I2C_ADDR, LSM6DSOX_OUTX_L_G, buf, 6);
    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);
}

/* ============================================================
 * Read All IMU Data
 * ============================================================ */
void lsm6dsox_read_all(lsm6dsox_data_t *data)
{
    uint8_t buf[12];
    /* Read accel (6 bytes) starting from OUTX_L_XL */
    i2c1_read_bytes(LSM6DSOX_I2C_ADDR, LSM6DSOX_OUTX_L_XL, buf, 6);
    data->accel_x = (int16_t)((buf[1] << 8) | buf[0]);
    data->accel_y = (int16_t)((buf[3] << 8) | buf[2]);
    data->accel_z = (int16_t)((buf[5] << 8) | buf[4]);

    /* Read gyro (6 bytes) starting from OUTX_L_G */
    i2c1_read_bytes(LSM6DSOX_I2C_ADDR, LSM6DSOX_OUTX_L_G, buf, 6);
    data->gyro_x = (int16_t)((buf[1] << 8) | buf[0]);
    data->gyro_y = (int16_t)((buf[3] << 8) | buf[2]);
    data->gyro_z = (int16_t)((buf[5] << 8) | buf[4]);

    data->temperature = lsm6dsox_read_temperature();
}

/* ============================================================
 * Read Temperature
 * ============================================================ */
int16_t lsm6dsox_read_temperature(void)
{
    uint8_t buf[2];
    i2c1_read_bytes(LSM6DSOX_I2C_ADDR, LSM6DSOX_OUT_TEMP_L, buf, 2);
    return (int16_t)((buf[1] << 8) | buf[0]);
}

/* ============================================================
 * Check Data Availability
 * ============================================================ */
uint8_t lsm6dsox_data_available(void)
{
    uint8_t status = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_STATUS_REG);
    /* XLDA (bit 0) = new accel data, GDA (bit 1) = new gyro data */
    return status & 0x03;
}

/* ============================================================
 * FIFO Configuration
 * ============================================================ */
void lsm6dsox_config_fifo(uint8_t watermark)
{
    /* FIFO_CTRL3: Enable accel and gyro in FIFO */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_FIFO_CTRL3,
                    (2 << 0) | (2 << 4));  /* BDR_XL = 010, BDR_G = 010 */

    /* FIFO_CTRL1: Watermark (low 8 bits) */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_FIFO_CTRL1, watermark & 0xFF);

    /* FIFO_CTRL2: Watermark high bit + FIFO mode */
    uint8_t ctrl2 = i2c1_read_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_FIFO_CTRL2);
    ctrl2 = (ctrl2 & ~0x01) | ((watermark >> 8) & 0x01);
    ctrl2 &= ~(3 << 5);  /* FIFO_MODE = continuous */
    ctrl2 |= (2 << 5);   /* FIFO_MODE = 10 (continuous mode) */
    i2c1_write_byte(LSM6DSOX_I2C_ADDR, LSM6DSOX_FIFO_CTRL2, ctrl2);
}