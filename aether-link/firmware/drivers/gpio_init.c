// gpio_init.c — GPIO and Peripheral Initialization for Aether-Link
// Initializes all GPIO controllers, I2C, UART, and sets default states.
// Runs on MicroBlaze during boot ROM execution.

#include "../registers.h"
#include "../board.h"

// GPIO controller base addresses
static gpio_regs_t * const gpio_leds   = (gpio_regs_t *)GPIO_LEDS_BASE;
static gpio_regs_t * const gpio_qsfp0  = (gpio_regs_t *)GPIO_QSFP0_BASE;
static gpio_regs_t * const gpio_qsfp1  = (gpio_regs_t *)GPIO_QSFP1_BASE;
static gpio_regs_t * const gpio_sys    = (gpio_regs_t *)GPIO_SYSTEM_BASE;
static i2c_regs_t  * const i2c         = (i2c_regs_t *)I2C_MASTER_BASE;
static uart_regs_t * const uart        = (uart_regs_t *)UART_BASE;

// ============================================================================
// GPIO Initialization
// ============================================================================

void gpio_init_all(void) {
    // --- LED GPIO (Bank 16, 1.5V) ---
    // 4 bi-color LEDs: 8 output bits
    // Bits: [1:0]=LED0(GREEN,RED), [3:2]=LED1, [5:4]=LED2, [7:6]=LED3
    gpio_leds->GPIO_TRI = 0x00;   // All outputs
    gpio_leds->GPIO_DATA = 0x00;  // All LEDs off initially
    gpio_leds->GIER = 0x00;       // No interrupts from LEDs
    gpio_leds->IPIER = 0x00;

    // --- QSFP0 Control GPIO (Bank 18, 3.3V) ---
    // Bits: [0]=ModPrsL(in), [1]=ResetL(out), [2]=LPMode(out), [3]=IntL(in)
    gpio_qsfp0->GPIO_TRI = 0x09;  // Bits 0 and 3 are inputs (ModPrsL, IntL)
    gpio_qsfp0->GPIO_DATA = 0x02; // ResetL=0 (hold in reset), LPMode=0 (normal)
    gpio_qsfp0->GIER = 0x00;
    gpio_qsfp0->IPIER = 0x00;

    // --- QSFP1 Control GPIO (Bank 18, 3.3V) ---
    gpio_qsfp1->GPIO_TRI = 0x09;
    gpio_qsfp1->GPIO_DATA = 0x02;
    gpio_qsfp1->GIER = 0x00;
    gpio_qsfp1->IPIER = 0x00;

    // --- System GPIO (Bank 17, 3.3V) ---
    // Bits: [0]=CONFIG_DONE(in), [1]=PCIe_PERST#(in), [2]=DDR4_CAL_DONE(in)
    gpio_sys->GPIO_TRI = 0x07;   // All inputs
    gpio_sys->GIER = 0x00;
    gpio_sys->IPIER = 0x00;
}

// ============================================================================
// LED Control Functions
// ============================================================================

void led_set(uint8_t led_idx, uint8_t color) {
    // led_idx: 0-3, color: 0=off, 1=green, 2=red, 3=both(amber)
    if (led_idx > 3) return;

    uint32_t mask = 0x03 << (led_idx * 2);
    uint32_t val  = (color & 0x03) << (led_idx * 2);
    uint32_t data = gpio_leds->GPIO_DATA;
    data = (data & ~mask) | val;
    gpio_leds->GPIO_DATA = data;
}

void led_set_all_off(void) {
    gpio_leds->GPIO_DATA = 0x00;
}

void led_test_sequence(void) {
    // Quick LED test: cycle each LED green, then red, then off
    for (int i = 0; i < 4; i++) {
        led_set(i, LED_GREEN);
        for (volatile int d = 0; d < 500000; d++);  // ~2.5ms delay
        led_set(i, LED_RED);
        for (volatile int d = 0; d < 500000; d++);
        led_set(i, LED_OFF);
    }
}

// ============================================================================
// QSFP28 Control Functions
// ============================================================================

void qsfp_reset_control(uint8_t port, uint8_t assert_reset) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    uint32_t data = gpio->GPIO_DATA;
    if (assert_reset) {
        data &= ~0x02;  // ResetL = 0 (assert reset)
    } else {
        data |= 0x02;   // ResetL = 1 (release reset)
    }
    gpio->GPIO_DATA = data;
}

void qsfp_lpmode_control(uint8_t port, uint8_t enter_lpmode) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    uint32_t data = gpio->GPIO_DATA;
    if (enter_lpmode) {
        data |= 0x04;   // LPMode = 1
    } else {
        data &= ~0x04;  // LPMode = 0
    }
    gpio->GPIO_DATA = data;
}

