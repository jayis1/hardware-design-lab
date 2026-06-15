// main.c — Aether-Link MicroBlaze Firmware Main Entry Point
// Bare-metal firmware for XC7K325T FPGA management processor.
// Handles board initialization, peripheral setup, health monitoring,
// and management command processing from the host driver.
//
// Memory layout: This firmware runs from 64KB BRAM at 0x90000000.
// DDR4 is used for data buffers and connection state tables.

#include "board.h"
#include "registers.h"
#include "drivers/toe_driver.h"
#include "drivers/nvmeof_pdu.h"

// ============================================================================
// Forward Declarations
// ============================================================================

// Initialization functions (defined in separate driver files)
extern void clock_mmcm_init(void);
extern void gpio_init_all(void);
extern void i2c_init(void);
extern void uart_init(void);
extern void spi_flash_init(void);
extern void fan_control_init(void);
extern void power_monitor_init(void);
extern void error_handler_init(void);

// Driver initialization
extern int  toe_init(uint32_t conn_table_base, uint8_t *p0_mac,
                     uint8_t *p1_mac, uint32_t p0_ip, uint32_t p1_ip);
extern void toe_enable(void);
extern int  nvmeof_pdu_init(uint32_t data_buf_ddr4_base);

// Health monitoring
extern void temp_sensors_read(uint16_t *fpga_temp, uint16_t *qsfp0_temp);
extern uint32_t power_monitor_read_mw(void);
extern uint16_t fan_read_rpm(uint8_t fan_idx);
extern void fan_set_pwm(uint8_t duty);

// LED control
extern void led_set(uint8_t led_idx, uint8_t color);

// UART debug output
extern void uart_putc(char c);
extern void uart_puts(const char *s);
extern void uart_puthex(uint32_t val);

// QSFP28 control
extern uint8_t qsfp_module_present(uint8_t port);
extern void qsfp_reset_control(uint8_t port, uint8_t assert_reset);
extern void qsfp_lpmode_control(uint8_t port, uint8_t enter_lpmode);

// Watchdog
extern void wdt_init(void);
extern void wdt_pet(void);

// Error handler
extern void error_handler(uint8_t error_code);

// ============================================================================
// Global State
// ============================================================================

typedef enum {
    STATE_BOOTING,
    STATE_INIT_CLOCKS,
    STATE_INIT_PERIPHERALS,
    STATE_WAIT_PCIE,
    STATE_WAIT_DDR4,
    STATE_INIT_DRIVERS,
    STATE_RUNNING,
    STATE_ERROR,
    STATE_SHUTDOWN
} system_state_t;

static system_state_t g_state = STATE_BOOTING;
static uint32_t g_uptime_ms = 0;
static uint8_t  g_error_code = ERR_NONE;
static uint32_t g_boot_attempts = 0;

// Default network configuration (overridden by host management driver)
static uint8_t  g_port0_mac[6] = {0x00, 0x1A, 0x4B, 0x00, 0x00, 0x01};
static uint8_t  g_port1_mac[6] = {0x00, 0x1A, 0x4B, 0x00, 0x00, 0x02};
static uint32_t g_port0_ip = 0xC0A8010A;  // 192.168.1.10
static uint32_t g_port1_ip = 0xC0A8020A;  // 192.168.2.10

// Health monitoring
static uint16_t g_fpga_temp_raw = 0;
static uint16_t g_qsfp0_temp_raw = 0;
static uint32_t g_board_power_mw = 0;
static uint16_t g_fan0_rpm = 0;
static uint16_t g_fan1_rpm = 0;
static uint8_t  g_fan_pwm_duty = FAN_PWM_DEFAULT;

// ============================================================================
// Boot Logging
// ============================================================================

static void boot_log(const char *msg) {
    uart_puts("[BOOT] ");
    uart_puts(msg);
    uart_puts("\r\n");
}

static void boot_log_hex(const char *msg, uint32_t val) {
    uart_puts("[BOOT] ");
    uart_puts(msg);
    uart_puts(": 0x");
    uart_puthex(val);
    uart_puts("\r\n");
}

// ============================================================================
// System Tick Timer (based on MicroBlaze cycle counter)
// ============================================================================

