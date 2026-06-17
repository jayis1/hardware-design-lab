/**
 * MonitorScreen.js — Real-Time Magnetic Field Monitor
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Displays the current 3-axis magnetic field vector with numeric
 * readouts, a 3D vector visualization, and a rolling time-series
 * strip chart.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { useBle } from '../utils/BleContext';
import FieldVector3D from '../components/FieldVector3D';
import StripChart from '../components/StripChart';
import StatusBar from '../components/StatusBar';

const MODE_NAMES = {
  0: 'BOOT', 1: 'IDLE', 2: 'SURVEY', 3: 'CALIB', 4: 'MONITOR', 5: 'CONFIG',
};

export default function MonitorScreen({ navigation }) {
  const ble = useBle();
  const [history, setHistory] = useState([]);
  const [selectedAxis, setSelectedAxis] = useState('total');
  const maxHistory = 300;

  // Accumulate field data history for strip chart
  useEffect(() => {
    if (ble.fieldData) {
      setHistory((prev) => {
        const updated = [...prev, ble.fieldData];
        if (updated.length > maxHistory) updated.shift();
        return updated;
      });
    }
  }, [ble.fieldData]);

  // Start streaming when screen loads
  useEffect(() => {
    if (ble.connectionState === 'connected') {
      ble.startStream();
    }
    return () => {
      ble.stopStream();
    };
  }, [ble.connectionState]);

  const field = ble.fieldData;
  const hasData = field !== null;

  // Get the selected axis value for the strip chart
  const getAxisValue = (d) => {
    if (!d) return 0;
    switch (selectedAxis) {
      case 'bx': return d.bx;
      case 'by': return d.by;
      case 'bz': return d.bz;
      case 'total': return d.bTotal;
      default: return d.bTotal;
    }
  };

  const chartData = history.map((d, i) => ({
    x: i,
    y: getAxisValue(d),
  }));

  return (
    <ScrollView style={styles.container}>
      <StatusBar
        connectionState={ble.connectionState}
        status={ble.status}
        deviceInfo={ble.deviceInfo}
      />

      {/* Connection prompt */}
      {ble.connectionState !== 'connected' && (
        <View style={styles.connectPrompt}>
          <Text style={styles.connectText}>Not connected to FerroProbe</Text>
          <TouchableOpacity
            style={styles.connectButton}
            onPress={() => ble.scan()}
          >
            <Text style={styles.connectButtonText}>Scan for Devices</Text>
          </TouchableOpacity>
          {ble.scanResults.map((d) => (
            <TouchableOpacity
              key={d.id}
              style={styles.deviceItem}
              onPress={() => ble.connect(d.id)}
            >
              <Text style={styles.deviceName}>{d.name || 'Unknown'}</Text>
              <Text style={styles.deviceId}>{d.id}</Text>
            </TouchableOpacity>
          ))}
          {ble.error && (
            <Text style={styles.errorText}>Error: {ble.error}</Text>
          )}
        </View>
      )}

      {hasData && ble.connectionState === 'connected' && (
        <>
          {/* Main field readout */}
          <View style={styles.readoutContainer}>
            <Text style={styles.readoutLabel}>Total Field |B|</Text>
            <Text style={styles.readoutValue}>
              {(field.bTotal / 1000).toFixed(3)}
            </Text>
            <Text style={styles.readoutUnit}>µT</Text>
          </View>

          {/* 3-Axis numeric display */}
          <View style={styles.axisGrid}>
            <View style={styles.axisBox}>
              <Text style={styles.axisLabel}>Bx</Text>
              <Text style={styles.axisValue}>{field.bx.toFixed(1)}</Text>
              <Text style={styles.axisUnit}>nT</Text>
            </View>
            <View style={styles.axisBox}>
              <Text style={styles.axisLabel}>By</Text>
              <Text style={styles.axisValue}>{field.by.toFixed(1)}</Text>
              <Text style={styles.axisUnit}>nT</Text>
            </View>
            <View style={styles.axisBox}>
              <Text style={styles.axisLabel}>Bz</Text>
              <Text style={styles.axisValue}>{field.bz.toFixed(1)}</Text>
              <Text style={styles.axisUnit}>nT</Text>
            </View>
          </View>

          {/* Anomaly indicator */}
          {field.isAnomaly && (
            <View style={styles.anomalyBadge}>
              <Text style={styles.anomalyText}>⚠ ANOMALY DETECTED</Text>
            </View>
          )}

          {/* 3D vector visualization */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>3D Field Vector</Text>
            <FieldVector3D bx={field.bx} by={field.by} bz={field.bz} />
          </View>

          {/* Strip chart */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>Time Series</Text>
            <View style={styles.axisSelector}>
              {['total', 'bx', 'by', 'bz'].map((axis) => (
                <TouchableOpacity
                  key={axis}
                  style={[
                    styles.axisButton,
                    selectedAxis === axis && styles.axisButtonActive,
                  ]}
                  onPress={() => setSelectedAxis(axis)}
                >
                  <Text
                    style={[
                      styles.axisButtonText,
                      selectedAxis === axis && styles.axisButtonTextActive,
                    ]}
                  >
                    {axis === 'total' ? '|B|' : axis.toUpperCase()}
                  </Text>
                </TouchableOpacity>
              ))}
            </View>
            <StripChart data={chartData} color="#2196F3" />
          </View>

          {/* GPS and system info */}
          <View style={styles.infoGrid}>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>GPS Fix</Text>
              <Text style={styles.infoValue}>
                {field.hasGpsFix ? `Yes (${field.satellites} sats)` : 'No'}
              </Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Temperature</Text>
              <Text style={styles.infoValue}>{field.temperature}°C</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Device Mode</Text>
              <Text style={styles.infoValue}>
                {ble.status ? MODE_NAMES[ble.status.mode] || 'Unknown' : 'Unknown'}
              </Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Battery</Text>
              <Text style={styles.infoValue}>
                {ble.status ? `${ble.status.battery}%` : 'N/A'}
              </Text>
            </View>
          </View>

          {/* Control buttons */}
          <View style={styles.controlRow}>
            <TouchableOpacity
              style={[styles.controlButton, styles.surveyButton]}
              onPress={() => ble.startSurvey()}
            >
              <Text style={styles.controlButtonText}>▶ Start Survey</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.controlButton, styles.stopButton]}
              onPress={() => ble.stopSurvey()}
            >
              <Text style={styles.controlButtonText}>■ Stop</Text>
            </TouchableOpacity>
          </View>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  connectPrompt: {
    padding: 20,
    alignItems: 'center',
  },
  connectText: {
    color: '#888',
    fontSize: 16,
    marginBottom: 15,
  },
  connectButton: {
    backgroundColor: '#2196F3',
    paddingHorizontal: 30,
    paddingVertical: 12,
    borderRadius: 8,
    marginBottom: 15,
  },
  connectButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  deviceItem: {
    padding: 15,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    marginBottom: 8,
    width: '100%',
  },
  deviceName: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  deviceId: {
    color: '#666',
    fontSize: 12,
  },
  errorText: {
    color: '#f44336',
    fontSize: 14,
    marginTop: 10,
  },
  readoutContainer: {
    alignItems: 'center',
    paddingVertical: 20,
    backgroundColor: '#1a1a2e',
    marginHorizontal: 15,
    marginTop: 15,
    borderRadius: 12,
  },
  readoutLabel: {
    color: '#888',
    fontSize: 14,
    marginBottom: 5,
  },
  readoutValue: {
    color: '#2196F3',
    fontSize: 48,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  readoutUnit: {
    color: '#888',
    fontSize: 16,
  },
  axisGrid: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginHorizontal: 15,
    marginTop: 10,
  },
  axisBox: {
    flex: 1,
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 10,
    paddingVertical: 12,
    marginHorizontal: 4,
  },
  axisLabel: {
    color: '#888',
    fontSize: 12,
  },
  axisValue: {
    color: '#fff',
    fontSize: 20,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  axisUnit: {
    color: '#666',
    fontSize: 10,
  },
  anomalyBadge: {
    backgroundColor: '#f44336',
    padding: 8,
    borderRadius: 6,
    marginHorizontal: 15,
    marginTop: 10,
    alignItems: 'center',
  },
  anomalyText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  section: {
    backgroundColor: '#1a1a2e',
    marginHorizontal: 15,
    marginTop: 15,
    borderRadius: 12,
    padding: 15,
  },
  sectionTitle: {
    color: '#888',
    fontSize: 14,
    marginBottom: 10,
  },
  axisSelector: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 10,
  },
  axisButton: {
    paddingVertical: 6,
    paddingHorizontal: 16,
    borderRadius: 6,
    backgroundColor: '#2a2a3e',
  },
  axisButtonActive: {
    backgroundColor: '#2196F3',
  },
  axisButtonText: {
    color: '#888',
    fontSize: 12,
  },
  axisButtonTextActive: {
    color: '#fff',
    fontWeight: 'bold',
  },
  infoGrid: {
    backgroundColor: '#1a1a2e',
    marginHorizontal: 15,
    marginTop: 15,
    borderRadius: 12,
    padding: 15,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  infoLabel: {
    color: '#888',
    fontSize: 14,
  },
  infoValue: {
    color: '#fff',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  controlRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginHorizontal: 15,
    marginVertical: 20,
  },
  controlButton: {
    flex: 1,
    paddingVertical: 14,
    borderRadius: 10,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  surveyButton: {
    backgroundColor: '#4CAF50',
  },
  stopButton: {
    backgroundColor: '#f44336',
  },
  controlButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
});