uint8_t qsfp_module_present(uint8_t port) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    // ModPrsL is active low: 0 = present, 1 = absent
    return ((gpio->GPIO_DATA & 0x01) == 0) ? 1 : 0;
}

uint8_t qsfp_interrupt_active(uint8_t port) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    // IntL is active low: 0 = interrupt asserted
    return ((gpio->GPIO_DATA & 0x08) == 0) ? 1 : 0;
}

// ============================================================================
// System Status Functions
// ============================================================================

uint8_t config_done(void) {
    return (gpio_sys->GPIO_DATA & 0x01) ? 1 : 0;
}

uint8_t pcie_perst_asserted(void) {
    // PERST# is active low: 0 = reset asserted
    return ((gpio_sys->GPIO_DATA & 0x02) == 0) ? 1 : 0;
}

uint8_t ddr4_calibration_done(void) {
    return (gpio_sys->GPIO_DATA & 0x04) ? 1 : 0;
}

// ============================================================================
// I2C Initialization
// ============================================================================

void i2c_init(void) {
    // Soft reset the I2C controller
    i2c->SOFTR = 0x0000000A;  // Write 0xA to reset
    for (volatile int i = 0; i < 1000; i++);  // Wait for reset to complete

    // Configure for 400 kHz Fast Mode
    // Input clock: 200 MHz (MicroBlaze system clock)
    // SCL period = 2.5 µs → 400 kHz
    // 200 MHz → 5 ns per cycle → 500 cycles per SCL period
    // THIGH = 250 cycles (1.25 µs), TLOW = 250 cycles (1.25 µs)
    i2c->THIGH   = 250;
    i2c->TLOW    = 250;
    i2c->TBUF    = 250;  // Bus free time between stop/start
    i2c->THD_STA = 100;  // Hold time after (repeated) start
    i2c->TSU_STA = 100;  // Setup time for repeated start
    i2c->TSU_STO = 100;  // Setup time for stop
    i2c->THD_DAT = 50;   // Data hold time

    // Clear FIFOs and enable the controller
    i2c->CR = I2C_CR_CLR_FIFO;
    for (volatile int i = 0; i < 100; i++);
    i2c->CR = I2C_CR_EN;  // Enable, 7-bit addressing, master mode
}

// ============================================================================
// UART Initialization
// ============================================================================

void uart_init(void) {
    // Reset TX and RX FIFOs
    uart->CTRL = UART_CTRL_RST_TX_FIFO | UART_CTRL_RST_RX_FIFO;
    for (volatile int i = 0; i < 100; i++);

    // Configure for 115200 baud, 8N1
    // Input clock: 200 MHz
    // Baud rate divisor = 200,000,000 / (16 × 115200) = 108.5 → 109
    // Actual baud = 200,000,000 / (16 × 109) = 114,679 (0.45% error, acceptable)
    uart->CTRL = 109;  // Divisor in lower bits, enable RX/TX
}

void uart_putc(char c) {
    while (uart->STAT & UART_SR_TX_FIFO_FULL);  // Wait while TX FIFO full
    uart->TX_FIFO = (uint32_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');  // CR before LF for terminals
        uart_putc(*s++);
    }
}

char uart_getc(void) {
    while (!(uart->STAT & UART_SR_RX_FIFO_VALID_DATA));  // Wait for data
    return (char)(uart->RX_FIFO & 0xFF);
}

int uart_data_available(void) {
    return (uart->STAT & UART_SR_RX_FIFO_VALID_DATA) ? 1 : 0;
}

// ============================================================================
// Fan PWM Control (via EMC2301 I2C Fan Controller)
// ============================================================================

// EMC2301 registers
#define EMC2301_REG_PWM_CTRL     0x00
#define EMC2301_REG_PWM_DUTY      0x30
#define EMC2301_REG_TACH_LO0      0x3E
#define EMC2301_REG_TACH_HI0      0x3F
#define EMC2301_REG_TACH_LO1      0x40
#define EMC2301_REG_TACH_HI1      0x41
#define EMC2301_REG_CONFIG        0x20

// Simple I2C write (blocking, for bare-metal)
static int i2c_write(uint8_t dev_addr, uint8_t reg, uint8_t data) {
    // Wait for bus idle
    while (!(i2c->SR & I2C_SR_TX_FIFO_EMPTY));

    // Set target address and clear FIFO
    i2c->ADR = dev_addr;
    i2c->CR = I2C_CR_EN | I2C_CR_CLR_FIFO | I2C_CR_MSMS | I2C_CR_TX;
    for (volatile int i = 0; i < 100; i++);

    // Write register address
    i2c->TX_FIFO = reg;
    while (i2c->SR & I2C_SR_TX_FIFO_EMPTY);  // Wait for byte to be sent
    // Check for ACK
    if (i2c->SR & I2C_SR_RXAK) { i2c->CR = I2C_CR_EN; return -1; }

    // Write data
    i2c->TX_FIFO = data;
    while (i2c->SR & I2C_SR_TX_FIFO_EMPTY);
    if (i2c->SR & I2C_SR_RXAK) { i2c->CR = I2C_CR_EN; return -1; }

    // Generate STOP
    i2c->CR = I2C_CR_EN;
    return 0;
}

