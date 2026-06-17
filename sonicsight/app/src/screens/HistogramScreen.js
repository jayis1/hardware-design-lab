/**
 * HistogramScreen.js — SonicSight Companion
 * Browse, search, and filter past scan results with thumbnails.
 * @author jayis1
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  TextInput,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const SAMPLE_SCANS = [
  { id: 'S001', date: '2026-06-17', time: '14:32', label: 'Oak #3 — Hyde Park',
    velMean: 2850, decayPct: 8.2, sensors: 32 },
  { id: 'S002', date: '2026-06-17', time: '11:15', label: 'Pine Beam — Barn',
    velMean: 3100, decayPct: 3.1, sensors: 24 },
  { id: 'S003', date: '2026-06-16', time: '09:42', label: 'Glulam Joint — Bridge',
    velMean: 2650, decayPct: 12.5, sensors: 32 },
  { id: 'S004', date: '2026-06-15', time: '16:20', label: 'Elm — Cemetery Row',
    velMean: 2200, decayPct: 22.0, sensors: 28 },
  { id: 'S005', date: '2026-06-14', time: '08:05', label: 'CFRP Panel — Lab Test',
    velMean: 4500, decayPct: 0.5, sensors: 32 },
];

const HistogramScreen = ({ navigation }) => {
  const [search, setSearch] = useState('');
  const [filterMinVel, setFilterMinVel] = useState('');

  const filtered = SAMPLE_SCANS.filter((s) => {
    if (search && !s.label.toLowerCase().includes(search.toLowerCase())) return false;
    if (filterMinVel && s.velMean < parseInt(filterMinVel)) return false;
    return true;
  });

  const renderItem = ({ item }) => (
    <TouchableOpacity style={styles.card}
      onPress={() => navigation?.navigate('Detail', { scan: item })}>
      <View style={styles.thumb}>
        <Icon name="image-filter-center-focus" size={36} color="#00d2ff" />
      </View>
      <View style={styles.info}>
        <Text style={styles.label}>{item.label}</Text>
        <Text style={styles.date}>{item.date} {item.time}</Text>
        <View style={styles.tagRow}>
          <View style={styles.tag}><Text style={styles.tagText}>{item.velMean} m/s</Text></View>
          <View style={[styles.tag, styles.tagDanger]}><Text style={styles.tagText}>
            {item.decayPct}% decay</Text></View>
          <View style={styles.tag}><Text style={styles.tagText}>{item.sensors}ch</Text></View>
        </View>
      </View>
      <Icon name="chevron-right" size={24} color="#555" />
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.searchRow}>
        <Icon name="magnify" size={20} color="#888" style={{ marginRight: 4 }} />
        <TextInput style={styles.searchInput}
          placeholder="Search scans..." placeholderTextColor="#555"
          value={search} onChangeText={setSearch} />
        <TextInput style={styles.filterInput}
          placeholder="Min m/s" placeholderTextColor="#555"
          keyboardType="numeric" value={filterMinVel} onChangeText={setFilterMinVel} />
      </View>
      <FlatList data={filtered} renderItem={renderItem}
        keyExtractor={(item) => item.id}
        contentContainerStyle={{ paddingBottom: 20 }} />
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 12 },
  searchRow: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 8,
    marginBottom: 12, borderWidth: 1, borderColor: '#16213e',
  },
  searchInput: { flex: 1, color: '#fff', fontSize: 14 },
  filterInput: { width: 80, color: '#fff', fontSize: 14, textAlign: 'right' },
  card: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 12,
    marginBottom: 8, borderWidth: 1, borderColor: '#16213e',
  },
  thumb: {
    width: 56, height: 56, borderRadius: 8,
    backgroundColor: '#16213e', justifyContent: 'center',
    alignItems: 'center', marginRight: 12,
  },
  info: { flex: 1 },
  label: { color: '#fff', fontSize: 14, fontWeight: '600' },
  date: { color: '#777', fontSize: 11, marginTop: 2 },
  tagRow: { flexDirection: 'row', marginTop: 4 },
  tag: {
    backgroundColor: '#16213e', borderRadius: 4, paddingHorizontal: 6,
    paddingVertical: 2, marginRight: 4,
  },
  tagDanger: { backgroundColor: '#4a1515' },
  tagText: { color: '#aaa', fontSize: 10 },
});

export default HistogramScreen;