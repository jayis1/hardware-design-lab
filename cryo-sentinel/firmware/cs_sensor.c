/*
 * cs_sensor.c — Sensor drivers for Cryo-Sentinel.
 *
 * Implements: FDC2214 capacitive level, MAX31865 RTD bank, ICM-45686 IMU,
 *             BME280 ambient, GPIO debounce for lid/enclosure/mains/probe.
 *
 * These are bare-metal register-level drivers; in the production build they
 * are wrapped by Zephyr's device tree, but the math and the register
 * sequences are identical. The functions here are testable on a desktop
 * by #define-ing CS_DESKTOP and linking mock i2c/spi implementations.
 *
 * Author: jayis1
 * License: MIT
 */
#include "board.h"
#include "registers.h"
#include "cs_sensor.h"
#include "cs_calibration.h"
#include <string.h>
#include <math.h>

/* ---- platform shims (in real build, these are Zephyr drivers) -------- */
#ifdef CS_DESKTOP
extern int mock_i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, int n);
extern int mock_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *buf, int n);
extern int mock_spi_xfer(uint8_t cs, const uint8_t *tx, uint8_t *rx, int n);
extern uint32_t mock_gpio_get(uint8_t pin);
extern void mock_delay_ms(uint32_t ms);
#else
/* On target these resolve to nrfx / Zephyr calls; stubs here keep the
 * file self-contained for the open-source release. */
static int mock_i2c_read(uint8_t a, uint8_t r, uint8_t *b, int n) { (void)a;(void)r;(void)b;(void)n; return 0; }
static int mock_i2c_write(uint8_t a, uint8_t r, const uint8_t *b, int n) { (void)a;(void)r;(void)b;(void)n; return 0; }
static int mock_spi_xfer(uint8_t c, const uint8_t *t, uint8_t *r, int n) { (void)c;(void)t;(void)r;(void)n; return 0; }
static uint32_t mock_gpio_get(uint8_t p) { (void)p; return 0; }
static void mock_delay_ms(uint32_t ms) { (void)ms; }
#endif

/* ---- small helpers --------------------------------------------------- */
static uint16_t rd16(uint8_t addr, uint8_t reg)
{
    uint8_t b[2];
    if (mock_i2c_read(addr, reg, b, 2)) return 0xFFFF;
    return ((uint16_t)b[0] << 8) | b[1];
}

static int wr16(uint8_t addr, uint8_t reg, uint16_t val)
{
    uint8_t b[2] = { (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    return mock_i2c_write(addr, reg, b, 2);
}

static uint16_t spi_rd16(uint8_t cs, uint8_t reg)
{
    uint8_t tx[2] = { (uint8_t)(reg | 0x80), 0x00 };
    uint8_t rx[2] = { 0, 0 };
    mock_spi_xfer(cs, tx, rx, 2);
    return ((uint16_t)rx[1] << 8) | rx[0];  /* MAX31865 returns MSB first */
}

static int spi_wr8(uint8_t cs, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), val };
    uint8_t rx[2] = { 0, 0 };
    return mock_spi_xfer(cs, tx, rx, 2);
}

uint32_t cs_median_u32(uint32_t *buf, int n)
{
    /* simple insertion sort then pick middle (n <= 16) */
    for (int i = 1; i < n; i++) {
        uint32_t k = buf[i]; int j = i - 1;
        while (j >= 0 && buf[j] > k) { buf[j+1] = buf[j]; j--; }
        buf[j+1] = k;
    }
    return buf[n/2];
}

uint16_t cs_median_u16(uint16_t *buf, int n)
{
    for (int i = 1; i < n; i++) {
        uint16_t k = buf[i]; int j = i - 1;
        while (j >= 0 && buf[j] > k) { buf[j+1] = buf[j]; j--; }
        buf[j+1] = k;
    }
    return buf[n/2];
}