// MicroBlaze has a 64-bit cycle counter accessible via MFSR/MTSR
// We use it for simple timing without a full interrupt-driven tick.

static inline uint32_t get_cycle_count(void) {
    uint32_t cycles;
    // MFSR: Move From Special Register — reads the lower 32 bits of the
    // 64-bit cycle counter. The upper bits are in SPR 0x0101.
    __asm__ volatile (
        "mfs %0, r0, 0x0100\n\t"  // SPR 0x0100 = CYCLE_COUNT_LOW
        : "=r"(cycles)
    );
    return cycles;
}

static void delay_ms(uint32_t ms) {
    // 200 MHz clock → 200,000 cycles per millisecond
    uint32_t target = get_cycle_count() + (ms * 200000UL);
    while (get_cycle_count() < target) {
        __asm__ volatile ("" ::: "memory");  // Prevent optimization
    }
}

static void delay_us(uint32_t us) {
    // 200 MHz clock → 200 cycles per microsecond
    uint32_t target = get_cycle_count() + (us * 200UL);
    while (get_cycle_count() < target) {
        __asm__ volatile ("" ::: "memory");
    }
}

// ============================================================================
// Watchdog Interface
// ============================================================================

#define WDT_BASE            0x80090000
#define WDT_REG_CONTROL     0x00
#define WDT_REG_COUNT       0x04
#define WDT_REG_RESET_VALUE 0x08
#define WDT_CTRL_ENABLE     0x01
#define WDT_CTRL_PET        0x02

static volatile uint32_t * const wdt_regs = (volatile uint32_t *)WDT_BASE;

void wdt_init(void) {
    // Set timeout: 1 second at 200 MHz = 200,000,000 cycles
    wdt_regs[WDT_REG_RESET_VALUE / 4] = 200000000;
    wdt_regs[WDT_REG_CONTROL / 4] = WDT_CTRL_ENABLE;
}

void wdt_pet(void) {
    wdt_regs[WDT_REG_CONTROL / 4] = WDT_CTRL_ENABLE | WDT_CTRL_PET;
}

// ============================================================================
// PCIe Link Status Check
// ============================================================================

static uint8_t pcie_link_is_up(void) {
    volatile uint32_t *pcie_regs = (volatile uint32_t *)PCIE_CFG_BASE;
    uint32_t link_status = pcie_regs[PCIE_REG_LINK_STATUS / 4];
    return (link_status & PCIE_LINK_UP) ? 1 : 0;
}

static uint8_t pcie_get_link_speed(void) {
    volatile uint32_t *pcie_regs = (volatile uint32_t *)PCIE_CFG_BASE;
    uint32_t link_status = pcie_regs[PCIE_REG_LINK_STATUS / 4];
    return link_status & PCIE_LINK_SPEED_MASK;
}

static uint8_t pcie_get_link_width(void) {
    volatile uint32_t *pcie_regs = (volatile uint32_t *)PCIE_CFG_BASE;
    uint32_t link_status = pcie_regs[PCIE_REG_LINK_STATUS / 4];
    return (link_status & PCIE_LINK_WIDTH_MASK) >> PCIE_LINK_WIDTH_SHIFT;
}

// ============================================================================
// DDR4 Calibration Status Check
// ============================================================================

static uint8_t ddr4_calibration_done(void) {
    // Read from system GPIO: bit 2 = DDR4_CAL_DONE
    volatile uint32_t *gpio_sys = (volatile uint32_t *)GPIO_SYSTEM_BASE;
    return (gpio_sys[0] & 0x04) ? 1 : 0;
}

// ============================================================================
// Health Monitoring Task
// ============================================================================

