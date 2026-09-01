/*
 * CordCanary telemetry formatter
 * Author: jayis1
 */

#include <stdio.h>

#include "comms.h"

void comms_format_frame(char *buffer,
                        size_t buffer_size,
                        cc_mode_t mode,
                        const cc_thermal_frame_t *thermal,
                        const cc_current_frame_t *current,
                        const cc_strain_frame_t *strain,
                        const cc_motion_frame_t *motion,
                        const cc_power_frame_t *power,
                        const cc_inference_t *inf)
{
    (void) snprintf(buffer,
                    buffer_size,
                    "CC|mode=%s|state=%s|risk=%.2f|hotspot=%.2f|current=%.2f|noise=%.2f|bend=%.1f|pull=%.1f|wobble=%.2f|humidity=%.1f|battery=%.1f|urgent=%u",
                    cc_mode_name(mode),
                    cc_state_name(inf->state),
                    inf->risk_score,
                    thermal->hotspot_delta_c,
                    current->rms_current_a,
                    current->hf_noise_score,
                    strain->bend_radius_mm,
                    strain->pull_force_n,
                    motion->wobble_score,
                    thermal->humidity_pct,
                    power->battery_pct,
                    inf->urgent_unplug ? 1U : 0U);
}

void comms_format_registers(char *buffer, size_t buffer_size, const cc_register_bank_t *bank)
{
    (void) snprintf(buffer,
                    buffer_size,
                    "REG hotspot=%.2f rise=%.2f current=%.2f crest=%.2f noise=%.2f leak=%.2f bend=%.1f pull=%.1f wobble=%.2f batt=%.1f risk=%.2f health=%.2f flags=%.0f",
                    bank->thermal_hotspot,
                    bank->thermal_rise_rate,
                    bank->current_rms,
                    bank->current_crest,
                    bank->current_hf_noise,
                    bank->current_leakage,
                    bank->strain_bend,
                    bank->strain_pull,
                    bank->motion_wobble,
                    bank->power_battery,
                    bank->risk_score,
                    bank->outlet_health,
                    bank->alert_flags);
}
