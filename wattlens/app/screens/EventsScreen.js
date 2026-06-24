/**
 * EventsScreen.js — Power quality event log
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { useBle } from '../utils/BleContext';

export default function EventsScreen() {
  const { events, connectionState } = useBle();

  const renderEvent = ({ item }) => {
    const date = new Date(item.timestamp * 1000);
    const timeStr = date.toLocaleTimeString();
    const dateStr = date.toLocaleDateString();

    // Color code by event type
    let typeColor = '#888';
    let typeIcon = 'ℹ';
    if (item.type === 'SAG') { typeColor = '#FFAA00'; typeIcon = '⚠'; }
    else if (item.type === 'SWELL') { typeColor = '#FF6600'; typeIcon = '⚠'; }
    else if (item.type === 'INTERRUPTION') { typeColor = '#FF0000'; typeIcon = '⛔'; }
    else if (item.type === 'FLICKER') { typeColor = '#AA00FF'; typeIcon = '⚡'; }
    else if (item.type === 'HARMONIC_EXCEED') { typeColor = '#FFAA00'; typeIcon = '📊'; }
    else if (item.type === 'NILM_ON') { typeColor = '#00CC44'; typeIcon = '🔌'; }
    else if (item.type === 'NILM_OFF') { typeColor = '#0088FF'; typeIcon = '🔌'; }

    const phaseLabel = item.phase < 3 ? `L${item.phase + 1}` : 'ALL';

    return (
      <View style={styles.eventCard}>
        <View style={styles.eventHeader}>
          <Text style={[styles.eventType, { color: typeColor }]}>
            {typeIcon} {item.type}
          </Text>
          <Text style={styles.eventPhase}>{phaseLabel}</Text>
        </View>
        <View style={styles.eventDetails}>
          <Text style={styles.eventTime}>{dateStr} {timeStr}</Text>
          <Text style={styles.eventSeverity}>
            Severity: {item.severity.toFixed(2)}
            {item.durationMs > 0 ? ` · ${item.durationMs}ms` : ''}
          </Text>
        </View>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Power Quality Events</Text>
      <Text style={styles.subtitle}>
        {events.length} event{events.length !== 1 ? 's' : ''} logged
      </Text>

      {events.length > 0 ? (
        <FlatList
          data={events}
          keyExtractor={(item, index) => `${item.timestamp}-${index}`}
          renderItem={renderEvent}
          contentContainerStyle={styles.list}
        />
      ) : (
        <View style={styles.empty}>
          <Text style={styles.emptyText}>No events recorded</Text>
          <Text style={styles.emptyHint}>
            Events (sag, swell, interruption, flicker, harmonic exceedance, load on/off)
            will appear here as they occur.
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a14', padding: 16 },
  title: { color: '#FFF', fontSize: 20, fontWeight: '700', textAlign: 'center', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 13, textAlign: 'center', marginBottom: 16 },
  list: { paddingBottom: 20 },
  eventCard: {
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12,
    marginBottom: 8, borderLeftWidth: 4, borderLeftColor: '#0080FF',
  },
  eventHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  eventType: { fontSize: 15, fontWeight: '700' },
  eventPhase: { color: '#888', fontSize: 13, fontWeight: '600' },
  eventDetails: { flexDirection: 'row', justifyContent: 'space-between' },
  eventTime: { color: '#999', fontSize: 12 },
  eventSeverity: { color: '#CCC', fontSize: 12 },
  empty: { flex: 1, justifyContent: 'center', alignItems: 'center', paddingHorizontal: 20 },
  emptyText: { color: '#888', fontSize: 18, marginBottom: 8 },
  emptyHint: { color: '#555', fontSize: 13, textAlign: 'center', lineHeight: 20 },
});