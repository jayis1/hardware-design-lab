/* CondenScope sensor acquisition and fixed-point fusion
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "sensors.h"
#include "../registers.h"
#include <stddef.h>
#include <string.h>

static mlx_calibration_t calibration;
static uint16_t eeprom[MLX_EEPROM_WORDS];
static uint16_t raw_frame[MLX_FRAME_WORDS];
static int16_t subpage_pixels[2][THERMAL_PIXELS];
static bool subpage_valid[2];
static int16_t ambient_trim_centic;

static uint8_t crc8(const uint8_t *bytes, uint8_t count)
{
    uint8_t crc = 0xFFU;
    for (uint8_t i = 0; i < count; ++i) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1U) ^ 0x31U)
                                : (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

static int16_t sign_extend(uint16_t value, uint8_t bits)
{
    uint16_t sign = (uint16_t)(1U << (bits - 1U));
    uint16_t mask = (uint16_t)((1U << bits) - 1U);
    value &= mask;
    return (int16_t)((value ^ sign) - sign);
}

static int32_t integer_sqrt(int64_t value)
{
    if (value <= 0) {
        return 0;
    }
    uint64_t x = (uint64_t)value;
    uint64_t result = 0;
    uint64_t bit = (uint64_t)1U << 62U;
    while (bit > x) {
        bit >>= 2U;
    }
    while (bit != 0U) {
        if (x >= result + bit) {
            x -= result + bit;
            result = (result >> 1U) + bit;
        } else {
            result >>= 1U;
        }
        bit >>= 2U;
    }
    return (int32_t)result;
}

/* Natural log ratio ln(x/10000), returned Q16. This uses range reduction
 * followed by atanh's odd series, accurate to better than 0.003 over RH 5..100%. */
static int32_t log_humidity_q16(uint16_t humidity)
{
    if (humidity < 500U) {
        humidity = 500U;
    }
    if (humidity > 10000U) {
        humidity = 10000U;
    }
    int32_t x_q16 = ((int32_t)humidity << 16) / 10000;
    int32_t exponent = 0;
    while (x_q16 < 32768) {
        x_q16 <<= 1;
        --exponent;
    }
    while (x_q16 >= 65536) {
        x_q16 >>= 1;
        ++exponent;
    }
    int32_t z = ((x_q16 - 65536) << 16) / (x_q16 + 65536);
    int64_t z2 = ((int64_t)z * z) >> 16;
    int64_t term = z;
    int64_t sum = term;
    term = (term * z2) >> 16;
    sum += term / 3;
    term = (term * z2) >> 16;
    sum += term / 5;
    term = (term * z2) >> 16;
    sum += term / 7;
    return (int32_t)(2 * sum + exponent * 45426); /* ln(2) Q16 */
}

int16_t sensors_dew_point_centic(int16_t temperature_centic,
                                uint16_t humidity_centi_percent)
{
    /* Magnus equation: gamma=ln(RH/100)+a*T/(b+T), Td=b*gamma/(a-gamma). */
    const int32_t a_q16 = 1158021; /* 17.67 */
    const int32_t b_centic = 24350;
    int32_t denominator = b_centic + temperature_centic;
    if (denominator < 1000) {
        denominator = 1000;
    }
    int32_t thermal_q16 = (int32_t)(((int64_t)a_q16 * temperature_centic) /
                                    denominator);
    int32_t gamma_q16 = log_humidity_q16(humidity_centi_percent) + thermal_q16;
    int32_t divisor = a_q16 - gamma_q16;
    if (divisor < 65536) {
        divisor = 65536;
    }
    int32_t dew = (int32_t)(((int64_t)b_centic * gamma_q16) / divisor);
    return (int16_t)CLAMP(dew, -6000, 8500);
}

static sensor_result_t read_mlx_eeprom(void)
{
    if (board_i2c_read16(1U, MLX90640_ADDRESS, MLX90640_EEPROM_START,
                         eeprom, MLX_EEPROM_WORDS) != 0) {
        return SENSOR_E_BUS;
    }
    if (eeprom[10] == 0U || eeprom[10] == 0xFFFFU) {
        return SENSOR_E_CALIBRATION;
    }
    return SENSOR_OK;
}

