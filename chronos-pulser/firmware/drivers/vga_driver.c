// vga_driver.c — LMH6517 Variable Gain Amplifier Driver
// 1.2 GHz bandwidth, SPI-configurable gain (-2.5 to +42.5 dB)
// Differential input from directional coupler, differential output to ADC
// Full implementation with bandwidth control, power management, and self-test

#include "vga_driver.h"
#include "../registers.h"
#include "../board.h"

static volatile uint32_t *spi_cfg_base = (volatile uint32_t *)SPI_CFG_BASE;
static bool driver_initialized = false;
static bool vga_powered = false;
static uint8_t current_gain = VGA_GAIN_MID;
static uint8_t current_bandwidth = VGA_BW_FULL;
static uint8_t current_aux_mode = VGA_AUX_DISABLED;

// LMH6517 SPI register map (16-bit registers, 8-bit address)
#define VGA_SPI_REG_GAIN         0x00
#define VGA_SPI_REG_BANDWIDTH    0x01
#define VGA_SPI_REG_AUX          0x02
#define VGA_SPI_REG_POWER        0x03
#define VGA_SPI_REG_STATUS       0x04

// LMH6517 SPI command format:
// [15:12] Reserved
// [11:8]  Register address
// [7:0]   Data

// SPI transaction for VGA (4-wire SPI: CS, SCLK, MOSI, MISO)
static int vga_spi_write(uint8_t reg, uint8_t data) {
    uint16_t cmd = ((reg & 0x0F) << 8) | (data & 0xFF);

    // Assert CS (active low)
    gpio_set(GPIO_VGA_SPI_CS, 0);

    // Wait for SPI to be ready
    uint32_t timeout = 1000;
    while (timeout > 0) {
        if (!(REG_READ(SPI_STATUS_REG(SPI_CFG_BASE)) & SPI_STATUS_BUSY)) break;
        timeout--;
    }
    if (timeout == 0) {
        gpio_set(GPIO_VGA_SPI_CS, 1);
        return -1;
    }

    // Write 16-bit command
    REG_WRITE(SPI_TX_REG(SPI_CFG_BASE), cmd | SPI_TX_START);

    // Wait for completion
    timeout = 1000;
    while (timeout > 0) {
        if (!(REG_READ(SPI_STATUS_REG(SPI_CFG_BASE)) & SPI_STATUS_BUSY)) break;
        timeout--;
    }

    // De-assert CS
    gpio_set(GPIO_VGA_SPI_CS, 1);

    if (timeout == 0) return -2;
    return 0;
}

static int vga_spi_read(uint8_t reg, uint8_t *data) {
    // LMH6517 read: write address with MSB=1, then read data
    uint16_t cmd = (1 << 15) | ((reg & 0x0F) << 8);

    gpio_set(GPIO_VGA_SPI_CS, 0);

    uint32_t timeout = 1000;
    while (timeout > 0) {
        if (!(REG_READ(SPI_STATUS_REG(SPI_CFG_BASE)) & SPI_STATUS_BUSY)) break;
        timeout--;
    }
    if (timeout == 0) {
        gpio_set(GPIO_VGA_SPI_CS, 1);
        return -1;
    }

    REG_WRITE(SPI_TX_REG(SPI_CFG_BASE), cmd | SPI_TX_START);

    timeout = 1000;
    while (timeout > 0) {
        if (!(REG_READ(SPI_STATUS_REG(SPI_CFG_BASE)) & SPI_STATUS_BUSY)) break;
        timeout--;
    }

    if (timeout == 0) {
        gpio_set(GPIO_VGA_SPI_CS, 1);
        return -2;
    }

    // Read response
    uint32_t rx = REG_READ(SPI_RX_REG(SPI_CFG_BASE));
    *data = rx & 0xFF;

    gpio_set(GPIO_VGA_SPI_CS, 1);
    return 0;
}

int vga_driver_init(void) {
    if (driver_initialized) return 0;

    // Configure SPI for VGA (CPOL=0, CPHA=0, ~10 MHz)
    uint32_t spi_ctrl = SPI_CTRL_ENABLE;
    REG_WRITE(SPI_CTRL_REG(SPI_CFG_BASE), spi_ctrl);

    // Set clock divider: 100 MHz / 10 = 10 MHz SPI clock
    REG_WRITE(SPI_DIV_REG(SPI_CFG_BASE), 10);

    // Ensure VGA is powered up
    vga_power_up();

    // Set default gain
    int ret = vga_spi_write(VGA_SPI_REG_GAIN, VGA_GAIN_MID);
    if (ret != 0) return -1;

    // Set full bandwidth
    ret = vga_spi_write(VGA_SPI_REG_BANDWIDTH, VGA_BW_FULL);
    if (ret != 0) return -2;

    // Disable auxiliary output
    ret = vga_spi_write(VGA_SPI_REG_AUX, VGA_AUX_DISABLED);
    if (ret != 0) return -3;

    current_gain = VGA_GAIN_MID;
    current_bandwidth = VGA_BW_FULL;
    current_aux_mode = VGA_AUX_DISABLED;

    driver_initialized = true;
    return 0;
}

