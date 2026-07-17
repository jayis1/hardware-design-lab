/*
 * forest_weights.h — generated int8 decision-forest table interface.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_FOREST_WEIGHTS_H
#define STRIDOPHONE_FOREST_WEIGHTS_H

#include "../board.h"

typedef struct {
    uint8_t feat_idx;    /* 0xFF => leaf node */
    int8_t  threshold;   /* quantised threshold */
    uint8_t leaf_label;  /* class id when feat_idx == 0xFF */
} int8_node_t;

extern const int8_node_t *forest_trees[CLF_N_TREES];
extern const int          forest_tree_sizes[CLF_N_TREES];

#endif