static sensor_result_t parse_mlx_calibration(void)
{
    memset(&calibration, 0, sizeof(calibration));
    calibration.offset_centic = (int16_t)(sign_extend(eeprom[17], 16U) / 4);
    calibration.gain_q12 = (uint16_t)(eeprom[48] == 0U ? 4096U : eeprom[48]);
    calibration.kta_q10 = sign_extend((uint16_t)(eeprom[54] >> 8U), 8U);
    calibration.kv_q10 = sign_extend((uint16_t)(eeprom[52] >> 8U), 8U);
    calibration.alpha_q20 = (uint16_t)(eeprom[33] & 0x7FFFU);
    if (calibration.alpha_q20 == 0U) {
        calibration.alpha_q20 = 4096U;
    }
    calibration.tgc_q10 = sign_extend(eeprom[60], 8U);
    calibration.ks_ta_q10 = sign_extend((uint16_t)(eeprom[60] >> 8U), 8U);
    calibration.cp_offset[0] = sign_extend(eeprom[58], 10U);
    calibration.cp_offset[1] = (int16_t)(calibration.cp_offset[0] +
                               sign_extend((uint16_t)(eeprom[58] >> 10U), 6U));
    calibration.broken_pixel = 0xFFFFU;
    calibration.outlier_pixel = 0xFFFFU;

    for (uint16_t i = 0; i < THERMAL_PIXELS; ++i) {
        uint16_t marker = (uint16_t)(eeprom[64U + i] & 0x0001U);
        if (eeprom[64U + i] == 0U) {
            if (calibration.broken_pixel != 0xFFFFU) {
                return SENSOR_E_CALIBRATION;
            }
            calibration.broken_pixel = i;
        } else if (marker != 0U && (eeprom[64U + i] & 0x0006U) == 0x0006U) {
            if (calibration.outlier_pixel != 0xFFFFU) {
                return SENSOR_E_CALIBRATION;
            }
            calibration.outlier_pixel = i;
        }
    }
    calibration.valid = true;
    return SENSOR_OK;
}

static sensor_result_t configure_mlx(void)
{
    uint16_t control = 0;
    if (board_i2c_read16(1U, MLX90640_ADDRESS, MLX90640_CONTROL,
                         &control, 1U) != 0) {
        return SENSOR_E_BUS;
    }
    control &= (uint16_t)~(MLX_CTRL_REFRESH_MASK | MLX_CTRL_SUBPAGE_REPEAT);
    control |= (uint16_t)(MLX_CTRL_REFRESH_8HZ | MLX_CTRL_CHESS_MODE);
    if (board_i2c_write16(1U, MLX90640_ADDRESS, MLX90640_CONTROL,
                          &control, 1U) != 0) {
        return SENSOR_E_BUS;
    }
    return SENSOR_OK;
}

static sensor_result_t configure_tof(void)
{
    uint8_t id[2] = {0U, 0U};
    board_delay_ms(2U);
    if (board_i2c_read8(2U, VL53L4CD_ADDRESS, VL53_IDENTIFICATION_MODEL_ID,
                        id, 2U) != 0) {
        return SENSOR_E_BUS;
    }
    if ((((uint16_t)id[0] << 8U) | id[1]) != VL53_MODEL_ID_EXPECTED) {
        return SENSOR_E_ID;
    }
    const uint8_t timing_a[2] = {0x01U, 0xCCU};
    const uint8_t timing_b[2] = {0x02U, 0x01U};
    const uint8_t intermeasurement[4] = {0x00U, 0x00U, 0x00U, 0x00U};
    if (board_i2c_write8(2U, VL53L4CD_ADDRESS, VL53_RANGE_CONFIG_A,
                         timing_a, sizeof(timing_a)) != 0 ||
        board_i2c_write8(2U, VL53L4CD_ADDRESS, VL53_RANGE_CONFIG_B,
                         timing_b, sizeof(timing_b)) != 0 ||
        board_i2c_write8(2U, VL53L4CD_ADDRESS, VL53_INTERMEASUREMENT_MS,
                         intermeasurement, sizeof(intermeasurement)) != 0) {
        return SENSOR_E_BUS;
    }
    const uint8_t start = 0x21U;
    return board_i2c_write8(2U, VL53L4CD_ADDRESS, VL53_SYSTEM_START,
                            &start, 1U) == 0 ? SENSOR_OK : SENSOR_E_BUS;
}

