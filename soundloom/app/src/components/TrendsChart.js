/**
 * TrendsChart.js — Simple SVG line chart for SVI trends
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, StyleSheet, Dimensions, Text } from 'react-native';
import { Svg, Polyline, Line, Text as SvgText, Circle, G, Rect } from 'react-native-svg';

const { width } = Dimensions.get('window');

export function TrendsChart({ pods = [], days = 7, data = null }) {
  const chartW = width - 60;
  const chartH = 140;

  // Generate demo data if no data provided
  const getData = () => {
    if (data && data.length > 0) return data;
    const points = [];
    const n = days * 24; // hourly points
    for (let i = 0; i < n; i++) {
      points.push({
        ts: Date.now() - (n - i) * 3600000,
        svi: 50 + Math.sin(i / 10) * 20 + Math.random() * 10,
      });
    }
    return points;
  };

  const chartData = getData();
  if (chartData.length === 0) return <Text style={styles.empty}>No data</Text>;

  // Scale data to chart
  const minSvi = 0;
  const maxSvi = 100;
  const toX = (i) => (i / (chartData.length - 1)) * chartW;
  const toY = (svi) => chartH - ((svi - minSvi) / (maxSvi - minSvi)) * chartH;

  // Build polyline points
  const polylinePoints = chartData.map((p, i) => `${toX(i)},${toY(p.svi)}`).join(' ');

  // Y-axis labels
  const yLabels = [0, 25, 50, 75, 100];

  return (
    <View style={styles.container}>
      <Svg width={chartW + 40} height={chartH + 30}>
        {/* Y-axis grid lines */}
        {yLabels.map(v => {
          const y = toY(v);
          return (
            <G key={`y-${v}`}>
              <Line x1={30} y1={y} x2={30 + chartW} y2={y} stroke="#E0E0E0" strokeWidth={0.5} />
              <SvgText x={25} y={y + 4} fill="#9E9E9E" fontSize={9} textAnchor="end">{v}</SvgText>
            </G>
          );
        })}

        {/* Data line */}
        <G transform="translate(30, 0)">
          <Polyline points={polylinePoints} fill="none" stroke="#2E7D32" strokeWidth={2} />
          {/* Area fill (semi-transparent) */}
          <Polyline
            points={`0,${chartH} ${polylinePoints} ${chartW},${chartH}`}
            fill="#2E7D32" opacity={0.1} stroke="none"
          />
        </G>

        {/* X-axis */}
        <Line x1={30} y1={chartH} x2={30 + chartW} y2={chartH} stroke="#BDBDBD" strokeWidth={1} />
        <SvgText x={30} y={chartH + 18} fill="#9E9E9E" fontSize={9}>-{days}d</SvgText>
        <SvgText x={30 + chartW} y={chartH + 18} fill="#9E9E9E" fontSize={9} textAnchor="end">now</SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', padding: 5 },
  empty: { fontSize: 14, color: '#9E9E9E', textAlign: 'center', padding: 20 },
});