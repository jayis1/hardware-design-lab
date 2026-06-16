// components/TelemetryGauge.js — Reusable telemetry gauge component
// Displays a circular gauge with value, label, unit, and color thresholds
// Used for temperature, pulse frequency, VGA gain, and other metrics

import React, { useMemo } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Svg, { Circle, Text as SvgText, G } from 'react-native-svg';

// Color interpolation between thresholds
function interpolateColor(value, min, max, colorLow, colorMid, colorHigh) {
  if (value <= min) return colorLow;
  if (value >= max) return colorHigh;

  const midPoint = (min + max) / 2;
  if (value <= midPoint) {
    const ratio = (value - min) / (midPoint - min);
    return interpolateHex(colorLow, colorMid, ratio);
  } else {
    const ratio = (value - midPoint) / (max - midPoint);
    return interpolateHex(colorMid, colorHigh, ratio);
  }
}

function interpolateHex(c1, c2, ratio) {
  const r1 = parseInt(c1.slice(1, 3), 16);
  const g1 = parseInt(c1.slice(3, 5), 16);
  const b1 = parseInt(c1.slice(5, 7), 16);
  const r2 = parseInt(c2.slice(1, 3), 16);
  const g2 = parseInt(c2.slice(3, 5), 16);
  const b2 = parseInt(c2.slice(5, 7), 16);

  const r = Math.round(r1 + (r2 - r1) * ratio);
  const g = Math.round(g1 + (g2 - g1) * ratio);
  const b = Math.round(b1 + (b2 - b1) * ratio);

  return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`;
}

export default function TelemetryGauge({
  value = 0,
  min = 0,
  max = 100,
  label = 'Gauge',
  unit = '',
  precision = 1,
  size = 120,
  strokeWidth = 10,
  colorLow = '#00ff88',
  colorMid = '#ffaa00',
  colorHigh = '#ff4444',
  warningThreshold = null,
  criticalThreshold = null,
  style = {},
}) {
  const radius = (size - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;
  const center = size / 2;

  // Calculate arc
  const normalizedValue = Math.min(Math.max(value, min), max);
  const ratio = (normalizedValue - min) / (max - min);
  const arcLength = circumference * ratio * 0.75;  // 270° arc (0.75 of full circle)
  const arcOffset = circumference * 0.125;  // Start at -135° (bottom-left)

  // Determine color based on thresholds
  const gaugeColor = useMemo(() => {
    if (criticalThreshold !== null && value >= criticalThreshold) {
      return colorHigh;
    }
    if (warningThreshold !== null && value >= warningThreshold) {
      return colorMid;
    }
    return interpolateColor(value, min, max, colorLow, colorMid, colorHigh);
  }, [value, min, max, colorLow, colorMid, colorHigh, warningThreshold, criticalThreshold]);

  // Format display value
  const displayValue = Number.isInteger(value) ? value.toString() : value.toFixed(precision);

  // Determine status text
  const statusText = useMemo(() => {
    if (criticalThreshold !== null && value >= criticalThreshold) return 'CRITICAL';
    if (warningThreshold !== null && value >= warningThreshold) return 'WARNING';
    return 'NORMAL';
  }, [value, warningThreshold, criticalThreshold]);

  const statusColor = useMemo(() => {
    if (statusText === 'CRITICAL') return colorHigh;
    if (statusText === 'WARNING') return colorMid;
    return colorLow;
  }, [statusText, colorLow, colorMid, colorHigh]);

  return (
    <View style={[styles.container, { width: size, height: size + 40 }, style]}>
      <Svg width={size} height={size}>
        {/* Background circle */}
        <Circle
          cx={center}
          cy={center}
          r={radius}
          stroke="#2a2a4a"
          strokeWidth={strokeWidth}
          fill="none"
        />
        {/* Value arc */}
        <Circle
          cx={center}
          cy={center}
          r={radius}
          stroke={gaugeColor}
          strokeWidth={strokeWidth}
          fill="none"
          strokeDasharray={`${arcLength} ${circumference}`}
          strokeDashoffset={-arcOffset}
          strokeLinecap="round"
          transform={`rotate(135, ${center}, ${center})`}
        />
        {/* Center value text */}
        <SvgText
          x={center}
          y={center - 4}
          textAnchor="middle"
          fontSize={size * 0.18}
          fontWeight="bold"
          fill="#ffffff"
        >
          {displayValue}
        </SvgText>
        {/* Unit text */}
        <SvgText
          x={center}
          y={center + 16}
          textAnchor="middle"
          fontSize={size * 0.10}
          fill="#a0a0a0"
        >
          {unit}
        </SvgText>
      </Svg>
      {/* Label and status */}
      <View style={styles.labelContainer}>
        <Text style={styles.label}>{label}</Text>
        <Text style={[styles.status, { color: statusColor }]}>{statusText}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  labelContainer: {
    alignItems: 'center',
    marginTop: 4,
  },
  label: {
    color: '#c0c0c0',
    fontSize: 12,
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  status: {
    fontSize: 10,
    fontWeight: 'bold',
    marginTop: 2,
  },
});
