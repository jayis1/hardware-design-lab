/**
 * MonitorScreen.js — Real-time Plant Stress Monitor
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays real-time plant stress data: action potential signals,
 * environmental conditions, VPD, sap flow, and leaf wetness.
 * Shows a color-coded stress level indicator.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { useBle } from '../utils/BleContext';
import { CMD, RSP, parseSample, parseStatus, computeVPD, classifyVPD } from '../utils/protocol';
import StatusBar from '../components/StatusBar';
import StressGauge from '../components/StressGauge';

export default function MonitorScreen() {
  const { connected, scanning, scanAndConnect, disconnect, sendCommand, setFrameHandler, deviceInfo } = useBle();
  const [sample, setSample] = useState(null);
  const [status, setStatus] = useState(null);
  const [streaming, setStreaming] = useState(false);

  // Setup frame handler
  useEffect(() => {
    setFrameHandler((opcode, payload) => {
      switch (opcode) {
        case RSP.DATA:
          setSample(parseSample(payload));
          break;
        case RSP.STATUS:
          setStatus(parseStatus(payload));
          break;
        case RSP.STREAM:
          setSample({ ...sample, ...require('../utils/protocol').parseStream(payload) });
          break;
        case RSP.OK:
          break;
      }
    });
  }, [setFrameHandler]);

  // Poll for sample data when connected
  useEffect(() => {
    if (!connected) return;
    const interval = setInterval(() => {
      sendCommand(CMD.GET_SAMPLE);
    }, 1000);
    return () => clearInterval(interval);
  }, [connected, sendCommand]);

  // Request status on connect
  useEffect(() => {
    if (connected) {
      sendCommand(CMD.GET_STATUS);
      sendCommand(CMD.GET_VERSION);
    }
  }, [connected]);

  const handleStreamToggle = () => {
    if (streaming) {
      sendCommand(CMD.STOP_STREAM);
      setStreaming(false);
    } else {
      sendCommand(CMD.START_STREAM);
      setStreaming(true);
    }
  };

  const vpd = sample ? computeVPD(sample.tempC || 25, sample.rhPct || 50) : 0;
  const vpdClass = classifyVPD(vpd);
  const stressLevel = sample ? (sample.anomaly ? 'HIGH' : 'NORMAL') : '—';

  return (
    <ScrollView style={styles.container}>
      {/* Connection status */}
      <StatusBar
        connected={connected}
        scanning={scanning}
        deviceName={deviceInfo?.name}
        batteryPct={sample?.batteryPct || status?.batteryPct}
        onConnect={scanAndConnect}
        onDisconnect={disconnect}
      />

      {connected ? (
        <View style={styles.content}>
          {/* Stress level gauge */}
          <StressGauge
            stressLevel={stressLevel}
            apRate={sample?.apRateHz || 0}
            vpd={vpd}
          />

          {/* Action potential data */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>⚡ Plant Action Potentials</Text>
            <View style={styles.dataRow}>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>CH1</Text>
                <Text style={styles.dataValue}>{sample?.apCh1Uv ?? '—'} µV</Text>
              </View>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>CH2</Text>
                <Text style={styles.dataValue}>{sample?.apCh2Uv ?? '—'} µV</Text>
              </View>
            </View>
            <View style={styles.dataRow}>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>RMS</Text>
                <Text style={styles.dataValue}>{sample?.apRmsUv ?? '—'} µV</Text>
              </View>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>Rate</Text>
                <Text style={styles.dataValue}>{sample?.apRateHz?.toFixed(1) ?? '—'} Hz</Text>
              </View>
            </View>
          </View>

          {/* Environmental data */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>🌿 Environment</Text>
            <View style={styles.dataRow}>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>Temp</Text>
                <Text style={styles.dataValue}>{sample?.tempC?.toFixed(1) ?? '—'} °C</Text>
              </View>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>Humidity</Text>
                <Text style={styles.dataValue}>{sample?.rhPct?.toFixed(0) ?? '—'} %</Text>
              </View>
            </View>
            <View style={styles.dataRow}>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>VPD</Text>
                <Text style={styles.dataValue}>{vpd.toFixed(2)} kPa</Text>
                <Text style={styles.dataSub}>{vpdClass}</Text>
              </View>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>Light</Text>
                <Text style={styles.dataValue}>{sample?.lux ?? '—'} lx</Text>
              </View>
            </View>
          </View>

          {/* Sap flow */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>💧 Sap Flow</Text>
            <View style={styles.dataRow}>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>Velocity</Text>
                <Text style={styles.dataValue}>{sample?.sapFlowVh?.toFixed(1) ?? '—'} cm/h</Text>
              </View>
              <View style={styles.dataCell}>
                <Text style={styles.dataLabel}>Leaf Wetness</Text>
                <Text style={styles.dataValue}>{sample?.leafWetPct ?? '—'} %</Text>
              </View>
            </View>
            <TouchableOpacity
              style={styles.triggerButton}
              onPress={() => sendCommand(CMD.TRIGGER_HEATER)}
            >
              <Text style={styles.triggerButtonText}>🔥 Trigger Heat Pulse</Text>
            </TouchableOpacity>
          </View>

          {/* Streaming toggle */}
          <TouchableOpacity
            style={[styles.streamButton, streaming ? styles.streamActive : null]}
            onPress={handleStreamToggle}
          >
            <Text style={styles.streamButtonText}>
              {streaming ? '⏸ Stop Streaming' : '▶ Start Streaming'}
            </Text>
          </TouchableOpacity>
        </View>
      ) : (
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedIcon}>🌱</Text>
          <Text style={styles.disconnectedText}>Not Connected</Text>
          <Text style={styles.disconnectedSubtext}>
            Connect to a FloraPulse device to monitor plant stress in real-time
          </Text>
          <TouchableOpacity style={styles.connectButton} onPress={scanAndConnect}>
            <Text style={styles.connectButtonText}>Scan & Connect</Text>
          </TouchableOpacity>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1b0d',
  },
  content: {
    padding: 16,
  },
  card: {
    backgroundColor: '#1a2e1a',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#2d4a2d',
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#4CAF50',
    marginBottom: 12,
  },
  dataRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 8,
  },
  dataCell: {
    alignItems: 'center',
    flex: 1,
  },
  dataLabel: {
    fontSize: 12,
    color: '#888',
    marginBottom: 4,
  },
  dataValue: {
    fontSize: 18,
    fontWeight: '600',
    color: '#fff',
  },
  dataSub: {
    fontSize: 10,
    color: '#aaa',
    marginTop: 2,
  },
  triggerButton: {
    backgroundColor: '#ff6b35',
    borderRadius: 8,
    padding: 10,
    alignItems: 'center',
    marginTop: 8,
  },
  triggerButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 14,
  },
  streamButton: {
    backgroundColor: '#2196F3',
    borderRadius: 12,
    padding: 14,
    alignItems: 'center',
    marginTop: 8,
  },
  streamActive: {
    backgroundColor: '#f44336',
  },
  streamButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 16,
  },
  disconnected: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingTop: 100,
    paddingHorizontal: 40,
  },
  disconnectedIcon: {
    fontSize: 64,
    marginBottom: 20,
  },
  disconnectedText: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 8,
  },
  disconnectedSubtext: {
    fontSize: 14,
    color: '#888',
    textAlign: 'center',
    marginBottom: 30,
  },
  connectButton: {
    backgroundColor: '#4CAF50',
    borderRadius: 12,
    paddingVertical: 14,
    paddingHorizontal: 40,
  },
  connectButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 16,
  },
});