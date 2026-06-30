/**
 * DashboardScreen.tsx — Real-time pest pressure dashboard
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView, RefreshControl } from 'react-native';
import { Card, Title, Paragraph, Badge, Button, IconButton, ActivityIndicator } from 'react-native-paper';
import { useApp } from '../context/AppContext';
import { SPECIES_NAMES, SEVERITY_COLORS, SEVERITY_LABELS } from '../types';

export default function DashboardScreen() {
  const { gatewayStatus, detections, nodes, loading, error, refreshData, lastUpdate } = useApp();

  const recentDetections = detections.slice(0, 10);
  const pestDetections = detections.filter(d => d.severity !== 'none');
  const highSeverity = pestDetections.filter(d => d.severity === 'high');
  const totalNodes = nodes.length;
  const onlineNodes = nodes.filter(n => n.battery_pct > 0).length;

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={loading} onRefresh={refreshData} />
      }
    >
      {/* Gateway status banner */}
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.row}>
            <Title>SpectraPest Gateway</Title>
            <Badge
              style={[
                styles.statusBadge,
                { backgroundColor: gatewayStatus?.online ? '#4CAF50' : '#E57373' }
              ]}
            >
              {gatewayStatus?.online ? 'ONLINE' : 'OFFLINE'}
            </Badge>
          </View>
          <View style={styles.statsRow}>
            <View style={styles.stat}>
              <Text style={styles.statLabel}>Nodes</Text>
              <Text style={styles.statValue}>{totalNodes}</Text>
              <Text style={styles.statSubtext}>{onlineNodes} online</Text>
            </View>
            <View style={styles.stat}>
              <Text style={styles.statLabel}>Detections</Text>
              <Text style={styles.statValue}>{detections.length}</Text>
              <Text style={styles.statSubtext}>{pestDetections.length} pests</Text>
            </View>
            <View style={styles.stat}>
              <Text style={styles.statLabel}>High Priority</Text>
              <Text style={styles.statValue}>{highSeverity.length}</Text>
              <Text style={styles.statSubtext}>urgent alerts</Text>
            </View>
          </View>
          {lastUpdate && (
            <Text style={styles.updateText}>
              Last updated: {lastUpdate.toLocaleTimeString()}
            </Text>
          )}
        </Card.Content>
      </Card>

      {/* Error display */}
      {error && (
        <Card style={[styles.card, styles.errorCard]}>
          <Card.Content>
            <View style={styles.row}>
              <Text style={styles.errorText}>⚠ {error}</Text>
              <Button mode="text" onPress={refreshData}>Retry</Button>
            </View>
          </Card.Content>
        </Card>
      )}

      {/* Recent detections list */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Recent Detections</Title>
          {recentDetections.length === 0 ? (
            <Text style={styles.emptyText}>No detections yet</Text>
          ) : (
            recentDetections.map((det, idx) => (
              <View key={det.id || idx} style={styles.detectionRow}>
                <View
                  style={[
                    styles.severityDot,
                    { backgroundColor: SEVERITY_COLORS[det.severity] || '#CCC' }
                  ]}
                />
                <View style={styles.detectionInfo}>
                  <Text style={styles.speciesName}>
                    {SPECIES_NAMES[det.species_id] || 'Unknown species'}
                  </Text>
                  <Text style={styles.detectionMeta}>
                    Node {det.node_id} • {det.confidence.toFixed(0)}% • {det.timestamp}
                  </Text>
                </View>
                <Text style={[styles.severityLabel, { color: SEVERITY_COLORS[det.severity] }]}>
                  {SEVERITY_LABELS[det.severity] || 'N/A'}
                </Text>
              </View>
            ))
          )}
        </Card.Content>
      </Card>

      {/* Node battery summary */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Node Status</Title>
          {nodes.map((node, idx) => (
            <View key={node.id || idx} style={styles.nodeRow}>
              <Text style={styles.nodeLabel}>{node.label || `Node ${node.node_addr}`}</Text>
              <View style={styles.nodeBatteryRow}>
                <Text style={styles.batteryText}>{node.battery_pct}%</Text>
                {node.solar_charging && (
                  <Text style={styles.chargingText}>☀</Text>
                )}
              </View>
              <Text style={styles.nodeField}>{node.field_name}</Text>
            </View>
          ))}
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F1F8E9',
    padding: 8,
  },
  card: {
    marginBottom: 12,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  statsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 16,
    marginBottom: 8,
  },
  stat: {
    alignItems: 'center',
  },
  statLabel: {
    fontSize: 12,
    color: '#757575',
  },
  statValue: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#2E7D32',
  },
  statSubtext: {
    fontSize: 10,
    color: '#9E9E9E',
  },
  statusBadge: {
    color: '#fff',
  },
  updateText: {
    fontSize: 11,
    color: '#9E9E9E',
    marginTop: 4,
  },
  errorCard: {
    backgroundColor: '#FFEBEE',
  },
  errorText: {
    color: '#D32F2F',
    flex: 1,
  },
  emptyText: {
    color: '#9E9E9E',
    fontStyle: 'italic',
    paddingVertical: 20,
    textAlign: 'center',
  },
  detectionRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 12,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#E0E0E0',
  },
  severityDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 12,
  },
  detectionInfo: {
    flex: 1,
  },
  speciesName: {
    fontSize: 14,
    fontWeight: '500',
    color: '#333',
  },
  detectionMeta: {
    fontSize: 11,
    color: '#9E9E9E',
    marginTop: 2,
  },
  severityLabel: {
    fontSize: 12,
    fontWeight: 'bold',
  },
  nodeRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 8,
    justifyContent: 'space-between',
  },
  nodeLabel: {
    fontSize: 14,
    flex: 2,
  },
  nodeBatteryRow: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
  },
  batteryText: {
    fontSize: 13,
    color: '#558B2F',
  },
  chargingText: {
    marginLeft: 4,
    fontSize: 14,
  },
  nodeField: {
    fontSize: 11,
    color: '#9E9E9E',
    flex: 1,
  },
});