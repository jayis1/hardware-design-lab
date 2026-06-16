// main.c — Chronos Pulser Main Firmware
// SPL/board init, peripheral setup, main loop, error handling
// Runs on VexRiscv RV32IM soft CPU at 100 MHz
// Total system initialization and command processing loop

#include "registers.h"
#include "board.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Driver headers
#include "drivers/clock_config.h"
#include "drivers/gpio_init.h"
#include "drivers/adc_driver.h"
#include "drivers/tdc_driver.h"
#include "drivers/pulse_driver.h"
#include "drivers/vga_driver.h"
#include "drivers/i2c_driver.h"
#include "drivers/spi_flash.h"
#include "drivers/usb_protocol.h"
#include "drivers/temp_monitor.h"
#include "drivers/error_handler.h"
#include "drivers/calibration.h"
#include "drivers/led_controller.h"

//=============================================================================
// Firmware Version
//=============================================================================

#define FW_VERSION_MAJOR        1
#define FW_VERSION_MINOR        0
#define FW_VERSION_PATCH        0
#define FW_VERSION_BUILD        20260616
#define FW_VERSION_STRING       "Chronos Pulser FW v1.0.0"

//=============================================================================
// System State Machine
//=============================================================================

typedef enum {
    STATE_BOOT,
    STATE_INIT_CLOCKS,
    STATE_INIT_GPIO,
    STATE_INIT_DDR3,
    STATE_INIT_PERIPHERALS,
    STATE_CALIBRATION,
    STATE_READY,
    STATE_ACQUIRING,
    STATE_ERROR,
    STATE_SHUTDOWN
} system_state_t;

static system_state_t g_system_state = STATE_BOOT;
static uint32_t g_boot_errors = 0;
static uint32_t g_uptime_seconds = 0;
static bool g_usb_connected = false;
static bool g_calibration_valid = false;

//=============================================================================
// Forward Declarations
//=============================================================================

static void system_state_machine(void);
static int  init_ddr3(void);
static int  init_peripherals(void);
static int  load_calibration(void);
static void main_command_loop(void);
static void health_monitor(void);
static void process_usb_command(const usb_command_t *cmd);
static void handle_acquisition_request(const usb_command_t *cmd);
static void handle_pulse_config(const usb_command_t *cmd);
static void handle_tdc_request(const usb_command_t *cmd);
static void handle_system_command(const usb_command_t *cmd);
static void update_status_leds(void);
static void check_temperature(void);

//=============================================================================
// Boot Entry Point
//=============================================================================

// Called from startup.S after basic CPU init
void _start(void) __attribute__((noreturn));
void _start(void) {
    // Disable interrupts during boot
    CSR_WRITE(mstatus, 0);
    CSR_WRITE(mie, 0);

    // Initialize stack pointer (set by linker script)
    extern uint32_t _stack_top;
    __asm__ volatile ("la sp, _stack_top");

    // Clear BSS
    extern uint32_t _bss_start, _bss_end;
    uint32_t *bss = &_bss_start;
    while (bss < &_bss_end) {
        *bss++ = 0;
    }

    // Jump to main
    main();

    // Should never return
    while (1) {
        WFI();
    }
}

//=============================================================================
// Main Function
//=============================================================================

