/**
 * GestureBadge.tsx — Current gesture classification indicator.
 *
 * Shows the currently detected gesture name, confidence percentage,
 * and which finger triggered it.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, StyleSheet, Text } from 'react-native';
import { GESTURE_NAMES, FINGER_NAMES } from '../ble/protocol';

interface GestureBadgeProps {
  gestureId: number;
  confidence: number;  // 0.0 to 1.0
  finger: number;      // 0-4, or -1 for all
}

/**
 * GestureBadge — displays the current gesture with confidence.
 * Author: jayis1
 */
export default function GestureBadge({ gestureId, confidence, finger }: GestureBadgeProps) {
  const gestureName = GESTURE_NAMES[gestureId] || 'None';
  const fingerName = finger >= 0 && finger < 5 ? FINGER_NAMES[finger] : 'All';
  const pct = Math.round(confidence * 100);

  // Color based on confidence
  const color = confidence > 0.75 ? '#e94560' : confidence > 0.5 ? '#533483' : '#555';

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Gesture</Text>
      <View style={[styles.badge, { borderColor: color }]}>
        <Text style={[styles.gestureName, { color }]}>{gestureName}</Text>
        <Text style={styles.fingerLabel}>Finger: {fingerName}</Text>
        <Text style={styles.confidenceLabel}>{pct}% confidence</Text>
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
    alignItems: 'center',
  },
  title: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  badge: {
    borderWidth: 2,
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    minWidth: 150,
  },
  gestureName: {
    fontSize: 20,
    fontWeight: 'bold',
  },
  fingerLabel: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
  },
  confidenceLabel: {
    color: '#aaa',
    fontSize: 12,
    marginTop: 2,
  },
});