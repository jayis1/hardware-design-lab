/*
 * Pantry Warden power driver simulation
 * Author: jayis1
 */

#include <math.h>

#include "power.h"

static float clampf(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

void power_init(pw_power_driver_t *driver, pw_power_frame_t *frame)
{
    driver->battery_capacity_mah = 2200.0f;
    driver->nominal_voltage_v = 3.78f;

    frame->battery_pct = 96.0f;
    frame->bus_voltage_v = 4.98f;
    frame->current_ma = 61.0f;
    frame->estimated_hours_left = 520.0f;
    frame->charging = 0;
}

void power_step(pw_power_driver_t *driver,
                pw_power_frame_t *frame,
                const pw_gas_frame_t *gas,
                pw_mode_t mode,
                unsigned tick)
{
    const float mode_current = (mode == PW_MODE_NIGHT_SWEEP) ? 24.0f : (mode == PW_MODE_CLEANOUT ? 36.0f : 0.0f);
    const float gas_current = gas->fan_duty_pct * 0.62f;
    const float base_current = 58.0f + gas_current + mode_current;
    const float periodic_charge = (tick >= 46U) ? 140.0f : 0.0f;
    const float consumption_pct = base_current / (driver->battery_capacity_mah * 4.5f);

    frame->charging = (periodic_charge > 0.0f) ? 1 : 0;
    frame->current_ma = base_current - periodic_charge;
    frame->battery_pct = clampf(frame->battery_pct - consumption_pct + (frame->charging ? 2.6f : 0.0f),
                                0.0f,
                                100.0f);
    frame->bus_voltage_v = frame->charging ? 5.02f : driver->nominal_voltage_v + (frame->battery_pct / 100.0f) * 0.36f;
    frame->estimated_hours_left = clampf((frame->battery_pct / 100.0f) * (driver->battery_capacity_mah / fmaxf(base_current, 20.0f)) * 48.0f,
                                         0.0f,
                                         900.0f);
}
