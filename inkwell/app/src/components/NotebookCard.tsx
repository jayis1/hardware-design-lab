// NotebookCard.tsx — Card showing a saved session in the notebook list
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { SessionRecord } from '../db/database';

type Props = {
  session: SessionRecord;
  onPress: (id: number) => void;
};

export default function NotebookCard({ session, onPress }: Props) {
  const date = new Date(session.startedAt);
  const dateStr = date.toLocaleDateString(undefined, {
    year: 'numeric', month: 'short', day: 'numeric',
    hour: '2-digit', minute: '2-digit',
  });
  const min = Math.floor(session.durationMs / 60000);
  const sec = Math.floor((session.durationMs % 60000) / 1000);

  return (
    <TouchableOpacity style={styles.card} onPress={() => onPress(session.id)}>
      <View style={styles.header}>
        <Text style={styles.date}>{dateStr}</Text>
        <Text style={styles.duration}>{min}m {sec}s</Text>
      </View>
      <View style={styles.stats}>
        <Text style={styles.stat}>{session.strokeCount} strokes</Text>
        <Text style={styles.stat}>avg {session.meanPressureMN} mN</Text>
      </View>
      {session.note ? <Text style={styles.note}>{session.note}</Text> : null}
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  card:     { backgroundColor: '#fff', padding: 16, marginHorizontal: 12,
              marginVertical: 6, borderRadius: 10, elevation: 2 },
  header:   { flexDirection: 'row', justifyContent: 'space-between' },
  date:     { fontSize: 15, fontWeight: '600', color: '#222' },
  duration: { fontSize: 13, color: '#888' },
  stats:    { flexDirection: 'row', gap: 16, marginTop: 6 },
  stat:     { fontSize: 12, color: '#666' },
  note:     { fontSize: 13, color: '#444', marginTop: 8, fontStyle: 'italic' },
});