/*
 * segment.h - Particle ROI segmentation header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef SEGMENT_H
#define SEGMENT_H
#include <stdint.h>
#include "board.h"   /* roi_t, ROI_MAX */

int segment_init(void);
int segment_frame(const uint8_t *frame565, int w, int h,
                  roi_t *out_rois, int max_rois);

#endif /* SEGMENT_H */