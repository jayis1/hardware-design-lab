/**
 * Terramesh — Dashboard Screen
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Main dashboard showing node summary, classification status, and
 * recent sensor readings in a card-based layout.
 */

import React, { useEffect, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  RefreshControl,
  TouchableOpacity,
  Dimensions,
} from 'react-native';
import { useDevices } from '../components/DeviceContext';
import { protocol } from '../utils/protocol';

const { width } = Dimensions.get('window');
const CARD_MARGIN = 8;
const CARD_WIDTH = (width - CARD_MARGIN * 3) / 2;

const DashboardScreen = ({ navigation }) => {
  const {
    nodes,
    getNodeCount,
    getOnlineCount,
    getCriticalCount,
    getWarningCount,
    gatewayConnected,
  } = useDevices();

  const [refreshing, setRefreshing] = useState(false);

  const onRefresh = async () => {
    setRefreshing(true);
    // In production, trigger BLE scan or API poll
    setTimeout(() => setRefreshing(false), 2000);
  };

  const nodeList = Object.values(nodes).sort((a, b) => a.id.localeCompare(b.id));

  const StatCard = ({ title, value, color, icon }) => (
    <View style={[styles.statCard, { borderLeftColor: color }]}>
      <Text style={styles.statIcon}>{icon}</Text>
      <Text style={[styles.statValue, { color }]}>{value}</Text>
      <Text style={styles.statTitle}>{title}</Text>
    </View>
  );

  const NodeCard = ({ node, onPress }) => {
    const clsColor = protocol.classificationColor(node.classification);
    const clsText = protocol.classificationToString(node.classification);
    const batteryPct = Math.min(100, Math.round((node.battery / 7200) * 100));

    return (
      <TouchableOpacity style={styles.nodeCard} onPress={onPress}>
        <View style={styles.nodeHeader}>
          <Text style={styles.nodeId}>{node.id}</Text>
          <View style={[styles.statusDot, { backgroundColor: clsColor }]} />
        </View>
        <Text style={[styles.classification, { color: clsColor }]}>{clsText}</Text>
        <View style={styles.nodeSensors}>
          <Text style={styles.sensorText}>
            Tilt: {node.sensors.tiltX.toFixed(3)}°
          </Text>
          <Text style={styles.sensorText}>
            Pore P: {node.sensors.porePressureShallow.toFixed(1)} kPa
          </Text>
          <Text style={styles.sensorText}>
            Moisture: {node.sensors.moisture.toFixed(1)}%
          </Text>
          <Text style={styles.sensorText}>
            Temp: {node.sensors.temperature.toFixed(1)}°C
          </Text>
        </View>
        <View style={styles.nodeFooter}>
          <Text style={styles.footerText}>
            RSSI: {node.rssi} dBm
          </Text>
          <Text style={styles.footerText}>
            Bat: {batteryPct}%
          </Text>
        </View>
      </TouchableOpacity>
    );
  };

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} tintColor="#00d4aa" />
      }
    >
      {/* Connection Status */}
      <View style={[styles.connectionBar, {
        backgroundColor: gatewayConnected ? '#003d33' : '#3d0000'
      }]}>
        <Text style={styles.connectionText}>
          {gatewayConnected ? '● Gateway Connected' : '○ Gateway Offline'}
        </Text>
      </View>

      {/* Summary Stats */}
      <View style={styles.statsRow}>
        <StatCard title="Total Nodes" value={getNodeCount()} color="#00d4aa" icon="📡" />
        <StatCard title="Online" value={getOnlineCount()} color="#4fc3f7" icon="🟢" />
        <StatCard title="Warning" value={getWarningCount()} color="#ffa500" icon="⚠️" />
        <StatCard title="Critical" value={getCriticalCount()} color="#ff4444" icon="🚨" />
      </View>

      {/* Node List */}
      <Text style={styles.sectionTitle}>Nodes</Text>
      <View style={styles.nodeGrid}>
        {nodeList.length === 0 ? (
          <View style={styles.emptyState}>
            <Text style={styles.emptyIcon}>📡</Text>
            <Text style={styles.emptyText}>No nodes detected</Text>
            <Text style={styles.emptySubtext}>
              Ensure the gateway is connected and nodes are within range
            </Text>
          </View>
        ) : (
          nodeList.map(node => (
            <NodeCard
              key={node.id}
              node={node}
              onPress={() => navigation.navigate('NodeDetail', { nodeId: node.id })}
            />
          ))
        )}
      </View>

      {/* Last Update */}
      <Text style={styles.updateText}>
        Last updated: {new Date().toLocaleTimeString()}
      </Text>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
  },
  connectionBar: {
    padding: 10,
    marginHorizontal: 10,
    marginTop: 10,
    borderRadius: 8,
  },
  connectionText: {
    color: '#e0e0e0',
    fontSize: 14,
    fontWeight: '600',
    textAlign: 'center',
  },
  statsRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    padding: 8,
  },
  statCard: {
    width: CARD_WIDTH,
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    margin: CARD_MARGIN / 2,
    borderLeftWidth: 3,
  },
  statIcon: {
    fontSize: 20,
    marginBottom: 4,
  },
  statValue: {
    fontSize: 28,
    fontWeight: 'bold',
  },
  statTitle: {
    fontSize: 12,
    color: '#6c6c80',
    marginTop: 2,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#e0e0e0',
    paddingHorizontal: 16,
    paddingTop: 16,
    paddingBottom: 8,
  },
  nodeGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    padding: 8,
  },
  nodeCard: {
    width: CARD_WIDTH,
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 14,
    margin: CARD_MARGIN / 2,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  nodeHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 4,
  },
  nodeId: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#e0e0e0',
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
  },
  classification: {
    fontSize: 12,
    fontWeight: '600',
    marginBottom: 8,
  },
  nodeSensors: {
    marginBottom: 8,
  },
  sensorText: {
    fontSize: 11,
    color: '#a0a0b0',
    lineHeight: 16,
  },
  nodeFooter: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    borderTopWidth: 1,
    borderTopColor: '#2a2a3e',
    paddingTop: 6,
  },
  footerText: {
    fontSize: 10,
    color: '#6c6c80',
  },
  emptyState: {
    width: '100%',
    alignItems: 'center',
    padding: 40,
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
  updateText: {
    fontSize: 11,
    color: '#4a4a5a',
    textAlign: 'center',
    padding: 16,
  },
});

export default DashboardScreen;
