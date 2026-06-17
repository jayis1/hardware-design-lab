/**
 * FieldVector3D.js — 3D Magnetic Field Vector Visualization
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Renders a simple 3D representation of the magnetic field vector
 * using projected isometric drawing (no OpenGL dependency).
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function FieldVector3D({ bx, by, bz }) {
  // Normalize the vector for display (±100 µT → ±1.0)
  const maxField = 100000; // 100 µT in nT
  const nx = Math.max(-1, Math.min(1, bx / maxField));
  const ny = Math.max(-1, Math.min(1, by / maxField));
  const nz = Math.max(-1, Math.min(1, bz / maxField));

  // Isometric projection: project 3D unit to 2D screen coordinates
  // Using a simple isometric transform:
  //   screen_x = (x - y) * cos(30°)
  //   screen_y = (x + y) * sin(30°) - z
  const iso = (x, y, z) => {
    const angle = Math.PI / 6; // 30 degrees
    const cos30 = Math.cos(angle);
    const sin30 = Math.sin(angle);
    return {
      x: (x - y) * cos30 * 60, // Scale factor 60px per unit
      y: (x + y) * sin30 * 60 - z * 60,
    };
  };

  // Origin point (center of the view)
  const origin = { x: 0, y: 0 };

  // Axis endpoints (unit vectors)
  const xAxisEnd = iso(1, 0, 0);
  const yAxisEnd = iso(0, 1, 0);
  const zAxisEnd = iso(0, 0, 1);

  // Vector endpoint
  const vecEnd = iso(nx, ny, nz);

  // Total field magnitude for display
  const totalNt = Math.sqrt(bx * bx + by * by + bz * bz);
  const totalUt = (totalNt / 1000).toFixed(2);

  // Declination and inclination (for display)
  const declination = Math.atan2(by, bx) * 180 / Math.PI;
  const inclination = Math.atan2(bz, Math.sqrt(bx * bx + by * by)) * 180 / Math.PI;

  return (
    <View style={styles.container}>
      <View style={styles.plotArea}>
        {/* We use absolute positioning to draw the 3D axes and vector */}
        {/* This is a simplified representation using nested Views */}

        {/* X axis (red) */}
        <View
          style={[
            styles.axisLine,
            {
              left: '50%',
              top: '50%',
              width: 30,
              transform: [{ rotate: '-30deg' }],
              backgroundColor: '#F44336',
            },
          ]}
        />
        <Text style={[styles.axisLabel, { left: '58%', top: '48%', color: '#F44336' }]}>X</Text>

        {/* Y axis (green) */}
        <View
          style={[
            styles.axisLine,
            {
              left: '50%',
              top: '50%',
              width: 30,
              transform: [{ rotate: '30deg' }],
              backgroundColor: '#4CAF50',
            },
          ]}
        />
        <Text style={[styles.axisLabel, { left: '42%', top: '48%', color: '#4CAF50' }]}>Y</Text>

        {/* Z axis (blue) */}
        <View
          style={[
            styles.axisLine,
            {
              left: '50%',
              top: '38%',
              width: 30,
              backgroundColor: '#2196F3',
            },
          ]}
        />
        <Text style={[styles.axisLabel, { left: '49%', top: '30%', color: '#2196F3' }]}>Z</Text>

        {/* Field vector (yellow) */}
        <View
          style={[
            styles.axisLine,
            {
              left: '50%',
              top: `${50 + vecEnd.y * 0.3}%`,
              width: 40,
              transform: [{ rotate: `${vecEnd.x * 30}deg` }],
              backgroundColor: '#FFEB3B',
              height: 3,
            },
          ]}
        />
        <Text style={[styles.axisLabel, { left: '52%', top: '52%', color: '#FFEB3B' }]}>B</Text>

        {/* Center dot */}
        <View style={styles.centerDot} />
      </View>

      {/* Vector info */}
      <View style={styles.infoContainer}>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>|B|</Text>
          <Text style={styles.infoValue}>{totalUt} µT</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Dec</Text>
          <Text style={styles.infoValue}>{declination.toFixed(1)}°</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Inc</Text>
          <Text style={styles.infoValue}>{inclination.toFixed(1)}°</Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
  },
  plotArea: {
    width: '100%',
    height: 160,
    position: 'relative',
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
  },
  axisLine: {
    position: 'absolute',
    height: 2,
    borderRadius: 1,
  },
  axisLabel: {
    position: 'absolute',
    fontSize: 10,
    fontWeight: 'bold',
  },
  centerDot: {
    position: 'absolute',
    left: '49%',
    top: '49%',
    width: 4,
    height: 4,
    borderRadius: 2,
    backgroundColor: '#fff',
  },
  infoContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    width: '100%',
    marginTop: 10,
  },
  infoRow: {
    alignItems: 'center',
  },
  infoLabel: {
    color: '#888',
    fontSize: 10,
  },
  infoValue: {
    color: '#fff',
    fontSize: 14,
    fontFamily: 'monospace',
  },
});