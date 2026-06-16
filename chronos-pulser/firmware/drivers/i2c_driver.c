// i2c_driver.c — I2C Master Driver Implementation
// Hardware I2C controller via Wishbone peripheral at 0x20004000
// Used for TMP117 temperature sensor communication
// Supports 100 kHz and 400 kHz modes, 7-bit addressing

#include "i2c_driver.h"
#include "../registers.h"
#include "../board.h"

static volatile uint32_t *i2c_base = (volatile uint32_t *)I2C_BASE;
static bool driver_initialized = false;
static uint32_t current_speed = I2C_SPEED_DEFAULT;

int i2c_init(void) {
    if (driver_initialized) return 0;

    // Reset I2C controller
    REG_WRITE(I2C_CTRL_REG, 0);

    // Calculate prescaler for 400 kHz from 100 MHz system clock
    // I2C clock = sys_clk / (4 * (prescaler + 1))
    // For 400 kHz: prescaler = 100,000,000 / (4 * 400,000) - 1 = 61.5 → 61
    uint32_t prescaler = (CLOCK_SYS_FREQ / (4 * I2C_SPEED_FAST)) - 1;
    if (prescaler > 255) prescaler = 255;

    // Set prescaler in control register
    uint32_t ctrl = I2C_CTRL_ENABLE | (prescaler & 0xFF);
    REG_WRITE(I2C_CTRL_REG, ctrl);

    current_speed = I2C_SPEED_FAST;
    driver_initialized = true;
    return 0;
}

int i2c_deinit(void) {
    if (!driver_initialized) return -1;
    REG_WRITE(I2C_CTRL_REG, 0);
    driver_initialized = false;
    return 0;
}

int i2c_set_speed(uint32_t speed_hz) {
    if (!driver_initialized) return -1;
    if (speed_hz == 0 || speed_hz > 1000000) return I2C_ERR_PARAM;

    uint32_t prescaler = (CLOCK_SYS_FREQ / (4 * speed_hz)) - 1;
    if (prescaler > 255) prescaler = 255;

    uint32_t ctrl = REG_READ(I2C_CTRL_REG);
    ctrl = (ctrl & ~0xFF) | (prescaler & 0xFF) | I2C_CTRL_ENABLE;
    REG_WRITE(I2C_CTRL_REG, ctrl);

    current_speed = speed_hz;
    return 0;
}

// Low-level I2C transfer using hardware controller
int i2c_transfer(const i2c_transaction_t *trans) {
    if (!driver_initialized) return I2C_ERR_BUSY;
    if (!trans) return I2C_ERR_PARAM;

    // Set target address
    uint32_t addr_val = trans->address & 0x7F;
    if (trans->addr_mode == I2C_ADDR_10BIT) {
        addr_val |= (1 << 7);
    }
    REG_WRITE(I2C_ADDR_REG, addr_val);

    // Wait for bus to be free
    uint32_t timeout = trans->timeout_ms * 100000;
    while (timeout > 0) {
        if (!(REG_READ(I2C_STATUS_REG) & I2C_STATUS_BUSY)) break;
        timeout--;
    }
    if (timeout == 0) return I2C_ERR_TIMEOUT;

    // Transmit phase (if any)
    if (trans->tx_data && trans->tx_len > 0) {
        for (uint8_t i = 0; i < trans->tx_len; i++) {
            uint32_t tx_val = trans->tx_data[i] & 0xFF;
            if (i == 0) {
                tx_val |= I2C_TX_START;  // Start condition on first byte
            }
            if (i == trans->tx_len - 1 && !trans->rx_data) {
                tx_val |= I2C_TX_STOP;   // Stop after last byte if no read follows
            }
            tx_val |= I2C_TX_GO;

            REG_WRITE(I2C_TX_REG, tx_val);

            // Wait for completion
            timeout = trans->timeout_ms * 100000;
            while (timeout > 0) {
                uint32_t status = REG_READ(I2C_STATUS_REG);
                if (status & I2C_STATUS_COMPLETE) break;
                if (status & I2C_STATUS_NACK) return I2C_ERR_NACK_DATA;
                timeout--;
            }
            if (timeout == 0) return I2C_ERR_TIMEOUT;
        }
    }

    // Receive phase (if any)
    if (trans->rx_data && trans->rx_len > 0) {
        for (uint8_t i = 0; i < trans->rx_len; i++) {
            uint32_t tx_val = I2C_TX_READ | I2C_TX_GO;
            if (i == 0 && !trans->tx_data) {
                tx_val |= I2C_TX_START;  // Start if no prior transmit
            }
            if (i == trans->rx_len - 1) {
                tx_val |= I2C_TX_STOP;   // Stop after last byte
            }

            REG_WRITE(I2C_TX_REG, tx_val);

            // Wait for completion
            timeout = trans->timeout_ms * 100000;
            while (timeout > 0) {
                uint32_t status = REG_READ(I2C_STATUS_REG);
                if (status & I2C_STATUS_COMPLETE) break;
                if (status & I2C_STATUS_NACK) return I2C_ERR_NACK_DATA;
                timeout--;
            }
            if (timeout == 0) return I2C_ERR_TIMEOUT;

            // Read received data
            uint32_t rx = REG_READ(I2C_RX_REG);
            trans->rx_data[i] = rx & 0xFF;
        }
    }

    return I2C_ERR_NONE;
}

