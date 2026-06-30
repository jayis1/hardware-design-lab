/**
 * NodesScreen.tsx — SpectraPest node management and commissioning
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert } from 'react-native';
import {
  Card,
  Title,
  Paragraph,
  Button,
  Divider,
  TextInput,
  Switch,
  ProgressBar,
  Dialog,
  Portal,
  List,
  IconButton,
} from 'react-native-paper';
import { useApp } from '../context/AppContext';
import { SpectraPestNode } from '../types';

export default function NodesScreen() {
  const { nodes, api, loading, refreshData } = useApp();
  const [selectedNode, setSelectedNode] = useState<SpectraPestNode | null>(null);
  const [configDialog, setConfigDialog] = useState(false);
  const [otaDialog, setOtaDialog] = useState(false);
  const [otaProgress, setOtaProgress] = useState(0);

  const handleConfigure = (node: SpectraPestNode) => {
    setSelectedNode(node);
    setConfigDialog(true);
  };

  const handleOTA = (node: SpectraPestNode) => {
    setSelectedNode(node);
    setOtaProgress(0);
    setOtaDialog(true);

    /* Simulate OTA progress */
    const interval = setInterval(() => {
      setOtaProgress(p => {
        if (p >= 1) {
          clearInterval(interval);
          api.triggerOTA(node.id).catch(() => {});
          return 1;
        }
        return p + 0.1;
      });
    }, 200);
  };

  const handleSaveConfig = async () => {
    if (!selectedNode) return;
    try {
      await api.updateNodeConfig(selectedNode.id, selectedNode.config);
      setConfigDialog(false);
      Alert.alert('Success', 'Configuration updated');
      refreshData();
    } catch (err) {
      Alert.alert('Error', err instanceof Error ? err.message : 'Failed to update config');
    }
  };

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <ScrollView.RefreshControl refreshing={loading} onRefresh={refreshData} />
      }
    >
      <Card style={styles.card}>
        <Card.Content>
          <Title>SpectraPest Nodes ({nodes.length})</Title>
          <Paragraph style={styles.subtitle}>
            Manage mesh network nodes, configure detection parameters, and deploy OTA updates.
          </Paragraph>

          {nodes.length === 0 ? (
            <Text style={styles.emptyText}>
              No nodes found. Ensure your gateway is connected and nodes are powered on.
            </Text>
          ) : (
            nodes.map((node, idx) => (
              <View key={idx} style={styles.nodeCard}>
                <View style={styles.nodeHeader}>
                  <View style={styles.nodeInfo}>
                    <Text style={styles.nodeLabel}>{node.label || `Node ${node.node_addr}`}</Text>
                    <Text style={styles.nodeField}>Field: {node.field_name || 'Unassigned'}</Text>
                    <Text style={styles.nodeAddr}>
                      Mesh addr: 0x{node.node_addr.toString(16).toUpperCase().padStart(2, '0')}
                    </Text>
                  </View>
                  <View style={styles.nodeStatus}>
                    <View style={[
                      styles.statusDot,
                      { backgroundColor: node.battery_pct > 50 ? '#4CAF50' :
                        node.battery_pct > 20 ? '#FFB74D' : '#E57373' }
                    ]} />
                    <Text style={styles.batteryText}>{node.battery_pct}%</Text>
                    {node.solar_charging && <Text style={styles.solarIcon}>☀</Text>}
                  </View>
                </View>

                <Divider style={styles.divider} />

                <View style={styles.nodeDetails}>
                  <View style={styles.detailRow}>
                    <Text style={styles.detailLabel}>GPS:</Text>
                    <Text style={styles.detailValue}>
                      {node.gps.lat.toFixed(4)}, {node.gps.lon.toFixed(4)}
                    </Text>
                  </View>
                  <View style={styles.detailRow}>
                    <Text style={styles.detailLabel}>Firmware:</Text>
                    <Text style={styles.detailValue}>{node.firmware_version}</Text>
                  </View>
                  <View style={styles.detailRow}>
                    <Text style={styles.detailLabel}>Detection interval:</Text>
                    <Text style={styles.detailValue}>
                      {node.config.detection_interval_sec}s
                    </Text>
                  </View>
                  <View style={styles.detailRow}>
                    <Text style={styles.detailLabel}>LoRa freq:</Text>
                    <Text style={styles.detailValue}>
                      {(node.config.lora_frequency_hz / 1e6).toFixed(1)} MHz
                    </Text>
                  </View>
                  <View style={styles.detailRow}>
                    <Text style={styles.detailLabel}>Mesh role:</Text>
                    <Text style={styles.detailValue}>{node.config.mesh_role}</Text>
                  </View>
                  <View style={styles.detailRow}>
                    <Text style={styles.detailLabel}>Last seen:</Text>
                    <Text style={styles.detailValue}>{node.last_seen}</Text>
                  </View>
                </View>

                <View style={styles.nodeActions}>
                  <Button
                    mode="contained"
                    onPress={() => handleConfigure(node)}
                    style={styles.actionButton}
                    icon="cog"
                  >
                    Configure
                  </Button>
                  <Button
                    mode="outlined"
                    onPress={() => handleOTA(node)}
                    style={styles.actionButton}
                    icon="update"
                  >
                    OTA Update
                  </Button>
                </View>

                {node.latest_detection && (
                  <View style={styles.latestDetection}>
                    <Text style={styles.latestLabel}>Latest detection:</Text>
                    <Text style={styles.latestValue}>
                      {node.latest_detection.species} ({(node.latest_detection.confidence * 100).toFixed(0)}%)
                    </Text>
                  </View>
                )}
              </View>
            ))
          )}
        </Card.Content>
      </Card>

      {/* Configuration dialog */}
      <Portal>
        <Dialog visible={configDialog} onDismiss={() => setConfigDialog(false)}>
          <Dialog.Title>Configure {selectedNode?.label}</Dialog.Title>
          <Dialog.Content>
            {selectedNode && (
              <View>
                <Text style={styles.configLabel}>Detection interval (seconds)</Text>
                <TextInput
                  keyboardType="numeric"
                  value={selectedNode.config.detection_interval_sec.toString()}
                  onChangeText={(val) => {
                    if (selectedNode) {
                      setSelectedNode({
                        ...selectedNode,
                        config: {
                          ...selectedNode.config,
                          detection_interval_sec: parseInt(val) || 60,
                        },
                      });
                    }
                  }}
                  style={styles.configInput}
                />

                <Text style={styles.configLabel}>LoRa frequency (MHz)</Text>
                <TextInput
                  keyboardType="numeric"
                  value={(selectedNode.config.lora_frequency_hz / 1e6).toString()}
                  onChangeText={(val) => {
                    if (selectedNode) {
                      setSelectedNode({
                        ...selectedNode,
                        config: {
                          ...selectedNode.config,
                          lora_frequency_hz: parseFloat(val) * 1e6 || 915e6,
                        },
                      });
                    }
                  }}
                  style={styles.configInput}
                />

                <Text style={styles.configLabel}>Mesh role</Text>
                <TextInput
                  value={selectedNode.config.mesh_role}
                  onChangeText={(val) => {
                    if (selectedNode) {
                      setSelectedNode({
                        ...selectedNode,
                        config: { ...selectedNode.config, mesh_role: val as any },
                      });
                    }
                  }}
                  style={styles.configInput}
                />
              </View>
            )}
          </Dialog.Content>
          <Dialog.Actions>
            <Button onPress={() => setConfigDialog(false)}>Cancel</Button>
            <Button onPress={handleSaveConfig}>Save</Button>
          </Dialog.Actions>
        </Dialog>
      </Portal>

      {/* OTA update dialog */}
      <Portal>
        <Dialog visible={otaDialog} onDismiss={() => setOtaDialog(false)}>
          <Dialog.Title>OTA Firmware Update</Dialog.Title>
          <Dialog.Content>
            <Text style={styles.otaText}>
              Updating {selectedNode?.label}...
            </Text>
            <ProgressBar progress={otaProgress} color="#2E7D32" />
            <Text style={styles.otaProgress}>
              {Math.round(otaProgress * 100)}% complete
            </Text>
            {otaProgress >= 1 && (
              <Text style={styles.otaComplete}>
                ✓ Update complete! Node will reboot.
              </Text>
            )}
          </Dialog.Content>
          <Dialog.Actions>
            <Button onPress={() => setOtaDialog(false)} disabled={otaProgress < 1}>
              Close
            </Button>
          </Dialog.Actions>
        </Dialog>
      </Portal>
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
  subtitle: {
    fontSize: 12,
    color: '#757575',
    marginBottom: 16,
  },
  emptyText: {
    color: '#9E9E9E',
    fontStyle: 'italic',
    paddingVertical: 20,
    textAlign: 'center',
  },
  nodeCard: {
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
    backgroundColor: '#fff',
  },
  nodeHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  nodeInfo: {
    flex: 1,
  },
  nodeLabel: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333',
  },
  nodeField: {
    fontSize: 12,
    color: '#757575',
  },
  nodeAddr: {
    fontSize: 11,
    color: '#9E9E9E',
    fontFamily: 'monospace',
  },
  nodeStatus: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 6,
  },
  batteryText: {
    fontSize: 14,
    fontWeight: '500',
    color: '#558B2F',
  },
  solarIcon: {
    fontSize: 14,
    marginLeft: 4,
  },
  divider: {
    marginVertical: 8,
  },
  nodeDetails: {
    marginTop: 4,
  },
  detailRow: {
    flexDirection: 'row',
    paddingVertical: 2,
  },
  detailLabel: {
    fontSize: 12,
    color: '#757575',
    width: 130,
  },
  detailValue: {
    fontSize: 12,
    color: '#333',
    fontFamily: 'monospace',
  },
  nodeActions: {
    flexDirection: 'row',
    marginTop: 12,
    justifyContent: 'space-around',
  },
  actionButton: {
    flex: 1,
    marginHorizontal: 4,
  },
  latestDetection: {
    marginTop: 8,
    padding: 8,
    backgroundColor: '#E8F5E9',
    borderRadius: 4,
  },
  latestLabel: {
    fontSize: 11,
    color: '#757575',
  },
  latestValue: {
    fontSize: 13,
    fontWeight: '500',
    color: '#2E7D32',
  },
  configLabel: {
    fontSize: 12,
    color: '#757575',
    marginTop: 8,
    marginBottom: 4,
  },
  configInput: {
    marginBottom: 12,
  },
  otaText: {
    fontSize: 14,
    marginBottom: 12,
  },
  otaProgress: {
    fontSize: 12,
    color: '#757575',
    marginTop: 8,
    textAlign: 'center',
  },
  otaComplete: {
    fontSize: 13,
    color: '#4CAF50',
    marginTop: 12,
    textAlign: 'center',
  },
});