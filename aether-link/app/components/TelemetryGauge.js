// TelemetryGauge.js — Circular Gauge Component for Aether-Link App
// Displays temperature, power, or other metrics as a circular gauge
// with color-coded warning/critical zones and animated needle.

import React, { useEffect, useRef } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Animated,
} from 'react-native';
import Svg, { Circle, Path, G, Text as SvgText, Defs, LinearGradient, Stop } from 'react-native-svg';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// ============================================================================
// Animated Gauge Component
// ============================================================================

export default function TelemetryGauge({
  label,
  value,
  unit = '',
  min = 0,
  max = 100,
  warnThreshold = 80,
  critThreshold = 90,
  icon = 'chip',
  size = 110,
}) {
  const animatedValue = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    Animated.spring(animatedValue, {
      toValue: value,
      tension: 40,
      friction: 7,
      useNativeDriver: false,
    }).start();
  }, [value]);

  // Clamp value
  const clampedValue = Math.min(max, Math.max(min, value));
  const percentage = (clampedValue - min) / (max - min);

  // Determine color based on thresholds
  let gaugeColor = '#10B981';  // Green (normal)
  if (clampedValue >= critThreshold) {
    gaugeColor = '#EF4444';  // Red (critical)
  } else if (clampedValue >= warnThreshold) {
    gaugeColor = '#F59E0B';  // Amber (warning)
  }

  // SVG parameters
  const center = size / 2;
  const radius = (size / 2) - 12;
  const strokeWidth = 8;
  const arcRadius = radius - strokeWidth / 2;
  const circumference = Math.PI * arcRadius;

  // Arc angles (270° arc from 135° to 405°)
  const startAngle = 135;
  const endAngle = 405;
  const totalAngle = endAngle - startAngle;
  const sweepAngle = totalAngle * percentage;

  // Calculate arc path
  function polarToCartesian(cx, cy, r, angleDeg) {
    const angleRad = (angleDeg - 90) * Math.PI / 180;
    return {
      x: cx + r * Math.cos(angleRad),
      y: cy + r * Math.sin(angleRad),
    };
  }

  function describeArc(cx, cy, r, startDeg, endDeg) {
    const start = polarToCartesian(cx, cy, r, endDeg);
    const end = polarToCartesian(cx, cy, r, startDeg);
    const largeArcFlag = endDeg - startDeg <= 180 ? '0' : '1';
    return `M ${start.x} ${start.y} A ${r} ${r} 0 ${largeArcFlag} 0 ${end.x} ${end.y}`;
  }

  // Background arc (full 270°)
  const bgArcPath = describeArc(center, center, arcRadius, startAngle, endAngle);
  // Value arc
  const valueArcPath = describeArc(center, center, arcRadius, startAngle, startAngle + sweepAngle);

  // Warning zone marker
  const warnAngle = startAngle + totalAngle * ((warnThreshold - min) / (max - min));
  const warnPoint = polarToCartesian(center, center, arcRadius + 4, warnAngle);

  // Critical zone marker
  const critAngle = startAngle + totalAngle * ((critThreshold - min) / (max - min));
  const critPoint = polarToCartesian(center, center, arcRadius + 4, critAngle);

  return (
    <View style={[styles.container, { width: size }]}>
      <Svg width={size} height={size}>
        <Defs>
          <LinearGradient id={`gaugeGrad-${label}`} x1="0%" y1="0%" x2="100%" y2="0%">
            <Stop offset="0%" stopColor="#10B981" stopOpacity="1" />
            <Stop offset={`${(warnThreshold - min) / (max - min) * 100}%`} stopColor="#10B981" stopOpacity="1" />
            <Stop offset={`${(warnThreshold - min) / (max - min) * 100}%`} stopColor="#F59E0B" stopOpacity="1" />
            <Stop offset={`${(critThreshold - min) / (max - min) * 100}%`} stopColor="#F59E0B" stopOpacity="1" />
            <Stop offset={`${(critThreshold - min) / (max - min) * 100}%`} stopColor="#EF4444" stopOpacity="1" />
            <Stop offset="100%" stopColor="#EF4444" stopOpacity="1" />
          </LinearGradient>
        </Defs>

        {/* Background arc */}
        <Path
          d={bgArcPath}
          stroke="#1F2937"
          strokeWidth={strokeWidth}
          fill="none"
          strokeLinecap="round"
        />

        {/* Value arc */}
        <Path
          d={valueArcPath}
          stroke={gaugeColor}
          strokeWidth={strokeWidth}
          fill="none"
          strokeLinecap="round"
        />

        {/* Warning threshold tick */}
        <Circle
          cx={warnPoint.x}
          cy={warnPoint.y}
          r={2.5}
          fill="#F59E0B"
        />

        {/* Critical threshold tick */}
        <Circle
          cx={critPoint.x}
          cy={critPoint.y}
          r={2.5}
          fill="#EF4444"
        />

        {/* Min label */}
        <SvgText
          x={polarToCartesian(center, center, arcRadius + 14, startAngle).x}
          y={polarToCartesian(center, center, arcRadius + 14, startAngle).y}
          fontSize={9}
          fill="#6B7280"
          textAnchor="middle"
          alignmentBaseline="middle"
        >
          {min}
        </SvgText>

        {/* Max label */}
        <SvgText
          x={polarToCartesian(center, center, arcRadius + 14, endAngle).x}
          y={polarToCartesian(center, center, arcRadius + 14, endAngle).y}
          fontSize={9}
          fill="#6B7280"
          textAnchor="middle"
          alignmentBaseline="middle"
        >
          {max}
        </SvgText>
      </Svg>

      {/* Center value display */}
      <View style={[styles.centerDisplay, { width: size - 40, height: size - 40, top: 20, left: 20 }]}>
        <Icon name={icon} size={16} color={gaugeColor} />
        <Text style={[styles.valueText, { color: gaugeColor }]}>
          {clampedValue.toFixed(1)}
        </Text>
        <Text style={styles.unitText}>{unit}</Text>
      </View>

      {/* Label below gauge */}
      <Text style={styles.label}>{label}</Text>
    </View>
  );
}

// ============================================================================
// Styles
// ============================================================================

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    marginHorizontal: 4,
  },
  centerDisplay: {
    position: 'absolute',
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#0A0E1A',
    borderRadius: 999,
  },
  valueText: {
    fontSize: 20,
    fontWeight: '700',
    marginTop: 2,
  },
  unitText: {
    fontSize: 11,
    color: '#6B7280',
    marginTop: -2,
  },
  label: {
    fontSize: 12,
    fontWeight: '600',
    color: '#9CA3AF',
    marginTop: 4,
  },
});
