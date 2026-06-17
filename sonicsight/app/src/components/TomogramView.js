/**
 * TomogramView.js — SonicSight Companion
 * SVG-based 2D tomogram renderer with colour map overlay, crosshair, and
 * transducer position markers.
 * @author jayis1
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import Svg, { Rect, Circle, Line, Text as SvgText, Polygon } from 'react-native-svg';

/* Turbo colour map (256-entry, blue→cyan→green→yellow→red) */
const COLOUR_MAP = (() => {
  const cmap = [];
  for (let i = 0; i < 256; i++) {
    const r = Math.min(255, Math.max(0, Math.round(
      255 * (i < 64 ? 0 : i < 128 ? (i - 64) / 64 : i < 192 ? 1 : 1 - (i - 192) / 64)
    )));
    const g = Math.min(255, Math.max(0, Math.round(
      255 * (i < 64 ? i / 64 : i < 192 ? 1 : 1 - (i - 192) / 64)
    )));
    const b = Math.min(255, Math.max(0, Math.round(
      255 * (i < 64 ? 1 : i < 128 ? 1 - (i - 128) / 64 : 0)
    )));
    cmap.push(`rgb(${r},${g},${b})`);
  }
  return cmap;
})();

const TomogramView = ({ data, width, height, crosshair, velMin, velMax }) => {
  if (!data || !width || !height) {
    return (
      <View style={[styles.placeholder, { width, height }]}>
        <SvgText x={width / 2} y={height / 2} fill="#555" fontSize={12}
          textAnchor="middle">No data</SvgText>
      </View>
    );
  }

  const gridSize = 64;
  const cellW = width / gridSize;
  const cellH = height / gridSize;

  const pixels = [];
  for (let y = 0; y < gridSize; y++) {
    for (let x = 0; x < gridSize; x++) {
      const vel = data[y * gridSize + x];
      if (vel > 0) {
        /* Map velocity 500–4000 to colour index 0–255 */
        let idx = Math.round((vel - 500) / 13.73);
        idx = Math.max(0, Math.min(255, idx));
        pixels.push(
          <Rect key={`${x}-${y}`}
                x={x * cellW} y={y * cellH}
                width={cellW + 0.5} height={cellH + 0.5}
                fill={COLOUR_MAP[idx]}
                opacity={0.9} />
        );
      }
    }
  }

  /* Transducer positions (32 equally spaced around circle) */
  const cx = width / 2, cy = height / 2;
  const radius = Math.min(cx, cy) * 0.85;
  const sensors = [];
  for (let i = 0; i < 32; i++) {
    const angle = (i / 32) * 2 * Math.PI - Math.PI / 2;
    const sx = cx + radius * Math.cos(angle);
    const sy = cy + radius * Math.sin(angle);
    sensors.push(
      <Circle key={`s${i}`} cx={sx} cy={sy} r={3} fill="#fff" stroke="#000" strokeWidth={0.5} />
    );
  }

  /* Crosshair */
  const ch = crosshair || { x: width / 2, y: height / 2 };

  return (
    <Svg width={width} height={height}>
      {pixels}
      {sensors}
      {/* Crosshair lines */}
      <Line x1={0} y1={ch.y} x2={width} y2={ch.y} stroke="#fff" strokeWidth={0.5}
            strokeDasharray="2,2" opacity={0.6} />
      <Line x1={ch.x} y1={0} x2={ch.x} y2={height} stroke="#fff" strokeWidth={0.5}
            strokeDasharray="2,2" opacity={0.6} />
      <Circle cx={ch.x} cy={ch.y} r={5} fill="none" stroke="#ffe" strokeWidth={1.5} />

      {/* Colour bar */}
      <Rect x={width - 14} y={8} width={6} height={height - 16} fill="none"
            stroke="#333" strokeWidth={0.5} />
      {Array.from({ length: 64 }, (_, i) => (
        <Rect key={`cb${i}`} x={width - 14}
              y={8 + (i / 64) * (height - 16)}
              width={6} height={(height - 16) / 64}
              fill={COLOUR_MAP[Math.round((1 - i / 64) * 255)]}
              opacity={0.8} />
      ))}
    </Svg>
  );
};

const styles = StyleSheet.create({
  placeholder: {
    backgroundColor: '#000',
    justifyContent: 'center',
    alignItems: 'center',
  },
});

export default TomogramView;