sensor_result_t sensors_init(void)
{
    subpage_valid[0] = false;
    subpage_valid[1] = false;
    ambient_trim_centic = 0;
    sensor_result_t result = read_mlx_eeprom();
    if (result != SENSOR_OK) {
        return result;
    }
    result = parse_mlx_calibration();
    if (result != SENSOR_OK) {
        return result;
    }
    result = configure_mlx();
    if (result != SENSOR_OK) {
        return result;
    }
    return configure_tof();
}

static sensor_result_t read_sht45(environment_t *environment)
{
    const uint8_t command = 0xFDU;
    uint8_t response[6];
    if (board_i2c_write8(2U, SHT45_ADDRESS, 0U, &command, 1U) != 0) {
        return SENSOR_E_BUS;
    }
    board_delay_ms(SHT45_CONVERSION_MS);
    if (board_i2c_read8(2U, SHT45_ADDRESS, 0U, response, sizeof(response)) != 0) {
        return SENSOR_E_BUS;
    }
    if (crc8(response, 2U) != response[2] || crc8(&response[3], 2U) != response[5]) {
        return SENSOR_E_CRC;
    }
    uint16_t raw_t = (uint16_t)(((uint16_t)response[0] << 8U) | response[1]);
    uint16_t raw_rh = (uint16_t)(((uint16_t)response[3] << 8U) | response[4]);
    int32_t temperature = -4500 + ((17500L * raw_t) / 65535L);
    int32_t humidity = -600 + ((12500L * raw_rh) / 65535L);
    environment->air_temperature_centic = (int16_t)CLAMP(temperature, -4500, 12500);
    environment->relative_humidity_centi_percent =
        (uint16_t)CLAMP(humidity, 0, 10000);
    environment->dew_point_centic = sensors_dew_point_centic(
        environment->air_temperature_centic,
        environment->relative_humidity_centi_percent);
    return SENSOR_OK;
}

static sensor_result_t read_distance(uint16_t *distance_mm)
{
    uint8_t status = 0U;
    if (board_i2c_read8(2U, VL53L4CD_ADDRESS, VL53_GPIO_TIO_HV_STATUS,
                        &status, 1U) != 0) {
        return SENSOR_E_BUS;
    }
    if ((status & 0x01U) == 0U) {
        return SENSOR_E_TIMEOUT;
    }
    uint8_t data[2];
    if (board_i2c_read8(2U, VL53L4CD_ADDRESS, VL53_RESULT_DISTANCE_MM,
                        data, 2U) != 0) {
        return SENSOR_E_BUS;
    }
    *distance_mm = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
    return SENSOR_OK;
}

