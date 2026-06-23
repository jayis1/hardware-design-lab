/**
 * LiveTraceScreen.js — Real-time 16-channel waveform display for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Displays real-time electrophysiology waveforms from all 16 channels
 * with spike markers, filter status, and timebase controls.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { useBle } from '../utils/BleContext';
import ChannelWaveform from '../components/ChannelWaveform';

export default function LiveTraceScreen() {
  const { waveform, connected, status, sendCommand } = useBle();
  const [activeChannels, setActiveChannels] = useState(16);
  const [displayMode, setDisplayMode] = useState('stacked'); // 'stacked' or 'overlay'
  const scrollRef = useRef(null);

  // Auto-scroll to keep latest data visible.
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollToEnd({ animated: true });
    }
  }, [waveform]);

  const handleStartAcq = () => {
    sendCommand(0x02, [0x60, 0x06, 0x00, 0x00]);  // 1kSPS, gain 12, no filter override
  };

  const handleStopAcq = () => {
    sendCommand(0x03);
  };

  // Generate empty waveform data if not connected.
  const displayData = waveform.length > 0
    ? waveform
    : Array.from({ length: 16 }, () => new Array(64).fill(0));

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Live Electrophysiology</Text>
        <View style={styles.modeButtons}>
          <TouchableOpacity
            style={[styles.modeBtn, displayMode === 'stacked' && styles.modeBtnActive]}
            onPress={() => setDisplayMode('stacked')}
          >
            <Text style={styles.modeBtnText}>Stacked</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.modeBtn, displayMode === 'overlay' && styles.modeBtnActive]}
            onPress={() => setDisplayMode('overlay')}
          >
            <Text style={styles.modeBtnText}>Overlay</Text>
          </TouchableOpacity>
        </View>
      </View>

      <View style={styles.statusBar}>
        <Text style={styles.statusText}>
          {connected ? `Connected • ${status?.sampleRate || 1000} SPS` : 'Not connected'}
        </Text>
        <Text style={styles.statusText}>
          Channels: {activeChannels}/16
        </Text>
      </View>

      <View style={styles.controls}>
        <TouchableOpacity style={styles.controlBtn} onPress={handleStartAcq}>
          <Text style={styles.controlBtnText}>Start</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.controlBtn, styles.stopBtn]} onPress={handleStopAcq}>
          <Text style={styles.controlBtnText}>Stop</Text>
        </TouchableOpacity>
      </View>

      <ScrollView ref={scrollRef} style={styles.waveformScroll}>
        {displayData.slice(0, activeChannels).map((samples, ch) => (
          <ChannelWaveform
            key={ch}
            channel={ch}
            samples={samples}
            color={getChannelColor(ch)}
            height={displayMode === 'stacked' ? 60 : 200}
            overlay={displayMode === 'overlay'}
          />
        ))}
      </ScrollView>
    </View>
  );
}

function getChannelColor(ch) {
  const colors = [
    '#4CAF50', '#2196F3', '#FF9800', '#E91E63',
    '#9C27B0', '#00BCD4', '#FFEB3B', '#FF5722',
    '#4CAF50', '#2196F3', '#FF9800', '#E91E63',
    '#9C27B0', '#00BCD4', '#FFEB3B', '#FF5722',
  ];
  return colors[ch % colors.length];
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1a0a' },
  header: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', padding: 16, borderBottomWidth: 1, borderBottomColor: '#2a3a2a',
  },
  title: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  modeButtons: { flexDirection: 'row' },
  modeBtn: { paddingHorizontal: 12, paddingVertical: 6, borderRadius: 6, backgroundColor: '#2a3a2a' },
  modeBtnActive: { backgroundColor: '#4CAF50' },
  modeBtnText: { color: '#fff', fontSize: 12 },
  statusBar: {
    flexDirection: 'row', justifyContent: 'space-between',
    padding: 8, backgroundColor: '#1a2a1a',
  },
  statusText: { color: '#aaa', fontSize: 12 },
  controls: { flexDirection: 'row', justifyContent: 'center', padding: 8, gap: 12 },
  controlBtn: {
    backgroundColor: '#4CAF50', paddingHorizontal: 24, paddingVertical: 8, borderRadius: 8,
  },
  stopBtn: { backgroundColor: '#FF5722' },
  controlBtnText: { color: '#fff', fontWeight: 'bold' },
  waveformScroll: { flex: 1, padding: 8 },
});