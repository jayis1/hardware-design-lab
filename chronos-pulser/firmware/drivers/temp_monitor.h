// temp_monitor.h — TMP117 Temperature Monitor Driver Header
// Texas Instruments TMP117AIDRVR: ±0.1°C accuracy, I2C interface
// Used for FPGA die temperature monitoring and thermal protection

#ifndef TEMP_MONITOR_H
#define TEMP_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

// TMP117 I2C register addresses
#define TMP117_REG_TEMP_RESULT      0x00
#define TMP117_REG_CONFIGURATION    0x01
#define TMP117_REG_TEMP_HIGH_LIMIT  0x02
#define TMP117_REG_TEMP_LOW_LIMIT   0x03
#define TMP117_REG_EEPROM_UNLOCK    0x04
#define TMP117_REG_EEPROM1          0x05
#define TMP117_REG_EEPROM2          0x06
#define TMP117_REG_TEMP_OFFSET      0x07
#define TMP117_REG_EEPROM3          0x08
#define TMP117_REG_DEVICE_ID        0x0F

// TMP117 configuration register bit definitions
#define TMP117_CFG_AVG_0            0x0000  // No averaging
#define TMP117_CFG_AVG_8            0x0020  // 8-sample averaging
#define TMP117_CFG_AVG_32           0x0040  // 32-sample averaging
#define TMP117_CFG_AVG_64           0x0060  // 64-sample averaging
#define TMP117_CFG_CONV_15_5MS      0x0000  // 15.5 ms conversion
#define TMP117_CFG_CONV_125MS       0x0080  // 125 ms
#define TMP117_CFG_CONV_250MS       0x0100  // 250 ms
#define TMP117_CFG_CONV_500MS       0x0180  // 500 ms
#define TMP117_CFG_CONV_1S          0x0200  // 1 second
#define TMP117_CFG_CONV_4S          0x0280  // 4 seconds
#define TMP117_CFG_CONV_8S          0x0300  // 8 seconds
#define TMP117_CFG_CONV_16S         0x0380  // 16 seconds
#define TMP117_CFG_CONTINUOUS       0x0000  // Continuous conversion mode
#define TMP117_CFG_SHUTDOWN         0x0400  // Shutdown mode
#define TMP117_CFG_ONE_SHOT         0x0C00  // One-shot mode
#define TMP117_CFG_ALERT_ACTIVE     0x2000  // Alert pin active
#define TMP117_CFG_DATA_READY       0x8000  // Data ready flag

// TMP117 device ID expected value
#define TMP117_DEVICE_ID            0x0117

// Temperature conversion: raw 16-bit signed → °C
// LSB = 0.0078125°C (1/128°C)
#define TMP117_LSB_C                0.0078125f
#define TMP117_RAW_TO_C(raw)        ((float)((int16_t)(raw)) * TMP117_LSB_C)

// Default alert limits
#define TMP117_DEFAULT_HIGH_C       70.0f
#define TMP117_DEFAULT_LOW_C        -10.0f

// Function prototypes
int   temp_monitor_init(void);
int   temp_monitor_deinit(void);
float temp_monitor_read(void);
int   temp_monitor_read_raw(int16_t *raw);
bool  temp_monitor_alert_active(void);
int   temp_monitor_set_alert_limits(float low_c, float high_c);
int   temp_monitor_set_mode(uint16_t mode);
int   temp_monitor_get_config(uint16_t *config);
int   temp_monitor_verify_device(void);
int   temp_monitor_set_offset(float offset_c);
float temp_monitor_get_offset(void);

#endif // TEMP_MONITOR_H
