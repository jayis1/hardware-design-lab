/**
 * NyquistPlot.tsx — Interactive Nyquist impedance plot.
 *
 * Renders the -Im(Z) vs. Re(Z) Nyquist diagram from sweep data points.
 * Uses react-native-svg for rendering.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useMemo } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import Svg, { Polyline, Circle, Line, Text as SvgText } from 'react-native-svg';
import { SweepPoint } from '../ble/protocol';

interface NyquistPlotProps {
  points: SweepPoint[];
  width?: number;
  height?: number;
}

const COLORS = {
  background: '#1a1a2e',
  axis: '#444466',
  grid: '#222244',
  curve: '#00b4ff',
  point: '#00e676',
  label: '#8888aa',
};

export default function NyquistPlot({
  points,
  width = Dimensions.get('window').width - 32,
  height = 280,
}: NyquistPlotProps) {
  const { polylinePoints, pointCircles, axisLabels, origin } = useMemo(() => {
    if (points.length === 0) {
      return {
        polylinePoints: '',
        pointCircles: [],
        axisLabels: [],
        origin: { x: width / 2, y: height / 2 },
      };
    }

    // Compute scaling: map Re(Z) to X, -Im(Z) to Y
    const reValues = points.map((p) => p.reZ / 1000);  // mΩ → Ω
    const imValues = points.map((p) => -p.imZ / 1000);  // negate for Nyquist
    const reMin = Math.min(...reValues, 0);
    const reMax = Math.max(...reValues);
    const imMin = Math.min(...imValues, 0);
    const imMax = Math.max(...imValues, ...reValues.map((r) => r * 0.5));  // ensure some headroom

    const padding = 40;
    const plotW = width - padding * 2;
    const plotH = height - padding * 2;

    const reRange = reMax - reMin || 1;
    const imRange = imMax - imMin || 1;

    const toX = (re: number) => padding + ((re - reMin) / reRange) * plotW;
    const toY = (im: number) => padding + plotH - ((im - imMin) / imRange) * plotH;

    const polyPts = points.map((p) => `${toX(p.reZ / 1000)},${toY(-p.imZ / 1000)}`).join(' ');
    const circles = points.map((p, i) => ({
      cx: toX(p.reZ / 1000),
      cy: toY(-p.imZ / 1000),
      key: i,
    }));

    const labels = [
      { x: padding, y: height - 20, text: `${reMin.toFixed(1)}Ω` },
      { x: width - padding, y: height - 20, text: `${reMax.toFixed(1)}Ω` },
      { x: 10, y: padding, text: `${imMax.toFixed(1)}Ω` },
      { x: 10, y: height - padding, text: `${imMin.toFixed(1)}Ω` },
    ];

    // Origin (0,0) in plot coordinates
    const origX = toX(0);
    const origY = toY(0);

    return {
      polylinePoints: polyPts,
      pointCircles: circles,
      axisLabels: labels,
      origin: { x: origX, y: origY },
    };
  }, [points, width, height]);

  if (points.length === 0) {
    return (
      <View style={styles.emptyContainer}>
        <Text style={styles.emptyText}>Waiting for sweep data…</Text>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Nyquist Plot (−Im Z vs. Re Z)</Text>
      <Svg width={width} height={height}>
        {/* Grid lines */}
        <Line x1={origin.x} y1={40} x2={origin.x} y2={height - 40}
          stroke={COLORS.axis} strokeWidth={1} strokeDasharray="4,4" />
        <Line x1={40} y1={origin.y} x2={width - 40} y2={origin.y}
          stroke={COLORS.axis} strokeWidth={1} strokeDasharray="4,4" />

        {/* Impedance curve */}
        <Polyline
          points={polylinePoints}
          fill="none"
          stroke={COLORS.curve}
          strokeWidth={2}
        />

        {/* Data points */}
        {pointCircles.map((p) => (
          <Circle key={p.key} cx={p.cx} cy={p.cy} r={3} fill={COLORS.point} />
        ))}

        {/* Axis labels */}
        {axisLabels.map((label, i) => (
          <SvgText key={i} x={label.x} y={label.y}
            fill={COLORS.label} fontSize={10} textAnchor="middle">
            {label.text}
          </SvgText>
        ))}

        {/* Axis titles */}
        <SvgText x={width / 2} y={height - 5}
          fill={COLORS.label} fontSize={11} textAnchor="middle">
          Re(Z) [Ω]
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: COLORS.background,
    borderRadius: 8,
    padding: 8,
    marginVertical: 8,
  },
  title: {
    color: '#e0e0e0',
    fontSize: 13,
    fontWeight: 'bold',
    textAlign: 'center',
    marginBottom: 4,
  },
  emptyContainer: {
    height: 280,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: COLORS.background,
    borderRadius: 8,
  },
  emptyText: {
    color: '#666688',
    fontSize: 14,
  },
});