static void health_monitor_task(void) {
    // Read temperature sensors
    temp_sensors_read(&g_fpga_temp_raw, &g_qsfp0_temp_raw);

    // Read power monitor
    g_board_power_mw = power_monitor_read_mw();

    // Read fan speeds
    g_fan0_rpm = fan_read_rpm(0);
    g_fan1_rpm = fan_read_rpm(1);

    // Thermal management: adjust fan speed based on FPGA temperature
    // TMP117 returns temperature in 0.0078125°C LSB (7.8125 m°C)
    // Convert to centi-Celsius for threshold comparison
    uint16_t fpga_temp_cC = (uint16_t)(((uint32_t)g_fpga_temp_raw * 78125) / 100000);

    if (fpga_temp_cC > TEMP_CRIT_C) {
        // Critical: max fan speed, signal host to reduce queue depth
        g_fan_pwm_duty = FAN_PWM_MAX;
        led_set(3, LED_RED);  // Error LED red
        if (fpga_temp_cC > TEMP_SHUTDOWN_C) {
            // Shutdown: trigger PCIe hot-remove
            error_handler(ERR_TEMP_CRITICAL);
        }
    } else if (fpga_temp_cC > TEMP_WARN_C) {
        // Warning: increase fan speed, signal host to reduce throughput
        g_fan_pwm_duty = 200;  // ~78% duty
        led_set(3, LED_AMBER);
    } else if (fpga_temp_cC > 6000) {
        // Normal warm: moderate fan speed
        g_fan_pwm_duty = 120;  // ~47% duty
        led_set(3, LED_OFF);
    } else {
        // Cool: quiet fan speed
        g_fan_pwm_duty = FAN_PWM_DEFAULT;
        led_set(3, LED_OFF);
    }

    fan_set_pwm(g_fan_pwm_duty);

    // Check for fan failure (RPM = 0 when PWM > 0)
    if (g_fan_pwm_duty > FAN_PWM_MIN && g_fan0_rpm == 0) {
        error_handler(ERR_FAN_FAILED);
    }

    // Check power limit (75W max for PCIe x8 slot)
    if (g_board_power_mw > 75000) {
        error_handler(ERR_POWER_OVER_LIMIT);
    }
}

// ============================================================================
// Status LED Update
// ============================================================================

static void update_status_leds(void) {
    // LED 0: System status (green=ok, red=error, amber=booting)
    switch (g_state) {
        case STATE_BOOTING:
        case STATE_INIT_CLOCKS:
        case STATE_INIT_PERIPHERALS:
            led_set(0, LED_AMBER);  // Booting
            break;
        case STATE_WAIT_PCIE:
        case STATE_WAIT_DDR4:
            led_set(0, LED_AMBER);  // Waiting for hardware
            break;
        case STATE_RUNNING:
            led_set(0, LED_GREEN);  // Operational
            break;
        case STATE_ERROR:
            led_set(0, LED_RED);    // Error
            break;
        case STATE_SHUTDOWN:
            led_set(0, LED_RED);    // Shutdown
            break;
        default:
            led_set(0, LED_OFF);
            break;
    }

    // LED 1: PCIe link status
    if (pcie_link_is_up()) {
        uint8_t speed = pcie_get_link_speed();
        if (speed == PCIE_LINK_SPEED_GEN4) {
            led_set(1, LED_GREEN);  // Gen4 = green
        } else if (speed >= PCIE_LINK_SPEED_GEN3) {
            led_set(1, LED_AMBER);  // Gen3 = amber
        } else {
            led_set(1, LED_RED);    // Gen1/2 = red
        }
    } else {
        led_set(1, LED_OFF);
    }

    // LED 2: Network link activity
    volatile uint32_t *toe_regs = (volatile uint32_t *)TOE_REG_BASE;
    uint32_t toe_status = toe_regs[TOE_REG_STATUS / 4];
    if (toe_status & TOE_STATUS_PORT0_LINK) {
        led_set(2, LED_GREEN);
    } else if (toe_status & TOE_STATUS_PORT1_LINK) {
        led_set(2, LED_AMBER);
    } else {
        led_set(2, LED_OFF);
    }
}

// ============================================================================
// QSFP28 Module Initialization
// ============================================================================

static void qsfp_modules_init(void) {
    for (uint8_t port = 0; port < NUM_QSFP28_PORTS; port++) {
        // Hold module in reset during initialization
        qsfp_reset_control(port, 1);
        delay_ms(10);

        // Check if module is present
        if (qsfp_module_present(port)) {
            boot_log_hex("QSFP module present on port", port);

            // Release reset
            qsfp_reset_control(port, 0);
            delay_ms(100);  // Wait for module to initialize

            // Ensure LPMode is deasserted
            qsfp_lpmode_control(port, 0);

            // Module EEPROM will be read by the TOE hardware during
            // link initialization to determine supported speeds and media type.
        } else {
            boot_log_hex("QSFP module NOT present on port", port);
        }
    }
}

