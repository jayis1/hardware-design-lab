/**
 * DashboardScreen — Hive list with health status cards
 * Author: jayis1
 * License: MIT
 */

import React, { useEffect } from 'react';
import {
  View, Text, FlatList, StyleSheet, TouchableOpacity, RefreshControl,
} from 'react-native';
import { useSelector, useDispatch } from 'react-redux';
import { fetchHives, setActiveHive, AppState } from '../store/hiveStore';
import { HiveData, ColonyState } from '../types';
import HiveCard from '../components/HiveCard';

export default function DashboardScreen({ navigation }: any) {
  const dispatch = useDispatch();
  const hives = useSelector((s: AppState) => s.hives);
  const loading = useSelector((s: AppState) => s.loading);
  const alerts = useSelector((s: AppState) => s.alerts);
  const lastSync = useSelector((s: AppState) => s.lastSync);

  useEffect(() => {
    dispatch(fetchHives() as any);
    // Refresh every 5 minutes
    const interval = setInterval(() => dispatch(fetchHives() as any), 300000);
    return () => clearInterval(interval);
  }, [dispatch]);

  const handleHivePress = (hive: HiveData) => {
    dispatch(setActiveHive(hive.id));
    navigation.navigate('HiveDetail', { hiveId: hive.id });
  };

  const handleAlertPress = (hiveId: string) => {
    dispatch(setActiveHive(hiveId));
    navigation.navigate('SwarmAlert', { hiveId });
  };

  const renderItem = ({ item }: { item: HiveData }) => (
    <HiveCard hive={item} onPress={() => handleHivePress(item)} />
  );

  const renderAlert = (alert: any) => (
    <TouchableOpacity
      key={alert.hiveId}
      style={[
        styles.alertCard,
        alert.severity === 'critical' ? styles.alertCritical : styles.alertWarning,
      ]}
      onPress={() => handleAlertPress(alert.hiveId)}
    >
      <Text style={styles.alertTitle}>
        {alert.severity === 'critical' ? '🚨' : '⚠️'} {alert.hiveName}
      </Text>
      <Text style={styles.alertMessage}>{alert.message}</Text>
      <Text style={styles.alertAction}>{alert.recommendedAction}</Text>
    </TouchableOpacity>
  );

  const healthSummary = () => {
    const healthy = hives.filter(h => h.colonyState === ColonyState.QueenrightHealthy).length;
    const needsAttention = hives.length - healthy;
    return { healthy, needsAttention, total: hives.length };
  };

  const summary = healthSummary();

  return (
    <View style={styles.container}>
      {/* Alert banner */}
      {alerts.length > 0 && (
        <View style={styles.alertsContainer}>
          {alerts.slice(0, 3).map(renderAlert)}
        </View>
      )}

      {/* Summary header */}
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Hive Fleet ({summary.total})</Text>
        <Text style={styles.headerSubtitle}>
          ✓ {summary.healthy} healthy  ·  ⚠ {summary.needsAttention} need attention
        </Text>
        <Text style={styles.syncText}>
          Last sync: {lastSync ? new Date(lastSync).toLocaleTimeString() : 'Never'}
        </Text>
      </View>

      {/* Hive list */}
      <FlatList
        data={hives}
        renderItem={renderItem}
        keyExtractor={item => item.id}
        refreshControl={
          <RefreshControl
            refreshing={loading}
            onRefresh={() => dispatch(fetchHives() as any)}
            tintColor="#E8A317"
          />
        }
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E' },
  header: {
    padding: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A2E',
  },
  headerTitle: { fontSize: 22, fontWeight: 'bold', color: '#E8A317' },
  headerSubtitle: { fontSize: 14, color: '#AAA', marginTop: 4 },
  syncText: { fontSize: 12, color: '#666', marginTop: 4 },
  alertsContainer: { padding: 8 },
  alertCard: {
    padding: 12,
    borderRadius: 8,
    marginBottom: 8,
  },
  alertCritical: {
    backgroundColor: 'rgba(220, 53, 69, 0.15)',
    borderLeftWidth: 4,
    borderLeftColor: '#DC3545',
  },
  alertWarning: {
    backgroundColor: 'rgba(255, 193, 7, 0.15)',
    borderLeftWidth: 4,
    borderLeftColor: '#FFC107',
  },
  alertTitle: { fontSize: 16, fontWeight: 'bold', color: '#FFF', marginBottom: 4 },
  alertMessage: { fontSize: 14, color: '#DDD', marginBottom: 4 },
  alertAction: { fontSize: 13, color: '#999', fontStyle: 'italic' },
  list: { padding: 8 },
});