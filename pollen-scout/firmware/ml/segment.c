/*
 * segment.c - Particle ROI segmentation for Pollen Scout
 * Adaptive thresholding + connected-component labeling on grayscale
 * converted from RGB565. Produces up to ROI_MAX bounding-box crops
 * sized between ROI_MIN_DIM and ROI_MAX_DIM pixels.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "segment.h"
#include "board.h"
#include <string.h>
#include <math.h>

/* Convert RGB565 pixel to grayscale (luma) 0..255 */
static inline uint8_t rgb565_to_gray(uint16_t px)
{
    uint8_t r = (px >> 11) & 0x1F;
    uint8_t g = (px >> 5)  & 0x3F;
    uint8_t b = (px)       & 0x1F;
    /* Y = 0.299 R + 0.587 G + 0.114 B, scaled to 8-bit */
    return (uint8_t)((r * 4898 + g * 9617 + b * 1868) >> 13);
}

/* Global (per-frame) mean & std */
static void frame_stats(const uint8_t *gray, int n, float *mean, float *std)
{
    int32_t sum = 0;
    for (int i = 0; i < n; i++) sum += gray[i];
    *mean = (float)sum / n;
    int64_t sq = 0;
    for (int i = 0; i < n; i++) {
        int32_t d = gray[i] - (int32_t)(*mean);
        sq += (int64_t)d * d;
    }
    *std = sqrtf((float)sq / n);
}

/* Adaptive threshold using local mean via box blur (stride 16) */
static void threshold_adaptive(const uint8_t *gray, uint8_t *bin,
                               int w, int h, float global_mean, float global_std)
{
    int T = (int)(global_mean - 0.5f * global_std);
    if (T < 5)   T = 5;
    if (T > 240) T = 240;
    for (int i = 0; i < w * h; i++) {
        bin[i] = (gray[i] < T) ? 1 : 0;   /* dark particles on bright bg */
    }
}

/* Connected components via two-pass labeling with union-find.
 * Returns number of components and fills bboxes[]. */
typedef struct { int16_t x0, y0, x1, y1, count; } bbox_t;

static int cc_label(const uint8_t *bin, int w, int h,
                    bbox_t *bboxes, int max_bb)
{
    static int16_t labels[IMG_WIDTH * IMG_HEIGHT];
    static int16_t parent[ROI_MAX + 1];
    int n_labels = 0;

    for (int i = 0; i < (int)(sizeof(parent)/sizeof(parent[0])); i++)
        parent[i] = (int16_t)i;

    /* find with path compression */
    #define FIND(x) ({ int _x=(x); while(parent[_x]!=_x){parent[_x]=parent[parent[_x]];_x=parent[_x];} _x; })
    #define UNION(a,b) ({ int _a=FIND(a),_b=FIND(b); if(_a!=_b) parent[_a]=_b; })

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (!bin[idx]) { labels[idx] = 0; continue; }
            int16_t left  = (x > 0)        ? labels[idx - 1]    : 0;
            int16_t up    = (y > 0)        ? labels[idx - w]    : 0;
            int16_t ul    = (x > 0 && y > 0) ? labels[idx - w - 1] : 0;
            int16_t ur    = (x < w-1 && y > 0) ? labels[idx - w + 1] : 0;

            int16_t m = 0;
            if (left) m = left;
            if (up && (!m || up < m)) m = up;
            if (ul && (!m || ul < m)) m = ul;
            if (ur && (!m || ur < m)) m = ur;

            if (!m) {
                if (n_labels >= ROI_MAX) { labels[idx] = 0; continue; }
                n_labels++;
                labels[idx] = (int16_t)n_labels;
            } else {
                labels[idx] = m;
                if (left && left != m) UNION(left, m);
                if (up   && up   != m) UNION(up,   m);
                if (ul   && ul   != m) UNION(ul,   m);
                if (ur   && ur   != m) UNION(ur,   m);
            }
        }
    }

    /* Resolve labels and accumulate bounding boxes */
    int real = 0;
    for (int i = 1; i <= n_labels; i++) {
        int root = FIND(i);
        if (root == i && real < max_bb) {
            bboxes[real].x0 = w; bboxes[real].y0 = h;
            bboxes[real].x1 = 0;  bboxes[real].y1 = 0;
            bboxes[real].count = 0;
            real++;
        }
    }
    /* Build a mapping from root label -> bbox index */
    static int16_t root_map[ROI_MAX + 1];
    int bbox_idx = 0;
    for (int i = 1; i <= n_labels; i++) {
        int r = FIND(i);
        if (r == i) { root_map[r] = bbox_idx++; }
        else root_map[r] = root_map[FIND(r)];
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (!labels[idx]) continue;
            int r = FIND(labels[idx]);
            int b = root_map[r];
            if (b < 0 || b >= real) continue;
            if (x < bboxes[b].x0) bboxes[b].x0 = x;
            if (x > bboxes[b].x1) bboxes[b].x1 = x;
            if (y < bboxes[b].y0) bboxes[b].y0 = y;
            if (y > bboxes[b].y1) bboxes[b].y1 = y;
            bboxes[b].count++;
        }
    }
    #undef FIND
    #undef UNION
    return real;
}

