/**
 * Sparkline — Mini trend line chart
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import Svg, { Polyline, Circle } from 'react-native-svg';

export default function Sparkline({ data, width, height, color }: {
  data: number[];
  width: number;
  height: number;
  color?: string;
}) {
  if (!data || data.length < 2) {
    return <View style={[styles.container, { width, height }]} />;
  }

  const min = Math.min(...data);
  const max = Math.max(...data);
  const range = max - min || 1;
  const stepX = width / (data.length - 1);

  const points = data.map((val, i) => {
    const x = i * stepX;
    const y = height - ((val - min) / range) * (height - 4) - 2;
    return `${x},${y}`;
  }).join(' ');

  const strokeColor = color || '#E8A317';

  return (
    <View style={[styles.container, { width, height }]}>
      <Svg width={width} height={height}>
        <Polyline
          points={points}
          fill="none"
          stroke={strokeColor}
          strokeWidth={1.5}
          strokeLinecap="round"
          strokeLinejoin="round"
        />
        <Circle
          cx={(data.length - 1) * stepX}
          cy={height - ((data[data.length - 1] - min) / range) * (height - 4) - 2}
          r={2}
          fill={strokeColor}
        />
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { justifyContent: 'center', alignItems: 'center' },
});