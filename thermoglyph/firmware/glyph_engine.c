/*
 * glyph_engine.c — Thermal glyph rendering engine
 *
 * Renders abstract glyph commands into a 12×8 thermal frame buffer.
 * Supports: pixels, arrows, text (5×3 font), shapes, animations, bars.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#include "glyph_engine.h"
#include "board.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* 5×3 thermal font (A–Z, 0–9, space, arrow, check)
 * Each glyph is 5 rows × 3 columns of bits.
 * Bit order: row 0 (top) → row 4 (bottom), col 0 (left) → col 2 (right).
 * 1 = active pixel, 0 = off.
 * Total: 38 characters (A-Z=26, 0-9=10, space, check, arrow)
 */
/* ------------------------------------------------------------------------- */
static const uint8_t font5x3[38][5] = {
    /* A */ {0b111, 0b101, 0b111, 0b101, 0b101},
    /* B */ {0b110, 0b101, 0b110, 0b101, 0b110},
    /* C */ {0b111, 0b100, 0b100, 0b100, 0b111},
    /* D */ {0b110, 0b101, 0b101, 0b101, 0b110},
    /* E */ {0b111, 0b100, 0b110, 0b100, 0b111},
    /* F */ {0b111, 0b100, 0b110, 0b100, 0b100},
    /* G */ {0b111, 0b100, 0b101, 0b101, 0b111},
    /* H */ {0b101, 0b101, 0b111, 0b101, 0b101},
    /* I */ {0b111, 0b010, 0b010, 0b010, 0b111},
    /* J */ {0b001, 0b001, 0b001, 0b101, 0b111},
    /* K */ {0b101, 0b110, 0b100, 0b110, 0b101},
    /* L */ {0b100, 0b100, 0b100, 0b100, 0b111},
    /* M */ {0b101, 0b111, 0b111, 0b101, 0b101},
    /* N */ {0b101, 0b111, 0b111, 0b111, 0b101},
    /* O */ {0b111, 0b101, 0b101, 0b101, 0b111},
    /* P */ {0b111, 0b101, 0b111, 0b100, 0b100},
    /* Q */ {0b111, 0b101, 0b101, 0b111, 0b011},
    /* R */ {0b111, 0b101, 0b110, 0b101, 0b101},
    /* S */ {0b011, 0b100, 0b010, 0b001, 0b110},
    /* T */ {0b111, 0b010, 0b010, 0b010, 0b010},
    /* U */ {0b101, 0b101, 0b101, 0b101, 0b111},
    /* V */ {0b101, 0b101, 0b101, 0b101, 0b010},
    /* W */ {0b101, 0b101, 0b111, 0b111, 0b101},
    /* X */ {0b101, 0b101, 0b010, 0b101, 0b101},
    /* Y */ {0b101, 0b101, 0b010, 0b010, 0b010},
    /* Z */ {0b111, 0b001, 0b010, 0b100, 0b111},
    /* 0 */ {0b111, 0b101, 0b101, 0b101, 0b111},
    /* 1 */ {0b010, 0b110, 0b010, 0b010, 0b111},
    /* 2 */ {0b111, 0b001, 0b111, 0b100, 0b111},
    /* 3 */ {0b111, 0b001, 0b011, 0b001, 0b111},
    /* 4 */ {0b101, 0b101, 0b111, 0b001, 0b001},
    /* 5 */ {0b111, 0b100, 0b111, 0b001, 0b111},
    /* 6 */ {0b111, 0b100, 0b111, 0b101, 0b111},
    /* 7 */ {0b111, 0b001, 0b010, 0b010, 0b010},
    /* 8 */ {0b111, 0b101, 0b111, 0b101, 0b111},
    /* 9 */ {0b111, 0b101, 0b111, 0b001, 0b111},
    /* space */ {0b000, 0b000, 0b000, 0b000, 0b000},
    /* check */ {0b000, 0b001, 0b010, 0b101, 0b010},
    /* arrow-right */ {0b000, 0b001, 0b111, 0b001, 0b000},
};

