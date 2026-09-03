/*
 * Pantry Warden comms formatter
 * Author: jayis1
 */

#include <stdio.h>

#include "comms.h"
#include "inference.h"

static const char *mode_name(pw_mode_t mode)
{
    switch (mode) {
    case PW_MODE_AUTO:
        return "AUTO";
    case PW_MODE_QUIET:
        return "QUIET";
    case PW_MODE_NIGHT_SWEEP:
        return "NIGHT_SWEEP";
    case PW_MODE_CLEANOUT:
        return "CLEANOUT";
    default:
        return "UNKNOWN";
    }
}

void comms_format_frame(char *buffer,
                        size_t buffer_size,
                        unsigned tick,
                        const pw_gas_frame_t *gas,
                        const pw_shelf_frame_t *shelf,
                        const pw_acoustic_frame_t *acoustic,
                        const pw_power_frame_t *power,
                        const pw_inference_t *inf,
                        pw_mode_t mode)
{
    (void)snprintf(buffer,
                   buffer_size,
                   "{\"tick\":%u,\"mode\":\"%s\",\"state\":\"%s\",\"temp\":%.2f,\"rh\":%.2f,\"co2\":%.1f,\"voc\":%.1f,\"mass\":%.2f,\"gap\":%.1f,\"moist\":%.1f,\"wing\":%.1f,\"chew\":%.1f,\"bat\":%.1f,\"health\":%.1f}",
                   tick,
                   mode_name(mode),
                   inference_state_name(inf->state),
                   gas->temp_c,
                   gas->humidity_pct,
                   gas->co2_ppm,
                   gas->voc_index,
                   shelf->total_mass_kg,
                   shelf->front_gap_mm,
                   shelf->moisture_strip_pct,
                   acoustic->wingbeat_score,
                   acoustic->chew_score,
                   power->battery_pct,
                   inf->shelf_health_score);
}
