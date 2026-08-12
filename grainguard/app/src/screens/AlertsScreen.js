/**
 * @file    AlertsScreen.js
 * @brief   Active alerts list with recommended actions and dismiss.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useMemo } from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity, Alert } from 'react-native';
import { useGrainGuard } from '../services/GrainGuardContext';

export default function AlertsScreen() {
  const { alerts, dismissAlert } = useGrainGuard();

  const sorted = useMemo(() => {
    return [...alerts].sort((a, b) => {
      if (a.level !== b.level) {
        return a.level === 'critical' ? -1 : 1;
      }
      return b.timestamp - a.timestamp;
    });
  }, [alerts]);

  const handleDismiss = (alert) => {
    Alert.alert(
      'Dismiss Alert',
      `Dismiss ${alert.level} alert for ${alert.siloName}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Dismiss', onPress: () => dismissAlert(alert.id) },
      ]
    );
  };

  const renderItem = ({ item }) => {
    const isCritical = item.level === 'critical';
    return (
      <TouchableOpacity
        style={[styles.alertCard, {
          borderLeftColor: isCritical ? '#D32F2F' : '#F9A825',
        }]}
        onPress={() => handleDismiss(item)}
        activeOpacity={0.7}
      >
        <View style={styles.alertHeader}>
          <Text style={styles.alertIcon}>{isCritical ? '🔴' : '🟡'}</Text>
          <View style={styles.alertInfo}>
            <Text style={styles.alertSilo}>{item.siloName}</Text>
            <Text style={styles.alertTime}>{formatTime(item.timestamp)}</Text>
          </View>
          <Text style={styles.alertSRI}>SRI {item.sri}</Text>
        </View>
        <Text style={styles.alertMsg}>{item.message}</Text>
        <Text style={styles.actionHint}>Tap to dismiss</Text>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Active Alerts</Text>
        <Text style={styles.headerCount}>{sorted.length}</Text>
      </View>
      <FlatList
        data={sorted}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyIcon}>✅</Text>
            <Text style={styles.emptyText}>No active alerts.</Text>
            <Text style={styles.emptyHint}>All silos are within safe parameters.</Text>
          </View>
        }
      />
    </View>
  );
}

function formatTime(ts) {
  if (!ts) return '';
  const d = new Date(ts);
  return d.toLocaleString();
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
    paddingHorizontal: 16, paddingVertical: 12, backgroundColor: '#1B5E20',
  },
  headerTitle: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  headerCount: {
    color: '#1B5E20', fontSize: 16, fontWeight: 'bold',
    backgroundColor: '#fff', borderRadius: 12, paddingHorizontal: 10, paddingVertical: 2,
  },
  list: { padding: 12 },
  alertCard: {
    backgroundColor: '#fff', borderRadius: 8, marginBottom: 10,
    padding: 14, borderLeftWidth: 4, elevation: 2,
  },
  alertHeader: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  alertIcon: { fontSize: 28, marginRight: 10 },
  alertInfo: { flex: 1 },
  alertSilo: { fontSize: 16, fontWeight: 'bold', color: '#333' },
  alertTime: { fontSize: 11, color: '#9E9E9E' },
  alertSRI: { fontSize: 18, fontWeight: 'bold', color: '#555' },
  alertMsg: { fontSize: 13, color: '#424242', marginBottom: 6 },
  actionHint: { fontSize: 10, color: '#BDBDBD', textAlign: 'right' },
  empty: { alignItems: 'center', padding: 60 },
  emptyIcon: { fontSize: 48, marginBottom: 12 },
  emptyText: { fontSize: 18, color: '#4CAF50', fontWeight: 'bold' },
  emptyHint: { fontSize: 13, color: '#9E9E9E', marginTop: 8 },
});