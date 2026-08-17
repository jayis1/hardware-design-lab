// ============================================================
// LignoScan App — TomogramView Component
// Reusable SVG-based tomogram renderer
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import Svg, { Circle, Path, G, Text as SvgText } from 'react-native-svg';
import { DECAY_COLORS, DECAY_LABELS } from '../utils/protocol';

const SCREEN_WIDTH = Dimensions.get('window').width;

export default function TomogramView({ tomogram, size, showSensors = true }) {
  const tSize = size || Math.min(SCREEN_WIDTH - 80, 300);

  if (!tomogram || !tomogram.cells) {
    return (
      <View style={[styles.empty, { width: tSize, height: tSize }]}>
        <Text style={styles.emptyText}>No data</Text>
      </View>
    );
  }

  const center = tSize / 2;
  const radius = tSize / 2 - 25;
  const nAngular = 16;
  const nRadial = Math.ceil(tomogram.nCells / nAngular);

  const cells = [];
  for (let i = 0; i < tomogram.cells.length; i++) {
    const cell = tomogram.cells[i];
    const aIdx = Math.floor(i / nRadial);
    const rIdx = i % nRadial;

    const rInner = (rIdx / nRadial) * radius;
    const rOuter = ((rIdx + 1) / nRadial) * radius;
    const angleStart = (aIdx / nAngular) * 2 * Math.PI - Math.PI / 2;
    const angleEnd = ((aIdx + 1) / nAngular) * 2 * Math.PI - Math.PI / 2;

    const x1 = center + rInner * Math.cos(angleStart);
    const y1 = center + rInner * Math.sin(angleStart);
    const x2 = center + rOuter * Math.cos(angleStart);
    const y2 = center + rOuter * Math.sin(angleStart);
    const x3 = center + rOuter * Math.cos(angleEnd);
    const y3 = center + rOuter * Math.sin(angleEnd);
    const x4 = center + rInner * Math.cos(angleEnd);
    const y4 = center + rInner * Math.sin(angleEnd);

    const largeArc = (angleEnd - angleStart) > Math.PI ? 1 : 0;
    const color = DECAY_COLORS[cell.classification] || '#888';

    const pathData = `M ${x1} ${y1} L ${x2} ${y2} A ${rOuter} ${rOuter} 0 ${largeArc} 1 ${x3} ${y3} L ${x4} ${y4} A ${rInner} ${rInner} 0 ${largeArc} 0 ${x1} ${y1} Z`;

    cells.push(
      <Path
        key={`cell-${i}`}
        d={pathData}
        fill={color}
        stroke="#fff"
        strokeWidth="0.5"
      />
    );
  }

  // Sensor markers
  const sensors = [];
  if (showSensors) {
    const nSensors = 12;
    for (let i = 0; i < nSensors; i++) {
      const angle = (i / nSensors) * 2 * Math.PI - Math.PI / 2;
      const x = center + (radius + 12) * Math.cos(angle);
      const y = center + (radius + 12) * Math.sin(angle);
      sensors.push(
        <G key={`s-${i}`}>
          <Circle cx={x} cy={y} r="5" fill="#1a3a1a" />
          <SvgText x={x} y={y + 2} fontSize="6" fill="#fff" textAnchor="middle">
            {i}
          </SvgText>
        </G>
      );
    }
  }

  return (
    <View style={styles.container}>
      <Svg width={tSize} height={tSize}>
        <Circle cx={center} cy={center} r={radius} fill="none" stroke="#1a3a1a" strokeWidth="2" />
        {cells}
        {sensors}
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', justifyContent: 'center' },
  empty: {
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#f9f9f0',
    borderRadius: 8,
  },
  emptyText: { color: '#999', fontSize: 14 },
});

// EOF — TomogramView.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT