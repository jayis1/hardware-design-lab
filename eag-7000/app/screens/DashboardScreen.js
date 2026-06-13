/**
 * EAG-7000 — Dashboard Screen
 *
 * Displays real-time system overview:
 * - CPU/NPU/DRAM utilization and temperature
 * - Power status (PoE+, voltage)
 * - Network status (ETH0, ETH1 link)
 * - CAN bus activity
 * - System uptime and heartbeat
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useContext } from 'react';
import { View, ScrollView, StyleSheet, RefreshControl } from 'react-native';
import { Text, Card, ProgressBar, Divider } from 'react-native-paper';
import { BLEContext } from '../components/BLEProvider';
import { MSG_TYPE, STATUS_FLAGS, parseStatusFlags, decodeMessage, decodeSensorData, SENSOR_TYPE } from '../utils/protocol';

// System metrics (updated from BLE notifications)
const initialMetrics = {
  cpuTemp: 0,        // °C × 100
  npuTemp: 0,        // °C × 100
  dramTemp: 0,       // °C × 100
  cpuUsage: 0,       // 0-100%
  npuUsage: 0,       // 0-100%
  dramUsage: 0,      // 0-100%
  uptime: 0,         // seconds
  statusFlags: 0,    // System status bitmask
  can0Count: 0,      // CAN bus 0 frame count
  can1Count: 0,      // CAN bus 1 frame count
  inputVoltage: 0,   // mV
  poeActive: false,
  eth0Link: false,
  eth1Link: false,
};

function DashboardScreen() {
  const { connected, device, subscribeToCharacteristic, writeCharacteristic } = useContext(BLEContext);
  const [metrics, setMetrics] = useState(initialMetrics);
  const [refreshing, setRefreshing] = useState(false);

  useEffect(() => {
    if (!connected) return;

    // Subscribe to BLE notifications from gateway
    const unsubscribe = subscribeToCharacteristic((data) => {
      const bytes = new Uint8Array(data);
      if (bytes.length < 4) return;

      const msg = decodeMessage(bytes);

      switch (msg.type) {
        case MSG_TYPE.HEARTBEAT:
          // Heartbeat: data = uptime seconds (lower 16 bits)
          setMetrics(prev => ({
            ...prev,
            uptime: (prev.uptime & 0xFFFF0000) | msg.data,
          }));
          break;

        case MSG_TYPE.STATUS:
          // Status: data = status flags bitmask
          setMetrics(prev => ({
            ...prev,
            statusFlags: msg.data,
            poeActive: !!(msg.data & STATUS_FLAGS.POE_POWERED),
            eth0Link: !!(msg.data & STATUS_FLAGS.ETH0_LINK),
            eth1Link: !!(msg.data & STATUS_FLAGS.ETH1_LINK),
          }));
          break;

        case MSG_TYPE.SENSOR_DATA:
          // Sensor data: decode temperature, voltage, etc.
          const sensor = decodeSensorData(bytes);
          setMetrics(prev => {
            const updated = { ...prev };
            switch (sensor.type) {
              case SENSOR_TYPE.TEMPERATURE:
                if (sensor.channel === 0) updated.cpuTemp = sensor.value;
                else if (sensor.channel === 1) updated.npuTemp = sensor.value;
                else if (sensor.channel === 2) updated.dramTemp = sensor.value;
                break;
              case SENSOR_TYPE.VOLTAGE:
                updated.inputVoltage = sensor.value;
                break;
            }
            return updated;
          });
          break;

        case MSG_TYPE.CAN_RX:
          // CAN frame received — increment frame counter
          const bus = bytes.length > 1 ? bytes[1] : 0;
          setMetrics(prev => ({
            ...prev,
            can0Count: bus === 0 ? prev.can0Count + 1 : prev.can0Count,
            can1Count: bus === 1 ? prev.can1Count + 1 : prev.can1Count,
          }));
          break;
      }
    });

    return () => {
      if (unsubscribe) unsubscribe();
    };
  }, [connected, subscribeToCharacteristic]);

  const onRefresh = async () => {
    setRefreshing(true);
    // Request status update from gateway
    if (connected) {
      await writeCharacteristic(new Uint8Array([MSG_TYPE.STATUS, 0x00, 0x00, 0x00]));
    }
    setRefreshing(false);
  };

  const formatTemp = (tempX100) => {
    if (tempX100 === 0) return '--.-°C';
    return `${(tempX100 / 100).toFixed(1)}°C`;
  };

  const formatVoltage = (mV) => {
    if (mV === 0) return '--.-V';
    return `${(mV / 1000).toFixed(1)}V`;
  };

  const formatUptime = (seconds) => {
    const hrs = Math.floor(seconds / 3600);
    const mins = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    return `${hrs}h ${mins}m ${secs}s`;
  };

  const connectionColor = connected ? '#4CAF50' : '#F44336';
  const connectionText = connected ? (device?.name || 'Connected') : 'Disconnected';

  return (
    <ScrollView
      style={styles.container}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
    >
      {/* Connection Status */}
      <Card style={styles.card}>
        <Card.Title title="EAG-7000 Gateway" subtitle={connectionText}
          left={(props) => <Icon name="router-wireless" size={24} color={connectionColor} />}
          titleStyle={{ color: '#E0E0E0' }}
          subtitleStyle={{ color: connectionColor }}
        />
      </Card>

      {/* System Health */}
      <Card style={styles.card}>
        <Card.Title title="System Health" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>SoC Temperature</Text>
            <Text style={styles.metricValue}>{formatTemp(metrics.cpuTemp)}</Text>
          </View>
          <ProgressBar progress={Math.min(metrics.cpuTemp / 9500, 1)} color="#FF9800" style={styles.progress} />

          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>NPU Temperature</Text>
            <Text style={styles.metricValue}>{formatTemp(metrics.npuTemp)}</Text>
          </View>
          <ProgressBar progress={Math.min(metrics.npuTemp / 8500, 1)} color="#FF5722" style={styles.progress} />

          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>DRAM Temperature</Text>
            <Text style={styles.metricValue}>{formatTemp(metrics.dramTemp)}</Text>
          </View>
          <ProgressBar progress={Math.min(metrics.dramTemp / 8500, 1)} color="#FFC107" style={styles.progress} />

          <Divider style={styles.divider} />

          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>Input Voltage</Text>
            <Text style={styles.metricValue}>{formatVoltage(metrics.inputVoltage)}</Text>
          </View>
          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>PoE+ Power</Text>
            <Text style={[styles.metricValue, { color: metrics.poeActive ? '#4CAF50' : '#757575' }]}>
              {metrics.poeActive ? 'Active' : 'Inactive'}
            </Text>
          </View>

          <Divider style={styles.divider} />

          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>Uptime</Text>
            <Text style={styles.metricValue}>{formatUptime(metrics.uptime)}</Text>
          </View>
        </Card.Content>
      </Card>

      {/* Network Status */}
      <Card style={styles.card}>
        <Card.Title title="Network" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>ETH0 (2.5GbE)</Text>
            <Text style={[styles.metricValue, { color: metrics.eth0Link ? '#4CAF50' : '#F44336' }]}>
              {metrics.eth0Link ? 'Link Up' : 'No Link'}
            </Text>
          </View>
          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>ETH1 (2.5GbE)</Text>
            <Text style={[styles.metricValue, { color: metrics.eth1Link ? '#4CAF50' : '#F44336' }]}>
              {metrics.eth1Link ? 'Link Up' : 'No Link'}
            </Text>
          </View>
        </Card.Content>
      </Card>

      {/* CAN Bus Summary */}
      <Card style={styles.card}>
        <Card.Title title="CAN-FD Buses" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>CAN0 (Industrial Bus 1)</Text>
            <Text style={styles.metricValue}>{metrics.can0Count} frames</Text>
          </View>
          <View style={styles.metricRow}>
            <Text style={styles.metricLabel}>CAN1 (Industrial Bus 2)</Text>
            <Text style={styles.metricValue}>{metrics.can1Count} frames</Text>
          </View>
        </Card.Content>
      </Card>

      {/* Status Flags */}
      <Card style={styles.card}>
        <Card.Title title="System Flags" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          {parseStatusFlags(metrics.statusFlags).map((flag, idx) => (
            <Text key={idx} style={styles.statusFlag}>✓ {flag}</Text>
          ))}
          {metrics.statusFlags === 0 && <Text style={styles.statusFlagEmpty}>No status flags active</Text>}
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

// Missing import for Icon
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F23',
  },
  card: {
    margin: 8,
    backgroundColor: '#1A1A2E',
    borderRadius: 8,
  },
  metricRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 4,
  },
  metricLabel: {
    color: '#B0B0B0',
    fontSize: 14,
  },
  metricValue: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: 'bold',
  },
  progress: {
    marginVertical: 4,
    height: 6,
  },
  divider: {
    marginVertical: 8,
    backgroundColor: '#333',
  },
  statusFlag: {
    color: '#4CAF50',
    fontSize: 13,
    paddingVertical: 2,
  },
  statusFlagEmpty: {
    color: '#757575',
    fontSize: 13,
  },
});

export default DashboardScreen;