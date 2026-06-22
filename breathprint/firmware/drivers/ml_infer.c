/*
 * ml_infer.c — Quantized random forest inference engine
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Implements a quantized (int8) random forest classifier entirely on the
 * STM32H733. The model is trained offline on a desktop (Python/scikit-learn),
 * exported to a flat binary format, and stored in the W25Q64 external flash.
 * At boot, the model is loaded into SRAM for fast inference.
 *
 * Model architecture:
 *   - 64 decision trees
 *   - Max depth: 8
 *   - 40 input features (5 features × 8 sensor channels)
 *   - 6 output classes (metabolic states)
 *   - int8 quantized thresholds
 *   - Total model size: ~120 KB
 *
 * Inference time: ~15 ms on Cortex-M7 @ 312 MHz
 */

#include "ml_infer.h"
#include "flash_drv.h"
#include "../board.h"

/* ========================================================================
 * Model Storage
 * ========================================================================
 *
 * The model is stored in external flash (W25Q64) at offset 0x000000.
 * Format:
 *   [0x00-0x03] Magic: 'BPM1'
 *   [0x04-0x07] Num trees (uint32)
 *   [0x08-0x0B] Max depth (uint32)
 *   [0x0C-0x0F] Num features (uint32)
 *   [0x10-0x13] Num classes (uint32)
 *   [0x14+]     Tree data (array of tree_t)
 *
 * Each tree node:
 *   struct node_t {
 *       int8_t  threshold;    // Split threshold (quantized)
 *       uint8_t feature_idx;  // Feature index to split on
 *       uint16_t left;        // Left child index (0xFFFF = leaf)
 *       uint16_t right;       // Right child index
 *       uint8_t  leaf_class;  // Class if leaf (0xFF = not leaf)
 *       uint8_t  padding;
 *   };  // 8 bytes per node
 *
 * Tree header:
 *   struct tree_header_t {
 *       uint16_t num_nodes;
 *       uint16_t root_idx;
 *   };  // 4 bytes
 *
 * Max nodes per tree = 2^8 - 1 = 255 → 255 × 8 = 2040 bytes per tree
 * 64 trees × 2044 bytes = ~130 KB total
 */

#define MODEL_MAGIC        0x31504D42  /* 'BPM1' */
#define MODEL_FLASH_ADDR   0x000000

#define MAX_TREES          64
#define MAX_NODES_PER_TREE 255
#define MAX_DEPTH          8

/* Node structure (8 bytes) */
typedef struct {
    int8_t   threshold;
    uint8_t  feature_idx;
    uint16_t left;
    uint16_t right;
    uint8_t  leaf_class;   /* 0xFF = not a leaf */
    uint8_t  padding;
} __attribute__((packed)) tree_node_t;

/* Tree structure */
typedef struct {
    uint16_t num_nodes;
    uint16_t root_idx;
    tree_node_t nodes[MAX_NODES_PER_TREE];
} tree_t;

/* Model header */
typedef struct {
    uint32_t magic;
    uint32_t num_trees;
    uint32_t max_depth;
    uint32_t num_features;
    uint32_t num_classes;
} model_header_t;

/* ========================================================================
 * Model in SRAM
 * ========================================================================
 */

static model_header_t model_header;
static tree_t trees[MAX_TREES];
static uint8_t model_loaded = 0;

/* Class vote counts */
static uint8_t class_votes[NUM_CLASSES];

/* Class names (for display) */
static const char *class_names[NUM_CLASSES] = {
    "Baseline",
    "Ketotic",
    "Post-Meal",
    "Post-Exercise",
    "Gut Ferment",
    "Unknown"
};

/* ========================================================================
 * Feature quantization
 * The model was trained on float features, then quantized to int8.
 * We need to apply the same quantization at inference time.
 * Quantization: q = clamp(feature / scale + zero_point, -128, 127)
 * ======================================================================== */

/* Quantization parameters (loaded from flash along with model) */
static float quant_scale[NUM_FEATURES];
static int8_t quant_zero_point[NUM_FEATURES];

static int8_t quantize_feature(float value, int feature_idx) {
    if (feature_idx < 0 || feature_idx >= NUM_FEATURES) return 0;

    float q = value / quant_scale[feature_idx] + (float)quant_zero_point[feature_idx];
    if (q > 127.0f) q = 127.0f;
    if (q < -128.0f) q = -128.0f;
    return (int8_t)q;
}

