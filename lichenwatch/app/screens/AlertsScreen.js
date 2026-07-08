/*
 * AlertsScreen.js — History of state transitions and desiccation events.
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View, Text, FlatList, StyleSheet, TouchableOpacity,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { BleContext } from '../utils/ble';
import { STATE_NAMES, STATE_COLORS } from '../utils/lichen';

const ALERTS_KEY = 'lichenwatch_alerts';

export default function AlertsScreen() {
  const { bleState } = useContext(BleContext);
  const [alerts, setAlerts] = useState([]);

  const loadAlerts = async () => {
    const raw = await AsyncStorage.getItem(ALERTS_KEY);
    setAlerts(raw ? JSON.parse(raw) : []);
  };

  useEffect(() => {
    loadAlerts();
  }, [bleState.latest]);

  /* When a new status arrives with a bleaching state or high click count,
   * prepend an alert. */
  useEffect(() => {
    if (!bleState.latest) return;
    const s = bleState.latest;
    if (s.class_state === 2 || s.clicks > 20) {
      const alert = {
        id: Date.now(),
        timestamp: new Date().toISOString(),
        nodeId: bleState.connectedNodeId,
        type: s.class_state === 2 ? 'BLEACHING' : 'DESICCATION',
        state: STATE_NAMES[s.class_state],
        clicks: s.clicks,
        fv_fm: s.fv_fm,
      };
      const updated = [alert, ...alerts].slice(0, 100);
      setAlerts(updated);
      AsyncStorage.setItem(ALERTS_KEY, JSON.stringify(updated));
    }
  }, [bleState.latest]);

  const clearAlerts = async () => {
    await AsyncStorage.removeItem(ALERTS_KEY);
    setAlerts([]);
  };

  const renderItem = ({ item }) => {
    const isBleaching = item.type === 'BLEACHING';
    const color = isBleaching ? STATE_COLORS[2] : '#ef6c00';
    return (
      <View style={[styles.alertItem, { borderLeftColor: color }]}>
        <Icon
          name={isBleaching ? 'alert-octagon' : 'lightning-bolt'}
          size={28}
          color={color}
        />
        <View style={styles.alertBody}>
          <Text style={styles.alertTitle}>{item.type}</Text>
          <Text style={styles.alertMeta}>
            Node {item.nodeId} · {new Date(item.timestamp).toLocaleString()}
          </Text>
          <Text style={styles.alertDetail}>
            State: {item.state} · Fv/Fm: {item.fv_fm?.toFixed(3)} · Clicks: {item.clicks}
          </Text>
        </View>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Alerts ({alerts.length})</Text>
        {alerts.length > 0 && (
          <TouchableOpacity onPress={clearAlerts}>
            <Text style={styles.clearBtn}>Clear all</Text>
          </TouchableOpacity>
        )}
      </View>
      <FlatList
        data={alerts}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderItem}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Icon name="bell-off-outline" size={48} color="#ccc" />
            <Text style={styles.emptyText}>
              No alerts. Bleaching events and desiccation storms will appear
              here as they're received from your nodes.
            </Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#fafafa' },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 14,
  },
  title: { fontSize: 18, fontWeight: 'bold', color: '#1b3a1b' },
  clearBtn: { color: '#c62828', fontSize: 14 },
  alertItem: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 14,
    backgroundColor: '#fff',
    borderBottomWidth: 1,
    borderBottomColor: '#eee',
    borderLeftWidth: 4,
  },
  alertBody: { flex: 1, marginLeft: 12 },
  alertTitle: { fontSize: 15, fontWeight: 'bold', color: '#1b3a1b' },
  alertMeta: { fontSize: 11, color: '#888' },
  alertDetail: { fontSize: 12, color: '#555', marginTop: 2 },
  empty: { padding: 40, alignItems: 'center' },
  emptyText: { color: '#999', textAlign: 'center', marginTop: 12, fontSize: 13 },
});