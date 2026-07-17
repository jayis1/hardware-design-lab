/**
 * SpectrogramView.tsx — canvas-free SVG spectrogram renderer.
 *
 * Author : jayis1
 * License: MIT
 *
 * Renders a scrolling waterfall from SpectrogramRow updates. Each row is
 * 32 bins; we map bin values (0..255) to a green-to-red colour ramp and
 * stack rows top=oldest, bottom=newest.
 */
import React, { useRef, useState, useEffect } from 'react';
import { View, StyleSheet } from 'react-native';
import Svg, { Rect } from 'react-native-svg';
import { SpectrogramRow } from '../api';

const N_BINS = 32;
const MAX_ROWS = 120;
const ROW_H = 2;
const BIN_W = 8;

function color(v: number): string {
  /* 0 => dark blue, 128 => green, 255 => red. */
  if (v < 64)  return `rgb(0,0,${Math.floor(v*4)})`;
  if (v < 128) return `rgb(0,${Math.floor((v-64)*4)},128)`;
  if (v < 192) return `rgb(${Math.floor((v-128)*4)},255,${Math.floor(255-(v-128)*4)})`;
  return `rgb(255,${Math.floor(255-(v-192)*4)},0)`;
}

export default function SpectrogramView({ rows }: { rows: SpectrogramRow[] }) {
  const recent = rows.slice(-MAX_ROWS);
  const height = MAX_ROWS * ROW_H;
  const width  = N_BINS * BIN_W;

  return (
    <View style={styles.wrap}>
      <Svg width={width} height={height}>
        {recent.map((row, ri) =>
          row.bins.map((v, bi) => (
            <Rect
              key={`${row.ts}-${ri}-${bi}`}
              x={bi * BIN_W}
              y={(MAX_ROWS - recent.length + ri) * ROW_H}
              width={BIN_W}
              height={ROW_H}
              fill={color(v)}
            />
          ))
        )}
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { backgroundColor: '#000', borderRadius: 6, padding: 4 },
});