int vga_set_gain(uint8_t gain) {
    if (!driver_initialized) return -1;
    if (!vga_powered) return -2;

    int ret = vga_spi_write(VGA_SPI_REG_GAIN, gain);
    if (ret != 0) return ret;

    current_gain = gain;
    return 0;
}

int vga_set_bandwidth(uint8_t bw_mode) {
    if (!driver_initialized) return -1;
    if (bw_mode > VGA_BW_200MHZ) return -2;

    int ret = vga_spi_write(VGA_SPI_REG_BANDWIDTH, bw_mode);
    if (ret != 0) return ret;

    current_bandwidth = bw_mode;
    return 0;
}

int vga_set_aux_mode(uint8_t mode) {
    if (!driver_initialized) return -1;
    if (mode > VGA_AUX_ENABLED) return -2;

    int ret = vga_spi_write(VGA_SPI_REG_AUX, mode);
    if (ret != 0) return ret;

    current_aux_mode = mode;
    return 0;
}

int vga_get_gain(uint8_t *gain) {
    if (!driver_initialized) return -1;
    *gain = current_gain;
    return 0;
}

void vga_power_down(void) {
    if (!vga_powered) return;

    // Assert PD pin
    gpio_set(GPIO_VGA_PDWN, 1);

    // Write power-down via SPI
    vga_spi_write(VGA_SPI_REG_POWER, 0x01);

    vga_powered = false;
}

void vga_power_up(void) {
    if (vga_powered) return;

    // De-assert PD pin
    gpio_set(GPIO_VGA_PDWN, 0);

    // Write normal operation via SPI
    vga_spi_write(VGA_SPI_REG_POWER, 0x00);

    // Wait for VGA to wake up (~200 µs)
    for (volatile int d = 0; d < 20000; d++) NOP();

    vga_powered = true;
}

// Convert gain code to approximate dB value
// LMH6517: gain = -2.5 + code * (45.0 / 255) dB
float vga_code_to_db(uint8_t code) {
    return -2.5f + ((float)code * 45.0f / 255.0f);
}

// Convert dB target to closest gain code
uint8_t vga_db_to_code(float target_db) {
    if (target_db <= -2.5f) return 0;
    if (target_db >= 42.5f) return 255;

    float code_f = (target_db + 2.5f) * 255.0f / 45.0f;
    int code = (int)(code_f + 0.5f);
    if (code < 0) code = 0;
    if (code > 255) code = 255;
    return (uint8_t)code;
}

// Set gain in dB directly
int vga_set_gain_db(float gain_db) {
    uint8_t code = vga_db_to_code(gain_db);
    return vga_set_gain(code);
}

// Get current gain in dB
float vga_get_gain_db(void) {
    return vga_code_to_db(current_gain);
}

// Get bandwidth mode string
const char *vga_get_bandwidth_string(void) {
    switch (current_bandwidth) {
        case VGA_BW_FULL:    return "1200 MHz";
        case VGA_BW_750MHZ:  return "750 MHz";
        case VGA_BW_450MHZ:  return "450 MHz";
        case VGA_BW_200MHZ:  return "200 MHz";
        default:             return "Unknown";
    }
}

// Self-test: verify VGA communication and basic functionality
int vga_self_test(void) {
    if (!driver_initialized) return -1;

    // Test 1: Read back gain setting
    uint8_t readback;
    int ret = vga_spi_read(VGA_SPI_REG_GAIN, &readback);
    if (ret != 0) return -10;

    // Test 2: Write and verify gain
    uint8_t test_gain = 0x80;  // Mid-scale
    ret = vga_set_gain(test_gain);
    if (ret != 0) return -11;

    ret = vga_spi_read(VGA_SPI_REG_GAIN, &readback);
    if (ret != 0) return -12;
    if (readback != test_gain) return -13;

    // Test 3: Bandwidth switching
    ret = vga_set_bandwidth(VGA_BW_450MHZ);
    if (ret != 0) return -14;

    ret = vga_spi_read(VGA_SPI_REG_BANDWIDTH, &readback);
    if (ret != 0) return -15;
    if (readback != VGA_BW_450MHZ) return -16;

    // Test 4: Power cycle
    vga_power_down();
    if (vga_powered) return -17;

    vga_power_up();
    if (!vga_powered) return -18;

    // Restore defaults
    vga_set_gain(VGA_GAIN_MID);
    vga_set_bandwidth(VGA_BW_FULL);

    return 0;
}