#define FONT_SPACE    36
#define FONT_CHECK    37

/* Arrow bitmaps (8 directions, 5×5 centered in 8-row grid) */
static const uint8_t arrow_n[8]  = {0x00, 0x00, 0b00100,
                                     0b01110, 0b10101,
                                     0b00100, 0b00100, 0x00};
static const uint8_t arrow_ne[8] = {0x00, 0b00011, 0b00111,
                                     0b01101, 0b11000,
                                     0b00000, 0b00000, 0x00};
static const uint8_t arrow_e[8]  = {0x00, 0b00000, 0b00100,
                                     0b00110, 0b00111,
                                     0b00110, 0b00100, 0x00};
static const uint8_t arrow_se[8] = {0x00, 0x00, 0x00,
                                     0b11000, 0b01101,
                                     0b00111, 0b00011, 0x00};
static const uint8_t arrow_s[8]  = {0x00, 0b00100, 0b00100,
                                     0b00100, 0b10101,
                                     0b01110, 0b00100, 0x00};
static const uint8_t arrow_sw[8] = {0x00, 0x00, 0x00,
                                     0b00011, 0b10110,
                                     0b11100, 0b11000, 0x00};
static const uint8_t arrow_w[8]  = {0x00, 0b00100, 0b00110,
                                     0b00111, 0b00110,
                                     0b00100, 0x00, 0x00};
static const uint8_t arrow_nw[8] = {0b11000, 0b11100, 0b10110,
                                     0b00011, 0x00,
                                     0x00, 0x00, 0x00};

static const uint8_t *arrows[8] = {
    arrow_n, arrow_ne, arrow_e, arrow_se,
    arrow_s, arrow_sw, arrow_w, arrow_nw
};

/* Shape bitmaps (8 rows × 12 cols, rendered as row bitmasks where possible) */
/* For shapes that don't fit in a byte, we use 16-bit row masks */
static const uint16_t shape_ring[8] = {
    0b000011110000, 0b001100001100, 0b010000000010, 0b010000000010,
    0b010000000010, 0b010000000010, 0b001100001100, 0b000011110000
};
static const uint16_t shape_cross[8] = {
    0b000000110000, 0b000000110000, 0b000000110000, 0b111111111111,
    0b111111111111, 0b000000110000, 0b000000110000, 0b000000110000
};
static const uint16_t shape_wave[8] = {
    0b000110000000, 0b001001100000, 0b010000010000, 0b000000001100,
    0b000000000010, 0b000000001100, 0b010000010000, 0b001001100000
};
static const uint16_t shape_check[8] = {
    0b000000000100, 0b000000001010, 0b000000010001, 0b000000100000,
    0b000001000000, 0b000010000000, 0b000100000000, 0b001000000000
};
static const uint16_t shape_x[8] = {
    0b100000000001, 0b010000000010, 0b001000000100, 0b000100001000,
    0b000100001000, 0b001000000100, 0b010000000010, 0b100000000001
};
static const uint16_t shape_circle[8] = {
    0b000011110000, 0b001100001100, 0b010000000010, 0b010000000010,
    0b010000000010, 0b010000000010, 0b001100001100, 0b000001100000
};

static const uint16_t *shapes[6] = {
    shape_circle, shape_ring, shape_cross, shape_wave, shape_check, shape_x
};

/* ------------------------------------------------------------------------- */
/* State */
/* ------------------------------------------------------------------------- */
static glyph_cmd_t cmd_queue[GLYPH_QUEUE_LEN];
static volatile uint8_t queue_head = 0;  /* producer writes here */
static volatile uint8_t queue_tail = 0;  /* consumer reads here */
static glyph_cmd_t      current_cmd;
static thermal_frame_t  current_frame;
static int16_t          ambient_c100 = 2500;  /* 25.00 °C default */
static uint8_t          intensity_scale = 128;  /* 50% default */
static uint16_t         glyph_timer_ms = 0;    /* remaining duration */
static uint16_t         anim_phase = 0;        /* animation frame counter */
static int8_t           scroll_offset = 0;     /* text scroll position */

