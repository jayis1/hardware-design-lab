// temp_monitor.c — TMP117 Temperature Monitor Driver
// ±0.1°C accuracy digital temperature sensor via I2C
// Used for FPGA thermal monitoring and protection
// Full implementation with EEPROM access, one-shot mode, and statistics

#include "temp_monitor.h"
#include "../registers.h"
#include "../board.h"
#include "i2c_driver.h"

static bool driver_initialized = false;
static uint16_t current_config = 0;
static float current_offset = 0.0f;
static float last_temperature = 25.0f;
static float min_temperature = 999.0f;
static float max_temperature = -999.0f;
static uint32_t read_count = 0;
static uint32_t alert_count = 0;

int temp_monitor_init(void) {
    if (driver_initialized) return 0;

    // Verify device is present
    int ret = temp_monitor_verify_device();
    if (ret != 0) return ret;

    // Configure for continuous conversion, 8-sample averaging, 125 ms period
    uint16_t cfg = TMP117_CFG_CONTINUOUS | TMP117_CFG_AVG_8 | TMP117_CFG_CONV_125MS;
    ret = temp_monitor_set_mode(cfg);
    if (ret != 0) return ret;

    // Set default alert limits
    ret = temp_monitor_set_alert_limits(TMP117_DEFAULT_LOW_C, TMP117_DEFAULT_HIGH_C);
    if (ret != 0) {
        // Non-fatal — continue without alert limits
    }

    // Read initial temperature to populate statistics
    float init_temp = temp_monitor_read();
    if (init_temp > -900.0f) {
        last_temperature = init_temp;
        min_temperature = init_temp;
        max_temperature = init_temp;
    }

    current_config = cfg;
    driver_initialized = true;
    return 0;
}

int temp_monitor_deinit(void) {
    if (!driver_initialized) return -1;
    temp_monitor_set_mode(TMP117_CFG_SHUTDOWN);
    driver_initialized = false;
    return 0;
}

float temp_monitor_read(void) {
    int16_t raw;
    int ret = temp_monitor_read_raw(&raw);
    if (ret != 0) return -999.0f;

    float temp = TMP117_RAW_TO_C(raw) + current_offset;

    // Update statistics
    last_temperature = temp;
    if (temp < min_temperature) min_temperature = temp;
    if (temp > max_temperature) max_temperature = temp;
    read_count++;

    // Check alert
    if (temp_monitor_alert_active()) {
        alert_count++;
    }

    return temp;
}

int temp_monitor_read_raw(int16_t *raw) {
    if (!driver_initialized) return -1;

    uint8_t rx[2];
    int ret = i2c_read_burst(I2C_ADDR_TMP117, TMP117_REG_TEMP_RESULT, rx, 2);
    if (ret != 0) return ret;

    *raw = ((int16_t)rx[0] << 8) | rx[1];
    return 0;
}

// One-shot conversion: trigger single measurement, wait, read result
int temp_monitor_one_shot(float *temp_out) {
    if (!driver_initialized) return -1;

    // Set one-shot mode
    uint16_t cfg = TMP117_CFG_ONE_SHOT | TMP117_CFG_AVG_8;
    uint8_t data[2] = { (uint8_t)(cfg >> 8), (uint8_t)(cfg & 0xFF) };
    int ret = i2c_write_burst(I2C_ADDR_TMP117, TMP117_REG_CONFIGURATION, data, 2);
    if (ret != 0) return ret;

    // Wait for conversion (125 ms with 8x averaging)
    for (volatile int d = 0; d < 12500000; d++) NOP();  // ~125 ms at 100 MHz

    // Read result
    int16_t raw;
    ret = temp_monitor_read_raw(&raw);
    if (ret != 0) {
        // Restore continuous mode
        temp_monitor_set_mode(current_config);
        return ret;
    }

    *temp_out = TMP117_RAW_TO_C(raw) + current_offset;

    // Restore continuous mode
    temp_monitor_set_mode(current_config);

    return 0;
}

bool temp_monitor_alert_active(void) {
    if (!driver_initialized) return false;

    uint8_t rx[2];
    int ret = i2c_read_burst(I2C_ADDR_TMP117, TMP117_REG_CONFIGURATION, rx, 2);
    if (ret != 0) return false;

    uint16_t cfg = ((uint16_t)rx[0] << 8) | rx[1];
    return (cfg & TMP117_CFG_ALERT_ACTIVE) ? true : false;
}

