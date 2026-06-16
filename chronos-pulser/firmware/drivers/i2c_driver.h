// i2c_driver.h — I2C Master Driver Header
// Bit-banged I2C master for TMP117 temperature sensor and other peripherals
// Supports 100 kHz and 400 kHz modes, 7-bit and 10-bit addressing

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// I2C speed modes
#define I2C_SPEED_STANDARD      100000   // 100 kHz
#define I2C_SPEED_FAST          400000   // 400 kHz
#define I2C_SPEED_DEFAULT       I2C_SPEED_FAST

// I2C addressing modes
#define I2C_ADDR_7BIT           0
#define I2C_ADDR_10BIT          1

// I2C error codes
#define I2C_ERR_NONE            0
#define I2C_ERR_NACK_ADDR      -1
#define I2C_ERR_NACK_DATA      -2
#define I2C_ERR_BUSY           -3
#define I2C_ERR_TIMEOUT        -4
#define I2C_ERR_ARB_LOST       -5
#define I2C_ERR_PARAM          -6

// I2C transfer flags
#define I2C_FLAG_START          (1 << 0)
#define I2C_FLAG_STOP           (1 << 1)
#define I2C_FLAG_READ           (1 << 2)
#define I2C_FLAG_WRITE          0
#define I2C_FLAG_NO_STOP        (1 << 3)  // Don't send stop (for repeated start)

// I2C transaction structure
typedef struct {
    uint8_t  address;       // 7-bit or 10-bit address
    uint8_t  addr_mode;     // I2C_ADDR_7BIT or I2C_ADDR_10BIT
    uint8_t  *tx_data;      // Data to transmit (NULL if read-only)
    uint8_t  tx_len;        // Number of bytes to transmit
    uint8_t  *rx_data;      // Buffer for received data (NULL if write-only)
    uint8_t  rx_len;        // Number of bytes to receive
    uint32_t speed_hz;      // Bus speed for this transaction
    uint32_t timeout_ms;    // Timeout in milliseconds
} i2c_transaction_t;

// Function prototypes
int  i2c_init(void);
int  i2c_deinit(void);
int  i2c_set_speed(uint32_t speed_hz);
int  i2c_transfer(const i2c_transaction_t *trans);
int  i2c_write(uint8_t addr, uint8_t reg, uint8_t data);
int  i2c_read(uint8_t addr, uint8_t reg, uint8_t *data);
int  i2c_write_burst(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len);
int  i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
int  i2c_probe(uint8_t addr);
int  i2c_scan(uint8_t *found_addrs, uint8_t max_count, uint8_t *count);
void i2c_reset_bus(void);
bool i2c_is_busy(void);

#endif // I2C_DRIVER_H
