/**
 * EventsScreen.js — AP Event Log & Classification
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays detected plant action potential events with classification
 * (drought, wounding, light response, herbivory) and statistics.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, ScrollView } from 'react-native';
import { useBle } from '../utils/BleContext';
import { CMD, RSP, EVENT_CLASS } from '../utils/protocol';

const CLASS_NAMES = {
  [EVENT_CLASS.UNKNOWN]: 'Unknown',
  [EVENT_CLASS.DROUGHT]: 'Drought Stress',
  [EVENT_CLASS.WOUNDING]: 'Wounding',
  [EVENT_CLASS.LIGHT]: 'Light Response',
  [EVENT_CLASS.HERBIVORY]: 'Herbivory',
};

const CLASS_COLORS = {
  [EVENT_CLASS.UNKNOWN]: '#888',
  [EVENT_CLASS.DROUGHT]: '#ff9800',
  [EVENT_CLASS.WOUNDING]: '#f44336',
  [EVENT_CLASS.LIGHT]: '#2196F3',
  [EVENT_CLASS.HERBIVORY]: '#9c27b0',
};

const CLASS_ICONS = {
  [EVENT_CLASS.UNKNOWN]: '❓',
  [EVENT_CLASS.DROUGHT]: '🌵',
  [EVENT_CLASS.WOUNDING]: '🩹',
  [EVENT_CLASS.LIGHT]: '☀️',
  [EVENT_CLASS.HERBIVORY]: '🐛',
};

export default function EventsScreen() {
  const { connected, sendCommand, events, addEvent } = useBle();
  const [stats, setStats] = useState({
    total: 0,
    drought: 0,
    wounding: 0,
    light: 0,
    herbivory: 0,
    unknown: 0,
  });
  const [selectedClass, setSelectedClass] = useState(null);

  // Compute stats from events
  useEffect(() => {
    const newStats = { total: 0, drought: 0, wounding: 0, light: 0, herbivory: 0, unknown: 0 };
    events.forEach((e) => {
      newStats.total++;
      switch (e.classified) {
        case EVENT_CLASS.DROUGHT: newStats.drought++; break;
        case EVENT_CLASS.WOUNDING: newStats.wounding++; break;
        case EVENT_CLASS.LIGHT: newStats.light++; break;
        case EVENT_CLASS.HERBIVORY: newStats.herbivory++; break;
        default: newStats.unknown++; break;
      }
    });
    setStats(newStats);
  }, [events]);

  const filteredEvents = selectedClass !== null
    ? events.filter((e) => e.classified === selectedClass)
    : events;

  const renderEvent = ({ item, index }) => {
    const classColor = CLASS_COLORS[item.classified] || '#888';
    const className = CLASS_NAMES[item.classified] || 'Unknown';
    const icon = CLASS_ICONS[item.classified] || '❓';

    return (
      <View style={styles.eventCard}>
        <View style={styles.eventHeader}>
          <Text style={styles.eventIcon}>{icon}</Text>
          <View style={styles.eventInfo}>
            <Text style={[styles.eventClass, { color: classColor }]}>{className}</Text>
            <Text style={styles.eventTime}>
              CH{item.channel} · {item.duration_ms}ms · {new Date(item.timestamp).toLocaleTimeString()}
            </Text>
          </View>
          <Text style={styles.eventAmplitude}>{item.peak_uv} µV</Text>
        </View>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>⚡ AP Events</Text>
        <Text style={styles.headerSub}>Detected Plant Action Potentials</Text>
      </View>

      {connected ? (
        <View style={styles.content}>
          {/* Summary stats */}
          <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.statsScroll}>
            <TouchableOpacity
              style={[styles.statPill, selectedClass === null ? styles.statPillActive : null]}
              onPress={() => setSelectedClass(null)}
            >
              <Text style={styles.statPillValue}>{stats.total}</Text>
              <Text style={styles.statPillLabel}>Total</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.statPill, { borderColor: '#ff9800' }, selectedClass === 1 ? styles.statPillActive : null]}
              onPress={() => setSelectedClass(1)}
            >
              <Text style={[styles.statPillValue, { color: '#ff9800' }]}>{stats.drought}</Text>
              <Text style={styles.statPillLabel}>🌵 Drought</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.statPill, { borderColor: '#f44336' }, selectedClass === 2 ? styles.statPillActive : null]}
              onPress={() => setSelectedClass(2)}
            >
              <Text style={[styles.statPillValue, { color: '#f44336' }]}>{stats.wounding}</Text>
              <Text style={styles.statPillLabel}>🩹 Wounding</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.statPill, { borderColor: '#2196F3' }, selectedClass === 3 ? styles.statPillActive : null]}
              onPress={() => setSelectedClass(3)}
            >
              <Text style={[styles.statPillValue, { color: '#2196F3' }]}>{stats.light}</Text>
              <Text style={styles.statPillLabel}>☀️ Light</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.statPill, { borderColor: '#9c27b0' }, selectedClass === 4 ? styles.statPillActive : null]}
              onPress={() => setSelectedClass(4)}
            >
              <Text style={[styles.statPillValue, { color: '#9c27b0' }]}>{stats.herbivory}</Text>
              <Text style={styles.statPillLabel}>🐛 Herbivory</Text>
            </TouchableOpacity>
          </ScrollView>

          {/* Events list */}
          <FlatList
            data={filteredEvents}
            renderItem={renderEvent}
            keyExtractor={(item, index) => index.toString()}
            style={styles.eventsList}
            ListEmptyComponent={
              <View style={styles.emptyContainer}>
                <Text style={styles.emptyText}>No events detected yet</Text>
                <Text style={styles.emptySubtext}>
                  Plant action potential events will appear here when detected
                </Text>
              </View>
            }
          />
        </View>
      ) : (
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedIcon}>⚡</Text>
          <Text style={styles.disconnectedText}>Not Connected</Text>
          <Text style={styles.disconnectedSubtext}>
            Connect to FloraPulse to view detected events
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1b0d',
  },
  header: {
    padding: 16,
    paddingBottom: 8,
  },
  headerTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#4CAF50',
  },
  headerSub: {
    fontSize: 12,
    color: '#888',
    marginTop: 4,
  marginBottom: 12,
  },
  content: {
    flex: 1,
  },
  statsScroll: {
    paddingHorizontal: 16,
    marginBottom: 8,
  },
  statPill: {
    backgroundColor: '#1a2e1a',
    borderRadius: 10,
    paddingHorizontal: 16,
    paddingVertical: 10,
    marginRight: 10,
    alignItems: 'center',
    borderWidth: 2,
    borderColor: '#2d4a2d',
    minWidth: 80,
  },
  statPillActive: {
    backgroundColor: '#2d4a2d',
  },
  statPillValue: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
  },
  statPillLabel: {
    fontSize: 10,
    color: '#888',
    marginTop: 2,
  },
  eventsList: {
    flex: 1,
    paddingHorizontal: 16,
  },
  eventCard: {
    backgroundColor: '#1a2e1a',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#2d4a2d',
  },
  eventHeader: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  eventIcon: {
    fontSize: 28,
    marginRight: 12,
  },
  eventInfo: {
    flex: 1,
  },
  eventClass: {
    fontSize: 15,
    fontWeight: '600',
  },
  eventTime: {
    fontSize: 12,
    color: '#888',
    marginTop: 2,
  },
  eventAmplitude: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
  },
  emptyContainer: {
    alignItems: 'center',
    paddingTop: 60,
    paddingHorizontal: 40,
  },
  emptyText: {
    fontSize: 18,
    color: '#666',
    marginBottom: 8,
  },
  emptySubtext: {
    fontSize: 14,
    color: '#555',
    textAlign: 'center',
  },
  disconnected: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingTop: 80,
  },
  disconnectedIcon: { fontSize: 64, marginBottom: 20 },
  disconnectedText: { fontSize: 24, fontWeight: 'bold', color: '#fff', marginBottom: 8 },
  disconnectedSubtext: { fontSize: 14, color: '#888', textAlign: 'center' },
});