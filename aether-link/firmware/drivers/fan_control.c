// fan_control.c — Fan Control Driver for Aether-Link
// Manages dual 40mm PWM fans via EMC2301 I2C controller.
// Provides thermal management with temperature-based fan curves.

#include "../registers.h"
#include "../board.h"

// Fan control functions are implemented in gpio_init.c
// This file provides the higher-level thermal management logic.

extern void fan_control_init(void);
extern void fan_set_pwm(uint8_t duty);
extern uint16_t fan_read_rpm(uint8_t fan_idx);

// ============================================================================
// Fan Curve Definition
// ============================================================================
//
// Temperature (cC) → PWM Duty
//   < 40°C: 30 (minimum, ~12%)
//   40-55°C: linear ramp 30→80
//   55-70°C: linear ramp 80→150
//   70-85°C: linear ramp 150→200
//   85-95°C: linear ramp 200→255
//   > 95°C: 255 (maximum, 100%)

typedef struct {
    int16_t temp_min_cC;   // Minimum temperature for this segment (centi-Celsius)
    int16_t temp_max_cC;   // Maximum temperature for this segment
    uint8_t pwm_min;       // PWM at temp_min
    uint8_t pwm_max;       // PWM at temp_max
} fan_curve_segment_t;

static const fan_curve_segment_t fan_curve[] = {
    { 2000,  4000, FAN_PWM_MIN,    80  },  // 20-40°C: min to quiet
    { 4000,  5500, 80,             120 },  // 40-55°C: quiet to moderate
    { 5500,  7000, 120,            160 },  // 55-70°C: moderate to high
    { 7000,  8500, 160,            200 },  // 70-85°C: high to warning
    { 8500,  9500, 200,            FAN_PWM_MAX },  // 85-95°C: warning to max
    { 9500, 11000, FAN_PWM_MAX,    FAN_PWM_MAX },  // >95°C: max
};

#define FAN_CURVE_SEGMENTS (sizeof(fan_curve) / sizeof(fan_curve_segment_t))

// ============================================================================
// Thermal Fan Control
// ============================================================================

uint8_t fan_curve_lookup(int16_t temp_cC) {
    // Below minimum: use minimum PWM
    if (temp_cC < fan_curve[0].temp_min_cC) {
        return FAN_PWM_MIN;
    }

    // Above maximum: use maximum PWM
    if (temp_cC > fan_curve[FAN_CURVE_SEGMENTS - 1].temp_max_cC) {
        return FAN_PWM_MAX;
    }

    // Find the right segment and interpolate
    for (uint8_t i = 0; i < FAN_CURVE_SEGMENTS; i++) {
        if (temp_cC >= fan_curve[i].temp_min_cC &&
            temp_cC <= fan_curve[i].temp_max_cC) {

            // Linear interpolation within segment
            int16_t range = fan_curve[i].temp_max_cC - fan_curve[i].temp_min_cC;
            int16_t offset = temp_cC - fan_curve[i].temp_min_cC;
            uint8_t pwm_range = fan_curve[i].pwm_max - fan_curve[i].pwm_min;

            if (range == 0) return fan_curve[i].pwm_min;

            uint8_t pwm = fan_curve[i].pwm_min +
                          (uint8_t)(((uint16_t)offset * pwm_range) / (uint16_t)range);
            return pwm;
        }
    }

    return FAN_PWM_DEFAULT;  // Fallback
}

// ============================================================================
// Fan Health Monitoring
// ============================================================================

typedef enum {
    FAN_OK,
    FAN_STALLED,
    FAN_MISSING,
    FAN_FAILING
} fan_health_t;

fan_health_t fan_check_health(uint8_t fan_idx) {
    uint16_t rpm = fan_read_rpm(fan_idx);

    // Fan not spinning at all
    if (rpm == 0) {
        return FAN_STALLED;
    }

    // Fan spinning too slowly (below 500 RPM for a 40mm fan)
    if (rpm < 500) {
        return FAN_FAILING;
    }

    // Fan spinning too fast (above 15000 RPM — unlikely for 40mm)
    if (rpm > 15000) {
        return FAN_FAILING;
    }

    return FAN_OK;
}

// ============================================================================
// Fan Test Sequence
// ============================================================================

void fan_test_sequence(void) {
    // Ramp fan from min to max and back to verify operation
    for (uint8_t duty = FAN_PWM_MIN; duty <= FAN_PWM_MAX; duty += 10) {
        fan_set_pwm(duty);
        for (volatile int i = 0; i < 2000000; i++);  // ~10ms delay
    }
    for (uint8_t duty = FAN_PWM_MAX; duty >= FAN_PWM_MIN; duty -= 10) {
        fan_set_pwm(duty);
        for (volatile int i = 0; i < 2000000; i++);
    }
    fan_set_pwm(FAN_PWM_DEFAULT);
}
