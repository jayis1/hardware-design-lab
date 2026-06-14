// ============================================================================
// components/CircularGauge.js — Reusable circular gauge component (SVG)
// ============================================================================
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Circle, Text as SvgText } from 'react-native-svg';

export default function CircularGauge({ value, maxValue, unit, label, color = '#FF6B35', size = 120 }) {
  const strokeWidth = 8;
  const radius = (size - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;
  const progress = Math.min(value / maxValue, 1.0);
  const dashOffset = circumference * (1 - progress);
  const cx = size / 2;
  const cy = size / 2;

  return (
    <View style={styles.container}>
      <Svg width={size} height={size}>
        {/* Background track */}
        <Circle
          cx={cx}
          cy={cy}
          r={radius}
          stroke="#333"
          strokeWidth={strokeWidth}
          fill="none"
        />
        {/* Progress arc */}
        <Circle
          cx={cx}
          cy={cy}
          r={radius}
          stroke={color}
          strokeWidth={strokeWidth}
          fill="none"
          strokeDasharray={circumference}
          strokeDashoffset={dashOffset}
          strokeLinecap="round"
          rotation="-90"
          origin={`${cx}, ${cy}`}
        />
        {/* Value text */}
        <SvgText
          x={cx}
          y={cy - 6}
          textAnchor="middle"
          fill="#FFF"
          fontSize="20"
          fontWeight="bold"
        >
          {typeof value === 'number' ? value.toFixed(1) : value}
        </SvgText>
        {/* Unit text */}
        <SvgText
          x={cx}
          y={cy + 14}
          textAnchor="middle"
          fill="#888"
          fontSize="12"
        >
          {unit}
        </SvgText>
      </Svg>
      <Text style={styles.label}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', marginHorizontal: 8 },
  label: { color: '#AAA', fontSize: 11, marginTop: 4, textAlign: 'center' },
});