/* ========================================================================
 * Load model from external flash into SRAM
 * ======================================================================== */

int ml_model_load(void) {
    /* Read model header from flash */
    uint8_t header_buf[sizeof(model_header_t)];
    if (flash_read(MODEL_FLASH_ADDR, header_buf, sizeof(model_header_t)) < 0) {
        return -1;
    }

    memcpy(&model_header, header_buf, sizeof(model_header_t));

    /* Verify magic */
    if (model_header.magic != MODEL_MAGIC) {
        /* No model in flash — use default/builtin model */
        ml_load_builtin_model();
        return 0;
    }

    /* Validate dimensions */
    if (model_header.num_trees > MAX_TREES ||
        model_header.num_features != NUM_FEATURES ||
        model_header.num_classes != NUM_CLASSES) {
        return -1;
    }

    /* Read quantization parameters (stored after header) */
    uint32_t offset = sizeof(model_header_t);
    uint8_t quant_buf[NUM_FEATURES * sizeof(float)];
    if (flash_read(offset, quant_buf, NUM_FEATURES * sizeof(float)) < 0) {
        return -1;
    }
    memcpy(quant_scale, quant_buf, NUM_FEATURES * sizeof(float));
    offset += NUM_FEATURES * sizeof(float);

    if (flash_read(offset, (uint8_t *)quant_zero_point, NUM_FEATURES) < 0) {
        return -1;
    }
    offset += NUM_FEATURES;

    /* Read each tree */
    for (uint32_t t = 0; t < model_header.num_trees; t++) {
        /* Read tree header */
        uint8_t tree_hdr[4];
        if (flash_read(offset, tree_hdr, 4) < 0) return -1;

        memcpy(&trees[t].num_nodes, &tree_hdr[0], 2);
        memcpy(&trees[t].root_idx, &tree_hdr[2], 2);
        offset += 4;

        if (trees[t].num_nodes > MAX_NODES_PER_TREE) return -1;

        /* Read nodes */
        uint32_t nodes_size = trees[t].num_nodes * sizeof(tree_node_t);
        if (flash_read(offset, (uint8_t *)trees[t].nodes, nodes_size) < 0) {
            return -1;
        }
        offset += nodes_size;
    }

    model_loaded = 1;
    return 0;
}

/* ========================================================================
 * Load a small builtin model (fallback if flash has no model)
 * This is a minimal 4-tree model with depth 3 for basic functionality.
 * ========================================================================
 */

void ml_load_builtin_model(void) {
    model_header.magic = MODEL_MAGIC;
    model_header.num_trees = 4;
    model_header.max_depth = 3;
    model_header.num_features = NUM_FEATURES;
    model_header.num_classes = NUM_CLASSES;

    /* Set default quantization (scale=1, zero_point=0) */
    for (int i = 0; i < NUM_FEATURES; i++) {
        quant_scale[i] = 1.0f;
        quant_zero_point[i] = 0;
    }

    /* Build simple trees: split on acetone peak (feature 0) and H2 (feature 35) */
    for (int t = 0; t < 4; t++) {
        trees[t].num_nodes = 7;  /* Full binary tree of depth 3 */
        trees[t].root_idx = 0;

        /* Tree structure: splits on acetone (feat 0) and H2 auc (feat 35) */
        /* Root: split on acetone peak */
        trees[t].nodes[0].threshold = 50;
        trees[t].nodes[0].feature_idx = 0;
        trees[t].nodes[0].left = 1;
        trees[t].nodes[0].right = 2;
        trees[t].nodes[0].leaf_class = 0xFF;

        /* Left subtree: low acetone → baseline or post-meal */
        trees[t].nodes[1].threshold = 20;
        trees[t].nodes[1].feature_idx = 30;  /* H2 peak */
        trees[t].nodes[1].left = 3;
        trees[t].nodes[1].right = 4;
        trees[t].nodes[1].leaf_class = 0xFF;

        /* Right subtree: high acetone → ketotic */
        trees[t].nodes[2].threshold = 100;
        trees[t].nodes[2].feature_idx = 0;
        trees[t].nodes[2].left = 5;
        trees[t].nodes[2].right = 6;
        trees[t].nodes[2].leaf_class = 0xFF;

        /* Leaves */
        trees[t].nodes[3].leaf_class = STATE_BASELINE;
        trees[t].nodes[4].leaf_class = (t < 2) ? STATE_POSTMEAL : STATE_GUT_FERMENT;
        trees[t].nodes[5].leaf_class = STATE_KETOTIC;
        trees[t].nodes[6].leaf_class = (t < 2) ? STATE_KETOTIC : STATE_POSTEXERCISE;
    }

    model_loaded = 1;
}