// Convenience: write single register
int i2c_write(uint8_t addr, uint8_t reg, uint8_t data) {
    uint8_t tx_buf[2] = { reg, data };
    i2c_transaction_t trans = {
        .address = addr,
        .addr_mode = I2C_ADDR_7BIT,
        .tx_data = tx_buf,
        .tx_len = 2,
        .rx_data = NULL,
        .rx_len = 0,
        .speed_hz = current_speed,
        .timeout_ms = 10
    };
    return i2c_transfer(&trans);
}

// Convenience: read single register
int i2c_read(uint8_t addr, uint8_t reg, uint8_t *data) {
    uint8_t tx_buf[1] = { reg };
    i2c_transaction_t trans = {
        .address = addr,
        .addr_mode = I2C_ADDR_7BIT,
        .tx_data = tx_buf,
        .tx_len = 1,
        .rx_data = data,
        .rx_len = 1,
        .speed_hz = current_speed,
        .timeout_ms = 10
    };
    return i2c_transfer(&trans);
}

// Convenience: burst write
int i2c_write_burst(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t len) {
    if (len > 128) return I2C_ERR_PARAM;
    uint8_t tx_buf[129];
    tx_buf[0] = reg;
    for (uint8_t i = 0; i < len; i++) tx_buf[i + 1] = data[i];

    i2c_transaction_t trans = {
        .address = addr,
        .addr_mode = I2C_ADDR_7BIT,
        .tx_data = tx_buf,
        .tx_len = len + 1,
        .rx_data = NULL,
        .rx_len = 0,
        .speed_hz = current_speed,
        .timeout_ms = 20
    };
    return i2c_transfer(&trans);
}

// Convenience: burst read
int i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (len > 128) return I2C_ERR_PARAM;
    uint8_t tx_buf[1] = { reg };
    i2c_transaction_t trans = {
        .address = addr,
        .addr_mode = I2C_ADDR_7BIT,
        .tx_data = tx_buf,
        .tx_len = 1,
        .rx_data = data,
        .rx_len = len,
        .speed_hz = current_speed,
        .timeout_ms = 20
    };
    return i2c_transfer(&trans);
}

// Probe: check if device at address responds
int i2c_probe(uint8_t addr) {
    i2c_transaction_t trans = {
        .address = addr,
        .addr_mode = I2C_ADDR_7BIT,
        .tx_data = NULL,
        .tx_len = 0,
        .rx_data = NULL,
        .rx_len = 0,
        .speed_hz = current_speed,
        .timeout_ms = 5
    };

    // Send address with write bit and check for ACK
    uint32_t tx_val = (addr << 1) | I2C_TX_START | I2C_TX_STOP | I2C_TX_GO;
    REG_WRITE(I2C_ADDR_REG, addr & 0x7F);
    REG_WRITE(I2C_TX_REG, tx_val);

    uint32_t timeout = 500000;
    while (timeout > 0) {
        uint32_t status = REG_READ(I2C_STATUS_REG);
        if (status & I2C_STATUS_COMPLETE) return 0;  // ACK received
        if (status & I2C_STATUS_NACK) return I2C_ERR_NACK_ADDR;
        timeout--;
    }
    return I2C_ERR_TIMEOUT;
}

// Scan bus for devices (0x08–0x77 range)
int i2c_scan(uint8_t *found_addrs, uint8_t max_count, uint8_t *count) {
    if (!driver_initialized) return I2C_ERR_BUSY;
    *count = 0;

    for (uint8_t addr = 0x08; addr <= 0x77 && *count < max_count; addr++) {
        if (i2c_probe(addr) == 0) {
            found_addrs[(*count)++] = addr;
        }
    }
    return I2C_ERR_NONE;
}

// Reset I2C bus (clock out 9 cycles to clear stuck slave)
void i2c_reset_bus(void) {
    // Disable controller
    REG_WRITE(I2C_CTRL_REG, 0);

    // Manually toggle SCL via GPIO 9 times
    for (int i = 0; i < 9; i++) {
        // SCL low
        for (volatile int d = 0; d < 100; d++) NOP();
        // SCL high
        for (volatile int d = 0; d < 100; d++) NOP();
    }

    // Re-enable
    i2c_init();
}

bool i2c_is_busy(void) {
    if (!driver_initialized) return true;
    return (REG_READ(I2C_STATUS_REG) & I2C_STATUS_BUSY) ? true : false;
}
