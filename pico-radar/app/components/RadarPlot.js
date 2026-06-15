/**
 * components/RadarPlot.js — Reusable 2D radar point cloud visualization
 *
 * Renders detected targets on a top-down range/angle polar plot.
 * Points are color-coded by radial velocity:
 *   Blue = approaching (negative Doppler)
 *   Red = receding (positive Doppler)
 *   Green = stationary (near-zero Doppler)
 */

import React, { useMemo } from 'react';
import { View, StyleSheet } from 'react-native';
import { Svg, Circle, Line, G, Text as SvgText, Polygon } from 'react-native-svg';

const COLORS = {
  grid: '#333333',
  axis: '#555555',
  label: '#888888',
  approaching: '#007AFF',
  receding: '#FF3B30',
  stationary: '#4CD964',
  background: '#0A0A0A',
};

function velocityToColor(velocity) {
  if (Math.abs(velocity) < 0.2) return COLORS.stationary;
  if (velocity < 0) return COLORS.approaching;
  return COLORS.receding;
}

function polarToCartesian(range, angle, maxRange, size) {
  const cx = size / 2;
  const cy = size / 2;
  const r = (range / maxRange) * (size / 2 - 20);
  const rad = ((angle - 90) * Math.PI) / 180;
  return {
    x: cx + r * Math.cos(rad),
    y: cy + r * Math.sin(rad),
  };
}

export default function RadarPlot({ points = [], size = 300, maxRange = 12, showGrid = true }) {
  const cx = size / 2;
  const cy = size / 2;
  const maxR = size / 2 - 20;

  // Generate range rings
  const rangeRings = useMemo(() => {
    const rings = [];
    const step = maxRange <= 12 ? 2 : 10;
    for (let r = step; r <= maxRange; r += step) {
      const ringR = (r / maxRange) * maxR;
      rings.push({ range: r, radius: ringR });
    }
    return rings;
  }, [maxRange, maxR]);

  // Generate angle lines (every 15°)
  const angleLines = useMemo(() => {
    const lines = [];
    for (let a = -45; a <= 45; a += 15) {
      const rad = ((a - 90) * Math.PI) / 180;
      lines.push({
        angle: a,
        x2: cx + maxR * Math.cos(rad),
        y2: cy + maxR * Math.sin(rad),
      });
    }
    return lines;
  }, [cx, cy, maxR]);

  return (
    <View style={[styles.container, { width: size, height: size }]}>
      <Svg width={size} height={size}>
        {/* Background */}
        <Circle cx={cx} cy={cy} r={maxR + 5} fill={COLORS.background} />

        {/* Range rings */}
        {showGrid &&
          rangeRings.map((ring, i) => (
            <G key={`ring-${i}`}>
              <Circle
                cx={cx}
                cy={cy}
                r={ring.radius}
                fill="none"
                stroke={COLORS.grid}
                strokeWidth={0.5}
              />
              <SvgText
                x={cx + 4}
                y={cy - ring.radius + 4}
                fill={COLORS.label}
                fontSize={9}
              >
                {ring.range}m
              </SvgText>
            </G>
          ))}

        {/* Angle lines */}
        {showGrid &&
          angleLines.map((line, i) => (
            <G key={`angle-${i}`}>
              <Line
                x1={cx}
                y1={cy}
                x2={line.x2}
                y2={line.y2}
                stroke={COLORS.grid}
                strokeWidth={0.5}
              />
              <SvgText
                x={line.x2 + (line.x2 > cx ? 4 : -20)}
                y={line.y2 + 4}
                fill={COLORS.label}
                fontSize={8}
              >
                {line.angle}°
              </SvgText>
            </G>
          ))}

        {/* Center crosshair */}
        <Line x1={cx - 8} y1={cy} x2={cx + 8} y2={cy} stroke={COLORS.axis} strokeWidth={1} />
        <Line x1={cx} y1={cy - 8} x2={cx} y2={cy + 8} stroke={COLORS.axis} strokeWidth={1} />

        {/* Sensor FOV arc (±45°) */}
        <Circle cx={cx} cy={cy} r={3} fill={COLORS.axis} />

        {/* Detected points */}
        {points.map((point, i) => {
          if (!point.range || !point.angle) return null;
          const { x, y } = polarToCartesian(point.range, point.angle, maxRange, size);
          const color = velocityToColor(point.velocity || 0);
          const radius = Math.max(2, Math.min(6, point.intensity ? point.intensity * 5 : 3));

          return (
            <Circle
              key={`pt-${i}`}
              cx={x}
              cy={y}
              r={radius}
              fill={color}
              opacity={0.85}
            />
          );
        })}

        {/* Legend */}
        <G transform={`translate(${size - 80}, 10)`}>
          <Circle cx={0} cy={0} r={4} fill={COLORS.approaching} />
          <SvgText x={8} y={3} fill={COLORS.label} fontSize={9}>Approaching</SvgText>
          <Circle cx={0} cy={14} r={4} fill={COLORS.stationary} />
          <SvgText x={8} y={17} fill={COLORS.label} fontSize={9}>Stationary</SvgText>
          <Circle cx={0} cy={28} r={4} fill={COLORS.receding} />
          <SvgText x={8} y={31} fill={COLORS.label} fontSize={9}>Receding</SvgText>
        </G>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignSelf: 'center',
  },
});