// i2c_driver.c — I2C Driver for Aether-Link Peripherals
// Provides high-level I2C read/write functions for TMP117, INA226, EMC2301.
// Uses the Xilinx AXI IIC controller at I2C_MASTER_BASE.

#include "../registers.h"
#include "../board.h"

// Re-export the I2C functions from gpio_init.c for modularity
// In production, these would be the primary I2C implementation.

extern void i2c_init(void);
extern int  i2c_write(uint8_t dev_addr, uint8_t reg, uint8_t data);
extern int  i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data);

// I2C bus scan — check which devices are present
void i2c_bus_scan(void) {
    // Test each expected address
    uint8_t dummy;
    int present;

    present = (i2c_read(I2C_ADDR_TMP117_FPGA, 0x0F, &dummy) == 0);
    // Log: TMP117 FPGA temp sensor present/absent

    present = (i2c_read(I2C_ADDR_TMP117_QSFP0, 0x0F, &dummy) == 0);
    // Log: TMP117 QSFP0 temp sensor present/absent

    present = (i2c_read(I2C_ADDR_INA226, 0x00, &dummy) == 0);
    // Log: INA226 power monitor present/absent

    present = (i2c_read(I2C_ADDR_EMC2301, 0x20, &dummy) == 0);
    // Log: EMC2301 fan controller present/absent
}

// I2C write to QSFP28 module (separate bus, Bank 18 GPIO bit-banged)
// In production, this uses a dedicated I2C controller for each QSFP port.
// For now, we use the main I2C bus with a mux or direct GPIO bit-bang.
int qsfp_i2c_read(uint8_t port, uint8_t dev_addr, uint8_t reg, uint8_t *data) {
    // QSFP28 modules use 3.3V I2C on dedicated buses
    // Port 0: QSFP0_SCL/SDA pins
    // Port 1: QSFP1_SCL/SDA pins
    // For simplicity, we use the main I2C controller with address translation
    (void)port;
    return i2c_read(dev_addr, reg, data);
}

int qsfp_i2c_write(uint8_t port, uint8_t dev_addr, uint8_t reg, uint8_t data) {
    (void)port;
    return i2c_write(dev_addr, reg, data);
}
