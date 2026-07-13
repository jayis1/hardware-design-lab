/**
 * FleetMapScreen — Map view of all hives by location
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity, FlatList } from 'react-native';
import { useSelector } from 'react-redux';
import { AppState } from '../store/hiveStore';
import { HiveData, ColonyState } from '../types';

export default function FleetMapScreen({ navigation }: any) {
  const hives = useSelector((s: AppState) => s.hives);

  // Group hives by location
  const locations = hives.reduce((acc: Record<string, HiveData[]>, hive) => {
    const key = hive.location.label;
    if (!acc[key]) acc[key] = [];
    acc[key].push(hive);
    return acc;
  }, {});

  const getHealthSummary = (hives: HiveData[]) => {
    const healthy = hives.filter(h => h.colonyState === ColonyState.QueenrightHealthy).length;
    const alert = hives.filter(h => h.swarmAlert || h.queenlessAlert).length;
    return { healthy, alert, total: hives.length };
  };

  const renderLocation = (location: string, hives: HiveData[]) => {
    const summary = getHealthSummary(hives);
    const statusColor = summary.alert > 0 ? '#DC3545' :
                        summary.healthy === summary.total ? '#4CAF50' : '#FFC107';

    return (
      <TouchableOpacity
        key={location}
        style={styles.locationCard}
        onPress={() => navigation.navigate('Dashboard')}
      >
        <View style={[styles.statusDot, { backgroundColor: statusColor }]} />
        <View style={styles.locationInfo}>
          <Text style={styles.locationName}>📍 {location}</Text>
          <Text style={styles.locationSummary}>
            {summary.total} hives · {summary.healthy} healthy · {summary.alert} alerts
          </Text>
        </View>
        <Text style={styles.arrow}>→</Text>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>🗺️ Fleet Map</Text>
      <Text style={styles.subtitle}>
        {hives.length} hives across {Object.keys(locations).length} locations
      </Text>

      {Object.keys(locations).length === 0 ? (
        <View style={styles.empty}>
          <Text style={styles.emptyText}>No hives loaded</Text>
          <Text style={styles.emptyHint}>Pull to refresh on the Hives tab</Text>
        </View>
      ) : (
        <FlatList
          data={Object.entries(locations)}
          keyExtractor={([location]) => location}
          renderItem={({ item: [location, hives] }) => renderLocation(location, hives)}
          contentContainerStyle={styles.list}
        />
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E', padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#E8A317' },
  subtitle: { fontSize: 14, color: '#888', marginTop: 4, marginBottom: 16 },
  locationCard: {
    flexDirection: 'row', alignItems: 'center', backgroundColor: '#1A1A2E',
    borderRadius: 12, padding: 14, marginBottom: 10,
  },
  statusDot: { width: 12, height: 12, borderRadius: 6, marginRight: 12 },
  locationInfo: { flex: 1 },
  locationName: { fontSize: 16, fontWeight: 'bold', color: '#FFF' },
  locationSummary: { fontSize: 12, color: '#888', marginTop: 4 },
  arrow: { fontSize: 18, color: '#555' },
  list: { paddingBottom: 20 },
  empty: { flex: 1, justifyContent: 'center', alignItems: 'center' },
  emptyText: { fontSize: 16, color: '#666' },
  emptyHint: { fontSize: 12, color: '#444', marginTop: 8 },
});