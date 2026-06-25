/**
 * AlertsScreen.js — Push notification alerts for pest, compaction, etc.
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Alert } from 'react-native';
import { useSelector, useDispatch } from 'react-redux';
import { AlertItem } from '../components/AlertItem';

const ALERT_TYPES = {
  pest: { icon: '🐛', color: '#F44336', title: 'Pest Detected', severity: 'high' },
  compaction: { icon: '⚒️', color: '#FF9800', title: 'Compaction Risk', severity: 'medium' },
  low_battery: { icon: '🔋', color: '#FFC107', title: 'Low Battery', severity: 'low' },
  svi_drop: { icon: '📉', color: '#9C27B0', title: 'SVI Drop', severity: 'medium' },
  tilt: { icon: '📐', color: '#2196F3', title: 'Pod Displacement', severity: 'low' },
};

function generateDemoAlerts() {
  return [
    { id: 'a1', type: 'pest', podId: 'pod-001', podName: 'North Field A',
      timestamp: Date.now() - 3600000, message: 'Grub larvae chewing detected in topsoil (2-6 kHz, 23 events in 15 min)',
      severity: 'high' },
    { id: 'a2', type: 'compaction', podId: 'pod-002', podName: 'Vineyard Row 3',
      timestamp: Date.now() - 7200000, message: 'Compaction cracking rate increased 3× above baseline at 40 cm depth',
      severity: 'medium' },
    { id: 'a3', type: 'low_battery', podId: 'pod-003', podName: 'Greenhouse Bed',
      timestamp: Date.now() - 86400000, message: 'Battery at 18% (3.28V). Replace within 2 weeks.',
      severity: 'low' },
    { id: 'a4', type: 'svi_drop', podId: 'pod-001', podName: 'North Field A',
      timestamp: Date.now() - 172800000, message: 'SVI dropped from 78 to 58 in 24h after tillage event',
      severity: 'medium' },
    { id: 'a5', type: 'tilt', podId: 'pod-002', podName: 'Vineyard Row 3',
      timestamp: Date.now() - 259200000, message: 'Pod orientation changed 15° — possible animal disturbance',
      severity: 'low' },
  ];
}

export default function AlertsScreen({ navigation }) {
  const alerts = useSelector(state => state.alerts.alerts);
  const dispatch = useDispatch();
  const [demoAlerts, setDemoAlerts] = useState([]);

  useEffect(() => {
    if (alerts.length > 0) {
      setDemoAlerts(alerts);
    } else {
      setDemoAlerts(generateDemoAlerts());
    }
  }, [alerts]);

  const handleDismiss = (id) => {
    dispatch({ type: 'ALERTS_DISMISS', payload: id });
    setDemoAlerts(prev => prev.filter(a => a.id !== id));
  };

  const handleAction = (alert) => {
    if (alert.type === 'pest') {
      Alert.alert(
        'Pest Detection',
        `${alert.message}\n\nRecommended action: Inspect the area around ${alert.podName} for root-feeding larvae. Consider targeted biological control (e.g. beneficial nematodes).`,
        [{ text: 'OK' }]
      );
    } else if (alert.type === 'compaction') {
      Alert.alert(
        'Compaction Risk',
        `${alert.message}\n\nRecommended action: Reduce machinery traffic in this zone. Consider cover cropping with deep-rooted species (daikon, chicory).`,
        [{ text: 'OK' }]
      );
    } else {
      Alert.alert(ALERT_TYPES[alert.type]?.title || 'Alert', alert.message, [{ text: 'OK' }]);
    }
  };

  const renderItem = ({ item }) => (
    <AlertItem
      alert={item}
      typeInfo={ALERT_TYPES[item.type] || { icon: '⚠️', color: '#757575', title: 'Unknown' }}
      onDismiss={() => handleDismiss(item.id)}
      onAction={() => handleAction(item)}
      onViewPod={() => navigation.navigate('PodDetail', { podId: item.podId })}
    />
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Alerts</Text>
        <Text style={styles.subtitle}>{demoAlerts.length} active alerts</Text>
      </View>
      <FlatList
        data={demoAlerts}
        keyExtractor={item => item.id}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { padding: 15, backgroundColor: '#2E7D32', alignItems: 'center' },
  title: { fontSize: 20, fontWeight: 'bold', color: '#FFFFFF' },
  subtitle: { fontSize: 12, color: '#E8F5E9' },
  list: { padding: 10 },
});