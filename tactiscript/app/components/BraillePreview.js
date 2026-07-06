/**
 * BraillePreview.js — Visual Braille cell preview component
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { patternToGrid } from '../utils/braille';

/**
 * Renders a single Braille cell as a 2×3 dot grid.
 * @param {number} pattern - 6-bit Braille pattern
 * @param {number} size - cell size in px (default 40)
 */
export default function BraillePreview({ pattern, size = 40 }) {
  const grid = patternToGrid(pattern);
  const dotSize = size / 4;
  const dotSpacing = size / 3;

  return (
    <View style={[styles.cell, { width: size, height: size }]}>
      {grid.map((row, rowIdx) => (
        <View key={rowIdx} style={styles.row}>
          {row.map((raised, colIdx) => (
            <View
              key={colIdx}
              style={[
                styles.dot,
                {
                  width: dotSize,
                  height: dotSize,
                  borderRadius: dotSize / 2,
                  margin: dotSpacing / 4,
                  backgroundColor: raised ? '#333' : '#ddd',
                },
              ]}
            />
          ))}
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  cell: {
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    margin: 4,
  },
  row: {
    flexDirection: 'row',
  },
  dot: {
    // styling applied inline
  },
});