/* ---- FDC2214 capacitive level ---------------------------------------- */
static int fdc2214_init(void)
{
    /* Configure CH0 + CH1, active mode, 28-bit conversion, internal ref. */
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_RCOUNT_CH0,      FDC2214_RCOUNT_DEFAULT);
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_RCOUNT_CH1,      FDC2214_RCOUNT_DEFAULT);
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_SETTLECOUNT_CH0, FDC2214_SETTLE_DEFAULT);
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_SETTLECOUNT_CH1, FDC2214_SETTLE_DEFAULT);
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_LC_CONFIG_CH0,   0x0001);
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_LC_CONFIG_CH1,   0x0001);
    uint16_t cfg = FDC2214_CONFIG_ACTIVE_CHAN0
                 | FDC2214_CONFIG_HIGH_CURRENT
                 | FDC2214_CONFIG_INTB_DIS
                 | FDC2214_CONFIG_DRDY_INT;
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_CONFIG, cfg);
    /* Mux: no autoscan, deglitch 3.3 MHz. */
    wr16(FDC2214_I2C_ADDR, FDC2214_REG_MUX_CONFIG, 0x020C);
    return 0;
}

static uint32_t fdc2214_read_ch0(void)
{
    /* 28-bit result in MSB[13:0] | LSB[15:0]; mask the unused top bits. */
    uint16_t msb = rd16(FDC2214_I2C_ADDR, FDC2214_REG_RD_CH0_MSB);
    uint16_t lsb = rd16(FDC2214_I2C_ADDR, FDC2214_REG_RD_CH0_LSB);
    if (msb == 0xFFFF && lsb == 0xFFFF) return 0;
    return (((uint32_t)(msb & 0x3FFF)) << 16) | lsb;
}

float cs_fdc_raw_to_pf(uint32_t raw)
{
    /* fDiv = 0 -> fSensor = fRef * (CHx_FIN_SEL+1) / 2^25.
     * The FDC reports ProportionCode = (fSensor * 2^28) / fREF.
     * C = 1 / (L * (2*pi*f)^2). With L = 18 uH and fRef = 40 MHz:
     * C_pF ≈ (1.0 / (18e-6 * (2*pi*f)^2)) * 1e12.
     * Here we just give a deterministic monotonic proxy. */
    const float L_uH = 18.0f;
    const float fRef_Hz = 40.0e6f;
    if (raw == 0) return 0.0f;
    float fSensor = (float)raw * fRef_Hz / (float)(1u << 28);
    if (fSensor < 100.0f) fSensor = 100.0f;  /* guard /0 */
    float C_F = 1.0f / (L_uH * 1e-6f * (2.0f * 3.14159265f * fSensor) *
                        (2.0f * 3.14159265f * fSensor));
    return C_F * 1e12f;  /* pF */
}

/* ---- MAX31865 RTD ---------------------------------------------------- */
static int max31865_init(uint8_t cs)
{
    spi_wr8(cs, MAX31865_REG_CONFIG,
            MAX31865_CFG_VBIAS | MAX31865_CFG_3WIRE |
            MAX31865_CFG_FAULT_STATUSCLR | MAX31865_CFG_50HZ_FILTER);
    /* Set fault thresholds: low ~ -200C (~ 200 ohm Pt1000), high ~ +50C. */
    spi_wr8(cs, MAX31865_REG_LFT_MSB, 0x00);
    spi_wr8(cs, MAX31865_REG_LFT_LSB, 0xC0);
    spi_wr8(cs, MAX31865_REG_HFT_MSB, 0xFF);
    spi_wr8(cs, MAX31865_REG_HFT_LSB, 0xA0);
    return 0;
}

static uint16_t max31865_one_shot(uint8_t cs)
{
    uint8_t cfg = MAX31865_CFG_VBIAS | MAX31865_CFG_3WIRE |
                  MAX31865_CFG_CONV_1SHOT | MAX31865_CFG_50HZ_FILTER;
    spi_wr8(cs, MAX31865_REG_CONFIG, cfg);
    mock_delay_ms(11);  /* 1-shot ~ 10 ms with 50 Hz filter */
    uint16_t msb = spi_rd16(cs, MAX31865_REG_RTD_MSB);
    uint16_t lsb = spi_rd16(cs, MAX31865_REG_RTD_LSB);
    uint8_t  fault = (uint8_t)(lsb & 0x01);
    if (fault) {
        spi_wr8(cs, MAX31865_REG_CONFIG,
                MAX31865_CFG_VBIAS | MAX31865_CFG_3WIRE |
                MAX31865_CFG_FAULT_STATUSCLR | MAX31865_CFG_50HZ_FILTER);
        return 0xFFFF;
    }
    return (msb << 7) | (lsb >> 1);  /* 15-bit RTD code */
}