// ============================================================================
// Main Initialization Sequence
// ============================================================================

static void system_init(void) {
    g_state = STATE_INIT_CLOCKS;
    boot_log("Initializing clock MMCM...");

    // Step 1: Initialize MMCM to generate system clocks
    clock_mmcm_init();
    boot_log("MMCM locked, system clocks stable");

    // Step 2: Initialize GPIO
    g_state = STATE_INIT_PERIPHERALS;
    boot_log("Initializing GPIO...");
    gpio_init_all();

    // Step 3: Initialize UART for debug output
    boot_log("Initializing UART (115200 8N1)...");
    uart_init();
    boot_log("UART ready");

    // Step 4: Initialize I2C bus
    boot_log("Initializing I2C (400 kHz)...");
    i2c_init();
    boot_log("I2C ready");

    // Step 5: Initialize SPI flash interface
    boot_log("Initializing QSPI flash...");
    spi_flash_init();
    boot_log("QSPI flash ready");

    // Step 6: Initialize fan controller
    boot_log("Initializing fan controller...");
    fan_control_init();
    fan_set_pwm(FAN_PWM_DEFAULT);
    boot_log("Fan controller ready, PWM set to default");

    // Step 7: Initialize power monitor
    boot_log("Initializing power monitor...");
    power_monitor_init();
    boot_log("Power monitor ready");

    // Step 8: Initialize error handler
    error_handler_init();

    // Step 9: Initialize QSFP28 modules
    boot_log("Initializing QSFP28 modules...");
    qsfp_modules_init();

    // Step 10: Initialize watchdog
    boot_log("Initializing watchdog (1s timeout)...");
    wdt_init();

    // Step 11: Wait for PCIe link training
    g_state = STATE_WAIT_PCIE;
    boot_log("Waiting for PCIe link training...");
    uint32_t pcie_timeout = 500;  // 500ms timeout
    while (!pcie_link_is_up() && pcie_timeout > 0) {
        delay_ms(10);
        pcie_timeout--;
        wdt_pet();
    }
    if (pcie_link_is_up()) {
        uint8_t speed = pcie_get_link_speed();
        uint8_t width = pcie_get_link_width();
        boot_log_hex("PCIe link up, speed", speed);
        boot_log_hex("PCIe link width", width);
    } else {
        boot_log("PCIe link training timeout!");
        error_handler(ERR_PCIE_LINK_FAILED);
        return;
    }

    // Step 12: Wait for DDR4 calibration
    g_state = STATE_WAIT_DDR4;
    boot_log("Waiting for DDR4 MIG calibration...");
    uint32_t ddr4_timeout = 2000;  // 2 second timeout (calibration can take ~100ms)
    while (!ddr4_calibration_done() && ddr4_timeout > 0) {
        delay_ms(1);
        ddr4_timeout--;
        wdt_pet();
    }
    if (ddr4_calibration_done()) {
        boot_log("DDR4 calibration complete");
    } else {
        boot_log("DDR4 calibration timeout!");
        error_handler(ERR_DDR4_CAL_FAILED);
        return;
    }

    // Step 13: Initialize TOE driver
    g_state = STATE_INIT_DRIVERS;
    boot_log("Initializing TCP Offload Engine...");
    int toe_ret = toe_init(DDR4_CHA_CONN_TABLE, g_port0_mac, g_port1_mac,
                           g_port0_ip, g_port1_ip);
    if (toe_ret != 0) {
        boot_log_hex("TOE init failed with code", toe_ret);
        error_handler(ERR_TOE_HW_FAULT);
        return;
    }
    toe_enable();
    boot_log("TOE initialized and enabled");

    // Step 14: Initialize NVMe-oF PDU engine
    boot_log("Initializing NVMe-oF PDU Engine...");
    int nvmeof_ret = nvmeof_pdu_init(DDR4_CHA_PDU_BUFFER);
    if (nvmeof_ret != 0) {
        boot_log_hex("NVMe-oF PDU init failed with code", nvmeof_ret);
        error_handler(ERR_NVMEOF_PDU_FAULT);
        return;
    }
    boot_log("NVMe-oF PDU Engine initialized");

    // Step 15: Transition to running state
    g_state = STATE_RUNNING;
    boot_log("Aether-Link firmware initialization complete");
    boot_log("System running — waiting for host driver commands");
}

