/*
 * braille.h — TactiScript Braille translation engine header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_BRAILLE_H
#define TACTISCRIPT_BRAILLE_H

#include <stdint.h>

/* Initialize the Braille translation tables (loads from flash) */
void braille_init(void);

/* Translate a single Unicode character to a 6-dot Braille pattern.
 *   grade: 1 = uncontracted (UEB Grade 1), 2 = contracted (UEB Grade 2)
 * Returns a 6-bit pattern where bits 0-5 map to Braille dots 1-6:
 *   bit 0 = dot 1 (top-left)
 *   bit 1 = dot 2 (middle-left)
 *   bit 2 = dot 3 (bottom-left)
 *   bit 3 = dot 4 (top-right)
 *   bit 4 = dot 5 (middle-right)
 *   bit 5 = dot 6 (bottom-right)
 * Returns 0 for characters with no Braille equivalent (e.g. unknown symbols).
 */
uint8_t braille_char_to_pattern(char c, uint8_t grade);

/* Translate a string of text into a sequence of Braille patterns.
 *   out: output buffer for patterns
 *   max_out: max patterns to write
 * Returns number of patterns written.
 */
uint16_t braille_translate(const char *text, uint8_t *out, uint16_t max_out,
                            uint8_t grade);

/* Get the number of cells needed for a string (accounting for
 * multi-cell contractions in Grade 2) */
uint16_t braille_cell_count(const char *text, uint8_t grade);

#endif /* TACTISCRIPT_BRAILLE_H */