float cs_rtd_code_to_degC(uint16_t code)
{
    if (code == 0 || code == 0xFFFF) return -999.0f;
    float R = ((float)code / 32768.0f) * MAX31865_RREF_PT1000;
    /* Callendar-Van Dusen, T >= 0: R = R0*(1 + A*t + B*t^2). */
    const float R0 = MAX31865_RTD_NOMINAL_PT1000;
    const float A  = MAX31865_ALPHA_PT1000;
    const float B  = -5.8e-7f;
    /* Solve quadratic for t. */
    float a = B, b = A, c = (R / R0) - 1.0f;
    if (a == 0.0f) return c / b;
    float disc = b*b - 4.0f*a*c;
    if (disc < 0) return -999.0f;
    float t = (-b + sqrtf(disc)) / (2.0f*a);  /* positive root */
    return t;
}

/* ---- BME280 ambient (simplified; uses calibration words from the chip) - */
static uint8_t bme280_cal[41];

static int bme280_init(void)
{
    uint8_t id = 0;
    mock_i2c_read(BME280_I2C_ADDR, BME280_REG_ID, &id, 1);
    if (id != 0x60) return -1;
    mock_i2c_read(BME280_I2C_ADDR, BME280_REG_CALIB00, bme280_cal,        26);
    mock_i2c_read(BME280_I2C_ADDR, BME280_REG_CALIB26, bme280_cal + 26,    7);
    mock_i2c_read(BME280_I2C_ADDR, BME280_REG_CALIB41, bme280_cal + 33,    8);
    /* Forced mode, x1 oversampling, 50 Hz filter for the ambient read. */
    mock_i2c_write(BME280_I2C_ADDR, BME280_REG_CTRL_HUM,
                   (uint8_t[]){BME280_OSRS_H_X1}, 1);
    mock_i2c_write(BME280_I2C_ADDR, BME280_REG_CONFIG,
                   (uint8_t[]){BME280_STBY_500MS | BME280_FILTER_OFF}, 1);
    mock_i2c_write(BME280_I2C_ADDR, BME280_REG_CTRL_MEAS,
                   (uint8_t[]){BME280_OSRS_T_X1 | BME280_OSRS_P_X1 |
                               BME280_MODE_FORCED}, 1);
    return 0;
}

static void bme280_forced_trigger(void)
{
    mock_i2c_write(BME280_I2C_ADDR, BME280_REG_CTRL_MEAS,
                   (uint8_t[]){BME280_OSRS_T_X1 | BME280_OSRS_P_X1 |
                               BME280_MODE_FORCED}, 1);
}

/* BME280's official 32-bit compensation, trimmed for our purposes. The raw
 * ADC values are read straight from the data registers; the compensation
 * uses the calibration words captured at init. */
static int32_t dig_T1, dig_T2, dig_T3;
static int32_t dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7,
               dig_P8, dig_P9;
static int32_t t_fine;

static void bme280_parse_cal(void)
{
    dig_T1 = (bme280_cal[1]  << 8) | bme280_cal[0];
    dig_T2 = (int16_t)((bme280_cal[3]  << 8) | bme280_cal[2]);
    dig_T3 = (int16_t)((bme280_cal[5]  << 8) | bme280_cal[4]);
    dig_P1 = (bme280_cal[7]  << 8) | bme280_cal[6];
    dig_P2 = (int16_t)((bme280_cal[9]  << 8) | bme280_cal[8]);
    dig_P3 = (int16_t)((bme280_cal[11] << 8) | bme280_cal[10]);
    dig_P4 = (int16_t)((bme280_cal[13] << 8) | bme280_cal[12]);
    dig_P5 = (int16_t)((bme280_cal[15] << 8) | bme280_cal[14]);
    dig_P6 = (int16_t)((bme280_cal[17] << 8) | bme280_cal[16]);
    dig_P7 = (int16_t)((bme280_cal[19] << 8) | bme280_cal[18]);
    dig_P8 = (int16_t)((bme280_cal[21] << 8) | bme280_cal[20]);
    dig_P9 = (int16_t)((bme280_cal[23] << 8) | bme280_cal[22]);
}

