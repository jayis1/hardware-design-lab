// RadargramCanvas.js — High-Performance Canvas Renderer for B-Scan
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Renders the live B-scan radargram as a color-mapped image.  Each column
// represents one A-scan trace (along-track position), and each row represents
// a depth bin.  The amplitude at each pixel is mapped to a color using the
// selected color map (greyscale, jet, or seismic).
//

import React, { useRef, useEffect, useMemo } from 'react';
import { View, StyleSheet, Dimensions, Text } from 'react-native';
import { COLOR_MAPS } from '../utils/protocol';

const { width } = Dimensions.get('window');

export default function RadargramCanvas({ traces, depths, colorMap = 'greyscale', zoom = 1 }) {
  const canvasRef = useRef(null);
  const displayWidth = width - 20;
  const displayHeight = 300;

  // Determine the maximum amplitude across all traces for normalization
  const maxAmplitude = useMemo(() => {
    if (!traces || traces.length === 0) return 1.0;
    let max = 0.001;
    for (const trace of traces) {
      if (!trace) continue;
      for (const v of trace) {
        const av = Math.abs(v);
        if (av > max) max = av;
      }
    }
    return max;
  }, [traces]);

  // Color map function
  const colorFn = COLOR_MAPS[colorMap] || COLOR_MAPS.greyscale;

  // Render the radargram using a grid of colored Views
  // In a production app, this uses react-native-canvas for GPU acceleration
  const numTraces = traces ? traces.length : 0;
  const numDepthBins = traces && traces[0] ? traces[0].length : 0;

  const cellWidth = displayWidth / Math.max(1, numTraces);
  const cellHeight = displayHeight / Math.max(1, numDepthBins);

  // For performance, we render a sampled grid (not every pixel)
  const colStep = Math.max(1, Math.floor(numTraces / (displayWidth / 2)));
  const rowStep = Math.max(1, Math.floor(numDepthBins / (displayHeight / 2)));

  const rows = [];
  for (let r = 0; r < numDepthBins; r += rowStep) {
    const row = [];
    for (let c = 0; c < numTraces; c += colStep) {
      const trace = traces[c];
      if (trace && r < trace.length) {
        row.push(colorFn(trace[r], maxAmplitude));
      } else {
        row.push('rgb(0,0,0)');
      }
    }
    rows.push(row);
  }

  const renderColWidth = displayWidth / Math.max(1, Math.ceil(numTraces / colStep));
  const renderRowHeight = displayHeight / Math.max(1, Math.ceil(numDepthBins / rowStep));

  return (
    <View style={styles.container}>
      {numTraces === 0 && (
        <Text style={styles.empty}>No radargram data yet. Start a survey.</Text>
      )}
      <View style={[styles.canvas, { width: displayWidth, height: displayHeight }]}>
        {rows.map((row, ri) => (
          <View key={`r${ri}`} style={{ flexDirection: 'row', height: renderRowHeight }}>
            {row.map((color, ci) => (
              <View
                key={`c${ci}`}
                style={{
                  width: renderColWidth,
                  height: renderRowHeight,
                  backgroundColor: color,
                }}
              />
            ))}
          </View>
        ))}
      </View>
      {/* Depth axis labels */}
      {depths && depths.length > 0 && (
        <View style={styles.depthAxis}>
          <Text style={styles.depthLabel}>{(depths[0] || 0).toFixed(2)} m</Text>
          <Text style={styles.depthLabel}>{(depths[Math.floor(depths.length / 4)] || 0).toFixed(2)} m</Text>
          <Text style={styles.depthLabel}>{(depths[Math.floor(depths.length / 2)] || 0).toFixed(2)} m</Text>
          <Text style={styles.depthLabel}>{(depths[Math.floor(depths.length * 3 / 4)] || 0).toFixed(2)} m</Text>
          <Text style={styles.depthLabel}>{(depths[depths.length - 1] || 0).toFixed(2)} m</Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 10,
    backgroundColor: '#0a0a1a',
  },
  canvas: {
    borderWidth: 1,
    borderColor: '#333',
    backgroundColor: '#000',
  },
  empty: {
    color: '#888',
    textAlign: 'center',
    marginTop: 120,
    fontSize: 16,
  },
  depthAxis: {
    position: 'absolute',
    right: 12,
    top: 10,
    height: 300,
    justifyContent: 'space-between',
    backgroundColor: 'rgba(0,0,0,0.5)',
    padding: 4,
    borderRadius: 4,
  },
  depthLabel: {
    color: '#0ff',
    fontSize: 10,
    fontFamily: 'monospace',
  },
});