/* ========================================================================
 * Predict class for a single tree
 * ========================================================================
 */

static uint8_t tree_predict(const tree_t *tree, const int8_t *features) {
    uint16_t node_idx = tree->root_idx;

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        if (node_idx >= tree->num_nodes) {
            return STATE_UNKNOWN;
        }

        const tree_node_t *node = &tree->nodes[node_idx];

        /* Check if leaf */
        if (node->leaf_class != 0xFF) {
            return node->leaf_class;
        }

        /* Traverse tree: go left if feature < threshold, else right */
        int8_t feature_val = features[node->feature_idx];
        if (feature_val <= node->threshold) {
            node_idx = node->left;
        } else {
            node_idx = node->right;
        }
    }

    return STATE_UNKNOWN;
}

/* ========================================================================
 * Run full inference: quantize features → predict all trees → majority vote
 * ========================================================================
 */

uint8_t ml_infer(const feature_vector_t *features) {
    if (!model_loaded) {
        return STATE_UNKNOWN;
    }

    /* Flatten feature vector and quantize to int8 */
    int8_t quant_features[NUM_FEATURES];
    int idx = 0;
    for (int s = 0; s < NUM_SENSORS; s++) {
        quant_features[idx++] = quantize_feature(features->sensors[s].peak, idx);
        quant_features[idx++] = quantize_feature(features->sensors[s].rise_time, idx);
        quant_features[idx++] = quantize_feature(features->sensors[s].plateau, idx);
        quant_features[idx++] = quantize_feature(features->sensors[s].decay_tau, idx);
        quant_features[idx++] = quantize_feature(features->sensors[s].auc, idx);
    }

    /* Reset vote counts */
    memset(class_votes, 0, sizeof(class_votes));

    /* Run all trees and collect votes */
    for (uint32_t t = 0; t < model_header.num_trees; t++) {
        uint8_t prediction = tree_predict(&trees[t], quant_features);
        if (prediction < NUM_CLASSES) {
            class_votes[prediction]++;
        }
    }

    /* Find majority class */
    uint8_t best_class = STATE_UNKNOWN;
    uint8_t best_votes = 0;
    for (int c = 0; c < NUM_CLASSES; c++) {
        if (class_votes[c] > best_votes) {
            best_votes = class_votes[c];
            best_class = (uint8_t)c;
        }
    }

    return best_class;
}

/* ========================================================================
 * Get class name string
 * ========================================================================
 */

const char *ml_class_name(uint8_t class_id) {
    if (class_id < NUM_CLASSES) return class_names[class_id];
    return "Unknown";
}

/* ========================================================================
 * Get confidence score (fraction of trees voting for winning class)
 * ========================================================================
 */

float ml_confidence(void) {
    if (model_header.num_trees == 0) return 0.0f;

    uint8_t max_votes = 0;
    for (int c = 0; c < NUM_CLASSES; c++) {
        if (class_votes[c] > max_votes) max_votes = class_votes[c];
    }

    return (float)max_votes / (float)model_header.num_trees;
}

/* ========================================================================
 * Update model from BLE (OTA model update)
 * Writes new model data to external flash
 * ========================================================================
 */

int ml_model_update(const uint8_t *data, uint32_t offset, uint32_t len) {
    /* Erase flash sector if writing at start */
    if (offset == 0) {
        /* Erase first 256 KB (4 sectors of 64 KB) */
        for (int i = 0; i < 4; i++) {
            if (flash_erase_sector(i * 0x10000) < 0) return -1;
        }
    }

    /* Write data to flash */
    return flash_write(MODEL_FLASH_ADDR + offset, data, len);
}

int ml_model_finalize(void) {
    /* Verify the new model by reading and checking magic */
    uint8_t header_buf[sizeof(model_header_t)];
    if (flash_read(MODEL_FLASH_ADDR, header_buf, sizeof(model_header_t)) < 0) {
        return -1;
    }

    model_header_t new_header;
    memcpy(&new_header, header_buf, sizeof(model_header_t));

    if (new_header.magic != MODEL_MAGIC) {
        return -1;
    }

    /* Reload model into SRAM */
    return ml_model_load();
}