// ============================================================================
// Main Management Loop
// ============================================================================

static void management_loop(void) {
    uint32_t health_check_counter = 0;
    uint32_t led_update_counter = 0;

    while (1) {
        // Pet the watchdog
        wdt_pet();

        // Process any pending management commands from host
        // (In production, this would check a command queue populated
        //  by the PCIe BAR0 vendor-specific register writes.)
        // For now, we poll the debug UART for management commands.

        // Health monitoring: run every 500ms
        health_check_counter++;
        if (health_check_counter >= 500) {
            health_check_counter = 0;
            health_monitor_task();
        }

        // LED update: run every 200ms
        led_update_counter++;
        if (led_update_counter >= 200) {
            led_update_counter = 0;
            update_status_leds();
        }

        // Update uptime
        g_uptime_ms++;

        // Small delay to pace the loop (~1ms per iteration at 200MHz)
        delay_us(900);  // 900µs delay + ~100µs work = ~1ms loop
    }
}

// ============================================================================
// Error Handler
// ============================================================================

void error_handler(uint8_t error_code) {
    g_state = STATE_ERROR;
    g_error_code = error_code;

    boot_log_hex("ERROR: System entered error state, code", error_code);

    // Set error LED
    led_set(0, LED_RED);
    led_set(3, LED_RED);

    // Log error to DDR4 error log ring buffer
    volatile uint32_t *error_log_base = (volatile uint32_t *)DDR4_CHA_ERROR_LOG;
    volatile uint32_t *error_log_idx  = (volatile uint32_t *)AETHER_REG_ERROR_LOG_IDX;

    // Error log entry: [timestamp_ms, error_code, state, reserved]
    uint32_t idx = *error_log_idx;
    uint32_t *entry = (uint32_t *)(DDR4_CHA_ERROR_LOG + (idx * 16));
    entry[0] = g_uptime_ms;
    entry[1] = error_code;
    entry[2] = (uint32_t)g_state;
    entry[3] = 0xDEADBEEF;  // Marker

    // Advance ring buffer index (16KB / 16B = 1024 entries)
    *error_log_idx = (idx + 1) & 0x3FF;

    // For fatal errors, attempt recovery or shutdown
    switch (error_code) {
        case ERR_TEMP_CRITICAL:
            // Trigger PCIe hot-remove request
            g_state = STATE_SHUTDOWN;
            boot_log("Thermal shutdown initiated");
            // Set fans to max
            fan_set_pwm(FAN_PWM_MAX);
            // Signal PCIe removal
            volatile uint32_t *pcie_regs = (volatile uint32_t *)PCIE_CFG_BASE;
            pcie_regs[PCIE_REG_DEVICE_CONTROL / 4] |= 0x80000000;  // Initiate hot-remove
            break;

        case ERR_PCIE_LINK_FAILED:
        case ERR_DDR4_CAL_FAILED:
            // Hardware failure — increment boot attempt counter
            g_boot_attempts++;
            if (g_boot_attempts < 3) {
                boot_log("Attempting system restart...");
                // Trigger FPGA reconfiguration from golden image
                // (In production, this would use ICAP to load fallback bitstream)
            } else {
                g_state = STATE_SHUTDOWN;
                boot_log("Max boot attempts reached — permanent shutdown");
            }
            break;

        case ERR_FAN_FAILED:
            // Reduce power to prevent overheating
            boot_log("Fan failure detected — reducing power limit");
            // Signal host to reduce queue depth
            break;

        case ERR_POWER_OVER_LIMIT:
            boot_log("Power over limit — throttling");
            // Reduce TOE throughput
            break;

        default:
            // Non-fatal: log and continue
            g_state = STATE_RUNNING;  // Attempt to recover
            break;
    }
}

// ============================================================================
// C Runtime Environment Setup (bare-metal)
// ============================================================================