int main(void) {
    int ret;

    // --- Phase 1: System Identification ---
    uint32_t sys_id = REG_READ(SYS_ID_REG);
    uint16_t device_id = (sys_id >> 16) & 0xFFFF;

    if (device_id != SYS_ID_DEVICE_CHRONOS) {
        // Wrong device — halt with error LED
        led_set_all(LED_COLOR_RED);
        error_handler_record(ERR_HARDWARE, 0, "Wrong device ID");
        while (1) { WFI(); }
    }

    g_system_state = STATE_INIT_CLOCKS;
    led_set_all(LED_STATUS_BOOTING);

    // --- Phase 2: Clock Initialization ---
    ret = clock_init();
    if (ret != 0) {
        g_boot_errors |= (1 << 0);
        error_handler_record(ERR_HARDWARE, ret, "Clock init failed");
        g_system_state = STATE_ERROR;
        led_set_all(LED_COLOR_RED);
        while (1) { WFI(); }
    }

    // --- Phase 3: GPIO Initialization ---
    g_system_state = STATE_INIT_GPIO;
    ret = gpio_init();
    if (ret != 0) {
        g_boot_errors |= (1 << 1);
        error_handler_record(ERR_HARDWARE, ret, "GPIO init failed");
        g_system_state = STATE_ERROR;
        led_set_all(LED_COLOR_RED);
        while (1) { WFI(); }
    }

    // --- Phase 4: DDR3 Calibration ---
    g_system_state = STATE_INIT_DDR3;
    ret = init_ddr3();
    if (ret != 0) {
        g_boot_errors |= (1 << 2);
        error_handler_record(ERR_HARDWARE, ret, "DDR3 init failed");
        g_system_state = STATE_ERROR;
        led_set_all(LED_COLOR_RED);
        while (1) { WFI(); }
    }

    // --- Phase 5: Peripheral Initialization ---
    g_system_state = STATE_INIT_PERIPHERALS;
    ret = init_peripherals();
    if (ret != 0) {
        g_boot_errors |= (1 << 3);
        error_handler_record(ERR_HARDWARE, ret, "Peripheral init failed");
        g_system_state = STATE_ERROR;
        led_set_all(LED_COLOR_RED);
        while (1) { WFI(); }
    }

    // --- Phase 6: Calibration ---
    g_system_state = STATE_CALIBRATION;
    led_set_all(LED_STATUS_CALIBRATING);
    ret = load_calibration();
    if (ret == 0) {
        g_calibration_valid = true;
    } else {
        // Calibration not loaded — run factory calibration
        ret = calibration_run_factory();
        if (ret == 0) {
            g_calibration_valid = true;
        } else {
            g_boot_errors |= (1 << 4);
            error_handler_record(ERR_NOT_CALIBRATED, ret, "Calibration failed");
            // Continue in uncalibrated mode
        }
    }

    // --- Phase 7: Enable Interrupts ---
    CSR_WRITE(mtvec, (uint32_t)irq_handler);
    REG_WRITE(SYS_INT_EN_REG, 0xFFFFFFFF);  // Enable all interrupts
    CSR_SET(mie, MIE_MEIE | MIE_MTIE);
    CSR_SET(mstatus, MSTATUS_MIE);

    // --- Phase 8: Ready ---
    g_system_state = STATE_READY;
    led_set_all(LED_STATUS_READY);

    // Log boot complete
    error_handler_record(ERR_NONE, 0, FW_VERSION_STRING " booted successfully");

    // --- Main Loop ---
    while (1) {
        system_state_machine();
        main_command_loop();
        health_monitor();
        update_status_leds();
    }

    return 0;  // Unreachable
}

//=============================================================================
// DDR3 Initialization
//=============================================================================

static int init_ddr3(void) {
    // The DDR3 controller (LiteDRAM) is initialized by the FPGA gateware
    // before the soft CPU is released from reset. We just verify it's ready.

    // Wait for DDR3 calibration done flag
    uint32_t timeout = 10000000;  // ~100 ms at 100 MHz
    while (timeout > 0) {
        uint32_t status = REG_READ(SYS_STATUS_REG);
        if (status & SYS_STATUS_DDR3_CAL) {
            return 0;
        }
        timeout--;
    }

    return -1;  // DDR3 calibration timeout
}

//=============================================================================
// Peripheral Initialization
//=============================================================================

static int init_peripherals(void) {
    int ret;

    // 1. Initialize I2C for temperature sensor
    ret = i2c_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "I2C init failed");
        return -1;
    }

    // 2. Initialize temperature monitor
    ret = temp_monitor_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "Temp monitor init failed");
        // Non-fatal — continue
    }

    // 3. Initialize SPI flash
    ret = spi_flash_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "SPI flash init failed");
        return -2;
    }

    // 4. Initialize ADC driver
    ret = adc_driver_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "ADC driver init failed");
        return -3;
    }

    // 5. Configure ADC defaults via SPI
    ret = adc_configure_defaults();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "ADC config failed");
        return -4;
    }

    // 6. Verify ADC device
    ret = adc_verify_device();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "ADC verify failed");
        return -5;
    }

    // 7. Initialize VGA driver
    ret = vga_driver_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "VGA driver init failed");
        return -6;
    }

    // 8. Set VGA default gain
    ret = vga_set_gain(0x40);  // Mid-scale gain (~10 dB)
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "VGA gain set failed");
        // Non-fatal
    }

    // 9. Initialize TDC driver
    ret = tdc_driver_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "TDC driver init failed");
        return -7;
    }

    // 10. Initialize pulse generator driver
    ret = pulse_driver_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "Pulse driver init failed");
        return -8;
    }

    // 11. Initialize USB protocol handler
    ret = usb_protocol_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "USB protocol init failed");
        // Non-fatal — device can work standalone
    }

    // 12. Initialize LED controller
    ret = led_controller_init();
    if (ret != 0) {
        error_handler_record(ERR_HARDWARE, ret, "LED controller init failed");
        // Non-fatal
    }

    return 0;
}