static int frame_gray_mem[IMG_WIDTH * IMG_HEIGHT];

int segment_init(void)
{
    return 0;
}

int segment_frame(const uint8_t *frame565, int w, int h,
                  roi_t *out_rois, int max_rois)
{
    /* Step 1: RGB565 -> grayscale */
    uint8_t *gray = (uint8_t *)frame_gray_mem;  /* reuse int buffer as bytes */
    int n = w * h;
    const uint16_t *px = (const uint16_t *)frame565;
    for (int i = 0; i < n; i++) gray[i] = rgb565_to_gray(px[i]);

    /* Step 2: stats + threshold */
    float mean, std;
    frame_stats(gray, n, &mean, &std);
    static uint8_t bin[IMG_WIDTH * IMG_HEIGHT];
    threshold_adaptive(gray, bin, w, h, mean, std);

    /* Step 3: connected components */
    bbox_t bboxes[ROI_MAX];
    int nbb = cc_label(bin, w, h, bboxes, ROI_MAX);

    /* Step 4: filter by size, crop ROI, resize to ROI_MAX_DIM */
    int out_count = 0;
    for (int b = 0; b < nbb && out_count < max_rois; b++) {
        int bw = bboxes[b].x1 - bboxes[b].x0 + 1;
        int bh = bboxes[b].y1 - bboxes[b].y0 + 1;
        if (bw < ROI_MIN_DIM || bh < ROI_MIN_DIM) continue;
        if (bw > ROI_MAX_DIM || bh > ROI_MAX_DIM) continue;
        if (bboxes[b].count < 20) continue;   /* reject noise */

        /* Crop grayscale into ROI (nearest-neighbor resize to ROI_MAX_DIM) */
        roi_t *r = &out_rois[out_count];
        r->cx = (uint16_t)((bboxes[b].x0 + bboxes[b].x1) / 2);
        r->cy = (uint16_t)((bboxes[b].y0 + bboxes[b].y1) / 2);
        int rw = bw, rh = bh;
        /* Simple: copy into top-left, pad with 0 */
        memset(r->pixels, 0, sizeof(r->pixels));
        for (int yy = 0; yy < rh && yy < ROI_MAX_DIM; yy++) {
            for (int xx = 0; xx < rw && xx < ROI_MAX_DIM; xx++) {
                r->pixels[yy * ROI_MAX_DIM + xx] =
                    gray[(bboxes[b].y0 + yy) * w + (bboxes[b].x0 + xx)];
            }
        }
        r->w = (uint8_t)rw;
        r->h = (uint8_t)rh;
        out_count++;
    }
    return out_count;
}