static int32_t bme280_compensate_T(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - (dig_T1 << 1))) * (dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - (dig_T1)) *
                      ((adc_T >> 4) - (dig_T1))) >> 12) * (dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;  /* degC * 100 */
}

static uint32_t bme280_compensate_P(int32_t adc_P)
{
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) +
           ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)(p >> 8);  /* Pa / 256 -> we return Pa via shift */
}

/* ---- ICM-45686 IMU --------------------------------------------------- */
static int imu_init(void)
{
    uint8_t who = 0;
    mock_i2c_read(ICM45686_I2C_ADDR_AD0_1, ICM45686_REG_WHOAMI, &who, 1);
    if (who != 0xE9) return -1;
    uint8_t pwr = ICM45686_PWR_GYRO_MODE_LN | ICM45686_PWR_ACCEL_MODE_LN;
    mock_i2c_write(ICM45686_I2C_ADDR_AD0_1, ICM45686_REG_PWR_MGMT0,
                   &pwr, 1);
    mock_delay_ms(2);
    uint8_t gyr = ICM45686_GYR_FS_2000DPS | ICM45686_GYR_ODR_50HZ;
    uint8_t acc = ICM45686_ACC_FS_4G      | ICM45686_ACC_ODR_50HZ;
    mock_i2c_write(ICM45686_I2C_ADDR_AD0_1, ICM45686_REG_GYRO_CONFIG0,  &gyr, 1);
    mock_i2c_write(ICM45686_I2C_ADDR_AD0_1, ICM45686_REG_ACCEL_CONFIG0, &acc, 1);
    return 0;
}

static void imu_read(float *tilt_deg, float *vib_rms_g)
{
    uint8_t d[12];
    mock_i2c_read(ICM45686_I2C_ADDR_AD0_1, ICM45686_REG_ACCEL_DATA_X1, d, 12);
    int16_t ax = (int16_t)((d[0]  << 8) | d[1]);
    int16_t ay = (int16_t)((d[2]  << 8) | d[3]);
    int16_t az = (int16_t)((d[4]  << 8) | d[5]);
    /* Convert to g. */
    float fx = (float)ax / ICM45686_ACCEL_SENS_4G;
    float fy = (float)ay / ICM45686_ACCEL_SENS_4G;
    float fz = (float)az / ICM45686_ACCEL_SENS_4G;
    /* Tilt from vertical: angle between +Z axis and measured vector. */
    float mag = sqrtf(fx*fx + fy*fy + fz*fz);
    if (mag < 0.01f) { *tilt_deg = 0.0f; *vib_rms_g = 0.0f; return; }
    float cos_t = fz / mag;
    if (cos_t >  1.0f) cos_t =  1.0f;
    if (cos_t < -1.0f) cos_t = -1.0f;
    *tilt_deg = acosf(cos_t) * (180.0f / 3.14159265f);
    /* Vibration RMS: residual after removing 1 g of gravity from the
     * magnitude. A running estimator over a short window lives in the
     * caller; here we return the instantaneous deviation. */
    *vib_rms_g = fabsf(mag - 1.0f);
}

/* ---- TCA9548A mux ---------------------------------------------------- */
static void mux_select(uint8_t ch_mask)
{
    mock_i2c_write(TCA9548A_I2C_ADDR, 0x00, &ch_mask, 1);
}