//=============================================================================
// Calibration Loading
//=============================================================================

static int load_calibration(void) {
    // Try to load TDC calibration from SPI flash
    tdc_calibration_t cal;
    int ret = spi_flash_read(FLASH_OFFSET_TDC_CAL, (uint8_t *)&cal, sizeof(cal));
    if (ret != 0) {
        return -1;
    }

    // Verify calibration CRC
    uint32_t computed_crc = calibration_compute_crc(&cal);
    if (computed_crc != cal.crc) {
        return -2;
    }

    // Load into TDC
    ret = tdc_load_calibration(&cal);
    if (ret != 0) {
        return -3;
    }

    // Load factory calibration data
    factory_cal_t factory_cal;
    ret = spi_flash_read(FLASH_OFFSET_FACTORY, (uint8_t *)&factory_cal, sizeof(factory_cal));
    if (ret == 0) {
        calibration_load_factory(&factory_cal);
    }

    return 0;
}

//=============================================================================
// System State Machine
//=============================================================================

static void system_state_machine(void) {
    switch (g_system_state) {
    case STATE_READY:
        // Check for USB connection changes
        {
            uint32_t usb_status = REG_READ(USB_STATUS_REG);
            bool was_connected = g_usb_connected;
            g_usb_connected = (usb_status & USB_STATUS_CONFIGURED) ? true : false;

            if (g_usb_connected && !was_connected) {
                led_set_all(LED_STATUS_READY);
                error_handler_record(ERR_NONE, 0, "USB connected");
            } else if (!g_usb_connected && was_connected) {
                led_set_all(LED_COLOR_DIM_GREEN);
                error_handler_record(ERR_NONE, 0, "USB disconnected");
            }
        }
        break;

    case STATE_ACQUIRING:
        // Check acquisition status
        {
            uint32_t acq_status = REG_READ(ACQ_STATUS_REG);
            if (acq_status & ACQ_STATUS_COMPLETE) {
                g_system_state = STATE_READY;
                led_set_all(LED_STATUS_READY);
            } else if (acq_status & ACQ_STATUS_ERROR) {
                g_system_state = STATE_READY;
                led_set_all(LED_STATUS_ERROR);
                error_handler_record(ERR_OVERFLOW, 0, "Acquisition error");
            }
        }
        break;

    case STATE_ERROR:
        // Stay in error state until reset or command
        break;

    case STATE_SHUTDOWN:
        // Over-temperature shutdown — wait for cool-down
        check_temperature();
        break;

    default:
        break;
    }
}

//=============================================================================
// Main Command Loop
//=============================================================================

static void main_command_loop(void) {
    usb_command_t cmd;

    // Check for incoming USB commands
    if (g_system_state == STATE_READY || g_system_state == STATE_ACQUIRING) {
        int ret = usb_protocol_poll(&cmd);
        if (ret == 0) {
            process_usb_command(&cmd);
        }
    }
}

//=============================================================================
// USB Command Processing
//=============================================================================

