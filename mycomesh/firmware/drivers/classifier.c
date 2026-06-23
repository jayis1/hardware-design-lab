/*
 * classifier.c — Activity state classifier for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Classifies 10-second epochs of fungal electrophysiology + environmental
 * data into one of five activity states using nearest-prototype matching
 * on a feature vector.  The feature vector is:
 *
 *   { spike_rate, propagation_count, mean_amplitude_uv,
 *     isi_cv_x100, soil_moisture, soil_temp_cx10, co2_ppm }
 *
 * Each feature is normalized to [0, 1] using expected ranges, then
 * Euclidean distance to five prototype centroids is computed.  The
 * nearest prototype determines the class label.  Prototypes are based
 * on published fungal electrophysiology literature and empirical data.
 */

#include "classifier.h"
#include <string.h>

/* ===================================================================== */
/*  Feature normalization ranges                                          */
/* ===================================================================== */

/* Expected ranges for each feature (min, max) used for normalization. */
#define SPIKE_RATE_MAX        50      /* spikes/sec (all channels)        */
#define PROP_COUNT_MAX        30      /* propagation events per epoch     */
#define AMPLITUDE_MAX_UV      5000    /* µV                                */
#define ISI_CV_MAX            200     /* CV × 100 (0–2.00)                */
#define MOISTURE_MIN_X10      50      /* 5% VWC                            */
#define MOISTURE_MAX_X10      800     /* 80% VWC                           */
#define TEMP_MIN_CX10         -100    /* -10°C                             */
#define TEMP_MAX_CX10         600     /* 60°C                              */
#define CO2_MIN_PPM           400     /* atmospheric baseline              */
#define CO2_MAX_PPM           5000    /* high activity / enclosed          */

/* ===================================================================== */
/*  Prototype centroids (normalized features)                            */
/* ===================================================================== */

typedef struct {
    float features[7];  /* 7 normalized features per prototype */
    const char *label;
} prototype_t;

static prototype_t g_prototypes[CLASS_COUNT];

/* Feature indices */
enum {
    FIDX_SPIKE_RATE = 0,
    FIDX_PROP_COUNT,
    FIDX_AMPLITUDE,
    FIDX_ISI_CV,
    FIDX_MOISTURE,
    FIDX_TEMP,
    FIDX_CO2,
    FIDX_COUNT
};

/* ===================================================================== */
/*  Normalization                                                         */
/* ===================================================================== */

static float norm_range(float val, float min, float max)
{
    if (max <= min) return 0.0f;
    float n = (val - min) / (max - min);
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n;
}

static void extract_features(const epoch_summary_t *epoch, float *features)
{
    features[FIDX_SPIKE_RATE] = norm_range((float)epoch->spike_rate,
                                            0, SPIKE_RATE_MAX);
    features[FIDX_PROP_COUNT] = norm_range((float)epoch->propagation_count,
                                            0, PROP_COUNT_MAX);
    features[FIDX_AMPLITUDE] = norm_range((float)epoch->mean_amplitude_uv,
                                           0, AMPLITUDE_MAX_UV);
    features[FIDX_ISI_CV] = norm_range((float)epoch->isi_cv_x100,
                                        0, ISI_CV_MAX);
    features[FIDX_MOISTURE] = norm_range((float)epoch->soil_moisture,
                                          MOISTURE_MIN_X10, MOISTURE_MAX_X10);
    features[FIDX_TEMP] = norm_range((float)epoch->soil_temp_cx10,
                                      TEMP_MIN_CX10, TEMP_MAX_CX10);
    features[FIDX_CO2] = norm_range((float)epoch->co2_ppm,
                                     CO2_MIN_PPM, CO2_MAX_PPM);
}

/* ===================================================================== */
/*  Initialization                                                        */
/* ===================================================================== */

