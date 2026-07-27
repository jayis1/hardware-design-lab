/**
 * BodePlot.tsx — Bode magnitude and phase plot.
 *
 * Renders the |Z(f)| and phase(f) Bode diagrams from sweep data.
 * Uses logarithmic frequency axis.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useMemo } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import Svg, { Polyline, Circle, Line, Text as SvgText } from 'react-native-svg';
import { SweepPoint } from '../ble/protocol';

interface BodePlotProps {
  points: SweepPoint[];
  width?: number;
  height?: number;
}

const COLORS = {
  background: '#1a1a2e',
  axis: '#444466',
  grid: '#222244',
  magCurve: '#ff9800',
  phaseCurve: '#2196f3',
  label: '#8888aa',
};

function log10(n: number): number {
  return Math.log(n) / Math.LN10;
}

export default function BodePlot({
  points,
  width = Dimensions.get('window').width - 32,
  height = 300,
}: BodePlotProps) {
  const { magPolyline, phasePolyline, freqLabels, magLabels, phaseLabels } = useMemo(() => {
    if (points.length === 0) {
      return { magPolyline: '', phasePolyline: '', freqLabels: [], magLabels: [], phaseLabels: [] };
    }

    const padding = 44;
    const plotW = width - padding * 2;
    const magH = (height - padding * 2) / 2;     // top half: magnitude
    const phaseH = (height - padding * 2) / 2;    // bottom half: phase

    // Log frequency range
    const freqs = points.map((p) => Math.max(p.freqHz, 0.001));
    const fMin = Math.min(...freqs);
    const fMax = Math.max(...freqs);
    const logFMin = log10(fMin);
    const logFMax = log10(fMax);
    const logFRange = logFMax - logFMin || 1;

    // Magnitude range (log scale)
    const mags = points.map((p) => Math.max(Math.abs(p.mag / 1000), 0.001));  // mΩ → Ω
    const magMin = log10(Math.min(...mags));
    const magMax = log10(Math.max(...mags));
    const magRange = magMax - magMin || 1;

    // Phase range (linear: -90 to +90 degrees)
    const phases = points.map((p) => p.phase / 1000);  // millideg → deg
    const phaseMin = -90;
    const phaseMax = 90;

    const toXF = (freq: number) => padding + ((log10(Math.max(freq, 0.001)) - logFMin) / logFRange) * plotW;
    const toYM = (mag: number) => padding + magH - ((log10(Math.max(mag, 0.001)) - magMin) / magRange) * (magH - 10);
    const toYP = (phase: number) => padding + magH + 10 + phaseH - ((phase - phaseMin) / (phaseMax - phaseMin)) * (phaseH - 10);

    const magPts = points.map((p) => `${toXF(p.freqHz)},${toYM(Math.abs(p.mag / 1000))}`).join(' ');
    const phasePts = points.map((p) => `${toXF(p.freqHz)},${toYP(p.phase / 1000)}`).join(' ');

    // Frequency axis labels (decade ticks)
    const freqLabs = [];
    for (let d = Math.ceil(logFMin); d <= Math.floor(logFMax); d++) {
      const x = padding + ((d - logFMin) / logFRange) * plotW;
      const label = d >= 6 ? `${(d / 3).toFixed(0)}MHz` : d >= 3 ? `${d / 3}kHz` : `${Math.pow(10, d).toFixed(0)}Hz`;
      freqLabs.push({ x, y: height - 8, text: label });
    }

    const magLabs = [
      { x: 8, y: padding + 10, text: `${Math.pow(10, magMax).toFixed(0)}Ω` },
      { x: 8, y: padding + magH - 5, text: `${Math.pow(10, magMin).toFixed(2)}Ω` },
    ];

    const phaseLabs = [
      { x: 8, y: padding + magH + 15, text: '+90°' },
      { x: 8, y: padding + magH + phaseH, text: '-90°' },
    ];

    return {
      magPolyline: magPts,
      phasePolyline: phasePts,
      freqLabels: freqLabs,
      magLabels: magLabs,
      phaseLabels: phaseLabs,
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
      <Text style={styles.title}>Bode Plot (|Z| and Phase vs. Frequency)</Text>
      <Svg width={width} height={height}>
        {/* Magnitude plot */}
        <SvgText x={width / 2} y={14} fill={COLORS.magCurve} fontSize={10} textAnchor="middle">
          |Z| [Ω]
        </SvgText>
        <Polyline points={magPolyline} fill="none" stroke={COLORS.magCurve} strokeWidth={2} />

        {/* Phase plot */}
        <SvgText x={width / 2} y={height / 2 + 8} fill={COLORS.phaseCurve} fontSize={10} textAnchor="middle">
          Phase [°]
        </SvgText>
        <Polyline points={phasePolyline} fill="none" stroke={COLORS.phaseCurve} strokeWidth={2} />

        {/* Frequency axis */}
        <Line x1={44} y1={height - 30} x2={width - 44} y2={height - 30}
          stroke={COLORS.axis} strokeWidth={1} />
        {freqLabels.map((l, i) => (
          <SvgText key={i} x={l.x} y={l.y} fill={COLORS.label} fontSize={9} textAnchor="middle">
            {l.text}
          </SvgText>
        ))}

        {/* Magnitude labels */}
        {magLabels.map((l, i) => (
          <SvgText key={i} x={l.x} y={l.y} fill={COLORS.label} fontSize={9}>
            {l.text}
          </SvgText>
        ))}

        {/* Phase labels */}
        {phaseLabels.map((l, i) => (
          <SvgText key={i} x={l.x} y={l.y} fill={COLORS.label} fontSize={9}>
            {l.text}
          </SvgText>
        ))}
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
    height: 300,
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