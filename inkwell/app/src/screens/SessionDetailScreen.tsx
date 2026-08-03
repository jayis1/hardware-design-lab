// SessionDetailScreen.tsx — Replay a saved session
//
// Loads the strokes for a session from the database and renders them on the
// StrokeCanvas. Shows summary stats and an export button.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState } from 'react';
import { View, Text, Button, StyleSheet, Dimensions, ScrollView } from 'react-native';
import StrokeCanvas from '../components/StrokeCanvas';
import { getSessionStrokes, StrokeRecord } from '../db/database';

const SCREEN_W = Dimensions.get('window').width;
const SCREEN_H = Dimensions.get('window').height - 220;

type Props = { route: any; navigation: any };

export default function SessionDetailScreen({ route, navigation }: Props) {
  const { sessionId } = route.params;
  const [strokes, setStrokes] = useState<StrokeRecord[]>([]);

  useEffect(() => {
    (async () => {
      const rows = await getSessionStrokes(sessionId);
      setStrokes(rows);
    })();
  }, [sessionId]);

  // Convert stored absolute-position strokes to pseudo-segments for the canvas.
  const segments = strokes.map(s => ({
    flags: { penDown: true, strokeStart: false, strokeEnd: false, opticalFlowValid: false },
    seq: s.seq, tsMs: s.tsMs,
    dxUm: 0, dyUm: 0,
    pressureMN: s.pressureMN, crc8: 0,
  }));

  const strokeCount = strokes.length;
  const meanPress = strokeCount
    ? Math.round(strokes.reduce((s, r) => s + r.pressureMN, 0) / strokeCount)
    : 0;
  const durationMs = strokes.length
    ? strokes[strokes.length - 1].tsMs - strokes[0].tsMs
    : 0;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Session #{sessionId}</Text>
      <View style={styles.stats}>
        <Text style={styles.stat}>{strokeCount} segments</Text>
        <Text style={styles.stat}>avg {meanPress} mN</Text>
        <Text style={styles.stat}>{(durationMs / 1000).toFixed(1)} s</Text>
      </View>
      <StrokeCanvas segments={segments} width={SCREEN_W} height={SCREEN_H} />
      <Button title="Export" onPress={() => navigation.navigate('Export', { sessionId })} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f9f7f1' },
  title:     { fontSize: 20, fontWeight: '700', padding: 12, textAlign: 'center' },
  stats:     { flexDirection: 'row', justifyContent: 'space-around', padding: 8 },
  stat:      { fontSize: 13, color: '#555' },
});