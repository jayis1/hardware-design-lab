/**
 * FingerCurlView.tsx — SVG visualization of finger curl angles.
 *
 * Renders 5 fingers as animated arcs showing the curl angle
 * (0 = straight, 1 = fully curled) in real-time.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet, Text } from 'react-native';
import Svg, { Path, Circle, G } from 'react-native-svg';
import { FINGER_NAMES } from '../ble/protocol';

interface FingerCurlViewProps {
  curls: number[];      // 5 values, 0.0 to 1.0
  velocities: number[]; // 5 values, 0.0 to 1.0
}

/**
 * FingerCurlView — renders 5 finger curl indicators as SVG arcs.
 * Author: jayis1
 */
export default function FingerCurlView({ curls, velocities }: FingerCurlViewProps) {
  const fingerWidth = 60;
  const totalWidth = fingerWidth * 5;

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Finger Curl</Text>
      <Svg width={totalWidth} height={140}>
        {curls.map((curl, i) => {
          const cx = i * fingerWidth + fingerWidth / 2;
          const cy = 70;
          const radius = 25;
          // Map curl (0-1) to arc angle (0 = straight up, 180 = fully curled down)
          const angle = curl * 180;
          const rad = (angle * Math.PI) / 180;

          // Draw arc from top to curled position
          const endX = cx + radius * Math.sin(rad);
          const endY = cy - radius * Math.cos(rad);

          // Finger tip circle — size based on velocity
          const tipRadius = 4 + velocities[i] * 6;
          const tipColor = velocities[i] > 0.3 ? '#e94560' : '#533483';

          return (
            <G key={i}>
              {/* Base circle (knuckle) */}
              <Circle cx={cx} cy={cy} r={3} fill="#0f3460" />

              {/* Finger arc */}
              <Path
                d={`M ${cx} ${cy - radius} A ${radius} ${radius} 0 0 1 ${endX} ${endY}`}
                stroke={tipColor}
                strokeWidth={3}
                fill="none"
                strokeLinecap="round"
              />

              {/* Finger tip */}
              <Circle cx={endX} cy={endY} r={tipRadius} fill={tipColor} />

              {/* Finger name */}
              <Text
                style={styles.fingerName}
              >
                {FINGER_NAMES[i]}
              </Text>
            </G>
          );
        })}
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#16213e',
    borderRadius: 12,
    padding: 16,
    marginVertical: 8,
    alignItems: 'center',
  },
  title: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  fingerName: {
    color: '#888',
    fontSize: 9,
  },
});