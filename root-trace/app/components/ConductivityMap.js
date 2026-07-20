/**
 * ConductivityMap.js — Real-time EIT conductivity map visualization
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useRef, useEffect, useMemo } from 'react';
import { View, StyleSheet, Text, Dimensions } from 'react-native';
import { Svg, Circle, Rect, Line, Text as SvgText, G } from 'react-native-svg';

const { width } = Dimensions.get('window');

// Color scale: viridis-like (dark blue -> green -> yellow)
function conductivityToColor(val) {
  // val: 0..1 (normalized conductivity)
  const v = Math.max(0, Math.min(1, val));
  // Simple blue-green-yellow colormap
  const r = Math.round(255 * Math.min(1, v * 2 - 0.5));
  const g = Math.round(255 * Math.min(1, v * 1.5));
  const b = Math.round(255 * Math.max(0, 1 - v * 2));
  return `rgb(${r}, ${g}, ${b})`;
}

// Electrode positions (16 around a circle)
function getElectrodePositions(cx, cy, r) {
  const positions = [];
  for (let i = 0; i < 16; i++) {
    const angle = (2 * Math.PI * i) / 16 - Math.PI / 2;
    positions.push({
      x: cx + r * Math.cos(angle),
      y: cy + r * Math.sin(angle),
      angle: angle,
      index: i,
    });
  }
  return positions;
}

export function ConductivityMapView({ frameData, showContacts, width: w, height: h }) {
  const size = Math.min(w || width - 80, h || width - 80);
  const cx = size / 2;
  const cy = size / 2;
  const r = size / 2 - 20;

  const electrodes = useMemo(() => getElectrodePositions(cx, cy, r), [cx, cy, r]);

  // Generate a 16x16 grid for the conductivity heatmap inside the circle
  const gridSize = 16;
  const cellSize = (r * 2) / gridSize;
  const cells = [];

  if (frameData && frameData.image) {
    for (let y = 0; y < gridSize; y++) {
      for (let x = 0; x < gridSize; x++) {
        // Map grid cell to image coordinates
        const imgX = Math.floor((x / gridSize) * 64);
        const imgY = Math.floor((y / gridSize) * 64);
        let val = 0;
        if (frameData.image[imgY] && frameData.image[imgY][imgX] !== undefined) {
          val = frameData.image[imgY][imgX] / 255;
        }
        // Check if cell is inside the circle
        const px = (x - gridSize / 2) * cellSize;
        const py = (y - gridSize / 2) * cellSize;
        if (Math.sqrt(px * px + py * py) > r) continue;

        cells.push({
          x: cx - r + x * cellSize,
          y: cy - r + y * cellSize,
          w: cellSize + 0.5,
          h: cellSize + 0.5,
          color: conductivityToColor(val),
        });
      }
    }
  }

  // Contact mask
  const contactMask = frameData?.contactMask || 0xFFFF;

  return (
    <View style={styles.container}>
      <Svg width={size} height={size}>
        {/* Background circle */}
        <Circle cx={cx} cy={cy} r={r + 2} fill="#0a1628" stroke="#3b82f6" strokeWidth="1" />

        {/* Conductivity cells */}
        {cells.map((cell, idx) => (
          <Rect
            key={idx}
            x={cell.x}
            y={cell.y}
            width={cell.w}
            height={cell.h}
            fill={cell.color}
            opacity={0.85}
          />
        ))}

        {/* Electrode ring */}
        {showContacts && electrodes.map((e, idx) => {
          const inContact = (contactMask >> idx) & 1;
          return (
            <G key={idx}>
              <Circle
                cx={e.x}
                cy={e.y}
                r={6}
                fill={inContact ? '#10b981' : '#ef4444'}
                stroke="#e2e8f0"
                strokeWidth="1"
              />
              <SvgText
                x={e.x + 10 * Math.cos(e.angle)}
                y={e.y + 10 * Math.sin(e.angle)}
                fontSize="9"
                fill="#94a3b8"
                textAnchor="middle"
              >
                {idx + 1}
              </SvgText>
            </G>
          );
        })}

        {/* Crosshair */}
        <Line x1={cx - 5} y1={cy} x2={cx + 5} y2={cy} stroke="#94a3b8" strokeWidth="0.5" />
        <Line x1={cx} y1={cy - 5} x2={cx} y2={cy + 5} stroke="#94a3b8" strokeWidth="0.5" />
      </Svg>

      {/* Legend */}
      <View style={styles.legend}>
        <Text style={styles.legendLabel}>Low σ</Text>
        <View style={styles.gradientBar} />
        <Text style={styles.legendLabel}>High σ</Text>
      </View>

      {frameData && frameData.biomass !== undefined && (
        <Text style={styles.biomassLabel}>
          Biomass Index: {frameData.biomass.toFixed(3)}
        </Text>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  legend: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 8,
  },
  legendLabel: {
    fontSize: 10,
    color: '#94a3b8',
  },
  gradientBar: {
    width: 120,
    height: 10,
    marginHorizontal: 8,
    borderRadius: 2,
    backgroundColor: 'linear-gradient(90deg, #1e3a8a, #10b981, #facc15)',
  },
  biomassLabel: {
    fontSize: 12,
    color: '#10b981',
    marginTop: 4,
    fontWeight: '600',
  },
});