/* ------------------------------------------------------------------------- */
/* Helpers */
/* ------------------------------------------------------------------------- */

static uint8_t queue_count(void)
{
    return (queue_head - queue_tail + GLYPH_QUEUE_LEN) % GLYPH_QUEUE_LEN;
}

static bool queue_full(void)
{
    return ((queue_head + 1) % GLYPH_QUEUE_LEN) == queue_tail;
}

/* Map polarity + intensity to a target temperature delta.
 * intensity: 0–255, scale: 0–255
 * Returns delta in °C × 100 (can be negative for cooling). */
static int16_t intensity_to_delta(uint8_t polarity, uint8_t intensity)
{
    /* Max delta: 10 °C (1000 in fixed-point) at full intensity + scale */
    int16_t scaled = (int16_t)((uint32_t)intensity *
                                (uint32_t)intensity_scale / 255U);
    int16_t delta = (int16_t)((uint32_t)scaled * 1000U / 255U);

    if (polarity == GLYPH_WARM) {
        return delta;  /* warm: above ambient */
    } else if (polarity == GLYPH_COOL) {
        return -delta; /* cool: below ambient */
    }
    return 0;  /* neutral */
}

static void clamp_target(int16_t *val)
{
    if (*val > (int16_t)TEMP_MAX_C) *val = (int16_t)TEMP_MAX_C;
    if (*val < (int16_t)TEMP_MIN_C) *val = (int16_t)TEMP_MIN_C;
}

static void frame_set_neutral(thermal_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    f->active = 0;
}

static void frame_set_cell(thermal_frame_t *f, uint8_t row, uint8_t col,
                           int16_t delta)
{
    if (row >= THERMAL_ROWS || col >= THERMAL_COLS) return;
    int16_t target = ambient_c100 + delta;
    clamp_target(&target);
    f->target[row][col] = target;
    f->active = 1;
}

static int16_t get_delta(const glyph_cmd_t *cmd)
{
    return intensity_to_delta(cmd->polarity, cmd->intensity);
}

/* ------------------------------------------------------------------------- */
/* Glyph renderers */
/* ------------------------------------------------------------------------- */

static void render_arrow(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    const uint8_t *bmp = arrows[cmd->direction % 8];
    int16_t d = get_delta(cmd);

    for (uint8_t r = 0; r < THERMAL_ROWS; r++) {
        uint8_t row_bits = bmp[r];
        for (uint8_t c = 0; c < THERMAL_COLS; c++) {
            if (row_bits & (1 << (4 - (c / 3)))) {
                frame_set_cell(f, r, c, d);
            }
        }
    }
}

static void render_shape(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    const uint16_t *bmp = shapes[cmd->shape % 6];
    int16_t d = get_delta(cmd);

    for (uint8_t r = 0; r < THERMAL_ROWS; r++) {
        uint16_t row_bits = bmp[r];
        for (uint8_t c = 0; c < THERMAL_COLS; c++) {
            if (row_bits & (1 << (THERMAL_COLS - 1 - c))) {
                frame_set_cell(f, r, c, d);
            }
        }
    }
}

/* Animated expanding ring (for SOS / alert) */
static void render_ring_anim(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    int16_t d = get_delta(cmd);
    uint8_t phase = (anim_phase / 5) % 4;  /* 4-phase expansion */

    /* Phase 0: center 2×2, 1: 4×4, 2: 6×6, 3: full 8×12 ring */
    int8_t center_r = THERMAL_ROWS / 2;
    int8_t center_c = THERMAL_COLS / 2;
    int8_t radius = phase + 1;

    for (uint8_t r = 0; r < THERMAL_ROWS; r++) {
        for (uint8_t c = 0; c < THERMAL_COLS; c++) {
            int8_t dr = (int8_t)r - center_r;
            int8_t dc = (int8_t)c - center_c;
            int16_t dist = dr * dr + dc * dc;
            int16_t ring_inner = (radius - 1) * (radius - 1);
            int16_t ring_outer = radius * radius;

            if (dist >= ring_inner && dist <= ring_outer) {
                frame_set_cell(f, r, c, d);
            }
        }
    }
}

