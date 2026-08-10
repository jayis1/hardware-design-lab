/**
 * @file    DiveProfileChart.js
 * @brief   SVG-based depth and current profile chart for dive replay.
 *          Renders depth (line) and current speed (color-coded bars)
 *          over the dive timeline.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import Svg, {
  Polyline, Line, Rect, Text as SvgText, Defs, LinearGradient, Stop,
} from 'react-native-svg';

export default function DiveProfileChart({ data, units = 'metric' }) {
  const screenWidth = Dimensions.get('window').width - 32;
  const chartHeight = 160;
  const padding = { top: 10, right: 10, bottom: 20, left: 35 };
  const plotWidth = screenWidth - padding.left - padding.right;
  const plotHeight = chartHeight - padding.top - padding.bottom;

  if (!data || data.length === 0) {
    return (
      <View style={styles.emptyContainer}>
        <Text style={styles.emptyText}>No data to display</Text>
      </View>
    );
  }

  // Compute scales
  const maxDepth = Math.max(...data.map(p => p.depth), 1);
  const maxSpeed = Math.max(...data.map(p =>
    Math.sqrt(p.vx ** 2 + p.vy ** 2 + p.vz ** 2)
  ), 0.5);
  const duration = data[data.length - 1].timestamp - data[0].timestamp || 1;

  // Scale functions
  const xScale = (timestamp) => {
    const t = timestamp - data[0].timestamp;
    return padding.left + (t / duration) * plotWidth;
  };

  const yScaleDepth = (depth) => {
    // Depth increases downward (like a depth gauge)
    return padding.top + (depth / maxDepth) * plotHeight;
  };

  // Build depth polyline points
  const depthPoints = data.map(p =>
    `${xScale(p.timestamp)},${yScaleDepth(p.depth)}`
  ).join(' ');

  // Build speed bars (as background fill)
  const speedBars = data.map((p, i) => {
    const speed = Math.sqrt(p.vx ** 2 + p.vy ** 2 + p.vz ** 2);
    const x = xScale(p.timestamp);
    const barWidth = Math.max(plotWidth / data.length - 1, 1);
    const speedRatio = Math.min(speed / maxSpeed, 1);

    // Color: green (slow) → yellow → red (fast)
    const r = Math.round(255 * speedRatio);
    const g = Math.round(255 * (1 - speedRatio * 0.5));
    const b = 0;
    const color = `rgb(${r}, ${g}, ${b})`;

    return (
      <Rect
        key={`bar-${i}`}
        x={x - barWidth / 2}
        y={padding.top}
        width={barWidth}
        height={plotHeight}
        fill={color}
        opacity={0.15}
      />
    );
  });

  // Y-axis labels (depth)
  const depthLabels = [];
  const numLabels = 4;
  for (let i = 0; i <= numLabels; i++) {
    const depth = (maxDepth / numLabels) * i;
    const y = yScaleDepth(depth);
    const label = units === 'imperial'
      ? `${(depth * 3.28).toFixed(0)}ft`
      : `${depth.toFixed(0)}m`;
    depthLabels.push(
      <SvgText key={`depth-label-${i}`} x={padding.left - 4} y={y + 3}
        fontSize={8} fill="#808080" textAnchor="end">
        {label}
      </SvgText>
    );
  }

  // X-axis labels (time)
  const timeLabels = [];
  const numTimeLabels = 5;
  for (let i = 0; i <= numTimeLabels; i++) {
    const t = (duration / numTimeLabels) * i;
    const x = padding.left + (t / duration) * plotWidth;
    const mins = Math.floor(t / 60);
    const secs = Math.floor(t % 60);
    const label = `${mins}:${secs.toString().padStart(2, '0')}`;
    timeLabels.push(
      <SvgText key={`time-label-${i}`} x={x} y={chartHeight - 5}
        fontSize={8} fill="#808080" textAnchor="middle">
        {label}
      </SvgText>
    );
  }

  return (
    <View style={styles.container}>
      <Svg width={screenWidth} height={chartHeight}>
        {/* Background speed bars */}
        {speedBars}

        {/* Y-axis (depth) */}
        <Line
          x1={padding.left} y1={padding.top}
          x2={padding.left} y2={padding.top + plotHeight}
          stroke="#808080" strokeWidth={1}
        />

        {/* X-axis (time) */}
        <Line
          x1={padding.left} y1={padding.top + plotHeight}
          x2={padding.left + plotWidth} y2={padding.top + plotHeight}
          stroke="#808080" strokeWidth={1}
        />

        {/* Depth line */}
        <Polyline
          points={depthPoints}
          fill="none"
          stroke="#0080FF"
          strokeWidth={2}
        />

        {/* Axis labels */}
        {depthLabels}
        {timeLabels}

        {/* Legend */}
        <SvgText x={screenWidth - 80} y={12} fontSize={8} fill="#0080FF">
          Depth
        </SvgText>
        <Rect x={screenWidth - 80} y={16} width={8} height={8}
          fill="rgb(0, 255, 0)" opacity={0.3} />
        <SvgText x={screenWidth - 68} y={22} fontSize={8} fill="#808080">
          Current
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 8,
    marginBottom: 16,
  },
  emptyContainer: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 40,
    alignItems: 'center',
  },
  emptyText: {
    fontSize: 14,
    color: '#808080',
  },
});