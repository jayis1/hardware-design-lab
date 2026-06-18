/**
 * WaveformScreen.js — Live AP Waveform Display
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays a scrolling waveform of plant action potential signals
 * from both ADS1292 channels, with adjustable timebase and threshold.
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Dimensions } from 'react-native';
import { useBle } from '../utils/BleContext';
import { CMD, RSP } from '../utils/protocol';
import { LineChart } from 'react-native-chart-kit';
import WaveformChart from '../components/WaveformChart';

const SCREEN_WIDTH = Dimensions.get('window').width;
const MAX_POINTS = 250; // 1 second at 250 SPS

export default function WaveformScreen() {
  const { connected, sendCommand, setFrameHandler } = useBle();
  const [ch1Data, setCh1Data] = useState([]);
  const [ch2Data, setCh2Data] = useState([]);
  const [streaming, setStreaming] = useState(false);
  const [threshold, setThreshold] = useState(50);
  const [timebase, setTimebase] = useState(1); // seconds visible
  const [detectedEvents, setDetectedEvents] = useState(0);

  const ch1BufferRef = useRef([]);
  const ch2BufferRef = useRef([]);

  // Setup frame handler for waveform data
  useEffect(() => {
    setFrameHandler((opcode, payload) => {
      if (opcode === RSP.WAVEFORM && payload.length >= 8) {
        const ch1 = toInt32(payload, 0);
        const ch2 = toInt32(payload, 4);

        // Convert to µV (gain 12, Vref 2.42V, 24-bit)
        const ch1Uv = ch1 * 24.1 / 1000;
        const ch2Uv = ch2 * 24.1 / 1000;

        ch1BufferRef.current.push(ch1Uv);
        ch2BufferRef.current.push(ch2Uv);

        if (ch1BufferRef.current.length > MAX_POINTS) {
          ch1BufferRef.current.shift();
        }
        if (ch2BufferRef.current.length > MAX_POINTS) {
          ch2BufferRef.current.shift();
        }

        // Check threshold crossings
        if (Math.abs(ch1Uv) > threshold) {
          setDetectedEvents((p) => p + 1);
        }
      }
    });
  }, [setFrameHandler, threshold]);

  // Update chart data periodically
  useEffect(() => {
    if (!streaming) return;
    const interval = setInterval(() => {
      setCh1Data([...ch1BufferRef.current]);
      setCh2Data([...ch2BufferRef.current]);
    }, 100); // 10 Hz chart update
    return () => clearInterval(interval);
  }, [streaming]);

  // Request waveform data when streaming
  useEffect(() => {
    if (!connected || !streaming) return;
    const interval = setInterval(() => {
      sendCommand(CMD.GET_WAVEFORM);
    }, 20); // 50 Hz request rate
    return () => clearInterval(interval);
  }, [connected, streaming, sendCommand]);

  const handleStartStop = () => {
    if (streaming) {
      sendCommand(CMD.STOP_STREAM);
      setStreaming(false);
    } else {
      setDetectedEvents(0);
      ch1BufferRef.current = [];
      ch2BufferRef.current = [];
      sendCommand(CMD.START_STREAM);
      setStreaming(true);
    }
  };

  const handleThresholdChange = (delta) => {
    const newThreshold = Math.max(10, Math.min(500, threshold + delta));
    setThreshold(newThreshold);
    // Send to device
    const payload = [
      newThreshold & 0xFF,
      (newThreshold >> 8) & 0xFF,
      (newThreshold >> 16) & 0xFF,
      (newThreshold >> 24) & 0xFF,
    ];
    sendCommand(CMD.SET_THRESHOLD, payload);
  };

  return (
    <View style={styles.container}>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        {/* Header */}
        <View style={styles.header}>
          <Text style={styles.headerTitle}>⚡ AP Waveform</Text>
          <Text style={styles.headerSub}>
            Plant Action Potential Signal (ADS1292, 250 SPS)
          </Text>
        </View>

        {connected ? (
          <>
            {/* CH1 Waveform */}
            <View style={styles.chartCard}>
              <Text style={styles.chartLabel}>CH1 — Stem Electrode (µV)</Text>
              <WaveformChart
                data={ch1Data}
                color="#4CAF50"
                threshold={threshold}
                height={150}
                width={SCREEN_WIDTH - 64}
              />
            </View>

            {/* CH2 Waveform */}
            <View style={styles.chartCard}>
              <Text style={styles.chartLabel}>CH2 — Leaf Electrode (µV)</Text>
              <WaveformChart
                data={ch2Data}
                color="#2196F3"
                threshold={threshold}
                height={150}
                width={SCREEN_WIDTH - 64}
              />
            </View>

            {/* Controls */}
            <View style={styles.controlsCard}>
              <View style={styles.thresholdControl}>
                <Text style={styles.controlLabel}>Detection Threshold: {threshold} µV</Text>
                <View style={styles.thresholdButtons}>
                  <TouchableOpacity
                    style={styles.thButton}
                    onPress={() => handleThresholdChange(-10)}
                  >
                    <Text style={styles.thButtonText}>−10</Text>
                  </TouchableOpacity>
                  <TouchableOpacity
                    style={styles.thButton}
                    onPress={() => handleThresholdChange(10)}
                  >
                    <Text style={styles.thButtonText}>+10</Text>
                  </TouchableOpacity>
                </View>
              </View>

              <View style={styles.statsRow}>
                <View style={styles.statCell}>
                  <Text style={styles.statLabel}>Events</Text>
                  <Text style={styles.statValue}>{detectedEvents}</Text>
                </View>
                <View style={styles.statCell}>
                  <Text style={styles.statLabel}>Points</Text>
                  <Text style={styles.statValue}>{ch1Data.length}</Text>
                </View>
                <View style={styles.statCell}>
                  <Text style={styles.statLabel}>Rate</Text>
                  <Text style={styles.statValue}>{streaming ? '250' : '—'} Hz</Text>
                </View>
              </View>
            </View>

            {/* Start/Stop button */}
            <TouchableOpacity
              style={[styles.streamButton, streaming ? styles.streamActive : null]}
              onPress={handleStartStop}
            >
              <Text style={styles.streamButtonText}>
                {streaming ? '⏸ Stop' : '▶ Start Waveform Capture'}
              </Text>
            </TouchableOpacity>
          </>
        ) : (
          <View style={styles.disconnected}>
            <Text style={styles.disconnectedIcon}>📊</Text>
            <Text style={styles.disconnectedText}>Not Connected</Text>
            <Text style={styles.disconnectedSubtext}>
              Connect to FloraPulse to view live plant electrophysiology waveforms
            </Text>
          </View>
        )}
      </ScrollView>
    </View>
  );
}

