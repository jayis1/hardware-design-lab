// power_monitor.c — Power Monitor Driver for Aether-Link
// Reads INA226 power monitor via I2C for board power tracking.
// Provides voltage, current, and power measurements for telemetry.

#include "../registers.h"
#include "../board.h"

// Power monitor functions are implemented in gpio_init.c
// This file provides the higher-level power management logic.

extern void power_monitor_init(void);
extern uint32_t power_monitor_read_mw(void);
extern uint16_t power_monitor_read_voltage_mv(void);
extern uint16_t power_monitor_read_current_ma(void);

// ============================================================================
// Power Limit Enforcement
// ============================================================================

// PCIe x8 slot power limit: 75W (PCIe CEM 4.0 spec)
#define PCIE_SLOT_POWER_LIMIT_MW   75000
#define POWER_WARN_THRESHOLD_MW    65000  // 65W — warn host
#define POWER_CRIT_THRESHOLD_MW    72000  // 72W — throttle

typedef enum {
    POWER_OK,
    POWER_WARNING,
    POWER_CRITICAL,
    POWER_OVER_LIMIT
} power_status_t;

power_status_t power_check_status(void) {
    uint32_t power_mw = power_monitor_read_mw();

    if (power_mw > PCIE_SLOT_POWER_LIMIT_MW) {
        return POWER_OVER_LIMIT;
    } else if (power_mw > POWER_CRIT_THRESHOLD_MW) {
        return POWER_CRITICAL;
    } else if (power_mw > POWER_WARN_THRESHOLD_MW) {
        return POWER_WARNING;
    }
    return POWER_OK;
}

// ============================================================================
// Power Telemetry
// ============================================================================

typedef struct {
    uint32_t power_mw;
    uint16_t voltage_mv;
    uint16_t current_ma;
    power_status_t status;
    uint32_t timestamp_ms;
} power_telemetry_t;

void power_get_telemetry(power_telemetry_t *telem, uint32_t uptime_ms) {
    telem->power_mw = power_monitor_read_mw();
    telem->voltage_mv = power_monitor_read_voltage_mv();
    telem->current_ma = power_monitor_read_current_ma();
    telem->status = power_check_status();
    telem->timestamp_ms = uptime_ms;
}

// ============================================================================
// Power-On Self Test
// ============================================================================

int power_self_test(void) {
    // Verify 12V rail is within spec (±8% = 11.04V to 12.96V)
    uint16_t voltage_mv = power_monitor_read_voltage_mv();
    if (voltage_mv < 11040 || voltage_mv > 12960) {
        return -1;  // 12V rail out of spec
    }

    // Verify current draw is reasonable at idle (<2A)
    uint16_t current_ma = power_monitor_read_current_ma();
    if (current_ma > 2000) {
        return -2;  // Excessive idle current
    }

    return 0;  // Power OK
}