/* Warm wave sweeping left-to-right (for "proceed forward") */
static void render_wave_anim(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    int16_t d_warm = intensity_to_delta(GLYPH_WARM, cmd->intensity);
    int16_t d_cool = intensity_to_delta(GLYPH_COOL, cmd->intensity);

    uint8_t phase = (anim_phase / 3) % THERMAL_COLS;

    for (uint8_t r = 0; r < THERMAL_ROWS; r++) {
        for (uint8_t c = 0; c < THERMAL_COLS; c++) {
            int8_t rel = (int8_t)c - (int8_t)phase;
            if (rel == 0) {
                frame_set_cell(f, r, c, d_warm);
            } else if (rel == 1 || rel == -1) {
                frame_set_cell(f, r, c, d_warm / 2);
            } else if (rel == 2 || rel == -2) {
                frame_set_cell(f, r, c, d_cool / 2);
            }
        }
    }
}

static int char_to_font_idx(char ch)
{
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a';
    if (ch >= '0' && ch <= '9') return 26 + (ch - '0');
    if (ch == ' ') return FONT_SPACE;
    if (ch == '+') return FONT_CHECK;
    return FONT_SPACE;
}

static void render_text_char(thermal_frame_t *f, char ch,
                             int8_t col_offset, int16_t delta)
{
    int idx = char_to_font_idx(ch);
    const uint8_t *glyph = font5x3[idx];

    for (uint8_t r = 0; r < 5; r++) {
        uint8_t row_bits = glyph[r];
        for (uint8_t c = 0; c < 3; c++) {
            int8_t col = col_offset + (int8_t)c;
            if (col >= 0 && col < (int8_t)THERMAL_COLS) {
                if (row_bits & (1 << (2 - c))) {
                    /* Center vertically: rows 1–5 (skip row 0) */
                    frame_set_cell(f, r + 1, (uint8_t)col, delta);
                }
            }
        }
    }
}

static void render_text(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    int16_t d = get_delta(cmd);

    if (cmd->scroll) {
        /* Scrolling: show one char at a time based on scroll_offset */
        int char_idx = scroll_offset / 4;  /* 4 frames per char position */
        if (char_idx >= 0 && char_idx < cmd->text_len) {
            render_text_char(f, cmd->text[char_idx], 4, d);
        }
    } else {
        /* Static: render up to 3 chars (3 cols + 1 gap each = 4 cols) */
        int8_t col = 0;
        for (int i = 0; i < cmd->text_len && i < 3; i++) {
            render_text_char(f, cmd->text[i], col, d);
            col += 4;
        }
    }
}

static void render_bar(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    /* Vertical bar: fill columns from left proportional to intensity.
     * Used for depth/progress meters. */
    int16_t d = get_delta(cmd);
    uint8_t fill_cols = (uint8_t)((uint32_t)cmd->intensity *
                                   THERMAL_COLS / 255U);

    for (uint8_t c = 0; c < fill_cols; c++) {
        for (uint8_t r = 0; r < THERMAL_ROWS; r++) {
            frame_set_cell(f, r, c, d);
        }
    }
}

static void render_pixel(thermal_frame_t *f, const glyph_cmd_t *cmd)
{
    /* Pixel command: intensity encodes row (high nibble) + col (low nibble)
     * in the text[0] field. polarity sets warm/cool. */
    uint8_t row = (cmd->text[0] >> 4) & 0x0F;
    uint8_t col = cmd->text[0] & 0x0F;
    int16_t d = get_delta(cmd);
    if (row < THERMAL_ROWS && col < THERMAL_COLS) {
        frame_set_cell(f, row, col, d);
    }
}

/* ------------------------------------------------------------------------- */
/* Public API */
/* ------------------------------------------------------------------------- */