function toInt32(buf, offset) {
  const val = buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24);
  return val | 0;
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1b0d',
  },
  scrollContent: {
    padding: 16,
  },
  header: {
    marginBottom: 16,
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
  chartCard: {
    backgroundColor: '#1a2e1a',
    borderRadius: 12,
    padding: 12,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#2d4a2d',
  },
  chartLabel: {
    fontSize: 13,
    color: '#aaa',
    marginBottom: 8,
  },
  controlsCard: {
    backgroundColor: '#1a2e1a',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#2d4a2d',
  },
  thresholdControl: {
    marginBottom: 16,
  },
  controlLabel: {
    fontSize: 14,
    color: '#fff',
    marginBottom: 8,
  },
  thresholdButtons: {
    flexDirection: 'row',
    gap: 12,
  },
  thButton: {
    backgroundColor: '#2d4a2d',
    borderRadius: 8,
    paddingVertical: 8,
    paddingHorizontal: 20,
  },
  thButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  statsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  statCell: {
    alignItems: 'center',
  },
  statLabel: {
    fontSize: 12,
    color: '#888',
    marginBottom: 4,
  },
  statValue: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#4CAF50',
  },
  streamButton: {
    backgroundColor: '#2196F3',
    borderRadius: 12,
    padding: 14,
    alignItems: 'center',
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
    alignItems: 'center',
    paddingTop: 80,
    paddingHorizontal: 40,
  },
  disconnectedIcon: { fontSize: 64, marginBottom: 20 },
  disconnectedText: { fontSize: 24, fontWeight: 'bold', color: '#fff', marginBottom: 8 },
  disconnectedSubtext: { fontSize: 14, color: '#888', textAlign: 'center' },
});