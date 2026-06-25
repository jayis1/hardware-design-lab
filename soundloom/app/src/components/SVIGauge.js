/**
 * SVIGauge.js — Circular gauge component for Soil Vitality Index
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Svg, Circle, G, Text as SvgText, Path } from 'react-native-svg';

function sviColor(svi) {
  if (svi >= 75) return '#4CAF50';
  if (svi >= 50) return '#FFEB3B';
  if (svi >= 30) return '#FF9800';
  return '#F44336';
}

export function SVIGauge({ value = 0, size = 160 }) {
  const stroke = 12;
  const radius = (size - stroke) / 2;
  const circumference = 2 * Math.PI * radius;
  const arcFraction = value / 100;
  const arcLength = circumference * 0.75 * arcFraction; // 270° gauge
  const color = sviColor(value);
  const cx = size / 2;
  const cy = size / 2;

  // Arc background (270° from 135° to 45° clockwise)
  const startAngle = 135;
  const endAngle = 405; // 135 + 270

  // Create arc path
  const polarToCartesian = (angle, r) => {
    const rad = (angle - 90) * Math.PI / 180;
    return { x: cx + r * Math.cos(rad), y: cy + r * Math.sin(rad) };
  };

  const bgStart = polarToCartesian(startAngle, radius);
  const bgEnd = polarToCartesian(endAngle, radius);
  const bgPath = `M ${bgStart.x} ${bgStart.y} A ${radius} ${radius} 0 1 1 ${bgEnd.x} ${bgEnd.y}`;

  // Value arc
  const valAngle = startAngle + 270 * arcFraction;
  const valEnd = polarToCartesian(valAngle, radius);
  const valPath = `M ${bgStart.x} ${bgStart.y} A ${radius} ${radius} 0 ${arcFraction > 0.5 ? 1 : 0} 1 ${valEnd.x} ${valEnd.y}`;

  return (
    <View style={[styles.container, { width: size, height: size }]}>
      <Svg width={size} height={size}>
        {/* Background arc */}
        <Path d={bgPath} stroke="#E0E0E0" strokeWidth={stroke} fill="none" strokeLinecap="round" />
        {/* Value arc */}
        <Path d={valPath} stroke={color} strokeWidth={stroke} fill="none" strokeLinecap="round" />
        {/* Value text */}
        <SvgText x={cx} y={cy + 5} fill="#424242" fontSize={size * 0.22} fontWeight="bold" textAnchor="middle">
          {Math.round(value)}
        </SvgText>
        <SvgText x={cx} y={cy + size * 0.18} fill="#9E9E9E" fontSize={size * 0.08} textAnchor="middle">
          / 100
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', justifyContent: 'center' },
});