void glyph_engine_init(void)
{
    memset(cmd_queue, 0, sizeof(cmd_queue));
    memset(&current_cmd, 0, sizeof(current_cmd));
    frame_set_neutral(&current_frame);
    queue_head = 0;
    queue_tail = 0;
    glyph_timer_ms = 0;
    anim_phase = 0;
    scroll_offset = 0;
}

bool glyph_engine_queue(const glyph_cmd_t *cmd)
{
    if (queue_full()) return false;
    cmd_queue[queue_head] = *cmd;
    queue_head = (queue_head + 1) % GLYPH_QUEUE_LEN;
    return true;
}

void glyph_engine_clear(void)
{
    queue_tail = queue_head;  /* empty the queue */
    memset(&current_cmd, 0, sizeof(current_cmd));
    frame_set_neutral(&current_frame);
    glyph_timer_ms = 0;
}

const glyph_cmd_t *glyph_engine_current(void)
{
    if (glyph_timer_ms > 0) return &current_cmd;
    return NULL;
}

void glyph_engine_set_ambient(int16_t ambient)
{
    ambient_c100 = ambient;
}

void glyph_engine_set_intensity_scale(uint8_t scale)
{
    intensity_scale = scale;
}

void glyph_engine_render(thermal_frame_t *frame)
{
    /* Check if current glyph has expired */
    if (glyph_timer_ms > 0) {
        uint16_t dur = current_cmd.duration_ms_lo |
                       ((uint16_t)current_cmd.duration_ms_hi << 8);
        uint16_t elapsed = (uint16_t)(1000U / GLYPH_FRAME_HZ);  /* 20 ms */
        if (elapsed >= glyph_timer_ms) {
            glyph_timer_ms = 0;
        } else {
            glyph_timer_ms -= elapsed;
        }
    }

    /* If current glyph expired, fetch next from queue */
    if (glyph_timer_ms == 0) {
        if (queue_count() > 0) {
            current_cmd = cmd_queue[queue_tail];
            queue_tail = (queue_tail + 1) % GLYPH_QUEUE_LEN;
            uint16_t dur = current_cmd.duration_ms_lo |
                           ((uint16_t)current_cmd.duration_ms_hi << 8);
            glyph_timer_ms = dur;
            anim_phase = 0;
            scroll_offset = 0;
        } else {
            /* No active glyph: neutral frame */
            frame_set_neutral(frame);
            return;
        }
    }

    /* Render current glyph */
    frame_set_neutral(frame);

    switch (current_cmd.cmd) {
    case GLYPH_CMD_CLEAR:
        frame_set_neutral(frame);
        glyph_timer_ms = 0;
        break;
    case GLYPH_CMD_ARROW:
        render_arrow(frame, &current_cmd);
        break;
    case GLYPH_CMD_SHAPE:
        render_shape(frame, &current_cmd);
        break;
    case GLYPH_CMD_TEXT:
        render_text(frame, &current_cmd);
        if (current_cmd.scroll) {
            scroll_offset++;
            if (scroll_offset >= (current_cmd.text_len * 4)) {
                scroll_offset = 0;
            }
        }
        break;
    case GLYPH_CMD_ANIM:
        /* Animation: shape determined by 'shape' field */
        if (current_cmd.shape == GLYPH_SHAPE_RING) {
            render_ring_anim(frame, &current_cmd);
        } else if (current_cmd.shape == GLYPH_SHAPE_WAVE) {
            render_wave_anim(frame, &current_cmd);
        } else {
            render_shape(frame, &current_cmd);
        }
        break;
    case GLYPH_CMD_BAR:
        render_bar(frame, &current_cmd);
        break;
    case GLYPH_CMD_PIXEL:
        render_pixel(frame, &current_cmd);
        break;
    case GLYPH_CMD_REPEAT:
        /* Re-queue the last glyph */
        glyph_engine_queue(&current_cmd);
        break;
    default:
        frame_set_neutral(frame);
        break;
    }

    anim_phase++;
}