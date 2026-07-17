/**
 * AlertsScreen.tsx — threshold rules + notification log.
 *
 * Author : jayis1
 * License: MIT
 */
import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Switch } from 'react-native';
import { useDevices } from '../context/DeviceContext';
import { AlertRule, speciesName } from '../api';

const DEFAULT_RULES: AlertRule[] = [
  { speciesId: 8,  pulseRateMin: 5, durationMin: 10, notify: true },   // termites
  { speciesId: 11, pulseRateMin: 1, durationMin: 15, notify: true },   // old-house borer
  { speciesId: 13, pulseRateMin: 3, durationMin: 5,  notify: true },   // granary weevil
  { speciesId: 15, pulseRateMin: 2, durationMin: 10, notify: true },   // red flour beetle
  { speciesId: 23, pulseRateMin: 1, durationMin: 10, notify: true },   // clothes moth
];

export default function AlertsScreen() {
  const { client } = useDevices();
  const [rules, setRules] = useState<AlertRule[]>(DEFAULT_RULES);
  const [log, setLog] = useState<string[]>([]);

  useEffect(() => {
    if (!client) return;
    client.getConfig().then((c) => {
      if (c.alertRules?.length) setRules(c.alertRules);
    }).catch(() => {});
  }, [client]);

  const toggle = (i: number) => {
    setRules((prev) => {
      const next = [...prev];
      next[i] = { ...next[i], notify: !next[i].notify };
      client?.putConfig({ alertRules: next }).catch(() => {});
      return next;
    });
  };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Alert Rules</Text>
      <FlatList
        data={rules}
        keyExtractor={(_, i) => String(i)}
        renderItem={({ item, index }) => (
          <View style={styles.ruleRow}>
            <View style={{ flex: 1 }}>
              <Text style={styles.ruleName}>{speciesName(item.speciesId)}</Text>
              <Text style={styles.ruleMeta}>
                pulse ≥ {item.pulseRateMin ?? 0}/s · ≥ {item.durationMin ?? 0} min
              </Text>
            </View>
            <Switch value={item.notify} onValueChange={() => toggle(index)} trackColor={{ true: '#39d353' }} />
          </View>
        )}
      />
      <Text style={styles.section}>Recent notifications</Text>
      {log.length === 0 ? (
        <Text style={styles.empty}>No notifications yet.</Text>
      ) : (
        log.map((line, i) => <Text key={i} style={styles.logLine}>{line}</Text>)
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f17', padding: 14 },
  h1: { color: '#e0e0e0', fontSize: 20, fontWeight: 'bold', marginBottom: 10 },
  ruleRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1a1a2e',
             borderRadius: 8, padding: 12, marginBottom: 6 },
  ruleName: { color: '#39d353', fontSize: 15, fontStyle: 'italic' },
  ruleMeta: { color: '#888', fontSize: 12, marginTop: 2 },
  section: { color: '#aaa', fontSize: 14, marginTop: 18, marginBottom: 6 },
  empty: { color: '#888', textAlign: 'center', marginTop: 20 },
  logLine: { color: '#c0c0d0', fontSize: 12, paddingVertical: 4 },
});