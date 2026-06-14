/**
 * WaveformView.js — Reusable waveform rendering component
 * Draws oscilloscope traces on an SVG canvas with grid, cursors, and trigger marker
 */

import React, { useRef, useEffect, useCallback } from 'react';
import { View, StyleSheet, Dimensions } from 'react-native';
import Svg, { Line, Rect, Text, G, Path, Defs, ClipPath } from 'react-native-svg';

const { width: SCREEN_WIDTH } = Dimensions.get('window');
const WAVEFORM_HEIGHT = 280;
const GRID_DIVISIONS_X = 10;  // 10 horizontal divisions
const GRID_DIVISIONS_Y = 8;   // 8 vertical divisions
const GRID_SUBDIVISIONS = 5;   // 5 subdivisions per major division

export default function WaveformView({
  channelData,
  colors,
  vdiv,
  tdiv,
  triggerLevel,
  triggerChannel,
  running,
}) {
  const viewWidth = SCREEN_WIDTH - 16;  // With margins

  // Draw grid lines
  const renderGrid = () => {
    const elements = [];

    // Major grid lines
    for (let i = 0; i <= GRID_DIVISIONS_X; i++) {
      const x = (i / GRID_DIVISIONS_X) * viewWidth;
      elements.push(
        <Line
          key={`vg-${i}`}
          x1={x} y1={0}
          x2={x} y2={WAVEFORM_HEIGHT}
          stroke="#333366"
          strokeWidth={i === GRID_DIVISIONS_X / 2 ? 1.5 : 0.5}
        />
      );
    }
    for (let i = 0; i <= GRID_DIVISIONS_Y; i++) {
      const y = (i / GRID_DIVISIONS_Y) * WAVEFORM_HEIGHT;
      elements.push(
        <Line
          key={`hg-${i}`}
          x1={0} y1={y}
          x2={viewWidth} y2={y}
          stroke="#333366"
          strokeWidth={i === GRID_DIVISIONS_Y / 2 ? 1.5 : 0.5}
        />
      );
    }

    // Subdivision dots
    for (let i = 0; i < GRID_DIVISIONS_X; i++) {
      for (let j = 1; j < GRID_SUBDIVISIONS; j++) {
        const x = ((i + j / GRID_SUBDIVISIONS) / GRID_DIVISIONS_X) * viewWidth;
        elements.push(
          <Rect key={`vd-${i}-${j}`} x={x - 0.5} y={WAVEFORM_HEIGHT / 2 - 1} width={1} height={2} fill="#333366" />
        );
      }
    }
    for (let i = 0; i < GRID_DIVISIONS_Y; i++) {
      for (let j = 1; j < GRID_SUBDIVISIONS; j++) {
        const y = ((i + j / GRID_SUBDIVISIONS) / GRID_DIVISIONS_Y) * WAVEFORM_HEIGHT;
        elements.push(
          <Rect key={`hd-${i}-${j}`} x={viewWidth / 2 - 1} y={y - 0.5} width={2} height={1} fill="#333366" />
        );
      }
    }

    return elements;
  };

  // Render a single channel waveform
  const renderChannel = (data, color, isDigital = false) => {
    if (!data || data.length === 0) return null;

    const points = [];
    const step = Math.max(1, Math.floor(data.length / viewWidth));

    for (let i = 0; i < data.length; i += step) {
      const x = (i / data.length) * viewWidth;
      let y;
      if (isDigital) {
        // Digital channels: 0 = bottom, 1 = top
        y = data[i] > 128 ? 10 : WAVEFORM_HEIGHT - 10;
      } else {
        // Analog channels: 0 = top (max voltage), 255 = bottom
        y = (data[i] / 255) * WAVEFORM_HEIGHT;
      }
      points.push(`${x},${y}`);
    }

    if (points.length < 2) return null;

    return (
      <Path
        d={`M ${points[0]} L ${points.slice(1).join(' L ')}`}
        stroke={color}
        strokeWidth={isDigital ? 2 : 1.5}
        fill="none"
        opacity={isDigital ? 0.8 : 0.9}
      />
    );
  };

  // Render trigger level marker
  const renderTriggerLevel = () => {
    const y = (triggerLevel / 255) * WAVEFORM_HEIGHT;
    return (
      <G>
        <Line
          x1={0} y1={y}
          x2={viewWidth} y2={y}
          stroke="#FF6600"
          strokeWidth={1}
          strokeDasharray="8 4"
          opacity={0.8}
        />
        {/* Trigger arrow */}
        <Path
          d={`M 0,${y} L 10,${y - 5} L 10,${y + 5} Z`}
          fill="#FF6600"
        />
      </G>
    );
  };

  // Channel labels
  const renderLabels = () => {
    const labels = [
      { key: 'ch1', label: '1', color: colors.ch1, offset: 0 },
      { key: 'ch2', label: '2', color: colors.ch2, offset: 1 },
      { key: 'ch3', label: '3', color: colors.ch3, offset: 2 },
      { key: 'ch4', label: '4', color: colors.ch4, offset: 3 },
    ];
    return labels.map((l) => (
      <G key={l.key}>
        <Rect
          x={4} y={4 + l.offset * 18}
          width={20} height={14}
          fill={l.color}
          rx={2}
          opacity={0.8}
        />
        <Text
          x={14} y={14 + l.offset * 18}
          fill="#000000"
          fontSize={10}
          fontWeight="bold"
          textAnchor="middle"
        >
          {l.label}
        </Text>
      </G>
    ));
  };

  return (
    <View style={styles.container}>
      <Svg width={viewWidth} height={WAVEFORM_HEIGHT}>
        {/* Background */}
        <Rect x={0} y={0} width={viewWidth} height={WAVEFORM_HEIGHT} fill="#0a0a1a" />

        {/* Grid */}
        {renderGrid()}

        {/* Waveform traces */}
        {renderChannel(channelData.ch1, colors.ch1)}
        {renderChannel(channelData.ch2, colors.ch2)}
        {renderChannel(channelData.ch3, colors.ch3)}
        {renderChannel(channelData.ch4, colors.ch4)}
        {renderChannel(channelData.d1, colors.d1, true)}
        {renderChannel(channelData.d2, colors.d2, true)}

        {/* Trigger level */}
        {renderTriggerLevel()}

        {/* Channel labels */}
        {renderLabels()}

        {/* Status indicator */}
        {running && (
          <G>
            <Circle cx={viewWidth - 10} cy={10} r={4} fill="#44FF44" />
          </G>
        )}
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#0a0a1a',
    borderRadius: 4,
    overflow: 'hidden',
  },
});