/*
 * CordCanary inference engine simulation
 * Author: jayis1
 */

#include <stdio.h>
#include <string.h>

#include "inference.h"

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

static void write_text(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0U) {
        return;
    }
    (void) snprintf(dst, dst_size, "%s", src);
}

void inference_init(cc_inference_t *inf)
{
    inf->state = CC_STATE_NOMINAL;
    inf->risk_score = 0.08f;
    inf->outlet_health_score = 0.94f;
    inf->confidence = 0.62f;
    inf->urgent_unplug = false;
    write_text(inf->advisory, sizeof(inf->advisory), "Connection healthy. Periodic monitoring active.");
    write_text(inf->root_cause, sizeof(inf->root_cause), "No stress pattern exceeds baseline.");
}

void inference_evaluate(cc_inference_t *inf,
                        const cc_thermal_frame_t *thermal,
                        const cc_current_frame_t *current,
                        const cc_strain_frame_t *strain,
                        const cc_motion_frame_t *motion,
                        const cc_power_frame_t *power,
                        cc_mode_t mode,
                        unsigned tick)
{
    float load_watch = 0.0f;
    float outlet_wear = 0.0f;
    float cord_fatigue = 0.0f;
    float damp_leakage = 0.0f;
    float arc_suspect = 0.0f;
    float battery_penalty = 0.0f;
    cc_state_t state = CC_STATE_NOMINAL;
    float winning_score = 0.05f;

    (void) tick;

    if (current->rms_current_a > 9.0f) {
        load_watch += 0.28f + (current->rms_current_a - 9.0f) * 0.04f;
    }
    if (thermal->cord_neck_temp_c - thermal->ambient_c > 7.0f) {
        load_watch += 0.18f;
    }
    if (thermal->hotspot_delta_c > 2.2f) {
        outlet_wear += 0.26f + (thermal->hotspot_delta_c - 2.2f) * 0.05f;
    }
    if (thermal->rise_rate_cpm > 0.8f) {
        outlet_wear += 0.12f;
        arc_suspect += 0.10f;
    }
    if (motion->wobble_score > 0.22f) {
        outlet_wear += 0.18f;
        arc_suspect += 0.12f;
    }
    if (strain->bend_radius_mm < 24.0f) {
        cord_fatigue += 0.34f + (24.0f - strain->bend_radius_mm) * 0.012f;
    }
    if (strain->pull_force_n > 3.5f) {
        cord_fatigue += 0.16f + (strain->pull_force_n - 3.5f) * 0.03f;
    }
    if (strain->fatigue_index > 0.45f) {
        cord_fatigue += 0.14f;
    }
    if (thermal->humidity_pct > 68.0f) {
        damp_leakage += 0.20f + (thermal->humidity_pct - 68.0f) * 0.01f;
    }
    if (current->leakage_ma > 1.2f) {
        damp_leakage += 0.18f + (current->leakage_ma - 1.2f) * 0.05f;
    }
    if (thermal->dew_margin_c < 4.0f) {
        damp_leakage += 0.16f;
    }
    if (current->hf_noise_score > 0.42f) {
        arc_suspect += 0.32f + (current->hf_noise_score - 0.42f) * 0.30f;
    }
    if (current->crest_factor > 1.7f) {
        arc_suspect += 0.18f;
    }
    if (current->transient_density > 0.22f) {
        arc_suspect += 0.16f;
    }
    if (mode == CC_MODE_GARAGE) {
        damp_leakage += 0.05f;
    }
    if (mode == CC_MODE_WORKSHOP) {
        arc_suspect += 0.03f;
    }
    if (power->battery_pct < 20.0f) {
        battery_penalty = 0.08f;
    }

    winning_score = load_watch;
    state = load_watch > 0.24f ? CC_STATE_LOAD_WATCH : CC_STATE_NOMINAL;

    if (outlet_wear > winning_score) {
        winning_score = outlet_wear;
        state = CC_STATE_OUTLET_WEAR;
    }
    if (cord_fatigue > winning_score) {
        winning_score = cord_fatigue;
        state = CC_STATE_CORD_FATIGUE;
    }
    if (damp_leakage > winning_score) {
        winning_score = damp_leakage;
        state = CC_STATE_DAMP_LEAKAGE;
    }
    if (arc_suspect > winning_score) {
        winning_score = arc_suspect;
        state = CC_STATE_ARC_SUSPECT;
    }
    if (winning_score < 0.24f) {
        state = CC_STATE_NOMINAL;
    }

    inf->state = state;
    inf->risk_score = clampf(winning_score + battery_penalty, 0.02f, 0.99f);
    inf->confidence = clampf(0.55f + winning_score * 0.45f, 0.55f, 0.98f);
    inf->outlet_health_score = clampf(1.0f - (outlet_wear * 0.55f + arc_suspect * 0.30f + cord_fatigue * 0.15f), 0.08f, 0.99f);
    inf->urgent_unplug = inf->risk_score > 0.74f || (state == CC_STATE_ARC_SUSPECT && current->hf_noise_score > 0.55f);

    switch (state) {
    case CC_STATE_LOAD_WATCH:
        write_text(inf->advisory, sizeof(inf->advisory),
                   "Heavy load detected. Keep airflow clear and watch heat rise.");
        write_text(inf->root_cause, sizeof(inf->root_cause),
                   "High RMS current with broad warming suggests legitimate load near comfort limits.");
        break;
    case CC_STATE_OUTLET_WEAR:
        write_text(inf->advisory, sizeof(inf->advisory),
                   "Plug face is heating faster than cord. Reseat or replace outlet.");
        write_text(inf->root_cause, sizeof(inf->root_cause),
                   "Localized blade-side hotspot plus wobble indicates poor receptacle grip or oxidation.");
        break;
    case CC_STATE_CORD_FATIGUE:
        write_text(inf->advisory, sizeof(inf->advisory),
                   "Cord bend is too tight for the present load. Relax routing and strain.");
        write_text(inf->root_cause, sizeof(inf->root_cause),
                   "Small bend radius and sustained pull force raise conductor fatigue risk.");
        break;
    case CC_STATE_DAMP_LEAKAGE:
        write_text(inf->advisory, sizeof(inf->advisory),
                   "Moisture risk rising. Dry the area and inspect for contamination.");
        write_text(inf->root_cause, sizeof(inf->root_cause),
                   "High humidity, low dew margin, and leakage current suggest surface tracking conditions.");
        break;
    case CC_STATE_ARC_SUSPECT:
        write_text(inf->advisory, sizeof(inf->advisory),
                   "Electrical instability suspected. Unplug soon and inspect plug, outlet, and load.");
        write_text(inf->root_cause, sizeof(inf->root_cause),
                   "Burst noise, elevated crest factor, and hotspot rise are consistent with intermittent arcing.");
        break;
    case CC_STATE_NOMINAL:
    default:
        write_text(inf->advisory, sizeof(inf->advisory),
                   "Connection healthy. Periodic monitoring active.");
        write_text(inf->root_cause, sizeof(inf->root_cause),
                   "No combined thermal, mechanical, or electrical signature exceeds baseline.");
        break;
    }
}