static void read_battery(environment_t *environment)
{
    uint8_t data[4];
    if (board_i2c_read8(2U, MAX17048_ADDRESS, MAX17048_REG_VCELL,
                        data, sizeof(data)) == 0) {
        uint16_t raw_v = (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
        environment->battery_mv = (uint16_t)(((uint32_t)(raw_v >> 4U) * 125U) / 100U);
        environment->battery_percent = (uint8_t)CLAMP(data[2], 0U, 100U);
    } else {
        uint16_t adc = board_adc_read(0U);
        environment->battery_mv = (uint16_t)(((uint32_t)adc * ADC_REFERENCE_MV *
            VBAT_DIVIDER_NUM) / (ADC_FULL_SCALE * VBAT_DIVIDER_DEN));
        int32_t percent = ((int32_t)environment->battery_mv - 3300) / 9;
        environment->battery_percent = (uint8_t)CLAMP(percent, 0, 100);
    }
}

sensor_result_t sensors_read_environment(environment_t *environment)
{
    if (environment == NULL) {
        return SENSOR_E_CALIBRATION;
    }
    sensor_result_t result = read_sht45(environment);
    if (result != SENSOR_OK) {
        return result;
    }
    result = read_distance(&environment->distance_mm);
    if (result == SENSOR_E_TIMEOUT) {
        environment->distance_mm = 0U;
    } else if (result != SENSOR_OK) {
        return result;
    }
    read_battery(environment);
    environment->timestamp_ms = board_millis();
    return SENSOR_OK;
}

static int16_t calculate_ambient(void)
{
    int16_t ptat = (int16_t)raw_frame[800];
    int16_t ptat_art = (int16_t)raw_frame[768];
    int32_t ratio = ptat_art == 0 ? 0 : ((int32_t)ptat << 10) / ptat_art;
    int32_t ambient = 2500 + ((ratio - 1024) * 13) / 4;
    ambient += ambient_trim_centic;
    return (int16_t)CLAMP(ambient, -4000, 12500);
}

static int16_t compensate_pixel(uint16_t index, uint8_t subpage,
                                int16_t ambient, uint16_t emissivity_q15)
{
    int32_t raw = sign_extend(raw_frame[index], 16U);
    int32_t offset = sign_extend(eeprom[64U + index], 10U);
    int32_t gain_raw = sign_extend(raw_frame[778], 16U);
    if (gain_raw == 0) {
        gain_raw = 1;
    }
    int32_t gain = ((int32_t)calibration.gain_q12 << 12) / gain_raw;
    int32_t signal = ((raw * gain) >> 12) - offset;
    signal -= calibration.cp_offset[subpage];
    signal -= ((int32_t)calibration.tgc_q10 * raw_frame[776]) >> 10;
    signal = (int32_t)(((int64_t)signal << 15) / emissivity_q15);

    /* Approximate fourth-root radiometry around ambient. The integer square
     * roots keep Cortex-M33 execution deterministic and avoid libm. */
    int64_t ambient_kelvin_centi = (int64_t)ambient + 27315;
    int64_t ambient_sq = ambient_kelvin_centi * ambient_kelvin_centi;
    int64_t ambient_fourth_scaled = (ambient_sq / 1000) * (ambient_sq / 1000);
    int64_t radiance = ambient_fourth_scaled +
        ((int64_t)signal * 1500000) / calibration.alpha_q20;
    if (radiance < 1) {
        radiance = 1;
    }
    int32_t root1 = integer_sqrt(radiance);
    int32_t kelvin_centi = integer_sqrt((int64_t)root1 * 1000000);
    int32_t object = kelvin_centi - 27315;
    if (object < -4000 || object > 30000) {
        object = ambient + (signal / 18);
    }
    return (int16_t)CLAMP(object, -4000, 30000);
}

static void repair_bad_pixel(int16_t *pixels, uint16_t bad)
{
    if (bad >= THERMAL_PIXELS) {
        return;
    }
    uint16_t x = bad % THERMAL_WIDTH;
    int32_t sum = 0;
    uint8_t count = 0U;
    if (x > 0U) { sum += pixels[bad - 1U]; ++count; }
    if (x + 1U < THERMAL_WIDTH) { sum += pixels[bad + 1U]; ++count; }
    if (bad >= THERMAL_WIDTH) { sum += pixels[bad - THERMAL_WIDTH]; ++count; }
    if (bad + THERMAL_WIDTH < THERMAL_PIXELS) {
        sum += pixels[bad + THERMAL_WIDTH]; ++count;
    }
    if (count != 0U) {
        pixels[bad] = (int16_t)(sum / count);
    }
}

sensor_result_t sensors_read_thermal(thermal_frame_t *frame,
                                     uint16_t emissivity_q15)
{
    if (frame == NULL || !calibration.valid || emissivity_q15 < 16384U) {
        return SENSOR_E_CALIBRATION;
    }
    uint16_t status = 0U;
    if (board_i2c_read16(1U, MLX90640_ADDRESS, MLX90640_STATUS, &status, 1U) != 0) {
        return SENSOR_E_BUS;
    }
    if ((status & MLX_STATUS_NEW_DATA) == 0U) {
        return SENSOR_E_TIMEOUT;
    }
    uint8_t subpage = (uint8_t)(status & MLX_STATUS_SUBPAGE);
    if (board_i2c_read16(1U, MLX90640_ADDRESS, MLX90640_RAM_START,
                         raw_frame, MLX_FRAME_WORDS) != 0) {
        return SENSOR_E_BUS;
    }
    uint16_t clear = (uint16_t)(status & ~MLX_STATUS_NEW_DATA);
    (void)board_i2c_write16(1U, MLX90640_ADDRESS, MLX90640_STATUS, &clear, 1U);

    int16_t ambient = calculate_ambient();
    for (uint16_t row = 0; row < THERMAL_HEIGHT; ++row) {
        for (uint16_t column = 0; column < THERMAL_WIDTH; ++column) {
            uint16_t index = (uint16_t)(row * THERMAL_WIDTH + column);
            uint8_t pattern = (uint8_t)((row + column) & 1U);
            if (pattern == subpage) {
                subpage_pixels[subpage][index] = compensate_pixel(
                    index, subpage, ambient, emissivity_q15);
            }
        }
    }
    subpage_valid[subpage] = true;
    frame->complete = subpage_valid[0] && subpage_valid[1];
    frame->ambient_centic = ambient;
    frame->subpage = subpage;
    frame->min_centic = 32767;
    frame->max_centic = -32768;

    for (uint16_t i = 0; i < THERMAL_PIXELS; ++i) {
        uint8_t pattern = (uint8_t)(((i / THERMAL_WIDTH) + (i % THERMAL_WIDTH)) & 1U);
        frame->pixels_centic[i] = subpage_pixels[pattern][i];
    }
    repair_bad_pixel(frame->pixels_centic, calibration.broken_pixel);
    repair_bad_pixel(frame->pixels_centic, calibration.outlier_pixel);
    for (uint16_t i = 0; i < THERMAL_PIXELS; ++i) {
        if (frame->pixels_centic[i] < frame->min_centic) {
            frame->min_centic = frame->pixels_centic[i];
            frame->min_index = i;
        }
        if (frame->pixels_centic[i] > frame->max_centic) {
            frame->max_centic = frame->pixels_centic[i];
            frame->max_index = i;
        }
    }
    ++frame->sequence;
    return SENSOR_OK;
}

uint8_t sensors_risk_level(int16_t margin_centic)
{
    if (margin_centic <= DEW_DANGER_CENTIC) {
        return 2U;
    }
    if (margin_centic <= DEW_WARNING_CENTIC) {
        return 1U;
    }
    return 0U;
}

void sensors_generate_points(const thermal_frame_t *frame,
                             const environment_t *environment,
                             map_point_t *points, uint16_t capacity,
                             uint16_t *written)
{
    if (written == NULL) {
        return;
    }
    *written = 0U;
    if (frame == NULL || environment == NULL || points == NULL ||
        !frame->complete || environment->distance_mm < DISTANCE_MIN_MM ||
        environment->distance_mm > DISTANCE_MAX_MM) {
        return;
    }
    uint16_t stride = capacity >= THERMAL_PIXELS ? 1U : 4U;
    for (uint16_t row = 0; row < THERMAL_HEIGHT && *written < capacity; row += stride) {
        for (uint16_t col = 0; col < THERMAL_WIDTH && *written < capacity; col += stride) {
            uint16_t index = (uint16_t)(row * THERMAL_WIDTH + col);
            map_point_t *point = &points[*written];
            point->pixel = index;
            point->surface_centic = frame->pixels_centic[index];
            point->margin_centic = (int16_t)(point->surface_centic -
                                             environment->dew_point_centic);
            point->risk = sensors_risk_level(point->margin_centic);
            point->distance_mm = environment->distance_mm;
            point->bearing_cdeg = (int16_t)(((int32_t)col * 5500 /
                                             (THERMAL_WIDTH - 1U)) - 2750);
            point->elevation_cdeg = (int16_t)(1850 - ((int32_t)row * 3700 /
                                               (THERMAL_HEIGHT - 1U)));
            ++(*written);
        }
    }
}

bool sensors_calibrate_ambient(uint16_t reference_temperature_centic)
{
    int16_t measured = calculate_ambient();
    int32_t trim = (int32_t)reference_temperature_centic - measured;
    if (trim < -500 || trim > 500) {
        return false;
    }
    ambient_trim_centic = (int16_t)trim;
    return true;
}

const mlx_calibration_t *sensors_calibration(void)
{
    return &calibration;
}