// Simple I2C read (blocking)
static int i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *data) {
    // Phase 1: Write register address (no STOP)
    while (!(i2c->SR & I2C_SR_TX_FIFO_EMPTY));
    i2c->ADR = dev_addr;
    i2c->CR = I2C_CR_EN | I2C_CR_CLR_FIFO | I2C_CR_MSMS | I2C_CR_TX;
    for (volatile int i = 0; i < 100; i++);

    i2c->TX_FIFO = reg;
    while (i2c->SR & I2C_SR_TX_FIFO_EMPTY);
    if (i2c->SR & I2C_SR_RXAK) { i2c->CR = I2C_CR_EN; return -1; }

    // Phase 2: Repeated START, switch to receive
    i2c->ADR = dev_addr;
    i2c->CR = I2C_CR_EN | I2C_CR_CLR_FIFO | I2C_CR_MSMS | I2C_CR_RSTA;
    for (volatile int i = 0; i < 100; i++);

    // Set NACK (single byte read) and switch to RX
    i2c->CR = I2C_CR_EN | I2C_CR_MSMS | I2C_CR_TXAK;
    // Dummy read to start reception
    (void)i2c->RX_FIFO;
    while (!(i2c->SR & I2C_SR_RX_FIFO_EMPTY));  // Wait for data
    *data = (uint8_t)(i2c->RX_FIFO & 0xFF);

    // Generate STOP
    i2c->CR = I2C_CR_EN;
    return 0;
}

void fan_control_init(void) {
    // Configure EMC2301: enable PWM, set base frequency to 25 kHz
    i2c_write(I2C_ADDR_EMC2301, EMC2301_REG_CONFIG, 0x01);  // Enable
    i2c_write(I2C_ADDR_EMC2301, EMC2301_REG_PWM_CTRL, 0x01); // PWM enabled
}

void fan_set_pwm(uint8_t duty) {
    // duty: 0-255 (0% to 100%)
    i2c_write(I2C_ADDR_EMC2301, EMC2301_REG_PWM_DUTY, duty);
}

uint16_t fan_read_rpm(uint8_t fan_idx) {
    uint8_t tach_lo, tach_hi;
    uint8_t reg_lo = (fan_idx == 0) ? EMC2301_REG_TACH_LO0 : EMC2301_REG_TACH_LO1;
    uint8_t reg_hi = (fan_idx == 0) ? EMC2301_REG_TACH_HI0 : EMC2301_REG_TACH_HI1;

    if (i2c_read(I2C_ADDR_EMC2301, reg_lo, &tach_lo) != 0) return 0;
    if (i2c_read(I2C_ADDR_EMC2301, reg_hi, &tach_hi) != 0) return 0;

    uint16_t tach_count = ((uint16_t)tach_hi << 8) | tach_lo;
    if (tach_count == 0xFFFF) return 0;  // Fan stalled

    // RPM = (60 × 25,000) / (tach_count × PPR)
    // For 25 kHz PWM base freq, 2 PPR:
    // RPM = 1,500,000 / tach_count
    if (tach_count == 0) return 0;
    return (uint16_t)(1500000UL / tach_count);
}

// ============================================================================
// Power Monitor (INA226)
// ============================================================================

#define INA226_REG_CONFIG      0x00
#define INA226_REG_SHUNT_V     0x01
#define INA226_REG_BUS_V       0x02
#define INA226_REG_POWER       0x03
#define INA226_REG_CURRENT     0x04
#define INA226_REG_CALIBRATION 0x05

// Shunt resistor: 2 mΩ (for 12V, 10A max → 20mV full scale)
#define SHUNT_RESISTOR_MOHM    2
#define MAX_CURRENT_MA          10000

void power_monitor_init(void) {
    // Configure INA226: 1.1ms conversion time, 4-sample averaging, shunt+bus continuous
    // Config value: 0x4527
    //   MODE[2:0] = 111 (shunt and bus, continuous)
    //   VSHCT[2:0] = 100 (1.1ms)
    //   VBUSCT[2:0] = 100 (1.1ms)
    //   AVG[2:0] = 010 (4 samples)
    //   RST = 0
    uint16_t config = 0x4527;
    i2c_write(I2C_ADDR_INA226, INA226_REG_CONFIG, (config >> 8) & 0xFF);
    i2c_write(I2C_ADDR_INA226, INA226_REG_CONFIG + 1, config & 0xFF);

    // Set calibration register
    // Current_LSB = Max_Current / 2^15 = 10000mA / 32768 = 0.305mA
    // Cal = 0.00512 / (Current_LSB × Rshunt) = 0.00512 / (0.000305 × 0.002)
    //     = 0.00512 / 0.00000061 = 8393
    uint16_t cal = 8393;
    i2c_write(I2C_ADDR_INA226, INA226_REG_CALIBRATION, (cal >> 8) & 0xFF);
    i2c_write(I2C_ADDR_INA226, INA226_REG_CALIBRATION + 1, cal & 0xFF);
}