// These symbols are defined in the linker script
extern uint32_t _stack_top;
extern uint32_t _bss_start;
extern uint32_t _bss_end;
extern uint32_t _data_start;
extern uint32_t _data_end;
extern uint32_t _data_load_start;

// Bare-metal entry point — called from reset vector
void _start(void) __attribute__((noreturn));
void _start(void) {
    // Initialize stack pointer
    __asm__ volatile (
        "la sp, _stack_top\n\t"
    );

    // Clear BSS section
    uint32_t *bss = &_bss_start;
    while (bss < &_bss_end) {
        *bss++ = 0;
    }

    // Copy initialized data from load address to run address
    uint32_t *data_src = &_data_load_start;
    uint32_t *data_dst = &_data_start;
    while (data_dst < &_data_end) {
        *data_dst++ = *data_src++;
    }

    // Jump to main
    system_init();

    // If init succeeded, enter management loop
    if (g_state == STATE_RUNNING) {
        management_loop();
    }

    // Should never reach here — if we do, halt
    while (1) {
        __asm__ volatile ("wfi");
    }
}

// ============================================================================
// Interrupt Handlers (stubs — expanded in production)
// ============================================================================

// TOE interrupt handler (called when hardware TOE signals an event)
void toe_interrupt_handler(void) __attribute__((interrupt));
void toe_interrupt_handler(void) {
    volatile uint32_t *toe_regs = (volatile uint32_t *)TOE_REG_BASE;
    uint32_t int_status = toe_regs[TOE_REG_INTERRUPT_STATUS / 4];

    if (int_status & TOE_INT_CONN_ESTABLISHED) {
        uint8_t conn_id = (int_status >> 8) & 0xFF;
        boot_log_hex("TOE: Connection established, ID", conn_id);
    }

    if (int_status & TOE_INT_CONN_CLOSED) {
        uint8_t conn_id = (int_status >> 16) & 0xFF;
        boot_log_hex("TOE: Connection closed, ID", conn_id);
    }

    if (int_status & TOE_INT_DATA_RECEIVED) {
        uint8_t conn_id = (int_status >> 24) & 0xFF;
        // Data will be processed by NVMe-oF PDU engine
    }

    if (int_status & TOE_INT_ERROR) {
        uint8_t conn_id = (int_status >> 8) & 0xFF;
        uint8_t err_code = int_status & 0xFF;
        boot_log_hex("TOE: Error on connection", conn_id);
        boot_log_hex("TOE: Error code", err_code);
    }

    // Clear interrupt
    toe_regs[TOE_REG_INTERRUPT_STATUS / 4] = int_status;
}

// NVMe-oF PDU engine interrupt handler
void nvmeof_interrupt_handler(void) __attribute__((interrupt));
void nvmeof_interrupt_handler(void) {
    volatile uint32_t *nvmeof_regs = (volatile uint32_t *)NVMEOF_REG_BASE;
    uint32_t status = nvmeof_regs[NVMEOF_REG_STATUS / 4];

    if (status & NVMEOF_STATUS_ERROR) {
        boot_log("NVMe-oF PDU: Error detected");
    }

    if (status & NVMEOF_STATUS_DIGEST_ERR) {
        boot_log("NVMe-oF PDU: Digest error");
    }
}

// ============================================================================
// Utility: Hex Output for Debug UART
// ============================================================================

void uart_puthex(uint32_t val) {
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        uart_putc(hex_chars[(val >> (i * 4)) & 0xF]);
    }
}

// ============================================================================
// Linker Script Reference (embedded for documentation)
// ============================================================================
//
// linker_script.ld:
//
// MEMORY {
//     BRAM (rwx) : ORIGIN = 0x90000000, LENGTH = 64K
// }
//
// SECTIONS {
//     .text : { *(.text*) } > BRAM
//     .rodata : { *(.rodata*) } > BRAM
//     .data : {
//         _data_load_start = LOADADDR(.data);
//         _data_start = .;
//         *(.data*)
//         _data_end = .;
//     } > BRAM
//     .bss : {
//         _bss_start = .;
//         *(.bss*)
//         _bss_end = .;
//     } > BRAM
//     _stack_top = ORIGIN(BRAM) + LENGTH(BRAM) - 4;
// }
