/**
 * EventLogScreen.js — Chronological event log with filtering
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Switch } from 'react-native';
import { useSelector, useDispatch } from 'react-redux';
import { EventItem } from '../components/EventItem';

const CLASS_NAMES = [
  'root_growth', 'root_hydraulic', 'earthworm_burrow', 'earthworm_cast',
  'arthropod_click', 'grub_chew', 'microbe_ebullition', 'water_infiltration',
  'compaction_crack', 'noise'
];

function generateDemoEvents(count = 200) {
  const events = [];
  for (let i = 0; i < count; i++) {
    const classId = Math.floor(Math.random() * 10);
    events.push({
      id: `ev-${Date.now()}-${i}`,
      timestamp: Date.now() - Math.random() * 86400000 * 7,
      classId,
      className: CLASS_NAMES[classId],
      channel: Math.floor(Math.random() * 12),
      confidence: 60 + Math.random() * 38,
      posX: (Math.random() * 100 - 50),
      posY: (Math.random() * 100 - 50),
      posZ: -(Math.random() * 60 + 5),
      podId: `pod-00${Math.floor(Math.random() * 3) + 1}`,
    });
  }
  return events.sort((a, b) => b.timestamp - a.timestamp);
}

export default function EventLogScreen({ navigation }) {
  const storedEvents = useSelector(state => state.events.events);
  const [events, setEvents] = useState([]);
  const [filterClass, setFilterClass] = useState(null);
  const [filterDepth, setFilterDepth] = useState(null);
  const [showOnlyPest, setShowOnlyPest] = useState(false);

  useEffect(() => {
    if (storedEvents.length > 0) {
      setEvents(storedEvents);
    } else {
      setEvents(generateDemoEvents());
    }
  }, [storedEvents]);

  const filteredEvents = events.filter(ev => {
    if (filterClass !== null && ev.classId !== filterClass) return false;
    if (showOnlyPest && ev.classId !== 5) return false;
    if (filterDepth !== null) {
      const depth = -ev.posZ;
      if (filterDepth === 'shallow' && depth > 25) return false;
      if (filterDepth === 'deep' && depth < 25) return false;
    }
    return true;
  });

  const renderItem = ({ item }) => (
    <EventItem
      event={item}
      onPress={() => navigation.navigate('EventDetail', { event: item })}
    />
  );

  return (
    <View style={styles.container}>
      {/* Filter bar */}
      <View style={styles.filterBar}>
        <ScrollView horizontal showsHorizontalScrollIndicator={false}>
          <TouchableOpacity
            style={[styles.filterChip, filterClass === null && styles.filterChipActive]}
            onPress={() => setFilterClass(null)}
          >
            <Text style={styles.filterChipText}>All</Text>
          </TouchableOpacity>
          {CLASS_NAMES.map((name, i) => (
            <TouchableOpacity
              key={name}
              style={[styles.filterChip, filterClass === i && styles.filterChipActive]}
              onPress={() => setFilterClass(filterClass === i ? null : i)}
            >
              <Text style={styles.filterChipText}>{name.replace(/_/g, ' ')}</Text>
            </TouchableOpacity>
          ))}
        </ScrollView>

        <View style={styles.filterRow}>
          <TouchableOpacity
            style={[styles.depthChip, filterDepth === null && styles.depthChipActive]}
            onPress={() => setFilterDepth(null)}
          >
            <Text style={styles.depthChipText}>All depths</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.depthChip, filterDepth === 'shallow' && styles.depthChipActive]}
            onPress={() => setFilterDepth('shallow')}
          >
            <Text style={styles.depthChipText}>Shallow (0-25cm)</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.depthChip, filterDepth === 'deep' && styles.depthChipActive]}
            onPress={() => setFilterDepth('deep')}
          >
            <Text style={styles.depthChipText}>Deep (25-60cm)</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.pestToggle}>
          <Text style={styles.pestToggleLabel}>Pest events only</Text>
          <Switch value={showOnlyPest} onValueChange={setShowOnlyPest} />
        </View>
      </View>

      <FlatList
        data={filteredEvents}
        keyExtractor={item => item.id}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

import { ScrollView } from 'react-native';

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  filterBar: { backgroundColor: '#FFFFFF', padding: 10, elevation: 2 },
  filterChip: { paddingHorizontal: 10, paddingVertical: 5, margin: 3, borderRadius: 14, backgroundColor: '#E0E0E0' },
  filterChipActive: { backgroundColor: '#2E7D32' },
  filterChipText: { fontSize: 11, color: '#616161' },
  filterRow: { flexDirection: 'row', marginTop: 8 },
  depthChip: { paddingHorizontal: 10, paddingVertical: 4, margin: 3, borderRadius: 12, backgroundColor: '#F5F5F5' },
  depthChipActive: { backgroundColor: '#388E3C' },
  depthChipText: { fontSize: 11, color: '#757575' },
  pestToggle: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginTop: 8, paddingHorizontal: 5 },
  pestToggleLabel: { fontSize: 13, color: '#F44336', fontWeight: 'bold' },
  list: { padding: 10 },
});