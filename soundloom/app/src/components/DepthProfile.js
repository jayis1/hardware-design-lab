/**
 * DepthProfile.js — Vertical temperature profile visualisation
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, StyleSheet, Dimensions, Text } from 'react-native';
import { Svg, Rect, Text as SvgText, Line, G, Circle } from 'react-native-svg';

const { width } = Dimensions.get('window');

export function DepthProfile({ temps = [], depths = [] }) {
  const chartW = width - 60;
  const chartH = 200;
  const minTemp = Math.min(...temps) - 2;
  const maxTemp = Math.max(...temps) + 2;
  const tempRange = maxTemp - minTemp || 1;

  const toX = (temp) => 30 + ((temp - minTemp) / tempRange) * (chartW - 50);
  const toY = (depth) => 20 + (depth / Math.max(...depths, 1)) * (chartH - 40);

  return (
    <View style={styles.container}>
      <Svg width={chartW + 40} height={chartH + 20}>
        {/* Depth grid */}
        {depths.map(d => {
          const y = toY(d);
          return (
            <G key={`d-${d}`}>
              <Line x1={30} y1={y} x2={30 + chartW - 20} y2={y} stroke="#E0E0E0" strokeWidth={0.5} />
              <SvgText x={5} y={y + 4} fill="#9E9E9E" fontSize={10}>{d}cm</SvgText>
            </G>
          );
        })}

        {/* Temperature line connecting points */}
        {temps.length > 1 && (
          <Polyline
            points={temps.map((t, i) => `${toX(t)},${toY(depths[i])}`).join(' ')}
            fill="none" stroke="#FF7043" strokeWidth={2}
            transform="translate(0, 0)"
          />
        )}

        {/* Data points */}
        {temps.map((t, i) => {
          const x = toX(t);
          const y = toY(depths[i]);
          return (
            <G key={`pt-${i}`}>
              <Circle cx={x} cy={y} r={5} fill="#FF7043" stroke="#FFFFFF" strokeWidth={1.5} />
              <SvgText x={x + 8} y={y + 4} fill="#424242" fontSize={11} fontWeight="bold">
                {t.toFixed(1)}°C
              </SvgText>
            </G>
          );
        })}

        {/* X-axis label */}
        <SvgText x={30 + (chartW - 50) / 2} y={chartH + 10} fill="#9E9E9E" fontSize={10} textAnchor="middle">
          Temperature (°C)
        </SvgText>
      </Svg>
    </View>
  );
}

import { Polyline } from 'react-native-svg';

const styles = StyleSheet.create({
  container: { alignItems: 'center', padding: 10 },
});