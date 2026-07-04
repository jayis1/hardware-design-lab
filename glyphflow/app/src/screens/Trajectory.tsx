/**
 * Trajectory.tsx — replay the 2D trajectory that produced the last character
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { bleClient } from '../components/BleManager';
import { StrokeCanvas } from '../components/StrokeCanvas';

export function TrajectoryScreen() {
  const [pts, setPts] = useState<{ x: number; y: number }[]>([]);
  const [history, setHistory] = useState<{ x: number; y: number }[][]>([]);

  useEffect(() => {
    const unsub = bleClient.onTrajectory((p) => {
      setPts(p);
      setHistory(h => [p, ...h].slice(0, 20));
    });
    return () => unsub();
  }, []);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Last Stroke Trajectory</Text>
      <StrokeCanvas points={pts} />
      <Text style={styles.meta}>{pts.length} points · class shown in Live</Text>

      <Text style={styles.subtitle}>Recent strokes</Text>
      {history.map((h, i) => (
        <TouchableOpacity key={i} onPress={() => setPts(h)}>
          <View style={styles.row}>
            <View style={styles.miniCanvas}>
              <StrokeCanvas points={h} speedMs={200} />
            </View>
            <Text style={styles.rowMeta}>stroke #{history.length - i}</Text>
          </View>
        </TouchableOpacity>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#fff', fontSize: 18, fontWeight: '600', marginBottom: 4 },
  meta: { color: '#666', fontSize: 12, marginBottom: 12 },
  subtitle: { color: '#aaa', fontSize: 14, marginTop: 12, marginBottom: 8 },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  miniCanvas: { width: 80, height: 80, marginRight: 12 },
  rowMeta: { color: '#888', fontSize: 12 },
});