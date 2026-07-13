/**
 * SpectrogramView — Real-time audio spectrogram visualization
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, StyleSheet, Text } from 'react-native';
import Svg, { Rect } from 'react-native-svg';
import { SpectrogramData } from '../types';

export default function SpectrogramView({ data, width, height }: {
  data: SpectrogramData | null;
  width: number;
  height: number;
}) {
  const [displayData, setDisplayData] = useState<number[][]>([]);
  const maxFrames = 60; // Show last 60 FFT frames (~15 seconds at 4 frames/sec)

  useEffect(() => {
    if (data && data.magnitudes) {
      setDisplayData(prev => {
        const updated = [...prev, ...data.magnitudes];
        return updated.slice(-maxFrames);
      });
    }
  }, [data]);

  const renderSpectrogram = () => {
    if (displayData.length === 0) {
      return (
        <View style={[styles.empty, { width, height }]}>
          <Text style={styles.emptyText}>No audio data</Text>
        </View>
      );
    }

    const numFrames = displayData.length;
    const numBins = displayData[0]?.length || 0;
    const frameWidth = width / maxFrames;
    const binHeight = height / Math.min(numBins, 128); // Limit display bins

    const rects: React.ReactElement[] = [];
    for (let f = 0; f < numFrames; f++) {
      for (let b = 0; b < Math.min(numBins, 128); b++) {
        const mag = displayData[f][b] || 0;
        // Normalize: -80 dB to 0 dB -> 0 to 1
        const normalized = Math.max(0, Math.min(1, (mag + 80) / 80));
        const color = magnitudeToColor(normalized);

        rects.push(
          <Rect
            key={`${f}-${b}`}
            x={f * frameWidth}
            y={height - (b + 1) * binHeight}
            width={frameWidth + 0.5}
            height={binHeight + 0.5}
            fill={color}
          />
        );
      }
    }

    return (
      <Svg width={width} height={height}>
        {rects}
      </Svg>
    );
  };

  return (
    <View style={styles.container}>
      {renderSpectrogram()}
      <View style={styles.legend}>
        <Text style={styles.legendText}>0 Hz</Text>
        <Text style={styles.legendText}>24 kHz</Text>
      </View>
    </View>
  );
}

function magnitudeToColor(mag: number): string {
  // Spectrogram color map: blue -> green -> yellow -> red
  if (mag < 0.25) {
    const t = mag / 0.25;
    return `rgb(0, 0, ${Math.round(t * 100)})`;
  } else if (mag < 0.5) {
    const t = (mag - 0.25) / 0.25;
    return `rgb(0, ${Math.round(t * 200)}, ${Math.round(100 - t * 50)})`;
  } else if (mag < 0.75) {
    const t = (mag - 0.5) / 0.25;
    return `rgb(${Math.round(t * 255)}, ${Math.round(200 - t * 50)}, 0)`;
  } else {
    const t = (mag - 0.75) / 0.25;
    return `rgb(255, ${Math.round(150 - t * 100)}, ${Math.round(t * 50)})`;
  }
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#000',
    borderRadius: 8,
    padding: 2,
  },
  empty: {
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#111',
  },
  emptyText: { color: '#555', fontSize: 14 },
  legend: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 4,
    paddingVertical: 2,
  },
  legendText: { fontSize: 10, color: '#555' },
});