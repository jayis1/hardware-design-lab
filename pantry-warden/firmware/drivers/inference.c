/*
 * Pantry Warden inference engine
 * Author: jayis1
 */

#include <math.h>

#include "inference.h"
#include "gas.h"

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

const char *inference_state_name(pw_state_t state)
{
    switch (state) {
    case PW_STATE_STABLE:
        return "STABLE";
    case PW_STATE_RESTOCKED:
        return "RESTOCKED";
    case PW_STATE_CONDENSATION_WATCH:
        return "CONDENSATION_WATCH";
    case PW_STATE_SPOILAGE_SUSPECT:
        return "SPOILAGE_SUSPECT";
    case PW_STATE_PEST_WATCH:
        return "PEST_WATCH";
    case PW_STATE_CRITICAL_INTERVENE:
        return "CRITICAL_INTERVENE";
    default:
        return "UNKNOWN";
    }
}

void inference_init(pw_inference_t *inf)
{
    inf->state = PW_STATE_STABLE;
    inf->shelf_health_score = 92.0f;
    inf->spoilage_confidence = 4.0f;
    inf->pest_confidence = 3.0f;
    inf->condensation_risk = 8.0f;
    inf->restock_confidence = 0.0f;
    inf->action = "No action";
}

static float compute_condensation(const pw_gas_frame_t *gas, const pw_shelf_frame_t *shelf)
{
    return clampf(fmaxf(0.0f, gas->humidity_pct - 60.0f) * 1.6f
                  + fmaxf(0.0f, 6.0f - gas->dew_margin_c) * 7.0f
                  + fmaxf(0.0f, shelf->moisture_strip_pct - 18.0f) * 0.9f,
                  0.0f,
                  100.0f);
}

static float compute_pest(const pw_shelf_frame_t *shelf, const pw_acoustic_frame_t *acoustic)
{
    return clampf(acoustic->wingbeat_score * 0.55f
                  + acoustic->chew_score * 0.60f
                  + shelf->disturbance_score * 0.35f,
                  0.0f,
                  100.0f);
}

static float compute_restock(float mass_delta_kg, const pw_gas_frame_t *gas, const pw_shelf_frame_t *shelf)
{
    const float mass_component = fmaxf(0.0f, mass_delta_kg) * 34.0f;
    const float geometry_component = fmaxf(0.0f, 68.0f - shelf->front_gap_mm) * 2.0f;
    const float chemistry_penalty = fmaxf(0.0f, gas->voc_index - 20.0f) * 2.5f;
    return clampf(mass_component + geometry_component - chemistry_penalty,
                  0.0f,
                  100.0f);
}

static float compute_spoilage(const pw_gas_frame_t *gas, const pw_shelf_frame_t *shelf)
{
    const float lift = gas_spoilage_lift(gas);
    const float bulge_component = fmaxf(0.0f, 62.0f - shelf->front_gap_mm) * 2.4f;
    const float optical_component = fmaxf(0.0f, 88.0f - shelf->optical_freshness_pct) * 1.8f;
    const float moisture_component = fmaxf(0.0f, shelf->moisture_strip_pct - 22.0f) * 0.9f;
    return clampf(lift + bulge_component + optical_component + moisture_component,
                  0.0f,
                  100.0f);
}

void inference_evaluate(pw_inference_t *inf,
                        const pw_gas_frame_t *gas,
                        const pw_shelf_frame_t *shelf,
                        const pw_acoustic_frame_t *acoustic,
                        const pw_power_frame_t *power,
                        float mass_delta_kg,
                        pw_mode_t mode)
{
    const float condensation = compute_condensation(gas, shelf);
    const float pest = compute_pest(shelf, acoustic);
    const float restock = compute_restock(mass_delta_kg, gas, shelf);
    const float spoilage = compute_spoilage(gas, shelf);
    float health = 100.0f;

    health -= spoilage * 0.42f;
    health -= pest * 0.28f;
    health -= condensation * 0.26f;
    health += (restock > 35.0f && spoilage < 25.0f) ? 5.0f : 0.0f;
    health -= (power->battery_pct < PW_LOW_BATTERY_PCT) ? 6.0f : 0.0f;
    health += (mode == PW_MODE_CLEANOUT) ? 3.0f : 0.0f;

    inf->spoilage_confidence = spoilage;
    inf->pest_confidence = pest;
    inf->condensation_risk = condensation;
    inf->restock_confidence = restock;
    inf->shelf_health_score = clampf(health, 0.0f, 100.0f);

    if (spoilage > 72.0f || (pest > 78.0f && condensation > 42.0f)) {
        inf->state = PW_STATE_CRITICAL_INTERVENE;
        inf->action = "Inspect right-front package, isolate suspect food, wipe shelf, deep-clean zone";
    } else if (pest > 62.0f) {
        inf->state = PW_STATE_PEST_WATCH;
        inf->action = "Inspect flour/cereal packages, install trap, run night sweep, vacuum crevices";
    } else if (spoilage > 54.0f) {
        inf->state = PW_STATE_SPOILAGE_SUSPECT;
        inf->action = "Check bulging or odor source; rotate and discard one suspect package if confirmed";
    } else if (condensation > 46.0f) {
        inf->state = PW_STATE_CONDENSATION_WATCH;
        inf->action = "Increase spacing, reduce humidity exposure, dry rear shelf wall, enable airflow";
    } else if (restock > 42.0f && spoilage < 24.0f && pest < 28.0f) {
        inf->state = PW_STATE_RESTOCKED;
        inf->action = "Restock logged; confirm oldest items remain in front for rotation";
    } else {
        inf->state = PW_STATE_STABLE;
        inf->action = "Shelf stable; continue routine rotation and weekly wipe-down";
    }
}
