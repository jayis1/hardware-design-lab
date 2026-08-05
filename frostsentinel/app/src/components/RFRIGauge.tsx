// src/components/RFRIGauge.tsx — RFRI radial gauge component
//
// Displays the Radiative Frost Risk Index as a semicircular gauge
// with color-coded zones (green/yellow/orange/red).
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Path, Circle, Text as SvgText } from 'react-native-svg';
import { rfriColor, RFRI_GREEN, RFRI_YELLOW, RFRI_RED } from '../ble/protocol';

interface RFRIGaugeProps {
  rfri: number;       // 0.0 - 1.0
  size?: number;      // pixel diameter
  label?: string;
}

export default function RFRIGauge({ rfri, size = 200, label = 'RFRI' }: RFRIGaugeProps) {
  const radius = size / 2;
  const centerX = radius;
  const centerY = radius;
  const strokeWidth = 16;
  const innerRadius = radius - strokeWidth;

  // Semicircle arc from 180° (left) to 0° (right)
  // Map rfri 0→1 to angle 180°→0°
  const angle = 180 - (rfri * 180);
  const angleRad = (angle * Math.PI) / 180;

  // Needle endpoint
  const needleX = centerX + innerRadius * Math.cos(angleRad);
  const needleY = centerY - innerRadius * Math.sin(angleRad);

  // Arc path for the colored zones
  const arcPath = (startAngle: number, endAngle: number, color: string) => {
    const start = (startAngle * Math.PI) / 180;
    const end = (endAngle * Math.PI) / 180;
    const x1 = centerX + innerRadius * Math.cos(start);
    const y1 = centerY - innerRadius * Math.sin(start);
    const x2 = centerX + innerRadius * Math.cos(end);
    const y2 = centerY - innerRadius * Math.sin(end);
    const largeArc = Math.abs(endAngle - startAngle) > 180 ? 1 : 0;
    return (
      <Path
        d={`M ${x1} ${y1} A ${innerRadius} ${innerRadius} 0 ${largeArc} 0 ${x2} ${y2}`}
        stroke={color}
        strokeWidth={strokeWidth}
        fill="none"
      />
    );
  };

  // Zone boundaries in degrees (180 = left, 0 = right)
  const greenEnd  = 180 - RFRI_GREEN * 180;
  const yellowEnd = 180 - RFRI_YELLOW * 180;
  const redEnd    = 180 - RFRI_RED * 180;

  const currentColor = rfriColor(rfri);
  const percentText = `${(rfri * 100).toFixed(0)}%`;

  return (
    <View style={styles.container}>
      <Svg width={size} height={size / 2 + 40}>
        {/* Background arc (full semicircle) */}
        <Path
          d={`M ${centerX - innerRadius} ${centerY} A ${innerRadius} ${innerRadius} 0 0 1 ${centerX + innerRadius} ${centerY}`}
          stroke="#333"
          strokeWidth={strokeWidth}
          fill="none"
        />
        {/* Green zone */}
        {arcPath(180, greenEnd, '#4CAF50')}
        {/* Yellow zone */}
        {arcPath(greenEnd, yellowEnd, '#FFC107')}
        {/* Orange zone */}
        {arcPath(yellowEnd, redEnd, '#FF9800')}
        {/* Red zone */}
        {arcPath(redEnd, 0, '#F44336')}
        {/* Needle */}
        <Path
          d={`M ${centerX} ${centerY} L ${needleX} ${needleY}`}
          stroke="#fff"
          strokeWidth={3}
          strokeLinecap="round"
        />
        {/* Center pivot */}
        <Circle cx={centerX} cy={centerY} r={6} fill="#fff" />
        {/* Value text */}
        <SvgText
          x={centerX}
          y={centerY + 28}
          fontSize="20"
          fontWeight="bold"
          fill={currentColor}
          textAnchor="middle"
        >
          {percentText}
        </SvgText>
        {/* Label */}
        <SvgText
          x={centerX}
          y={centerY + 45}
          fontSize="11"
          fill="#888"
          textAnchor="middle"
        >
          {label}
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    justifyContent: 'center',
    marginVertical: 10,
  },
});