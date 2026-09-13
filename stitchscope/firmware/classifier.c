/* StitchScope explainable fault classifier
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "stitchscope.h"
#include <limits.h>
#include <string.h>

#define FEATURE_COUNT 10u
#define Q12_ONE 4096u

static recipe_t active_recipe;
static stitch_class_t pending_class;
static uint8_t pending_count;
static uint16_t bobbin_trend;
static uint16_t nest_trend;

static int32_t feature_value(const stitch_features_t *f, size_t index) {
    switch (index) {
        case 0u: return f->force_peak_un;
        case 1u: return f->force_integral_un_bins;
        case 2u: return f->force_slope_un_per_bin;
        case 3u: return (int32_t)f->force_hysteresis;
        case 4u: return (int32_t)f->impulse_low;
        case 5u: return (int32_t)f->impulse_mid;
        case 6u: return (int32_t)f->impulse_high;
        case 7u: return f->feed_um;
        case 8u: return (int32_t)f->duration_us;
        case 9u: return (int32_t)f->impulse_phase_q15;
        default: return 0;
    }
}

static uint32_t abs_difference(int32_t a, int32_t b) {
    int64_t d = (int64_t)a - b;
    if (d < 0) {
        d = -d;
    }
    return d > UINT32_MAX ? UINT32_MAX : (uint32_t)d;
}

static uint16_t anomaly_score(const stitch_features_t *f) {
    uint64_t sum = 0u;
    for (size_t i = 0u; i < FEATURE_COUNT; ++i) {
        uint32_t deviation = active_recipe.deviation[i];
        if (deviation < 1u) {
            deviation = 1u;
        }
        uint32_t delta = abs_difference(feature_value(f, i), active_recipe.mean[i]);
        uint32_t normalized_q8 = (uint32_t)(((uint64_t)delta << 8) / deviation);
        if (normalized_q8 > 2048u) {
            normalized_q8 = 2048u;
        }
        sum += (uint64_t)normalized_q8 * normalized_q8;
    }
    uint32_t average = (uint32_t)(sum / FEATURE_COUNT);
    /* Cheap monotonic square-root approximation is sufficient for ranking. */
    uint32_t root = 0u;
    uint32_t bit = 1u << 15;
    while (bit != 0u) {
        uint32_t candidate = root | bit;
        if ((uint64_t)candidate * candidate <= average) {
            root = candidate;
        }
        bit >>= 1;
    }
    uint32_t scaled = root * 16u;
    return (uint16_t)(scaled > UINT16_MAX ? UINT16_MAX : scaled);
}

void classifier_init(void) {
    memset(&active_recipe, 0, sizeof(active_recipe));
    pending_class = CLASS_OK;
    pending_count = 0u;
    bobbin_trend = 0u;
    nest_trend = 0u;
}

void classifier_set_recipe(const recipe_t *recipe) {
    if (recipe != NULL) {
        active_recipe = *recipe;
    }
    pending_class = CLASS_OK;
    pending_count = 0u;
}

static stitch_class_t raw_class(const stitch_features_t *f) {
    int32_t normal_force = active_recipe.mean[0];
    uint32_t normal_impulse = 1u;
    uint32_t actual_impulse = f->impulse_low;
    for (size_t i = 4u; i <= 6u; ++i) {
        if (active_recipe.mean[i] > 0 && (uint32_t)active_recipe.mean[i] > normal_impulse) {
            normal_impulse = (uint32_t)active_recipe.mean[i];
        }
    }
    if (f->impulse_mid > actual_impulse) actual_impulse = f->impulse_mid;
    if (f->impulse_high > actual_impulse) actual_impulse = f->impulse_high;
    uint32_t normal_hysteresis = active_recipe.mean[3] > 0 ?
                                 (uint32_t)active_recipe.mean[3] : 1u;

    if (actual_impulse > normal_impulse * 4u && actual_impulse > 4000u) {
        return CLASS_HARD_STRIKE;
    }
    if (normal_force > 0 && f->force_peak_un < normal_force / 5) {
        return CLASS_TOP_THREAD_BREAK;
    }
    if (f->feed_confidence_q15 > 9000u && active_recipe.expected_feed_um > 0u) {
        uint32_t feed = f->feed_um < 0 ? (uint32_t)-f->feed_um : (uint32_t)f->feed_um;
        if (feed < active_recipe.expected_feed_um / 5u) {
            return CLASS_FEED_STALL;
        }
    }
    if (f->force_hysteresis > normal_hysteresis * 2u && nest_trend >= 2u) {
        return CLASS_BIRDNEST_GROWTH;
    }
    if (bobbin_trend >= 5u && f->anomaly_q12 > 4500u) {
        return CLASS_BOBBIN_DEPLETION;
    }
    if (f->anomaly_q12 > 9000u) {
        return CLASS_SKIP_SUSPECT;
    }
    return CLASS_OK;
}