static void process_usb_command(const usb_command_t *cmd) {
    switch (cmd->command_id) {
    case CMD_PING:
        usb_protocol_send_response(ERR_NONE, NULL, 0);
        break;

    case CMD_GET_INFO:
        handle_system_command(cmd);
        break;

    case CMD_RESET:
        REG_WRITE(SYS_RESET_REG, SYS_RESET_FULL);
        break;

    case CMD_ACQ_START:
        handle_acquisition_request(cmd);
        break;

    case CMD_ACQ_STOP:
        REG_WRITE(ACQ_CTRL_REG, ACQ_CTRL_ABORT);
        g_system_state = STATE_READY;
        led_set_all(LED_STATUS_READY);
        usb_protocol_send_response(ERR_NONE, NULL, 0);
        break;

    case CMD_ACQ_STATUS:
        {
            uint32_t acq_status = REG_READ(ACQ_STATUS_REG);
            usb_protocol_send_response(ERR_NONE,
                (uint8_t *)&acq_status, sizeof(acq_status));
        }
        break;

    case CMD_TDC_SINGLE:
        handle_tdc_request(cmd);
        break;

    case CMD_TDC_CALIBRATE:
        {
            tdc_calibration_t cal;
            int ret = tdc_run_calibration(&cal);
            if (ret == 0) {
                // Save to flash
                spi_flash_erase_sector(FLASH_OFFSET_TDC_CAL);
                spi_flash_write(FLASH_OFFSET_TDC_CAL, (uint8_t *)&cal, sizeof(cal));
                g_calibration_valid = true;
                usb_protocol_send_response(ERR_NONE, (uint8_t *)&cal, sizeof(cal));
            } else {
                usb_protocol_send_response(ERR_TIMEOUT, NULL, 0);
            }
        }
        break;

    case CMD_TDC_GET_CAL:
        {
            tdc_calibration_t cal;
            int ret = tdc_save_calibration(&cal);
            if (ret == 0) {
                usb_protocol_send_response(ERR_NONE, (uint8_t *)&cal, sizeof(cal));
            } else {
                usb_protocol_send_response(ERR_NOT_CALIBRATED, NULL, 0);
            }
        }
        break;

    case CMD_PULSE_CONFIG:
        handle_pulse_config(cmd);
        break;

    case CMD_PULSE_SINGLE:
        {
            int ret = pulse_single_shot();
            if (ret == 0) {
                usb_protocol_send_response(ERR_NONE, NULL, 0);
            } else {
                usb_protocol_send_response(ERR_BUSY, NULL, 0);
            }
        }
        break;

    case CMD_VGA_SET_GAIN:
        if (cmd->payload_length >= 1) {
            uint8_t gain = cmd->payload[0];
            int ret = vga_set_gain(gain);
            if (ret == 0) {
                usb_protocol_send_response(ERR_NONE, NULL, 0);
            } else {
                usb_protocol_send_response(ERR_PARAM, NULL, 0);
            }
        } else {
            usb_protocol_send_response(ERR_PARAM, NULL, 0);
        }
        break;

    case CMD_TEMP_READ:
        {
            float temp = temp_monitor_read();
            usb_protocol_send_response(ERR_NONE, (uint8_t *)&temp, sizeof(temp));
        }
        break;

    case CMD_LED_SET:
        if (cmd->payload_length >= 3) {
            uint32_t color = (cmd->payload[0] << 16)
                           | (cmd->payload[1] << 8)
                           | cmd->payload[2];
            led_set_all(color);
            usb_protocol_send_response(ERR_NONE, NULL, 0);
        } else {
            usb_protocol_send_response(ERR_PARAM, NULL, 0);
        }
        break;

    case CMD_FLASH_READ:
        if (cmd->payload_length >= 6) {
            uint32_t addr = (cmd->payload[0])
                          | (cmd->payload[1] << 8)
                          | (cmd->payload[2] << 16)
                          | (cmd->payload[3] << 24);
            uint16_t len = (cmd->payload[4]) | (cmd->payload[5] << 8);
            uint8_t buf[256];
            if (len <= sizeof(buf)) {
                int ret = spi_flash_read(addr, buf, len);
                if (ret == 0) {
                    usb_protocol_send_response(ERR_NONE, buf, len);
                } else {
                    usb_protocol_send_response(ERR_HARDWARE, NULL, 0);
                }
            } else {
                usb_protocol_send_response(ERR_PARAM, NULL, 0);
            }
        }
        break;

    case CMD_BOOTLOADER:
        // Set boot source to update image and reset
        REG_WRITE(SYS_BOOT_SRC_REG, 1);
        REG_WRITE(SYS_BOOT_SRC_REG, SYS_BOOT_COMMIT | 1);
        REG_WRITE(SYS_RESET_REG, SYS_RESET_FULL);
        break;

    default:
        usb_protocol_send_response(ERR_PARAM, NULL, 0);
        break;
    }
}

//=============================================================================
// Acquisition Request Handler
//=============================================================================

