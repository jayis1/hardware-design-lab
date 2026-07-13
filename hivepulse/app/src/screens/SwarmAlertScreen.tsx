/**
 * SwarmAlertScreen — Swarm prediction detail and recommended actions
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, ScrollView, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import { useSelector } from 'react-redux';
import { AppState } from '../store/hiveStore';
import { ColonyState } from '../types';

export default function SwarmAlertScreen({ route }: any) {
  const hiveId = route?.params?.hiveId;
  const hive = useSelector((s: AppState) => s.hives.find(h => h.id === hiveId));

  if (!hive) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>Hive not found</Text>
      </View>
    );
  }

  const isSwarm = hive.colonyState === ColonyState.PreparingSwarm;
  const isQueenless = hive.colonyState === ColonyState.Queenless;
  const isVarroa = hive.colonyState === ColonyState.VarroaHigh;

  const alertInfo = isSwarm ? {
    title: '🚨 Swarm Imminent',
    color: '#FFC107',
    bg: 'rgba(255,193,7,0.1)',
    message: `${hive.name} is showing acoustic signatures consistent with swarm preparation. ` +
             `The colony has been producing pre-swarm signals for the past 30 minutes with ` +
             `${(hive.confidence * 100).toFixed(0)}% confidence.`,
    timeline: 'Swarm expected within 3-10 days if no intervention is taken.',
    actions: [
      'Perform an artificial split: move 3-4 frames of brood + bees to a new hive body',
      'Add honey supers to relieve congestion in the brood chamber',
      'Check for queen cells — if present, the colony is committed to swarming',
      'Ensure the hive has adequate ventilation and space',
    ],
  } : isQueenless ? {
    title: '🚨 Queen Lost',
    color: '#DC3545',
    bg: 'rgba(220,53,69,0.1)',
    message: `${hive.name} is producing the characteristic "queenless roar" — ` +
             `a broadened, noisy acoustic spectrum with elevated 400-600 Hz energy. ` +
             `This pattern has persisted for 20+ minutes at ${(hive.confidence * 100).toFixed(0)}% confidence.`,
    timeline: 'Queen was likely lost within the past 4-12 hours. Action needed within 48 hours.',
    actions: [
      'Inspect the hive to confirm queen absence (look for eggs/larvae)',
      'If queenless confirmed: introduce a new mated queen in a cage',
      'Alternatively: merge with another colony using newspaper method',
      'If queen cells are present, let the colony requeen naturally',
    ],
  } : isVarroa ? {
    title: '⚠️ High Varroa Load',
    color: '#FF6B35',
    bg: 'rgba(255,107,53,0.1)',
    message: `${hive.name} shows spectral flattening consistent with high Varroa mite load. ` +
             `Confidence: ${(hive.confidence * 100).toFixed(0)}%.`,
    timeline: 'Treatment recommended within 7 days.',
    actions: [
      'Confirm with a sugar shake or alcohol wash test',
      'If >3% mite load: treat with formic acid (MAQS) or oxalic acid vapor',
      'Monitor monthly during active season',
      'Consider integrated pest management (IPM) with resistant stock',
    ],
  } : {
    title: '✓ No Active Alert',
    color: '#4CAF50',
    bg: 'rgba(76,175,80,0.1)',
    message: `${hive.name} is currently classified as healthy. No alerts active.`,
    timeline: '',
    actions: ['Continue routine monitoring'],
  };

  const handleActionTaken = () => {
    Alert.alert(
      'Mark Action Taken',
      'This will acknowledge the alert and add a note to the hive history.',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Confirm', onPress: () => console.log('Alert acknowledged') },
      ],
    );
  };

  return (
    <ScrollView style={styles.container}>
      <View style={[styles.alertBox, { backgroundColor: alertInfo.bg, borderColor: alertInfo.color }]}>
        <Text style={[styles.alertTitle, { color: alertInfo.color }]}>{alertInfo.title}</Text>
        <Text style={styles.hiveName}>{hive.name}</Text>
        <Text style={styles.confidence}>
          Confidence: {(hive.confidence * 100).toFixed(1)}%
        </Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>📊 Acoustic Analysis</Text>
        <Text style={styles.bodyText}>{alertInfo.message}</Text>
        {alertInfo.timeline ? (
          <Text style={styles.timeline}>⏱ {alertInfo.timeline}</Text>
        ) : null}
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>✅ Recommended Actions</Text>
        {alertInfo.actions.map((action, i) => (
          <View key={i} style={styles.actionItem}>
            <Text style={styles.actionNum}>{i + 1}</Text>
            <Text style={styles.actionText}>{action}</Text>
          </View>
        ))}
      </View>

      {/* Environmental context */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>📈 Context</Text>
        <Text style={styles.contextRow}>Weight: {hive.weight.toFixed(1)} kg</Text>
        <Text style={styles.contextRow}>Brood temp: {hive.temperatures[0]?.toFixed(1) ?? '--'}°C</Text>
        <Text style={styles.contextRow}>Entrance traffic: {hive.beesIn} in / {hive.beesOut} out</Text>
        <Text style={styles.contextRow}>CO₂: {hive.co2} ppm</Text>
      </View>

      <TouchableOpacity
        style={[styles.actionButton, { backgroundColor: alertInfo.color }]}
        onPress={handleActionTaken}
      >
        <Text style={styles.actionButtonText}>Acknowledge Alert</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E' },
  empty: { flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#0F0F1E' },
  emptyText: { color: '#666', fontSize: 16 },
  alertBox: { margin: 16, padding: 16, borderRadius: 12, borderWidth: 2, alignItems: 'center' },
  alertTitle: { fontSize: 22, fontWeight: 'bold' },
  hiveName: { fontSize: 18, color: '#FFF', marginTop: 4 },
  confidence: { fontSize: 14, color: '#888', marginTop: 4 },
  section: { backgroundColor: '#1A1A2E', margin: 8, borderRadius: 12, padding: 14 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#E8A317', marginBottom: 10 },
  bodyText: { fontSize: 14, color: '#CCC', lineHeight: 20 },
  timeline: { fontSize: 13, color: '#FFC107', marginTop: 8, fontWeight: '600' },
  actionItem: { flexDirection: 'row', marginBottom: 10 },
  actionNum: {
    width: 24, height: 24, borderRadius: 12, backgroundColor: '#E8A317',
    color: '#1A1A2E', textAlign: 'center', textAlignVertical: 'center',
    fontSize: 12, fontWeight: 'bold', marginRight: 10, marginTop: 2,
  },
  actionText: { flex: 1, fontSize: 14, color: '#CCC', lineHeight: 20 },
  contextRow: { fontSize: 14, color: '#AAA', marginBottom: 4 },
  actionButton: { margin: 16, padding: 16, borderRadius: 12, alignItems: 'center' },
  actionButtonText: { fontSize: 16, fontWeight: 'bold', color: '#FFF' },
});