static uint8_t persistence_required(stitch_class_t classification) {
    switch (classification) {
        case CLASS_HARD_STRIKE:
        case CLASS_TOP_THREAD_BREAK:
            return 1u;
        case CLASS_FEED_STALL:
        case CLASS_BIRDNEST_GROWTH:
            return 2u;
        case CLASS_BOBBIN_DEPLETION:
        case CLASS_SKIP_SUSPECT:
            return 3u;
        default:
            return 1u;
    }
}

stitch_class_t classifier_evaluate(stitch_features_t *features) {
    if (features == NULL) {
        return CLASS_UNTRAINED;
    }
    if (!active_recipe.trained || active_recipe.sample_count < 20u) {
        features->classification = CLASS_UNTRAINED;
        features->quality_q12 = 0u;
        return CLASS_UNTRAINED;
    }

    features->anomaly_q12 = anomaly_score(features);
    features->quality_q12 = features->anomaly_q12 >= Q12_ONE ? 0u :
                            (uint16_t)(Q12_ONE - features->anomaly_q12);

    uint32_t expected_integral = active_recipe.mean[1] > 0 ?
                                 (uint32_t)active_recipe.mean[1] : 1u;
    uint32_t actual_integral = features->force_integral_un_bins > 0 ?
                               (uint32_t)features->force_integral_un_bins : 0u;
    if (actual_integral < (expected_integral * 3u) / 4u) {
        if (bobbin_trend < 100u) ++bobbin_trend;
    } else if (bobbin_trend > 0u) {
        --bobbin_trend;
    }

    uint32_t expected_hysteresis = active_recipe.mean[3] > 0 ?
                                   (uint32_t)active_recipe.mean[3] : 1u;
    if (features->force_hysteresis > (expected_hysteresis * 3u) / 2u) {
        if (nest_trend < 100u) ++nest_trend;
    } else if (nest_trend > 0u) {
        --nest_trend;
    }

    stitch_class_t raw = raw_class(features);
    if (raw == CLASS_OK) {
        pending_class = CLASS_OK;
        pending_count = 0u;
        features->classification = CLASS_OK;
        return CLASS_OK;
    }
    if (raw != pending_class) {
        pending_class = raw;
        pending_count = 1u;
    } else if (pending_count < UINT8_MAX) {
        ++pending_count;
    }
    if (pending_count >= persistence_required(raw)) {
        features->classification = raw;
        return raw;
    }
    features->classification = CLASS_OK;
    return CLASS_OK;
}

void classifier_learn(recipe_t *recipe, const stitch_features_t *features) {
    if (recipe == NULL || features == NULL || recipe->trained) {
        return;
    }
    ++recipe->sample_count;
    for (size_t i = 0u; i < FEATURE_COUNT; ++i) {
        int32_t value = feature_value(features, i);
        int64_t delta = (int64_t)value - recipe->mean[i];
        recipe->mean[i] += (int32_t)(delta / (int64_t)recipe->sample_count);
        uint32_t absolute = delta < 0 ? (uint32_t)-delta : (uint32_t)delta;
        int64_t deviation_delta = (int64_t)absolute - recipe->deviation[i];
        recipe->deviation[i] += (int32_t)(deviation_delta / (int64_t)recipe->sample_count);
        if (recipe->deviation[i] == 0u) recipe->deviation[i] = 1u;
    }
    if (recipe->sample_count >= 30u) {
        recipe->trained = true;
        if (recipe->sensitivity_q12 == 0u) recipe->sensitivity_q12 = Q12_ONE;
    }
}

const char *classifier_name(stitch_class_t value) {
    static const char *const names[] = {
        "OK", "SKIP_SUSPECT", "TOP_THREAD_BREAK", "BOBBIN_DEPLETION",
        "FEED_STALL", "HARD_STRIKE", "BIRDNEST_GROWTH", "UNTRAINED"
    };
    return (unsigned)value < (sizeof(names) / sizeof(names[0])) ? names[value] : "INVALID";
}
