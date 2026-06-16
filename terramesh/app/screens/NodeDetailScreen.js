/**
 * Terramesh — Node Detail Screen
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Detailed view of a single Terramesh node with time-series charts
 * for all sensor channels, classification history, and configuration.
 */

import React, { useMemo } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  Dimensions,
} from 'react-native';
import { useDevices } from '../components/DeviceContext';
import { protocol } from '../utils/protocol';

const { width } = Dimensions.get('window');
const CHART_HEIGHT = 160;

const NodeDetailScreen = ({ route }) => {
  const { nodeId } = route.params;
  const { nodes } = useDevices();
  const node = nodes[nodeId];

  if (!node) {
    return (
      <View style={styles.container}>
        <Text style={styles.errorText}>Node {nodeId} not found</Text>
      </View>
    );
  }

  const clsColor = protocol.classificationColor(node.classification);
  const clsText = protocol.classificationToString(node.classification);
  const batteryPct = Math.min(100, Math.round((node.battery / 7200) * 100));

  // Generate chart data from history
  const chartData = useMemo(() => {
    const history = node.history || [];
    const labels = history.slice(-20).map((_, i) => `${i * 10}m`);
    const tiltData = history.slice(-20).map(h => Math.abs(h.tiltX || 0));
    const poreData = history.slice(-20).map(h => h.porePressureShallow || 0);
    const moistureData = history.slice(-20).map(h => h.moisture || 0);

    return { labels, tiltData, poreData, moistureData };
  }, [node.history]);

  const SensorChart = ({ title, data, color, unit, format }) => {
    const maxVal = Math.max(...data, 1);
    const chartWidth = width - 64;

    return (
      <View style={styles.chartCard}>
        <Text style={styles.chartTitle}>{title}</Text>
        <View style={styles.chartArea}>
          {data.map((val, i) => {
            const barHeight = (val / maxVal) * (CHART_HEIGHT - 20);
            return (
              <View key={i} style={styles.barContainer}>
                <View style={[styles.bar, {
                  height: Math.max(barHeight, 2),
                  backgroundColor: color,
                  width: Math.max(chartWidth / data.length - 2, 2),
                }]} />
              </View>
            );
          })}
        </View>
        <Text style={styles.chartValue}>
          Current: {format ? format(data[data.length - 1]) : data[data.length - 1]?.toFixed(2)} {unit}
        </Text>
      </View>
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* Node Identity */}
      <View style={styles.identityCard}>
        <View style={styles.identityHeader}>
          <Text style={styles.nodeId}>{node.id}</Text>
          <View style={[styles.statusBadge, { backgroundColor: clsColor }]}>
            <Text style={styles.statusText}>{clsText}</Text>
          </View>
        </View>
        <Text style={styles.nodeName}>{node.name}</Text>
        <View style={styles.identityStats}>
          <View style={styles.identityStat}>
            <Text style={styles.statLabel}>Battery</Text>
            <Text style={styles.statValue}>{batteryPct}%</Text>
          </View>
          <View style={styles.identityStat}>
            <Text style={styles.statLabel}>RSSI</Text>
            <Text style={styles.statValue}>{node.rssi} dBm</Text>
          </View>
          <View style={styles.identityStat}>
            <Text style={styles.statLabel}>SNR</Text>
            <Text style={styles.statValue}>{node.snr} dB</Text>
          </View>
          <View style={styles.identityStat}>
            <Text style={styles.statLabel}>Samples</Text>
            <Text style={styles.statValue}>{(node.history || []).length}</Text>
          </View>
        </View>
      </View>

      {/* Current Readings */}
      <View style={styles.readingsCard}>
        <Text style={styles.sectionTitle}>Current Readings</Text>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Pore Pressure (Shallow)</Text>
          <Text style={styles.readingValue}>
            {node.sensors.porePressureShallow.toFixed(1)} kPa
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Pore Pressure (Deep)</Text>
          <Text style={styles.readingValue}>
            {node.sensors.porePressureDeep.toFixed(1)} kPa
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Soil Moisture</Text>
          <Text style={styles.readingValue}>
            {node.sensors.moisture.toFixed(1)}% VWC
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Tilt X</Text>
          <Text style={styles.readingValue}>
            {node.sensors.tiltX.toFixed(3)}°
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Tilt Y</Text>
          <Text style={styles.readingValue}>
            {node.sensors.tiltY.toFixed(3)}°
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Acceleration</Text>
          <Text style={styles.readingValue}>
            X:{node.sensors.accelX} Y:{node.sensors.accelY} Z:{node.sensors.accelZ} mg
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Temperature</Text>
          <Text style={styles.readingValue}>
            {node.sensors.temperature.toFixed(1)}°C
          </Text>
        </View>
        <View style={styles.readingRow}>
          <Text style={styles.readingLabel}>Barometric Pressure</Text>
          <Text style={styles.readingValue}>
            {node.sensors.pressure.toFixed(0)} Pa
          </Text>
        </View>
      </View>

      {/* Time-series Charts */}
      <Text style={styles.sectionTitle}>History (Last 20 Samples)</Text>

      <SensorChart
        title="Tilt Magnitude"
        data={chartData.tiltData}
        color="#ffa500"
        unit="°"
        format={(v) => v.toFixed(3)}
      />
      <SensorChart
        title="Pore Pressure (Shallow)"
        data={chartData.poreData}
        color="#4fc3f7"
        unit="kPa"
        format={(v) => v.toFixed(1)}
      />
      <SensorChart
        title="Soil Moisture"
        data={chartData.moistureData}
        color="#00d4aa"
        unit="% VWC"
        format={(v) => v.toFixed(1)}
      />

      {/* Classification History */}
      <View style={styles.chartCard}>
        <Text style={styles.chartTitle}>Classification History</Text>
        <View style={styles.classHistory}>
          {(node.history || []).slice(-20).map((h, i) => {
            const c = protocol.classificationColor(h.classification);
            return (
              <View key={i} style={[styles.classDot, { backgroundColor: c }]} />
            );
          })}
        </View>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
  },
  errorText: {
    fontSize: 18,
    color: '#ff4444',
    textAlign: 'center',
    padding: 40,
  },
  identityCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  identityHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  nodeId: {
    fontSize: 22,
    fontWeight: 'bold',
    color: '#e0e0e0',
  },
  statusBadge: {
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderRadius: 12,
  },
  statusText: {
    fontSize: 12,
    fontWeight: '600',
    color: '#ffffff',
  },
  nodeName: {
    fontSize: 14,
    color: '#6c6c80',
    marginTop: 4,
  },
  identityStats: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 16,
    paddingTop: 12,
    borderTopWidth: 1,
    borderTopColor: '#2a2a3e',
  },
  identityStat: {
    alignItems: 'center',
  },
  statLabel: {
    fontSize: 11,
    color: '#6c6c80',
  },
  statValue: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
  },
  readingsCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
    paddingHorizontal: 10,
    paddingTop: 10,
    paddingBottom: 4,
  },
  readingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  readingLabel: {
    fontSize: 13,
    color: '#6c6c80',
  },
  readingValue: {
    fontSize: 13,
    color: '#e0e0e0',
    fontWeight: '500',
  },
  chartCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  chartTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: '#e0e0e0',
    marginBottom: 8,
  },
  chartArea: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    height: CHART_HEIGHT,
    paddingTop: 10,
  },
  barContainer: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'flex-end',
  },
  bar: {
    borderRadius: 2,
    minWidth: 2,
  },
  chartValue: {
    fontSize: 12,
    color: '#6c6c80',
    marginTop: 8,
    textAlign: 'center',
  },
  classHistory: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 4,
  },
  classDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
  },
});

export default NodeDetailScreen;