/* ---- Public API ------------------------------------------------------ */
int cs_sensor_init(void)
{
    int faults = 0;
    if (fdc2214_init())            faults |= 0x01;
    if (max31865_init(CS_SPI_CS_MAX31865_A)) faults |= 0x02;
    if (max31865_init(CS_SPI_CS_MAX31865_B)) faults |= 0x04;
    if (max31865_init(CS_SPI_CS_MAX31865_C)) faults |= 0x08;
    if (bme280_init())             { faults |= 0x10; }
    else                           { bme280_parse_cal(); }
    if (imu_init())                faults |= 0x20;
    return faults;
}

int cs_sensor_sample(cs_reading_t *r)
{
    memset(r, 0, sizeof(*r));

    /* 1. Capacitive level: 16 samples, median. */
    static uint32_t lvl_buf[CS_LEVEL_SAMPLES];
    for (int i = 0; i < CS_LEVEL_SAMPLES; i++) {
        lvl_buf[i] = fdc2214_read_ch0();
        mock_delay_ms(1);
    }
    uint32_t lvl_raw = cs_median_u32(lvl_buf, CS_LEVEL_SAMPLES);
    r->level_raw = lvl_raw;
    r->level_ok  = (lvl_raw != 0);
    if (cs_calibration_present())
        r->level_pct = cs_calibration_to_pct(lvl_raw);
    else
        r->level_pct = -1.0f;

    /* 2. RTDs: one-shot each. */
    uint16_t code_a = max31865_one_shot(CS_SPI_CS_MAX31865_A);
    uint16_t code_b = max31865_one_shot(CS_SPI_CS_MAX31865_B);
    uint16_t code_c = max31865_one_shot(CS_SPI_CS_MAX31865_C);
    r->rtd_degC[CS_RTD_TOP] = cs_rtd_code_to_degC(code_a);
    r->rtd_degC[CS_RTD_MID] = cs_rtd_code_to_degC(code_b);
    r->rtd_degC[CS_RTD_BOT] = cs_rtd_code_to_degC(code_c);
    r->rtd_ok[0] = (code_a != 0xFFFF);
    r->rtd_ok[1] = (code_b != 0xFFFF);
    r->rtd_ok[2] = (code_c != 0xFFFF);
    /* Gradient proxy: (top - bottom) degC; in steady state this is large. */
    if (r->rtd_ok[0] && r->rtd_ok[2])
        r->gradient_slope = r->rtd_degC[CS_RTD_TOP] - r->rtd_degC[CS_RTD_BOT];
    else
        r->gradient_slope = 0.0f;

    /* 3. IMU. */
    imu_read(&r->tilt_deg, &r->vib_rms_g);
    r->imu_ok = (r->tilt_deg >= 0.0f);

    /* 4. BME280 forced read. */
    bme280_forced_trigger();
    mock_delay_ms(10);
    uint8_t raw[8];
    mock_i2c_read(BME280_I2C_ADDR, BME280_REG_PRESS_MSB, raw, 8);
    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) |
                    (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) |
                    (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];
    if (adc_T != 0) {
        int32_t T100 = bme280_compensate_T(adc_T);
        r->ambient_degC = (float)T100 / 100.0f;
        r->ambient_hpa  = (float)bme280_compensate_P(adc_P) / 256.0f;
        r->ambient_rh   = (float)adc_H / 1024.0f;  /* simplified */
        r->ambient_ok   = true;
    } else {
        r->ambient_ok = false;
    }

    /* 5. GPIOs (debounced by repeated reads). */
    uint32_t s_lid  = 0, s_enc = 0, s_mains = 0, s_probe = 0;
    for (int i = 0; i < 3; i++) {
        s_lid   += (mock_gpio_get(CS_GPIO_LID_REED)   != 0);
        s_enc   += (mock_gpio_get(CS_GPIO_ENC_HALL)   != 0);
        s_mains += (mock_gpio_get(CS_GPIO_MAINS_OPTO) == 0); /* low = present */
        s_probe += (mock_gpio_get(CS_GPIO_PROBE_PRESENT) != 0);
        mock_delay_ms(CS_DEBOUNCE_MS / 3);
    }
    r->lid_open       = (s_lid   >= 2);
    r->enclosure_open = (s_enc   >= 2);
    r->mains_present  = (s_mains >= 2);
    r->probe_present  = (s_probe >= 2);
    return 0;
}