static void handle_acquisition_request(const usb_command_t *cmd) {
    if (g_system_state != STATE_READY) {
        usb_protocol_send_response(ERR_BUSY, NULL, 0);
        return;
    }

    // Parse acquisition parameters from command payload
    // Payload format: [sample_count:4][trigger_delay:2][decimation:1][averaging:1][flags:1]
    if (cmd->payload_length < 9) {
        usb_protocol_send_response(ERR_PARAM, NULL, 0);
        return;
    }

    uint32_t sample_count = (cmd->payload[0])
                          | (cmd->payload[1] << 8)
                          | (cmd->payload[2] << 16)
                          | (cmd->payload[3] << 24);
    uint16_t trigger_delay = (cmd->payload[4]) | (cmd->payload[5] << 8);
    uint8_t  decimation = cmd->payload[6];
    uint8_t  averaging = cmd->payload[7];
    uint8_t  flags = cmd->payload[8];

    // Configure acquisition sequencer
    uint32_t acq_config = ACQ_CONFIG_ENABLE_BOTH | ACQ_CONFIG_AUTO_UPLOAD;
    acq_config |= ((averaging & 0xFF) << 16);  // Post-trigger samples
    acq_config |= ((averaging & 0xFF) << 8);   // Pre-trigger samples

    REG_WRITE(ACQ_CONFIG_REG, acq_config);
    REG_WRITE(ACQ_TDC_BASE_REG, DMA_TDC_BUFFER);
    REG_WRITE(ACQ_ADC_BASE_REG, DMA_ADC_BUFFER);

    // Configure ADC
    adc_acq_config_t adc_cfg = {
        .sample_count = sample_count,
        .trigger_delay = trigger_delay,
        .decimation_factor = decimation,
        .averaging_depth = averaging,
        .continuous = (flags & 0x01) ? true : false,
        .triggered = (flags & 0x02) ? true : false,
        .use_dma = true,
        .dma_buffer_addr = DMA_ADC_BUFFER,
        .dma_buffer_size = sample_count * 2
    };
    adc_start_acquisition(&adc_cfg);

    // Configure TDC
    tdc_config_t tdc_cfg = {
        .continuous_mode = (flags & 0x01) ? true : false,
        .single_shot = false,
        .trigger_source = 0,
        .averaging_depth = averaging,
        .rising_edge = true,
        .falling_edge = false,
        .hit_threshold = 1,
        .dead_time = 0,
        .calibration_mode = false
    };
    tdc_configure(&tdc_cfg);
    tdc_start();

    // Configure pulse generator
    pulse_config_t pulse_cfg = {
        .frequency_hz = PULSE_DEFAULT_FREQ_HZ,
        .amplitude_dac = PULSE_DEFAULT_AMPLITUDE,
        .continuous = true,
        .burst_count = 0
    };
    pulse_configure(&pulse_cfg);
    pulse_start();

    // Start acquisition sequencer
    REG_WRITE(ACQ_CTRL_REG, ACQ_CTRL_START);

    g_system_state = STATE_ACQUIRING;
    led_set_all(LED_STATUS_ACQUIRING);

    usb_protocol_send_response(ERR_NONE, NULL, 0);
}

//=============================================================================
// Pulse Configuration Handler
//=============================================================================

static void handle_pulse_config(const usb_command_t *cmd) {
    if (cmd->payload_length < 8) {
        usb_protocol_send_response(ERR_PARAM, NULL, 0);
        return;
    }

    pulse_config_t cfg;
    cfg.frequency_hz = (cmd->payload[0])
                      | (cmd->payload[1] << 8)
                      | (cmd->payload[2] << 16)
                      | (cmd->payload[3] << 24);
    cfg.amplitude_dac = cmd->payload[4];
    cfg.continuous = cmd->payload[5] ? true : false;
    cfg.burst_count = cmd->payload[6];
    cfg.external_trigger = cmd->payload[7] ? true : false;

    if (cfg.frequency_hz < PULSE_MIN_FREQ_HZ || cfg.frequency_hz > PULSE_MAX_FREQ_HZ) {
        usb_protocol_send_response(ERR_PARAM, NULL, 0);
        return;
    }

    int ret = pulse_configure(&cfg);
    if (ret == 0) {
        usb_protocol_send_response(ERR_NONE, NULL, 0);
    } else {
        usb_protocol_send_response(ERR_STATE, NULL, 0);
    }
}

//=============================================================================
// TDC Request Handler
//=============================================================================

