/**
 * braille.js — Client-side Braille translation helpers
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Provides Unicode-to-Braille translation for the companion app,
 * so the app can preview what will be rendered on the ring.
 */

// Braille dot layout (6-dot cell):
//   1 4
//   2 5
//   3 6
// Bit encoding: bit0=dot1, bit1=dot2, ..., bit5=dot6

const UEB_GRADE1 = {
  ' ': 0x00,
  'a': 0x01, 'b': 0x03, 'c': 0x09, 'd': 0x19, 'e': 0x11,
  'f': 0x0B, 'g': 0x1B, 'h': 0x0A, 'i': 0x0E, 'j': 0x3E,
  'k': 0x05, 'l': 0x07, 'm': 0x0D, 'n': 0x15, 'o': 0x15,
  'p': 0x1D, 'q': 0x0F, 'r': 0x17, 's': 0x0E, 't': 0x2E,
  'u': 0x25, 'v': 0x27, 'w': 0x3A, 'x': 0x2D, 'y': 0x3D,
  'z': 0x1D,
  '1': 0x01, '2': 0x03, '3': 0x09, '4': 0x19, '5': 0x11,
  '6': 0x0B, '7': 0x1B, '8': 0x0A, '9': 0x0E, '0': 0x3E,
  '.': 0x0D, ',': 0x0C, '?': 0x0E, '!': 0x2C, ':': 0x0C,
  ';': 0x0C, '-': 0x2D, '\'': 0x20, '"': 0x0F,
};

// UEB Grade 2 common contractions
const UEB_GRADE2 = {
  'but': 0x0B, 'can': 0x09, 'do': 0x19, 'every': 0x11,
  'from': 0x07, 'go': 0x1B, 'have': 0x0A, 'just': 0x3E,
  'like': 0x07, 'more': 0x0D, 'not': 0x15, 'quite': 0x1D,
  'some': 0x17, 'that': 0x2E, 'us': 0x25, 'very': 0x27,
  'it': 0x0E, 'you': 0x3A, 'as': 0x01, 'will': 0x3A,
  'and': 0x27, 'for': 0x0B, 'of': 0x0E, 'the': 0x2E,
  'with': 0x37, 'this': 0x17, 'which': 0x2D,
  'be': 0x03, 'in': 0x07, 'is': 0x0E, 'are': 0x1D,
  'was': 0x2E, 'were': 0x25, 'by': 0x03, 'on': 0x15,
  'at': 0x1D,
};

// Number indicator (dots 3,4,5,6)
const NUMBER_INDICATOR = 0x1B;
// Capital indicator (dot 6)
const CAPITAL_INDICATOR = 0x20;

/**
 * Translate a single character to a Braille pattern
 * @param {string} c - character to translate
 * @param {number} grade - 1 or 2 (UEB grade)
 * @returns {number} 6-bit Braille pattern
 */
export function charToPattern(c, grade = 1) {
  const lower = c.toLowerCase();
  if (UEB_GRADE1[lower] !== undefined) {
    return UEB_GRADE1[lower];
  }
  if (UEB_GRADE1[c] !== undefined) {
    return UEB_GRADE1[c];
  }
  return 0x00;
}

/**
 * Translate a string to an array of Braille patterns
 * @param {string} text - input text
 * @param {number} grade - 1 or 2
 * @returns {number[]} array of 6-bit patterns
 */
export function translateText(text, grade = 1) {
  const patterns = [];
  let i = 0;

  while (i < text.length) {
    // Grade 2: try contraction first
    if (grade === 2) {
      let matched = false;
      // Try 4-char, 3-char, 2-char contractions
      for (let len = 4; len >= 2; len--) {
        const candidate = text.substring(i, i + len).toLowerCase();
        // Check word boundary
        const nextChar = text[i + len];
        if (nextChar && nextChar !== ' ' && !/[^\w]/.test(nextChar)) continue;

        if (UEB_GRADE2[candidate]) {
          patterns.push(UEB_GRADE2[candidate]);
          i += len;
          matched = true;
          break;
        }
      }
      if (matched) continue;
    }

    const c = text[i];

    // Number indicator for digits
    if (c >= '0' && c <= '9') {
      const prevWasDigit = i > 0 && text[i - 1] >= '0' && text[i - 1] <= '9';
      if (!prevWasDigit) {
        patterns.push(NUMBER_INDICATOR);
      }
    }

    // Capital indicator for uppercase
    if (c >= 'A' && c <= 'Z') {
      patterns.push(CAPITAL_INDICATOR);
    }

    patterns.push(charToPattern(c, grade));
    i++;
  }

  return patterns;
}

/**
 * Render a Braille pattern as a visual 2×3 dot grid for preview
 * @param {number} pattern - 6-bit Braille pattern
 * @returns {boolean[][]} 3×2 array (rows×cols), true = dot raised
 */
export function patternToGrid(pattern) {
  return [
    [(pattern & 0x01) !== 0, (pattern & 0x08) !== 0], // dots 1, 4
    [(pattern & 0x02) !== 0, (pattern & 0x10) !== 0], // dots 2, 5
    [(pattern & 0x04) !== 0, (pattern & 0x20) !== 0], // dots 3, 6
  ];
}

/**
 * Count the number of Braille cells needed for a string
 * @param {string} text
 * @param {number} grade
 * @returns {number}
 */
export function cellCount(text, grade = 1) {
  return translateText(text, grade).length;
}