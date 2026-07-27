/**
 * SoHGauge.tsx — Circular State-of-Health gauge widget.
 *
 * Renders a circular gauge showing the SoH score (0-100) with color
 * coding based on the quality verdict.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Circle, Path, Text as SvgText } from 'react-native-svg';

interface SoHGaugeProps {
  score: number;
  size?: number;
}

function getColor(score: number): string {
  if (score >= 85) return '#00e676';
  if (score >= 70) return '#76ff03';
  if (score >= 50) return '#ffeb3b';
  if (score >= 30) return '#ff9800';
  return '#f44336';
}

export default function SoHGauge({ score, size = 180 }: SoHGaugeProps) {
  const strokeWidth = 14;
  const radius = (size - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;
  const center = size / 2;

  // Arc from -220° to +40° (270° sweep)
  const startAngle = -220;
  const sweepAngle = 270;
  const fillAngle = (score / 100) * sweepAngle;

  const polarToCartesian = (angleDeg: number) => {
    const rad = (angleDeg * Math.PI) / 180;
    return {
      x: center + radius * Math.cos(rad),
      y: center + radius * Math.sin(rad),
    };
  };

  const start = polarToCartesian(startAngle);
  const end = polarToCartesian(startAngle + fillAngle);
  const bgEnd = polarToCartesian(startAngle + sweepAngle);

  const fillArc = `M ${start.x} ${start.y} A ${radius} ${radius} 0 ${
    fillAngle > 180 ? 1 : 0
  } 1 ${end.x} ${end.y}`;

  const bgArc = `M ${start.x} ${start.y} A ${radius} ${radius} 0 1 1 ${bgEnd.x} ${bgEnd.y}`;

  const color = getColor(score);

  return (
    <View style={styles.container}>
      <Svg width={size} height={size}>
        {/* Background arc */}
        <Path d={bgArc} fill="none" stroke="#222244" strokeWidth={strokeWidth}
          strokeLinecap="round" />

        {/* Fill arc */}
        <Path d={fillArc} fill="none" stroke={color} strokeWidth={strokeWidth}
          strokeLinecap="round" />

        {/* Score text */}
        <SvgText
          x={center}
          y={center - 5}
          fill={color}
          fontSize={size * 0.22}
          fontWeight="bold"
          textAnchor="middle"
        >
          {score}
        </SvgText>
        <SvgText
          x={center}
          y={center + 18}
          fill="#8888aa"
          fontSize={size * 0.08}
          textAnchor="middle"
        >
          SoH %
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    justifyContent: 'center',
    marginVertical: 16,
  },
});