/*
 * glyph_engine.h — Glyph rendering engine API
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef GLYPH_ENGINE_H
#define GLYPH_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

/* Thermal frame buffer: 12×8 cells, each cell is a target temperature
 * in fixed-point °C × 100 (e.g., 3800 = 38.00 °C).
 * A value of 0 means "neutral" (track ambient, no active heating/cooling). */
typedef struct {
    int16_t target[THERMAL_ROWS][THERMAL_COLS];  /* °C × 100 */
    uint8_t active;  /* 1 = frame has active cells, 0 = all neutral */
} thermal_frame_t;

/* Glyph command (from BLE or LoRa) */
typedef struct {
    uint8_t  cmd;        /* GLYPH_CMD_* */
    uint8_t  direction;  /* GLYPH_DIR_* (for arrows) */
    uint8_t  shape;      /* GLYPH_SHAPE_* (for shapes) */
    uint8_t  polarity;   /* GLYPH_WARM, GLYPH_COOL, GLYPH_NEUTRAL */
    uint8_t  intensity;  /* 0–255, maps to temperature delta from ambient */
    uint8_t  duration_ms_lo;  /* duration in ms (16-bit, split) */
    uint8_t  duration_ms_hi;
    uint8_t  text_len;   /* for GLYPH_CMD_TEXT */
    char     text[16];   /* null-terminated, max 15 chars */
    uint8_t  scroll;     /* 1 = scroll text across array */
} glyph_cmd_t;

/* Initialize the glyph engine */
void glyph_engine_init(void);

/* Queue a glyph command (thread-safe, called from comm_task) */
bool glyph_engine_queue(const glyph_cmd_t *cmd);

/* Render the next frame (called by glyph_task at 50 Hz).
 * Returns the frame to be passed to the thermal PID loop. */
void glyph_engine_render(thermal_frame_t *frame);

/* Clear all queued glyphs and reset to neutral */
void glyph_engine_clear(void);

/* Get the current active glyph command (for telemetry) */
const glyph_cmd_t *glyph_engine_current(void);

/* Set ambient temperature (for computing warm/cool deltas) */
void glyph_engine_set_ambient(int16_t ambient_c100);

/* Set intensity scaling (0–255, from app config; maps to temp delta) */
void glyph_engine_set_intensity_scale(uint8_t scale);

#endif /* GLYPH_ENGINE_H */