void classifier_init(void)
{
    /* IDLE: low spike rate, low propagation, low amplitude, normal env.
       Features: [rate, prop, amp, isi_cv, moisture, temp, co2] */
    g_prototypes[CLASS_IDLE].features[0] = 0.02f;  /* very low spike rate */
    g_prototypes[CLASS_IDLE].features[1] = 0.00f;  /* no propagation */
    g_prototypes[CLASS_IDLE].features[2] = 0.05f;  /* low amplitude */
    g_prototypes[CLASS_IDLE].features[3] = 0.50f;  /* moderate ISI CV */
    g_prototypes[CLASS_IDLE].features[4] = 0.50f;  /* normal moisture */
    g_prototypes[CLASS_IDLE].features[5] = 0.40f;  /* normal temp */
    g_prototypes[CLASS_IDLE].features[6] = 0.02f;  /* baseline CO2 */
    g_prototypes[CLASS_IDLE].label = "IDLE";

    /* FORAGE: regular spikes (1-5 Hz), low propagation, moderate amplitude */
    g_prototypes[CLASS_FORAGE].features[0] = 0.15f;  /* low-moderate rate */
    g_prototypes[CLASS_FORAGE].features[1] = 0.05f;  /* low propagation */
    g_prototypes[CLASS_FORAGE].features[2] = 0.20f;  /* moderate amplitude */
    g_prototypes[CLASS_FORAGE].features[3] = 0.30f;  /* regular (low CV) */
    g_prototypes[CLASS_FORAGE].features[4] = 0.60f;  /* good moisture */
    g_prototypes[CLASS_FORAGE].features[5] = 0.45f;  /* good temp */
    g_prototypes[CLASS_FORAGE].features[6] = 0.10f;  /* slightly elevated CO2 */
    g_prototypes[CLASS_FORAGE].label = "FORAGE";

    /* TRANSPORT: propagating waves across multiple channels */
    g_prototypes[CLASS_TRANSPORT].features[0] = 0.25f;  /* moderate rate */
    g_prototypes[CLASS_TRANSPORT].features[1] = 0.60f;  /* high propagation */
    g_prototypes[CLASS_TRANSPORT].features[2] = 0.35f;  /* moderate-high amp */
    g_prototypes[CLASS_TRANSPORT].features[3] = 0.25f;  /* regular waves */
    g_prototypes[CLASS_TRANSPORT].features[4] = 0.65f;  /* good moisture */
    g_prototypes[CLASS_TRANSPORT].features[5] = 0.50f;  /* optimal temp */
    g_prototypes[CLASS_TRANSPORT].features[6] = 0.15f;  /* moderate CO2 */
    g_prototypes[CLASS_TRANSPORT].label = "TRANSPORT";

    /* STRESS: high-rate bursting, elevated CO2, temperature excursion */
    g_prototypes[CLASS_STRESS].features[0] = 0.70f;  /* high rate */
    g_prototypes[CLASS_STRESS].features[1] = 0.20f;  /* some propagation */
    g_prototypes[CLASS_STRESS].features[2] = 0.60f;  /* high amplitude */
    g_prototypes[CLASS_STRESS].features[3] = 0.80f;  /* irregular (high CV) */
    g_prototypes[CLASS_STRESS].features[4] = 0.20f;  /* low moisture (drought) */
    g_prototypes[CLASS_STRESS].features[5] = 0.75f;  /* high temp */
    g_prototypes[CLASS_STRESS].features[6] = 0.60f;  /* high CO2 */
    g_prototypes[CLASS_STRESS].label = "STRESS";

    /* COMPETE: irregular high-amplitude, drying substrate */
    g_prototypes[CLASS_COMPETE].features[0] = 0.45f;  /* moderate-high rate */
    g_prototypes[CLASS_COMPETE].features[1] = 0.15f;  /* low propagation */
    g_prototypes[CLASS_COMPETE].features[2] = 0.80f;  /* very high amplitude */
    g_prototypes[CLASS_COMPETE].features[3] = 0.90f;  /* very irregular */
    g_prototypes[CLASS_COMPETE].features[4] = 0.15f;  /* dry substrate */
    g_prototypes[CLASS_COMPETE].features[5] = 0.55f;  /* moderate temp */
    g_prototypes[CLASS_COMPETE].features[6] = 0.40f;  /* elevated CO2 */
    g_prototypes[CLASS_COMPETE].label = "COMPETE";
}

/* ===================================================================== */
/*  Classification                                                        */
/* ===================================================================== */

uint8_t classifier_classify(const epoch_summary_t *epoch)
{
    float features[FIDX_COUNT];
    extract_features(epoch, features);

    /* Weight vector — emphasizes electrophysiology features over env. */
    static const float weights[FIDX_COUNT] = {
        2.0f,  /* spike_rate — most important */
        2.0f,  /* propagation_count — key for transport detection */
        1.5f,  /* amplitude — distinguishes stress/compete */
        1.5f,  /* isi_cv — distinguishes regular vs irregular */
        0.5f,  /* moisture — environmental context */
        0.5f,  /* temp — environmental context */
        1.0f,  /* co2 — stress indicator */
    };

    uint8_t best_class = CLASS_IDLE;
    float best_dist = 1e30f;

    for (uint8_t c = 0; c < CLASS_COUNT; c++) {
        float dist = 0.0f;
        for (uint8_t f = 0; f < FIDX_COUNT; f++) {
            float d = features[f] - g_prototypes[c].features[f];
            dist += weights[f] * d * d;
        }

        if (dist < best_dist) {
            best_dist = dist;
            best_class = c;
        }
    }

    return best_class;
}

/* ===================================================================== */
/*  Prototype update (adaptive learning)                                  */
/* ===================================================================== */

void classifier_update_prototype(uint8_t class_label,
                                 const epoch_summary_t *epoch)
{
    if (class_label >= CLASS_COUNT) return;

    float features[FIDX_COUNT];
    extract_features(epoch, features);

    /* Exponential moving average update with alpha = 0.05. */
    float alpha = 0.05f;
    for (uint8_t f = 0; f < FIDX_COUNT; f++) {
        g_prototypes[class_label].features[f] =
            (1.0f - alpha) * g_prototypes[class_label].features[f]
            + alpha * features[f];
    }
}

/* ===================================================================== */
/*  Label string                                                          */
/* ===================================================================== */

const char *classifier_label(uint8_t class_label)
{
    if (class_label >= CLASS_COUNT) return "UNKNOWN";
    return g_prototypes[class_label].label;
}