// CrisperCue UI card component
// Author: jayis1
import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

export default function MetricCard({ label, value, hint, accent = '#56D3A2' }) {
  return (
    <View style={styles.card}>
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, { color: accent }]}>{value}</Text>
      <Text style={styles.hint}>{hint}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#10212D',
    borderRadius: 16,
    padding: 14,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#1A3342',
  },
  label: { color: '#9EC5D1', fontSize: 13 },
  value: { color: '#FFFFFF', fontSize: 26, fontWeight: '700', marginTop: 8 },
  hint: { color: '#D7E8ED', marginTop: 6, lineHeight: 18 },
});