static void handle_tdc_request(const usb_command_t *cmd) {
    tdc_timestamp_t ts;
    int ret = tdc_single_shot(&ts);
    if (ret == 0) {
        // Convert to picoseconds
        uint64_t ps = tdc_timestamp_to_ps(&ts);
        usb_protocol_send_response(ERR_NONE, (uint8_t *)&ps, sizeof(ps));
    } else {
        usb_protocol_send_response(ERR_TIMEOUT, NULL, 0);
    }
}

//=============================================================================
// System Command Handler (Get Info)
//=============================================================================

static void handle_system_command(const usb_command_t *cmd) {
    (void)cmd;  // CMD_GET_INFO has no payload

    // Build device info structure
    struct __attribute__((packed)) {
        uint16_t device_id;
        uint8_t  hw_rev;
        uint8_t  hw_variant;
        uint8_t  fw_major;
        uint8_t  fw_minor;
        uint8_t  fw_patch;
        uint32_t fw_build;
        uint32_t capabilities;
        uint32_t uptime_s;
        float    temperature_c;
        uint8_t  calibration_valid;
        uint8_t  usb_speed;       // 0=USB2, 1=USB3
        uint8_t  power_good;
        uint8_t  reserved;
    } info;

    info.device_id = SYS_ID_DEVICE_CHRONOS;
    info.hw_rev = CHRONOS_PULSER_REV;
    info.hw_variant = CHRONOS_PULSER_VARIANT;
    info.fw_major = FW_VERSION_MAJOR;
    info.fw_minor = FW_VERSION_MINOR;
    info.fw_patch = FW_VERSION_PATCH;
    info.fw_build = FW_VERSION_BUILD;

    // Capabilities bitmap
    info.capabilities = (1 << 0)   // TDC present
                      | (1 << 1)   // ADC present
                      | (1 << 2)   // Pulse generator present
                      | (1 << 3)   // USB 3.0
                      | (1 << 4)   // Temperature sensor
                      | (1 << 5)   // SPI flash storage
                      | (250 << 16);  // ADC sample rate in MSPS

    info.uptime_s = g_uptime_seconds;
    info.temperature_c = temp_monitor_read();
    info.calibration_valid = g_calibration_valid ? 1 : 0;

    uint32_t usb_status = REG_READ(USB_STATUS_REG);
    info.usb_speed = (usb_status & USB_STATUS_USB3_ACTIVE) ? 1 : 0;

    uint32_t sys_status = REG_READ(SYS_STATUS_REG);
    info.power_good = (sys_status & SYS_STATUS_POWER_GOOD) ? 1 : 0;
    info.reserved = 0;

    usb_protocol_send_response(ERR_NONE, (uint8_t *)&info, sizeof(info));
}

//=============================================================================
// Health Monitor
//=============================================================================

static void health_monitor(void) {
    // Check temperature
    check_temperature();

    // Check power rails (via system status)
    uint32_t sys_status = REG_READ(SYS_STATUS_REG);
    if (!(sys_status & SYS_STATUS_POWER_GOOD)) {
        error_handler_record(ERR_HARDWARE, 0, "Power rail fault");
        g_system_state = STATE_ERROR;
        led_set_all(LED_COLOR_RED);
    }

    // Kick watchdog
    REG_WRITE(SYS_WDT_KICK_REG, SYS_WDT_KICK_MAGIC);

    // Update uptime
    static uint32_t last_uptime_lo = 0;
    uint32_t uptime_lo = REG_READ(SYS_UPTIME_LO_REG);
    if (uptime_lo < last_uptime_lo) {
        // Rollover — increment seconds counter
        g_uptime_seconds++;
    }
    last_uptime_lo = uptime_lo;
}

//=============================================================================
// Temperature Check
//=============================================================================

