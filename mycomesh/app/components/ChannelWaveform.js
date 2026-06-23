/**
 * ChannelWaveform.js — Single-channel waveform display component for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Renders a single channel of electrophysiology data as a waveform
 * using SVG.  Shows channel number, current amplitude, and color-coded
 * trace.  Supports both stacked (small) and overlay (large) modes.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Polyline, Line, Text as SvgText } from 'react-native-svg';

export default function ChannelWaveform({ channel, samples, color, height, overlay }) {
  const width = 300;
  const midY = height / 2;

  // Find the range for auto-scaling.
  let maxAbs = 100;
  for (const s of samples) {
    const a = Math.abs(s);
    if (a > maxAbs) maxAbs = a;
  }

  // Scale samples to fit within the waveform area.
  const scale = (midY - 4) / maxAbs;
  const points = samples.map((s, i) => {
    const x = (i / (samples.length - 1)) * width;
    const y = midY - s * scale;
    return `${x},${y}`;
  }).join(' ');

  // Current peak value.
  const currentVal = samples.length > 0 ? samples[samples.length - 1] : 0;

  return (
    <View style={[styles.container, { height }]}>
      <View style={styles.label}>
        <Text style={[styles.channelText, { color }]}>
          CH{channel.toString().padStart(2, '0')}
        </Text>
        <Text style={styles.valueText}>{currentVal} µV</Text>
      </View>
      <Svg width={width} height={height} style={styles.svg}>
        {/* Zero line */}
        <Line x1="0" y1={midY} x2={width} y2={midY} stroke="#333" strokeWidth="0.5" />
        {/* Waveform */}
        <Polyline
          points={points}
          fill="none"
          stroke={color}
          strokeWidth="1"
          strokeLinejoin="round"
          strokeLinecap="round"
        />
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#0a1a0a', borderRadius: 4, marginBottom: 2,
  },
  label: { width: 55, paddingLeft: 4 },
  channelText: { fontSize: 10, fontWeight: 'bold' },
  valueText: { color: '#666', fontSize: 9 },
  svg: { flex: 1 },
});