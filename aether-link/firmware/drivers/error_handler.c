// error_handler.c — Error Handler for Aether-Link Firmware
// Centralized error logging, recovery, and reporting.
// Logs errors to DDR4 ring buffer and signals host via PCIe AER.

#include "../registers.h"
#include "../board.h"

// ============================================================================
// Error Log Entry Structure
// ============================================================================

typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms;   // Uptime when error occurred
    uint8_t  error_code;     // Error code from board.h
    uint8_t  severity;       // 0=info, 1=warning, 2=error, 3=fatal
    uint8_t  source;         // Source module ID
    uint8_t  reserved;
    uint32_t data0;          // Error-specific data
    uint32_t data1;          // Error-specific data
    uint32_t marker;         // 0xDEADBEEF for validity check
} error_log_entry_t;

// Error severity levels
#define ERR_SEV_INFO     0
#define ERR_SEV_WARNING  1
#define ERR_SEV_ERROR    2
#define ERR_SEV_FATAL    3

// Error source modules
#define ERR_SRC_SYSTEM   0
#define ERR_SRC_CLOCK    1
#define ERR_SRC_DDR4     2
#define ERR_SRC_PCIE     3
#define ERR_SRC_TOE      4
#define ERR_SRC_NVMEOF   5
#define ERR_SRC_I2C      6
#define ERR_SRC_SPI      7
#define ERR_SRC_THERMAL  8
#define ERR_SRC_POWER    9
#define ERR_SRC_FAN      10
#define ERR_SRC_QSFP     11

// Error log ring buffer: 1024 entries × 16 bytes = 16KB
#define ERROR_LOG_MAX_ENTRIES  1024
#define ERROR_LOG_MARKER       0xDEADBEEF

static volatile uint32_t *error_log_idx = (volatile uint32_t *)AETHER_REG_ERROR_LOG_IDX;
static error_log_entry_t *error_log = (error_log_entry_t *)DDR4_CHA_ERROR_LOG;

// ============================================================================
// Initialization
// ============================================================================

void error_handler_init(void) {
    // Clear error log
    for (int i = 0; i < ERROR_LOG_MAX_ENTRIES; i++) {
        error_log[i].marker = 0;
    }
    *error_log_idx = 0;
}

// ============================================================================
// Error Logging
// ============================================================================

void error_log_entry(uint8_t error_code, uint8_t severity, uint8_t source,
                     uint32_t data0, uint32_t data1, uint32_t uptime_ms) {

    uint32_t idx = *error_log_idx;
    error_log_entry_t *entry = &error_log[idx];

    entry->timestamp_ms = uptime_ms;
    entry->error_code = error_code;
    entry->severity = severity;
    entry->source = source;
    entry->reserved = 0;
    entry->data0 = data0;
    entry->data1 = data1;
    entry->marker = ERROR_LOG_MARKER;

    // Advance ring buffer index
    *error_log_idx = (idx + 1) & (ERROR_LOG_MAX_ENTRIES - 1);
}

// ============================================================================
// Error Counters
// ============================================================================

static uint32_t error_counts[16];  // Count per error code

void error_increment_counter(uint8_t error_code) {
    if (error_code < 16) {
        error_counts[error_code]++;
    }
}

uint32_t error_get_count(uint8_t error_code) {
    if (error_code < 16) {
        return error_counts[error_code];
    }
    return 0;
}

// ============================================================================
// Error Recovery Actions
// ============================================================================

typedef enum {
    RECOVERY_NONE,
    RECOVERY_RETRY,
    RECOVERY_RESET_MODULE,
    RECOVERY_RESET_SYSTEM,
    RECOVERY_SHUTDOWN
} recovery_action_t;

recovery_action_t error_get_recovery_action(uint8_t error_code) {
    switch (error_code) {
        case ERR_MMCM_LOCK_FAILED:
            return RECOVERY_RESET_SYSTEM;
        case ERR_DDR4_CAL_FAILED:
            return RECOVERY_RESET_SYSTEM;
        case ERR_PCIE_LINK_FAILED:
            return RECOVERY_RETRY;  // Retry link training
        case ERR_CMAC_PLL_FAILED:
            return RECOVERY_RESET_MODULE;
        case ERR_I2C_BUS_FAILED:
            return RECOVERY_RETRY;
        case ERR_SPI_FLASH_FAILED:
            return RECOVERY_RESET_SYSTEM;
        case ERR_TEMP_CRITICAL:
            return RECOVERY_SHUTDOWN;
        case ERR_POWER_OVER_LIMIT:
            return RECOVERY_RESET_MODULE;  // Throttle
        case ERR_FAN_FAILED:
            return RECOVERY_RESET_MODULE;  // Reduce power
        case ERR_TOE_HW_FAULT:
            return RECOVERY_RESET_MODULE;
        case ERR_NVMEOF_PDU_FAULT:
            return RECOVERY_RESET_MODULE;
        case ERR_WATCHDOG_TIMEOUT:
            return RECOVERY_RESET_SYSTEM;
        case ERR_FW_UPDATE_FAILED:
            return RECOVERY_RESET_SYSTEM;
        case ERR_SECURE_BOOT_FAILED:
            return RECOVERY_SHUTDOWN;
        case ERR_QSFP_MODULE_FAULT:
            return RECOVERY_RESET_MODULE;
        default:
            return RECOVERY_NONE;
    }
}

// ============================================================================
// Error String Lookup (for debug UART)
// ============================================================================

static const char *error_strings[] = {
    "NONE",
    "MMCM_LOCK_FAILED",
    "DDR4_CAL_FAILED",
    "PCIE_LINK_FAILED",
    "CMAC_PLL_FAILED",
    "I2C_BUS_FAILED",
    "SPI_FLASH_FAILED",
    "TEMP_CRITICAL",
    "POWER_OVER_LIMIT",
    "FAN_FAILED",
    "TOE_HW_FAULT",
    "NVMEOF_PDU_FAULT",
    "WATCHDOG_TIMEOUT",
    "FW_UPDATE_FAILED",
    "SECURE_BOOT_FAILED",
    "QSFP_MODULE_FAULT"
};

const char *error_get_string(uint8_t error_code) {
    if (error_code < 16) {
        return error_strings[error_code];
    }
    return "UNKNOWN";
}

// ============================================================================
// Error Dump (for debug)
// ============================================================================

void error_dump_recent(uint8_t count, void (*putstr)(const char *),
                        void (*puthex)(uint32_t)) {
    uint32_t current_idx = *error_log_idx;
    uint32_t start_idx;

    if (current_idx >= count) {
        start_idx = current_idx - count;
    } else {
        start_idx = ERROR_LOG_MAX_ENTRIES - (count - current_idx);
    }

    putstr("=== Error Log Dump ===\r\n");
    for (uint8_t i = 0; i < count; i++) {
        uint32_t idx = (start_idx + i) & (ERROR_LOG_MAX_ENTRIES - 1);
        error_log_entry_t *entry = &error_log[idx];

        if (entry->marker != ERROR_LOG_MARKER) continue;

        putstr("[");
        puthex(entry->timestamp_ms);
        putstr("] ");
        putstr(error_get_string(entry->error_code));
        putstr(" sev=");
        puthex(entry->severity);
        putstr(" src=");
        puthex(entry->source);
        putstr(" d0=");
        puthex(entry->data0);
        putstr(" d1=");
        puthex(entry->data1);
        putstr("\r\n");
    }
    putstr("=== End Error Log ===\r\n");
}