uint32_t power_monitor_read_mw(void) {
    uint8_t power_hi, power_lo;
    if (i2c_read(I2C_ADDR_INA226, INA226_REG_POWER, &power_hi) != 0) return 0;
    if (i2c_read(I2C_ADDR_INA226, INA226_REG_POWER + 1, &power_lo) != 0) return 0;

    uint16_t power_raw = ((uint16_t)power_hi << 8) | power_lo;
    // Power_LSB = 25 × Current_LSB = 25 × 0.305mA = 7.625mW
    // Power_mW = power_raw × 7.625
    return (uint32_t)(power_raw * 7625UL / 1000);
}

uint16_t power_monitor_read_voltage_mv(void) {
    uint8_t bus_hi, bus_lo;
    if (i2c_read(I2C_ADDR_INA226, INA226_REG_BUS_V, &bus_hi) != 0) return 0;
    if (i2c_read(I2C_ADDR_INA226, INA226_REG_BUS_V + 1, &bus_lo) != 0) return 0;

    uint16_t bus_raw = ((uint16_t)bus_hi << 8) | bus_lo;
    // Bus voltage LSB = 1.25mV
    return (uint16_t)(bus_raw * 125UL / 100);
}

uint16_t power_monitor_read_current_ma(void) {
    uint8_t curr_hi, curr_lo;
    if (i2c_read(I2C_ADDR_INA226, INA226_REG_CURRENT, &curr_hi) != 0) return 0;
    if (i2c_read(I2C_ADDR_INA226, INA226_REG_CURRENT + 1, &curr_lo) != 0) return 0;

    uint16_t curr_raw = ((uint16_t)curr_hi << 8) | curr_lo;
    // Current_LSB = 0.305mA
    return (uint16_t)(curr_raw * 305UL / 1000);
}

// ============================================================================
// Temperature Sensors (TMP117)
// ============================================================================

#define TMP117_REG_TEMP        0x00
#define TMP117_REG_CONFIG      0x01
#define TMP117_REG_HIGH_LIMIT  0x02
#define TMP117_REG_LOW_LIMIT   0x03
#define TMP117_REG_DEVICE_ID   0x0F

void temp_sensor_init(uint8_t dev_addr) {
    // Configure TMP117: continuous conversion, 8-sample averaging, 15.5ms period
    // Config: 0x0220
    //   MOD[1:0] = 00 (continuous)
    //   AVG[1:0] = 01 (8 samples)
    //   CONV[2:0] = 010 (15.5ms)
    uint16_t config = 0x0220;
    i2c_write(dev_addr, TMP117_REG_CONFIG, (config >> 8) & 0xFF);
    i2c_write(dev_addr, TMP117_REG_CONFIG + 1, config & 0xFF);
}

void temp_sensors_read(uint16_t *fpga_temp, uint16_t *qsfp0_temp) {
    uint8_t temp_hi, temp_lo;

    // Read FPGA temperature sensor (addr 0x48)
    if (i2c_read(I2C_ADDR_TMP117_FPGA, TMP117_REG_TEMP, &temp_hi) == 0 &&
        i2c_read(I2C_ADDR_TMP117_FPGA, TMP117_REG_TEMP + 1, &temp_lo) == 0) {
        *fpga_temp = ((uint16_t)temp_hi << 8) | temp_lo;
    } else {
        *fpga_temp = 0xFFFF;  // Error indicator
    }

    // Read QSFP0 temperature sensor (addr 0x49)
    if (i2c_read(I2C_ADDR_TMP117_QSFP0, TMP117_REG_TEMP, &temp_hi) == 0 &&
        i2c_read(I2C_ADDR_TMP117_QSFP0, TMP117_REG_TEMP + 1, &temp_lo) == 0) {
        *qsfp0_temp = ((uint16_t)temp_hi << 8) | temp_lo;
    } else {
        *qsfp0_temp = 0xFFFF;
    }
}

// Convert TMP117 raw reading to Celsius × 100 (centi-Celsius)
// TMP117 LSB = 7.8125 m°C → multiply by 78125 then divide by 100000
uint16_t temp_raw_to_cC(uint16_t raw) {
    if (raw == 0xFFFF) return 0xFFFF;  // Error
    return (uint16_t)(((uint32_t)raw * 78125UL) / 100000);
}
