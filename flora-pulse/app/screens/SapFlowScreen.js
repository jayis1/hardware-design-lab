/**
 * SapFlowScreen.js — Sap Flow Measurement & Analysis
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays sap-flow measurement results: heat pulse velocity,
 * temperature curves, and time-to-peak analysis. Allows manual
 * triggering of heat pulse measurements.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { useBle } from '../utils/BleContext';
import { CMD, RSP } from '../utils/protocol';
import SapFlowChart from '../components/SapFlowChart';

export default function SapFlowScreen() {
  const { connected, sendCommand, setFrameHandler } = useBle();
  const [measuring, setMeasuring] = useState(false);
  const [velocity, setVelocity] = useState(0);
  const [deltaT, setDeltaT] = useState(0);
  const [tmax, setTmax] = useState(0);
  const [baselineTemp, setBaselineTemp] = useState(0);
  const [history, setHistory] = useState([]);

  useEffect(() => {
    setFrameHandler((opcode, payload) => {
      if (opcode === RSP.OK) {
        // Measurement triggered successfully
      }
      if (opcode === RSP.STATUS && payload.length >= 8) {
        setMeasuring(payload[7] !== 0);
      }
    });
  }, [setFrameHandler]);

  // Poll status for measuring state
  useEffect(() => {
    if (!connected) return;
    const interval = setInterval(() => {
      sendCommand(CMD.GET_STATUS);
    }, 2000);
    return () => clearInterval(interval);
  }, [connected, sendCommand]);

  // Update history when new measurement available
  const addMeasurement = useCallback((vel) => {
    const entry = {
      time: new Date().toLocaleTimeString(),
      velocity: vel,
    };
    setHistory((prev) => [entry, ...prev].slice(0, 20));
  }, []);

  const handleTrigger = () => {
    sendCommand(CMD.TRIGGER_HEATER);
    setMeasuring(true);
    // Simulate completion after 64 seconds (measurement cycle)
    setTimeout(() => {
      setMeasuring(false);
      // In a real app, the device would send the results via BLE
    }, 64000);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>💧 Sap Flow Monitor</Text>
        <Text style={styles.headerSub}>Heat-Pulse Velocity Measurement</Text>
      </View>

      {connected ? (
        <View style={styles.content}>
          {/* Current measurement */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Current Measurement</Text>
            <View style={styles.metricRow}>
              <View style={styles.metric}>
                <Text style={styles.metricLabel}>Velocity</Text>
                <Text style={styles.metricValue}>{velocity.toFixed(1)}</Text>
                <Text style={styles.metricUnit}>cm/h</Text>
              </View>
              <View style={styles.metric}>
                <Text style={styles.metricLabel}>ΔT (Peak)</Text>
                <Text style={styles.metricValue}>{deltaT.toFixed(2)}</Text>
                <Text style={styles.metricUnit}>°C</Text>
              </View>
              <View style={styles.metric}>
                <Text style={styles.metricLabel}>t_max</Text>
                <Text style={styles.metricValue}>{tmax.toFixed(1)}</Text>
                <Text style={styles.metricUnit}>s</Text>
              </View>
            </View>
            <View style={styles.metricRow}>
              <View style={styles.metric}>
                <Text style={styles.metricLabel}>Baseline T</Text>
                <Text style={styles.metricValue}>{baselineTemp.toFixed(1)}</Text>
                <Text style={styles.metricUnit}>°C</Text>
              </View>
              <View style={styles.metric}>
                <Text style={styles.metricLabel}>Status</Text>
                <Text style={[styles.metricValue, { color: measuring ? '#ff9800' : '#4CAF50' }]}>
                  {measuring ? 'MEASURING' : 'IDLE'}
                </Text>
              </View>
            </View>
          </View>

          {/* Temperature curves */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Temperature Response</Text>
            <SapFlowChart
              measuring={measuring}
              deltaT={deltaT}
              tmax={tmax}
              height={180}
            />
            <Text style={styles.chartCaption}>
              Upper sensor (downstream) temperature rise after heat pulse.
              Peak time (t_max) determines flow velocity.
            </Text>
          </View>

          {/* Trigger button */}
          <TouchableOpacity
            style={[styles.triggerButton, measuring ? styles.triggerDisabled : null]}
            onPress={handleTrigger}
            disabled={measuring}
          >
            <Text style={styles.triggerButtonText}>
              {measuring ? '⏳ Measuring... (64s)' : '🔥 Trigger Heat Pulse'}
            </Text>
          </TouchableOpacity>

          {/* Measurement history */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Recent Measurements</Text>
            {history.length > 0 ? (
              history.map((entry, i) => (
                <View key={i} style={styles.historyRow}>
                  <Text style={styles.historyTime}>{entry.time}</Text>
                  <Text style={styles.historyValue}>{entry.velocity.toFixed(1)} cm/h</Text>
                </View>
              ))
            ) : (
              <Text style={styles.emptyText}>No measurements yet</Text>
            )}
          </View>

          {/* Method info */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Method: T-max Heat Pulse</Text>
            <Text style={styles.methodText}>
              The T-max method determines sap flow velocity by measuring the time
              for a heat pulse to reach peak temperature at a sensor positioned
              above the heater. Flow velocity is computed as:
            </Text>
            <Text style={styles.formula}>v = √(α / (π × t_max))</Text>
            <Text style={styles.methodText}>
              where α is the thermal diffusivity of the stem (≈ 2.5×10⁻⁷ m²/s).
              Positive velocity indicates upward sap flow (transpiration).
            </Text>
          </View>
        </View>
      ) : (
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedIcon}>💧</Text>
          <Text style={styles.disconnectedText}>Not Connected</Text>
          <Text style={styles.disconnectedSubtext}>
            Connect to FloraPulse to monitor sap flow
          </Text>
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
  header: {
    padding: 16,
    paddingBottom: 8,
  },
  headerTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#4CAF50',
  },
  headerSub: {
    fontSize: 12,
    color: '#888',
    marginTop: 4,
  },
  content: {
    padding: 16,
    paddingTop: 0,
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
  metricRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 16,
  },
  metric: {
    alignItems: 'center',
    flex: 1,
  },
  metricLabel: {
    fontSize: 12,
    color: '#888',
    marginBottom: 4,
  },
  metricValue: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
  },
  metricUnit: {
    fontSize: 11,
    color: '#aaa',
    marginTop: 2,
  },
  chartCaption: {
    fontSize: 11,
    color: '#666',
    marginTop: 8,
    fontStyle: 'italic',
  },
  triggerButton: {
    backgroundColor: '#ff6b35',
    borderRadius: 12,
    padding: 14,
    alignItems: 'center',
    marginBottom: 12,
  },
  triggerDisabled: {
    backgroundColor: '#555',
  },
  triggerButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 16,
  },
  historyRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2d4a2d',
  },
  historyTime: {
    color: '#aaa',
    fontSize: 14,
  },
  historyValue: {
    color: '#4CAF50',
    fontSize: 14,
    fontWeight: '600',
  },
  emptyText: {
    color: '#666',
    fontSize: 14,
    textAlign: 'center',
    paddingVertical: 12,
  },
  methodText: {
    color: '#ccc',
    fontSize: 13,
    lineHeight: 20,
    marginBottom: 8,
  },
  formula: {
    color: '#4CAF50',
    fontSize: 16,
    textAlign: 'center',
    fontWeight: 'bold',
    marginVertical: 8,
  },
  disconnected: {
    alignItems: 'center',
    paddingTop: 80,
    paddingHorizontal: 40,
  },
  disconnectedIcon: { fontSize: 64, marginBottom: 20 },
  disconnectedText: { fontSize: 24, fontWeight: 'bold', color: '#fff', marginBottom: 8 },
  disconnectedSubtext: { fontSize: 14, color: '#888', textAlign: 'center' },
});