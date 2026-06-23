/**
 * SpikeRaster.js — Spike raster plot component for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Renders a spike raster plot where each row is a channel and each
 * dot represents a detected spike at its timestamp.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Circle, Line, Text as SvgText } from 'react-native-svg';

const CHANNELS = 16;
const HEIGHT = 200;
const WIDTH = 300;

export default function SpikeRaster({ spikes }) {
  if (!spikes || spikes.length === 0) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>No spikes detected yet</Text>
      </View>
    );
  }

  // Find time range.
  const timestamps = spikes.map((s) => s.timestampMs);
  const tMin = Math.min(...timestamps);
  const tMax = Math.max(...timestamps);
  const tRange = Math.max(tMax - tMin, 1);

  const channelHeight = HEIGHT / CHANNELS;

  return (
    <View style={styles.container}>
      <Svg width={WIDTH} height={HEIGHT} style={styles.svg}>
        {/* Channel separator lines */}
        {Array.from({ length: CHANNELS + 1 }, (_, i) => (
          <Line
            key={i}
            x1="30"
            y1={i * channelHeight}
            x2={WIDTH}
            y2={i * channelHeight}
            stroke="#333"
            strokeWidth="0.5"
          />
        ))}

        {/* Channel labels */}
        {Array.from({ length: CHANNELS }, (_, i) => (
          <SvgText
            key={i}
            x="2"
            y={i * channelHeight + channelHeight / 2 + 3}
            fill="#888"
            fontSize="8"
          >
            CH{i.toString().padStart(2, '0')}
          </SvgText>
        ))}

        {/* Spike dots */}
        {spikes.map((sp, i) => {
          if (sp.channel >= CHANNELS) return null;
          const x = 30 + ((sp.timestampMs - tMin) / tRange) * (WIDTH - 30);
          const y = sp.channel * channelHeight + channelHeight / 2;
          const radius = Math.min(Math.max(Math.abs(sp.amplitudeUv) / 200, 1), 3);
          const color = sp.amplitudeUv > 0 ? '#4CAF50' : '#FF5722';
          return (
            <Circle
              key={i}
              cx={x}
              cy={y}
              r={radius}
              fill={color}
              opacity={0.8}
            />
          );
        })}
      </Svg>
      <Text style={styles.timeLabel}>
        Time: {tMin}ms — {tMax}ms
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { backgroundColor: '#0a1a0a', borderRadius: 8, padding: 4 },
  svg: { backgroundColor: '#0a1a0a' },
  empty: { height: 100, justifyContent: 'center', alignItems: 'center' },
  emptyText: { color: '#666', fontSize: 14 },
  timeLabel: { color: '#666', fontSize: 10, textAlign: 'center', marginTop: 4 },
});