// components/AnalyteGauge.js — single analyte concentration gauge
// Author: jayis1  License: MIT
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { theme, ANALYTES } from '../utils/theme';

// Props: analyteKey (string), value (number), unitOverride (string)
export default function AnalyteGauge({ analyteKey, value, unitOverride }) {
  const meta = ANALYTES.find(a => a.key === analyteKey);
  if (!meta) return null;
  const unit = unitOverride || meta.unit;
  const isAlarm = value >= meta.alarm;
  const pct = Math.min(1, value / meta.alarm);
  return (
    <View style={styles.card}>
      <View style={styles.row}>
        <Text style={styles.label}>{meta.label}</Text>
        <Text style={[styles.value, { color: isAlarm ? theme.colors.alarm : theme.colors.text }]}>
          {formatNum(value)} <Text style={styles.unit}>{unit}</Text>
        </Text>
      </View>
      <View style={styles.barBg}>
        <View style={[styles.barFill, {
          width: `${pct * 100}%`,
          backgroundColor: isAlarm ? theme.colors.alarm : meta.color,
        }]} />
      </View>
      <Text style={styles.alarmLbl}>alarm @ {meta.alarm} {unit}</Text>
    </View>
  );
}

function formatNum(v) {
  if (v == null || isNaN(v)) return '—';
  if (v >= 100) return v.toFixed(0);
  if (v >= 10)  return v.toFixed(1);
  return v.toFixed(2);
}

const styles = StyleSheet.create({
  card: { backgroundColor: theme.colors.surface, borderRadius: theme.radius.md, padding: theme.spacing.sm, marginVertical: 4 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'baseline' },
  label: { color: theme.colors.textDim, fontSize: 12, fontFamily: theme.fonts.mono, textTransform: 'uppercase' },
  value: { fontSize: 22, fontWeight: '700', fontFamily: theme.fonts.mono },
  unit: { fontSize: 12, color: theme.colors.textDim },
  barBg: { height: 6, backgroundColor: theme.colors.bg, borderRadius: 3, marginTop: 6 },
  barFill: { height: 6, borderRadius: 3 },
  alarmLbl: { color: theme.colors.textDim, fontSize: 10, marginTop: 2, fontFamily: theme.fonts.mono },
});