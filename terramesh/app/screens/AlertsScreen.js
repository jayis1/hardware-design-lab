/**
 * Terramesh — Alerts Screen
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Displays active and historical alerts from the Terramesh mesh.
 * Supports acknowledgment and filtering by severity.
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
} from 'react-native';
import { useDevices } from '../components/DeviceContext';
import { protocol } from '../utils/protocol';

const AlertsScreen = () => {
  const { alerts, acknowledgeAlert, clearAlerts } = useDevices();
  const [filter, setFilter] = useState('all');

  const filteredAlerts = alerts.filter(a => {
    if (filter === 'all') return true;
    if (filter === 'unacknowledged') return !a.acknowledged;
    return a.type.toLowerCase() === filter;
  });

  const getAlertColor = (type) => {
    switch (type) {
      case 'CRITICAL': return '#ff4444';
      case 'WARNING': return '#ffa500';
      default: return '#6c6c80';
    }
  };

  const renderAlert = ({ item }) => (
    <TouchableOpacity
      style={[
        styles.alertCard,
        { borderLeftColor: getAlertColor(item.type) },
        item.acknowledged && styles.alertAcknowledged,
      ]}
      onPress={() => acknowledgeAlert(item.id)}
    >
      <View style={styles.alertHeader}>
        <View style={[styles.alertType, {
          backgroundColor: getAlertColor(item.type)
        }]}>
          <Text style={styles.alertTypeText}>{item.type}</Text>
        </View>
        <Text style={styles.alertTime}>
          {new Date(item.timestamp).toLocaleTimeString()}
        </Text>
      </View>
      <Text style={styles.alertMessage}>{item.message}</Text>
      <View style={styles.alertFooter}>
        <Text style={styles.alertNode}>{item.nodeId}</Text>
        {item.acknowledged ? (
          <Text style={styles.ackText}>✓ Acknowledged</Text>
        ) : (
          <Text style={styles.tapText}>Tap to acknowledge</Text>
        )}
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      {/* Filter bar */}
      <View style={styles.filterBar}>
        {['all', 'unacknowledged', 'critical', 'warning'].map(f => (
          <TouchableOpacity
            key={f}
            style={[styles.filterButton, filter === f && styles.filterActive]}
            onPress={() => setFilter(f)}
          >
            <Text style={[styles.filterText, filter === f && styles.filterTextActive]}>
              {f.charAt(0).toUpperCase() + f.slice(1)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Alert count */}
      <View style={styles.countBar}>
        <Text style={styles.countText}>
          {filteredAlerts.length} alert{filteredAlerts.length !== 1 ? 's' : ''}
          {filter !== 'all' ? ` (${filter})` : ''}
        </Text>
        {alerts.length > 0 && (
          <TouchableOpacity onPress={clearAlerts}>
            <Text style={styles.clearText}>Clear All</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Alert list */}
      <FlatList
        data={filteredAlerts}
        renderItem={renderAlert}
        keyExtractor={item => item.id}
        contentContainerStyle={styles.listContent}
        ListEmptyComponent={
          <View style={styles.emptyState}>
            <Text style={styles.emptyIcon}>✅</Text>
            <Text style={styles.emptyText}>No alerts</Text>
            <Text style={styles.emptySubtext}>
              All nodes are operating normally
            </Text>
          </View>
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
  },
  filterBar: {
    flexDirection: 'row',
    padding: 10,
    gap: 8,
  },
  filterButton: {
    paddingHorizontal: 14,
    paddingVertical: 8,
    borderRadius: 20,
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  filterActive: {
    backgroundColor: '#00d4aa',
    borderColor: '#00d4aa',
  },
  filterText: {
    fontSize: 12,
    color: '#6c6c80',
    fontWeight: '500',
  },
  filterTextActive: {
    color: '#0f0f1a',
  },
  countBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 16,
    paddingBottom: 8,
  },
  countText: {
    fontSize: 12,
    color: '#6c6c80',
  },
  clearText: {
    fontSize: 12,
    color: '#ff4444',
    fontWeight: '600',
  },
  listContent: {
    padding: 10,
  },
  alertCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
    borderLeftWidth: 3,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  alertAcknowledged: {
    opacity: 0.6,
  },
  alertHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  alertType: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 8,
  },
  alertTypeText: {
    fontSize: 10,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  alertTime: {
    fontSize: 11,
    color: '#6c6c80',
  },
  alertMessage: {
    fontSize: 14,
    color: '#e0e0e0',
    lineHeight: 20,
  },
  alertFooter: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 8,
    paddingTop: 6,
    borderTopWidth: 1,
    borderTopColor: '#2a2a3e',
  },
  alertNode: {
    fontSize: 11,
    color: '#4fc3f7',
    fontWeight: '500',
  },
  ackText: {
    fontSize: 11,
    color: '#00d4aa',
  },
  tapText: {
    fontSize: 11,
    color: '#6c6c80',
    fontStyle: 'italic',
  },
  emptyState: {
    alignItems: 'center',
    padding: 60,
  },
  emptyIcon: {
    fontSize: 48,
    marginBottom: 12,
  },
  emptyText: {
    fontSize: 18,
    color: '#e0e0e0',
    fontWeight: '600',
  },
  emptySubtext: {
    fontSize: 14,
    color: '#6c6c80',
    textAlign: 'center',
    marginTop: 8,
  },
});

export default AlertsScreen;