int temp_monitor_set_alert_limits(float low_c, float high_c) {
    if (!driver_initialized) return -1;

    int16_t low_raw = (int16_t)(low_c / TMP117_LSB_C);
    int16_t high_raw = (int16_t)(high_c / TMP117_LSB_C);

    uint8_t low_data[2] = { (uint8_t)(low_raw >> 8), (uint8_t)(low_raw & 0xFF) };
    uint8_t high_data[2] = { (uint8_t)(high_raw >> 8), (uint8_t)(high_raw & 0xFF) };

    int ret = i2c_write_burst(I2C_ADDR_TMP117, TMP117_REG_TEMP_LOW_LIMIT, low_data, 2);
    if (ret != 0) return ret;

    ret = i2c_write_burst(I2C_ADDR_TMP117, TMP117_REG_TEMP_HIGH_LIMIT, high_data, 2);
    return ret;
}

int temp_monitor_set_mode(uint16_t mode) {
    if (!driver_initialized) return -1;

    uint8_t data[2] = { (uint8_t)(mode >> 8), (uint8_t)(mode & 0xFF) };
    int ret = i2c_write_burst(I2C_ADDR_TMP117, TMP117_REG_CONFIGURATION, data, 2);
    if (ret != 0) return ret;

    current_config = mode;
    return 0;
}

int temp_monitor_get_config(uint16_t *config) {
    if (!driver_initialized) return -1;

    uint8_t rx[2];
    int ret = i2c_read_burst(I2C_ADDR_TMP117, TMP117_REG_CONFIGURATION, rx, 2);
    if (ret != 0) return ret;

    *config = ((uint16_t)rx[0] << 8) | rx[1];
    return 0;
}

int temp_monitor_verify_device(void) {
    uint8_t rx[2];
    int ret = i2c_read_burst(I2C_ADDR_TMP117, TMP117_REG_DEVICE_ID, rx, 2);
    if (ret != 0) return ret;

    uint16_t device_id = ((uint16_t)rx[0] << 8) | rx[1];
    if (device_id != TMP117_DEVICE_ID) return -2;

    return 0;
}

int temp_monitor_set_offset(float offset_c) {
    if (!driver_initialized) return -1;

    int16_t offset_raw = (int16_t)(offset_c / TMP117_LSB_C);
    uint8_t data[2] = { (uint8_t)(offset_raw >> 8), (uint8_t)(offset_raw & 0xFF) };

    int ret = i2c_write_burst(I2C_ADDR_TMP117, TMP117_REG_TEMP_OFFSET, data, 2);
    if (ret != 0) return ret;

    current_offset = offset_c;
    return 0;
}

float temp_monitor_get_offset(void) {
    return current_offset;
}

// Read EEPROM register (calibration data stored by TI)
int temp_monitor_read_eeprom(uint8_t eeprom_idx, uint16_t *value) {
    if (!driver_initialized) return -1;
    if (eeprom_idx > 3) return -2;

    uint8_t reg = TMP117_REG_EEPROM1 + eeprom_idx;
    uint8_t rx[2];
    int ret = i2c_read_burst(I2C_ADDR_TMP117, reg, rx, 2);
    if (ret != 0) return ret;

    *value = ((uint16_t)rx[0] << 8) | rx[1];
    return 0;
}

// Get temperature statistics
float temp_monitor_get_last(void) { return last_temperature; }
float temp_monitor_get_min(void) { return min_temperature; }
float temp_monitor_get_max(void) { return max_temperature; }
uint32_t temp_monitor_get_read_count(void) { return read_count; }
uint32_t temp_monitor_get_alert_count(void) { return alert_count; }

// Reset statistics
void temp_monitor_reset_stats(void) {
    min_temperature = 999.0f;
    max_temperature = -999.0f;
    read_count = 0;
    alert_count = 0;
}

// Self-test: verify sensor communication and data validity
int temp_monitor_self_test(void) {
    if (!driver_initialized) return -1;

    // Test 1: Read temperature — should be reasonable
    float temp = temp_monitor_read();
    if (temp < -50.0f || temp > 150.0f) return -10;  // Unreasonable

    // Test 2: Read config register
    uint16_t cfg;
    int ret = temp_monitor_get_config(&cfg);
    if (ret != 0) return -11;

    // Test 3: Verify device ID again
    ret = temp_monitor_verify_device();
    if (ret != 0) return -12;

    // Test 4: One-shot mode
    float one_shot_temp;
    ret = temp_monitor_one_shot(&one_shot_temp);
    if (ret != 0) return -13;
    if (one_shot_temp < -50.0f || one_shot_temp > 150.0f) return -14;

    // Test 5: Verify one-shot and continuous readings are close
    float diff = (one_shot_temp > temp) ? (one_shot_temp - temp) : (temp - one_shot_temp);
    if (diff > 5.0f) return -15;  // Should be within 5°C

    return 0;
}
