/**
 * ImpedancePlot.js — Polar impedance plane visualization
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Renders a polar plot of the 4-frequency I/Q impedance vectors
 * using react-native-svg. Each frequency is plotted as a vector from
 * the origin, color-coded by frequency.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Circle, Line, Text as SvgText, G } from 'react-native-svg';
import Colors from '../constants/Colors';

const FREQ_LABELS = ['1 kHz', '10 kHz', '100 kHz', '500 kHz'];
const FREQ_COLORS = [Colors.cyan, Colors.green, Colors.yellow, Colors.red];

const ImpedancePlot = ({ iq }) => {
  if (!iq || iq.length < 8) {
    return (
      <View style={styles.noDataContainer}>
        <Text style={styles.noDataText}>No impedance data</Text>
      </View>
    );

  // Parse I/Q pairs: iq = [[I1,Q1],[I2,Q2],[I3,Q3],[I4,Q4]]
  const pairs = [];
  for (let i = 0; i < 4; i++) {
    if (iq[i] && iq[i].length >= 2) {
      pairs.push({ I: iq[i][0], Q: iq[i][1], freq: FREQ_LABELS[i], color: FREQ_COLORS[i] });
    }
  }

  // Find max magnitude for scaling
  let maxMag = 0.001;
  pairs.forEach(p => {
    const mag = Math.sqrt(p.I * p.I + p.Q * p.Q);
    if (mag > maxMag) maxMag = mag;
  });

  const plotSize = 160;
  const center = plotSize / 2;
  const maxRadius = center - 15;
  const scale = maxRadius / maxMag;

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Impedance Plane (4 frequencies)</Text>
      <Svg width={plotSize} height={plotSize}>
        {/* Background circle */}
        <Circle cx={center} cy={center} r={maxRadius} fill="none" stroke="#333" strokeWidth={1} />
        <Circle cx={center} cy={center} r={maxRadius / 2} fill="none" stroke="#222" strokeWidth={0.5} />
        <Circle cx={center} cy={center} r={maxRadius / 4} fill="none" stroke="#222" strokeWidth={0.5} />

        {/* Axes */}
        <Line x1={center} y1={5} x2={center} y2={plotSize - 5} stroke="#444" strokeWidth={0.5} />
        <Line x1={5} y1={center} x2={plotSize - 5} y2={center} stroke="#444" strokeWidth={0.5} />

        {/* Vectors */}
        {pairs.map((p, i) => {
          const x2 = center + p.I * scale;
          const y2 = center - p.Q * scale;  // SVG Y is inverted
          return (
            <G key={i}>
              <Line
                x1={center} y1={center} x2={x2} y2={y2}
                stroke={p.color} strokeWidth={2}
              />
              <Circle cx={x2} cy={y2} r={3} fill={p.color} />
            </G>
          );
        })}

        {/* Center dot */}
        <Circle cx={center} cy={center} r={2} fill="#666" />
      </Svg>

      {/* Legend */}
      <View style={styles.legend}>
        {pairs.map((p, i) => (
          <View key={i} style={styles.legendItem}>
            <View style={[styles.legendDot, { backgroundColor: p.color }]} />
            <Text style={styles.legendText}>{p.freq}</Text>
          </View>
        ))}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    marginVertical: 10,
    padding: 10,
    backgroundColor: Colors.darkGray,
    borderRadius: 12,
  },
  title: {
    color: Colors.lightGray,
    fontSize: 12,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  noDataContainer: {
    alignItems: 'center',
    padding: 20,
  },
  noDataText: {
    color: Colors.gray,
    fontSize: 13,
  },
  legend: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'center',
    marginTop: 8,
  },
  legendItem: {
    flexDirection: 'row',
    alignItems: 'center',
    marginHorizontal: 6,
  },
  legendDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 4,
  },
  legendText: {
    color: Colors.gray,
    fontSize: 11,
  },
});

export default ImpedancePlot;