/**
 * BarChart.js — SVG bar chart for harmonic spectra and waveforms
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Rect, Line, Text as SvgText } from 'react-native-svg';

export default function BarChart({ data, width, height, color, max, labels, title, unit }) {
  const safeData = data || [];
  const dataMax = max || Math.max(...safeData, 1);
  const barCount = safeData.length;
  const chartW = width || 320;
  const chartH = height || 150;
  const barW = barCount > 0 ? chartW / barCount : 0;
  const barColor = color || '#0080FF';

  return (
    <View style={styles.container}>
      {title && <Text style={styles.title}>{title}</Text>}
      <Svg width={chartW} height={chartH}>
        {/* Background */}
        <Rect x={0} y={0} width={chartW} height={chartH} fill="#111" rx={4} />

        {/* Grid lines */}
        {[0.25, 0.5, 0.75].map((f, i) => (
          <Line
            key={i}
            x1={0}
            y1={chartH * f}
            x2={chartW}
            y2={chartH * f}
            stroke="#333"
            strokeWidth={0.5}
          />
        ))}

        {/* Bars */}
        {safeData.map((val, i) => {
          const h = (val / dataMax) * (chartH - 20);
          const y = chartH - h - 15;
          return (
            <Rect
              key={i}
              x={i * barW + 1}
              y={y}
              width={Math.max(barW - 2, 1)}
              height={Math.max(h, 0)}
              fill={barColor}
              rx={1}
            />
          );
        })}

        {/* X-axis labels (show every 5th) */}
        {labels &&
          labels.map((label, i) => {
            if (i % 5 !== 0 && i !== labels.length - 1) return null;
            return (
              <SvgText
                key={i}
                x={i * barW + barW / 2}
                y={chartH - 3}
                fontSize={8}
                fill="#888"
                textAnchor="middle"
              >
                {label}
              </SvgText>
            );
          })}

        {/* Max value label */}
        <SvgText x={4} y={12} fontSize={9} fill="#888">
          {dataMax.toFixed(1)} {unit || ''}
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { marginVertical: 4, alignItems: 'center' },
  title: { fontSize: 14, color: '#FFF', fontWeight: '600', marginBottom: 4 },
});