static void check_temperature(void) {
    float temp = temp_monitor_read();

    if (temp >= TEMP_SHUTDOWN_C) {
        // Emergency shutdown
        g_system_state = STATE_SHUTDOWN;
        led_set_all(LED_COLOR_RED);

        // Disable pulse generator and ADC
        pulse_stop();
        adc_power_down();
        vga_power_down();

        // Set overtemperature shutdown flag
        REG_SET_BITS(SYS_STATUS_REG, SYS_STATUS_OVERTEMP_SHDN);

        error_handler_record(ERR_OVERTEMP, (int)(temp * 100), "Thermal shutdown");
    } else if (temp >= TEMP_WARNING_C) {
        // Throttle: reduce pulse rate
        REG_SET_BITS(SYS_STATUS_REG, SYS_STATUS_OVERTEMP_WARN);
        led_set_all(LED_STATUS_OVERTEMP);

        // Reduce pulse frequency to 100 Hz
        pulse_config_t cfg;
        cfg.frequency_hz = 100;
        cfg.amplitude_dac = PULSE_DEFAULT_AMPLITUDE;
        cfg.continuous = true;
        cfg.burst_count = 0;
        cfg.external_trigger = false;
        pulse_configure(&cfg);
    } else if (temp < TEMP_NORMAL_MAX_C) {
        // Clear warning flags
        REG_CLR_BITS(SYS_STATUS_REG,
            SYS_STATUS_OVERTEMP_WARN | SYS_STATUS_OVERTEMP_SHDN);

        // If recovering from shutdown
        if (g_system_state == STATE_SHUTDOWN) {
            g_system_state = STATE_READY;
            adc_power_up();
            vga_power_up();
            led_set_all(LED_STATUS_READY);
            error_handler_record(ERR_NONE, 0, "Thermal recovery");
        }
    }
}

//=============================================================================
// Status LED Update
//=============================================================================

static void update_status_leds(void) {
    // LED state is set by state transitions
    // This function handles animations/blinking if needed

    static uint32_t led_toggle_counter = 0;
    led_toggle_counter++;

    if (g_system_state == STATE_ERROR) {
        // Blink red every 500 ms
        if ((led_toggle_counter % 50000000) == 0) {  // ~500ms at 100MHz
            static bool error_led_on = true;
            if (error_led_on) {
                led_set_all(LED_COLOR_OFF);
            } else {
                led_set_all(LED_COLOR_RED);
            }
            error_led_on = !error_led_on;
        }
    }

    if (g_system_state == STATE_ACQUIRING) {
        // Pulse cyan during acquisition
        if ((led_toggle_counter % 10000000) == 0) {  // ~100ms
            static bool acq_led_dim = true;
            if (acq_led_dim) {
                led_set_all(LED_COLOR_CYAN);
            } else {
                led_set_all(LED_COLOR_DIM_BLUE);
            }
            acq_led_dim = !acq_led_dim;
        }
    }
}

//=============================================================================
// Interrupt Handler
//=============================================================================

void irq_handler(void) __attribute__((interrupt("machine")));
void irq_handler(void) {
    uint32_t mcause = CSR_READ(mcause);

    if (mcause == MCAUSE_MTIMER) {
        // Machine timer interrupt — used for system tick
        // Clear timer interrupt
        CSR_WRITE(mtimecmp, CSR_READ(mtime) + 100000);  // 1 ms tick at 100 MHz
        return;
    }

    // External interrupt — read SYS_INT_STATUS to determine source
    uint32_t int_status = REG_READ(SYS_INT_STATUS_REG);

    if (int_status & (1 << IRQ_ADC_DMA_DONE)) {
        // ADC DMA complete — handled by acquisition sequencer
        REG_WRITE(SYS_INT_STATUS_REG, (1 << IRQ_ADC_DMA_DONE));
    }

    if (int_status & (1 << IRQ_USB_EP0)) {
        // USB endpoint 0 event
        REG_WRITE(SYS_INT_STATUS_REG, (1 << IRQ_USB_EP0));
    }

    if (int_status & (1 << IRQ_ACQ_COMPLETE)) {
        // Acquisition complete
        REG_WRITE(SYS_INT_STATUS_REG, (1 << IRQ_ACQ_COMPLETE));
    }

    if (int_status & (1 << IRQ_TEMP_ALERT)) {
        // Temperature alert
        REG_WRITE(SYS_INT_STATUS_REG, (1 << IRQ_TEMP_ALERT));
        check_temperature();
    }

    // Clear remaining interrupts
    REG_WRITE(SYS_INT_STATUS_REG, int_status);
}

//=============================================================================
// Panic Handler
//=============================================================================

void panic(const char *message) __attribute__((noreturn));
void panic(const char *message) {
    // Disable interrupts
    CSR_WRITE(mstatus, 0);

    // Set error LED
    led_set_all(LED_COLOR_RED);

    // Record error
    error_handler_record(ERR_HARDWARE, 0, message);

    // Halt
    while (1) {
        WFI();
    }
}
