/**
 * EmgBarChart.tsx — Animated EMG envelope bar chart component.
 *
 * Displays 5 EMG channel envelopes as animated vertical bars,
 * color-coded by channel. Updates in real-time from OSC data.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet, Text } from 'react-native';
import { FINGER_NAMES } from '../ble/protocol';

interface EmgBarChartProps {
  values: number[];  // 5 values, 0.0 to 1.0
}

const CHANNEL_COLORS = ['#e94560', '#0f3460', '#16213e', '#533483', '#e94560'];
const CHANNEL_LABELS = ['Flex. Dig.', 'Flex. Poll.', 'Ext. Dig.', 'Flex. Carpi', 'Ext. Carpi'];

/**
 * EmgBarChart — renders 5 animated EMG bars.
 * Author: jayis1
 */
export default function EmgBarChart({ values }: EmgBarChartProps) {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>EMG Envelopes</Text>
      <View style={styles.barRow}>
        {values.map((val, i) => {
          const height = Math.max(2, val * 120);
          return (
            <View key={i} style={styles.barContainer}>
              <View style={styles.barWrapper}>
                <View
                  style={[
                    styles.bar,
                    {
                      height,
                      backgroundColor: CHANNEL_COLORS[i % CHANNEL_COLORS.length],
                    },
                  ]}
                />
              </View>
              <Text style={styles.label}>{CHANNEL_LABELS[i]}</Text>
              <Text style={styles.value}>{(val * 100).toFixed(0)}%</Text>
            </View>
          );
        })}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#16213e',
    borderRadius: 12,
    padding: 16,
    marginVertical: 8,
  },
  title: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  barRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    alignItems: 'flex-end',
  },
  barContainer: {
    alignItems: 'center',
    width: 55,
  },
  barWrapper: {
    height: 120,
    width: 30,
    justifyContent: 'flex-end',
    backgroundColor: 'rgba(255,255,255,0.05)',
    borderRadius: 4,
  },
  bar: {
    width: 30,
    borderRadius: 4,
  },
  label: {
    color: '#888',
    fontSize: 8,
    marginTop: 4,
    textAlign: 'center',
  },
  value: {
    color: '#aaa',
    fontSize: 10,
    marginTop: 2,
  },
});