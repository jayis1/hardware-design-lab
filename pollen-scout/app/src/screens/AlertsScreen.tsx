/**
 * AlertsScreen.tsx - Configurable per-taxon threshold alerts
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import {
  View, Text, StyleSheet, ScrollView, Switch, TextInput, TouchableOpacity,
} from 'react-native';
import { useStation, makeDemoStation } from '../context/StationContext';
import { TAXA_NAMES } from '../types';

export default function AlertsScreen() {
  const { alerts, setAlert, removeAlert } = useStation();
  const { station: live } = useStation();
  const station = live ?? React.useMemo(() => makeDemoStation(), []);

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Alerts</Text>
      <Text style={s.subtitle}>
        Get a push notification when a taxon exceeds a threshold
        (grains/m³). One re-trigger per hour per rule.
      </Text>

      {alerts.map((rule) => (
        <View key={rule.id} style={s.row}>
          <View style={{ flex: 1 }}>
            <Text style={s.rowName}>{TAXA_NAMES[rule.classId] ?? `#${rule.classId}`}</Text>
            <Text style={s.rowSub}>
              {rule.enabled ? 'enabled' : 'disabled'} · last: {
                rule.lastTriggeredMs
                  ? new Date(rule.lastTriggeredMs).toLocaleString()
                  : 'never'
              }
            </Text>
            <View style={s.thrRow}>
              <Text style={s.thrLabel}>Threshold (grains/m³):</Text>
              <TextInput
                style={s.thrInput}
                keyboardType="numeric"
                value={String(rule.thresholdGrainsM3)}
                onChangeText={(t) => {
                  const n = parseInt(t, 10);
                  if (!isNaN(n)) setAlert({ ...rule, thresholdGrainsM3: n });
                }}
              />
            </View>
          </View>
          <Switch
            value={rule.enabled}
            onValueChange={(v) => setAlert({ ...rule, enabled: v })}
            trackColor={{ false: '#ccc', true: '#2E7D32' }}
          />
          <TouchableOpacity
            style={s.delBtn}
            onPress={() => removeAlert(rule.id)}
          >
            <Text style={s.delText}>✕</Text>
          </TouchableOpacity>
        </View>
      ))}

      <TouchableOpacity
        style={s.addBtn}
        onPress={() => {
          const id = `rule-${Date.now()}`;
          setAlert({
            id,
            classId: 30,
            thresholdGrainsM3: 50,
            enabled: true,
            lastTriggeredMs: 0,
          });
        }}
      >
        <Text style={s.addBtnText}>+ Add alert rule</Text>
      </TouchableOpacity>

      <Text style={s.sectionTitle}>Current station readings</Text>
      {station.taxa.map((t) => (
        <View key={t.classId} style={s.readRow}>
          <Text style={s.readName}>{t.name}</Text>
          <Text style={s.readVal}>{Math.round(t.concentration)} grains/m³</Text>
        </View>
      ))}

      <Text style={s.footer}>Pollen Scout · jayis1 · MIT</Text>
    </ScrollView>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 20, fontWeight: '800', color: '#2E7D32', margin: 16 },
  subtitle: { fontSize: 12, color: '#888', marginHorizontal: 16, marginBottom: 8 },
  row: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#fff', marginHorizontal: 12, marginVertical: 4, padding: 12, borderRadius: 8, elevation: 1 },
  rowName: { fontSize: 14, fontWeight: '700', color: '#333' },
  rowSub: { fontSize: 11, color: '#aaa', marginTop: 2 },
  thrRow: { flexDirection: 'row', alignItems: 'center', marginTop: 6 },
  thrLabel: { fontSize: 12, color: '#666', marginRight: 6 },
  thrInput: { borderWidth: 1, borderColor: '#ddd', borderRadius: 6, paddingHorizontal: 8, paddingVertical: 2, width: 80, fontSize: 13 },
  delBtn: { marginLeft: 8, padding: 6 },
  delText: { color: '#F44336', fontSize: 16, fontWeight: '700' },
  addBtn: { backgroundColor: '#2E7D32', margin: 12, padding: 12, borderRadius: 10, alignItems: 'center' },
  addBtnText: { color: '#fff', fontWeight: '700' },
  sectionTitle: { fontSize: 15, fontWeight: '700', color: '#333', margin: 12, marginBottom: 4 },
  readRow: { flexDirection: 'row', justifyContent: 'space-between', backgroundColor: '#fff', marginHorizontal: 12, marginVertical: 2, padding: 10, borderRadius: 6 },
  readName: { fontSize: 13, color: '#333' },
  readVal: { fontSize: 13, fontWeight: '700', color: '#2E7D32' },
  footer: { textAlign: 'center', color: '#999', fontSize: 11, margin: 16 },
});