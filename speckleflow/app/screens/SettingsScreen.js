/**
 * SettingsScreen.js — Device configuration
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useCallback } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView, Switch, Slider,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { useDevice, CMD } from '../utils/protocol';

export default function SettingsScreen() {
  const {
    connected, colormap, setColormap, windowSize, setWindowSize,
    fpsMode, setFpsMode, laserPower, setLaserPower, exposure, setExposure,
    sdLogging, setSdLogging, bleStreaming, setBleStreaming, sendCommand,
  } = useDevice();

  const handleColormap = useCallback((cm) => {
    const id = ['jet', 'thermal', 'grayscale', 'viridis', 'inferno'].indexOf(cm);
    setColormap(cm);
    sendCommand(CMD.SET_COLORMAP, id);
  }, [setColormap, sendCommand]);

  const handleWindow = useCallback((w) => {
    setWindowSize(w);
    sendCommand(CMD.SET_WINDOW, w);
  }, [setWindowSize, sendCommand]);

  const handleFps = useCallback((f) => {
    setFpsMode(f);
    sendCommand(CMD.SET_FRAME_RATE, f);
  }, [setFpsMode, sendCommand]);

  const handleLaserPower = useCallback((p) => {
    setLaserPower(p);
    sendCommand(CMD.SET_LASER_PWR, p);
  }, [setLaserPower, sendCommand]);

  const handleExposure = useCallback((e) => {
    setExposure(e);
    sendCommand(CMD.SET_EXPOSURE, e);
  }, [setExposure, sendCommand]);

  const handleSdLogging = useCallback((v) => {
    setSdLogging(v);
    sendCommand(CMD.SET_SD_LOG, v ? 1 : 0);
  }, [setSdLogging, sendCommand]);

  const handleBleStreaming = useCallback((v) => {
    setBleStreaming(v);
    sendCommand(CMD.SET_BLE_STREAM, v ? 1 : 0);
  }, [setBleStreaming, sendCommand]);

  if (!connected) {
    return (
      <View style={styles.emptyContainer}>
        <Icon name="settings" size={64} color="#444" />
        <Text style={styles.emptyText}>No device connected</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Device Settings</Text>

      {/* Imaging settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Imaging</Text>

        {/* Contrast window */}
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Contrast Window</Text>
          <View style={styles.segmented}>
            {[{l: '5×5', v: 0}, {l: '7×7', v: 1}, {l: '9×9', v: 2}].map(opt => (
              <TouchableOpacity
                key={opt.v}
                style={[styles.segBtn, windowSize === opt.v && styles.segBtnActive]}
                onPress={() => handleWindow(opt.v)}
              >
                <Text style={[styles.segBtnText, windowSize === opt.v && styles.segBtnTextActive]}>
                  {opt.l}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        {/* Frame rate */}
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Frame Rate</Text>
          <View style={styles.segmented}>
            {[{l: '30 fps', v: 0}, {l: '60 fps', v: 1}, {l: '120 fps', v: 2}].map(opt => (
              <TouchableOpacity
                key={opt.v}
                style={[styles.segBtn, fpsMode === opt.v && styles.segBtnActive]}
                onPress={() => handleFps(opt.v)}
              >
                <Text style={[styles.segBtnText, fpsMode === opt.v && styles.segBtnTextActive]}>
                  {opt.l}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        {/* Exposure */}
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Exposure: {exposure} ms</Text>
          <Slider
            style={styles.slider}
            minimumValue={1}
            maximumValue={20}
            step={1}
            value={exposure}
            onValueChange={handleExposure}
            minimumTrackTintColor="#00d4ff"
            maximumTrackTintColor="#333"
            thumbTintColor="#00d4ff"
          />
        </View>

        {/* Colormap */}
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Colormap</Text>
          <View style={styles.segmented}>
            {['jet', 'thermal', 'grayscale', 'viridis', 'inferno'].map(cm => (
              <TouchableOpacity
                key={cm}
                style={[styles.segBtn, colormap === cm && styles.segBtnActive]}
                onPress={() => handleColormap(cm)}
              >
                <Text style={[styles.segBtnText, colormap === cm && styles.segBtnTextActive]}>
                  {cm.charAt(0).toUpperCase() + cm.slice(1)}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      {/* Laser settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Laser (785 nm VCSEL)</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Power: {laserPower}%</Text>
          <Slider
            style={styles.slider}
            minimumValue={10}
            maximumValue={100}
            step={5}
            value={laserPower}
            onValueChange={handleLaserPower}
            minimumTrackTintColor="#f44336"
            maximumTrackTintColor="#333"
            thumbTintColor="#f44336"
          />
        </View>
        <Text style={styles.warning}>
          ⚠ Class 3R laser. Wear protective eyewear. Do not stare into beam.
        </Text>
      </View>

      {/* Data settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Data & Streaming</Text>

        <View style={styles.toggleRow}>
          <Text style={styles.settingLabel}>SD Card Logging</Text>
          <Switch
            value={sdLogging}
            onValueChange={handleSdLogging}
            trackColor={{ false: '#333', true: '#4caf50' }}
            thumbColor="#fff"
          />
        </View>

        <View style={styles.toggleRow}>
          <Text style={styles.settingLabel}>BLE Streaming</Text>
          <Switch
            value={bleStreaming}
            onValueChange={handleBleStreaming}
            trackColor={{ false: '#333', true: '#00d4ff' }}
            thumbColor="#fff"
          />
        </View>
      </View>

      {/* Device info */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>About</Text>
        <Text style={styles.infoText}>SpeckleFlow v1.0</Text>
        <Text style={styles.infoText}>Author: jayis1</Text>
        <Text style={styles.infoText}>License: CERN-OHL-S v2 / GPL-2.0 / MIT</Text>
        <Text style={styles.infoText}>Firmware: STM32H733 @ 480 MHz</Text>
        <Text style={styles.infoText}>FPGA: iCE40UP5K (contrast pipeline)</Text>
        <Text style={styles.infoText}>Camera: OV9281 Global Shutter</Text>
        <Text style={styles.infoText}>Laser: 785nm VCSEL 30mW Class 3R</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  emptyContainer: { flex: 1, backgroundColor: '#0f0f1e', alignItems: 'center', justifyContent: 'center' },
  emptyText: { color: '#666', fontSize: 18, marginTop: 16 },
  title: { color: '#00d4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 12 },
  cardTitle: { color: '#e0e0e0', fontSize: 14, fontWeight: '600', marginBottom: 8 },
  settingRow: { paddingVertical: 8 },
  settingLabel: { color: '#aaa', fontSize: 13, marginBottom: 6 },
  segmented: { flexDirection: 'row', gap: 4, flexWrap: 'wrap' },
  segBtn: { paddingHorizontal: 10, paddingVertical: 6, borderRadius: 4, backgroundColor: '#0f0f1e' },
  segBtnActive: { backgroundColor: '#0066cc' },
  segBtnText: { color: '#888', fontSize: 12 },
  segBtnTextActive: { color: '#fff' },
  slider: { width: '100%', height: 40 },
  toggleRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 8 },
  warning: { color: '#ff9800', fontSize: 11, marginTop: 8, fontStyle: 'italic' },
  infoText: { color: '#666', fontSize: 12, paddingVertical: 2 },
});