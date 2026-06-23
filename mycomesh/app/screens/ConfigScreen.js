/**
 * ConfigScreen.js — Node configuration for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Allows the user to configure sample rate, gain, filters, duty cycle,
 * LoRaWAN region, and electrode mapping.  Also provides firmware update
 * (DFU) and log download functionality.
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Switch, TextInput, Alert, Slider,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import { CMD, buildConfigPayload, buildStartAcqPayload } from '../utils/protocol';

export default function ConfigScreen() {
  const { connected, status, sendCommand } = useBle();

  const [sampleRate, setSampleRate] = useState(1000);
  const [gain, setGain] = useState(12);
  const [notchFreq, setNotchFreq] = useState(50);
  const [dutyCycle, setDutyCycle] = useState(5);
  const [region, setRegion] = useState(0);  // 0 = EU868, 1 = US915
  const [spikeThreshold, setSpikeThreshold] = useState(4.0);
  const [autoClassify, setAutoClassify] = useState(true);

  const handleApplyConfig = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to a MycoMesh node first.');
      return;
    }
    // Send config command: [duty_cycle, mains_hz, region].
    const payload = buildConfigPayload(dutyCycle, notchFreq, region);
    sendCommand(CMD.SET_CONFIG, payload);
    Alert.alert('Configuration Applied', 'Settings have been sent to the node.');
  };

  const handleStartAcq = () => {
    if (!connected) return;
    const payload = buildStartAcqPayload(sampleRate, gain, notchFreq);
    sendCommand(CMD.START_ACQ, payload);
  };

  const handleStopAcq = () => {
    sendCommand(CMD.STOP_ACQ);
  };

  const handleCalibrate = () => {
    if (!connected) return;
    Alert.alert('Calibration', 'Start electrode offset calibration?', [
      { text: 'Cancel' },
      { text: 'Calibrate', onPress: () => sendCommand(CMD.CALIBRATE, [0]) },
    ]);
  };

  const handleDfu = () => {
    if (!connected) return;
    Alert.alert(
      'Firmware Update',
      'Enter DFU mode? The device will disconnect and appear as a USB DFU device.',
      [
        { text: 'Cancel' },
        { text: 'Enter DFU', onPress: () => sendCommand(CMD.DFU_ENTER) },
      ]
    );
  };

  const handleDownloadLogs = () => {
    if (!connected) return;
    Alert.alert('Download Logs', 'Fetching log file list from device...');
    sendCommand(CMD.GET_LOG_LIST);
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Node Configuration</Text>
        <Text style={styles.connectionStatus}>
          {connected ? '● Connected' : '○ Disconnected'}
        </Text>
      </View>

      <ScrollView style={styles.content}>
        {/* Acquisition Settings */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Acquisition</Text>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Sample Rate</Text>
            <View style={styles.pickerRow}>
              {[125, 250, 500, 1000, 2000, 4000].map((sr) => (
                <TouchableOpacity
                  key={sr}
                  style={[styles.pickerBtn, sampleRate === sr && styles.pickerBtnActive]}
                  onPress={() => setSampleRate(sr)}
                >
                  <Text style={styles.pickerBtnText}>{sr >= 1000 ? `${sr/1000}k` : sr}</Text>
                </TouchableOpacity>
              ))}
            </View>
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>PGA Gain</Text>
            <View style={styles.pickerRow}>
              {[1, 2, 4, 6, 8, 12].map((g) => (
                <TouchableOpacity
                  key={g}
                  style={[styles.pickerBtn, gain === g && styles.pickerBtnActive]}
                  onPress={() => setGain(g)}
                >
                  <Text style={styles.pickerBtnText}>×{g}</Text>
                </TouchableOpacity>
              ))}
            </View>
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Mains Notch</Text>
            <View style={styles.pickerRow}>
              <TouchableOpacity
                style={[styles.pickerBtn, notchFreq === 50 && styles.pickerBtnActive]}
                onPress={() => setNotchFreq(50)}
              >
                <Text style={styles.pickerBtnText}>50 Hz</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.pickerBtn, notchFreq === 60 && styles.pickerBtnActive]}
                onPress={() => setNotchFreq(60)}
              >
                <Text style={styles.pickerBtnText}>60 Hz</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.pickerBtn, notchFreq === 0 && styles.pickerBtnActive]}
                onPress={() => setNotchFreq(0)}
              >
                <Text style={styles.pickerBtnText}>Off</Text>
              </TouchableOpacity>
            </View>
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Spike Threshold (k×MAD)</Text>
            <Slider
              style={styles.slider}
              minimumValue={2}
              maximumValue={8}
              step={0.5}
              value={spikeThreshold}
              onValueChange={setSpikeThreshold}
              minimumTrackTintColor="#4CAF50"
              maximumTrackTintColor="#333"
            />
            <Text style={styles.sliderValue}>{spikeThreshold.toFixed(1)}</Text>
          </View>
        </View>

        {/* Power Management */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Power Management</Text>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Duty Cycle: {dutyCycle}%</Text>
            <Slider
              style={styles.slider}
              minimumValue={1}
              maximumValue={100}
              step={1}
              value={dutyCycle}
              onValueChange={setDutyCycle}
              minimumTrackTintColor="#4CAF50"
              maximumTrackTintColor="#333"
            />
          </View>

          <Text style={styles.estimateText}>
            Estimated battery life: {estimateBatteryLife(dutyCycle)} months
          </Text>
        </View>

        {/* LoRaWAN Settings */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>LoRaWAN</Text>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Region</Text>
            <View style={styles.pickerRow}>
              <TouchableOpacity
                style={[styles.pickerBtn, region === 0 && styles.pickerBtnActive]}
                onPress={() => setRegion(0)}
              >
                <Text style={styles.pickerBtnText}>EU868</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.pickerBtn, region === 1 && styles.pickerBtnActive]}
                onPress={() => setRegion(1)}
              >
                <Text style={styles.pickerBtnText}>US915</Text>
              </TouchableOpacity>
            </View>
          </View>

          <View style={styles.configRow}>
            <Text style={styles.configLabel}>Auto-Classify</Text>
            <Switch
              value={autoClassify}
              onValueChange={setAutoClassify}
              trackColor={{ false: '#333', true: '#4CAF50' }}
            />
          </View>
        </View>

        {/* Actions */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Actions</Text>

          <TouchableOpacity style={styles.actionBtn} onPress={handleStartAcq}>
            <Text style={styles.actionBtnText}>Start Acquisition</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.actionBtn, styles.stopBtn]} onPress={handleStopAcq}>
            <Text style={styles.actionBtnText}>Stop Acquisition</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionBtn} onPress={handleCalibrate}>
            <Text style={styles.actionBtnText}>Calibrate Electrodes</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.actionBtn} onPress={handleDownloadLogs}>
            <Text style={styles.actionBtnText}>Download Logs</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.actionBtn, styles.dfuBtn]} onPress={handleDfu}>
            <Text style={styles.actionBtnText}>Enter DFU Mode</Text>
          </TouchableOpacity>
        </View>

        {/* Apply button */}
        <TouchableOpacity style={styles.applyBtn} onPress={handleApplyConfig}>
          <Text style={styles.applyBtnText}>Apply Configuration</Text>
        </TouchableOpacity>

        <View style={styles.footer}>
          <Text style={styles.footerText}>MycoMesh v1.0 — Author: jayis1 — GPL-2.0</Text>
        </View>
      </ScrollView>
    </View>
  );
}

function estimateBatteryLife(dutyCycle) {
  // Rough estimate: 34 Ah battery, 10.5 mA active, 0.005 mA sleep.
  const avgCurrent = (10.5 * dutyCycle + 0.005 * (100 - dutyCycle)) / 100;
  const hours = 34000 / avgCurrent;
  return (hours / 24 / 30).toFixed(1);
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1a0a' },
  header: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    padding: 16, borderBottomWidth: 1, borderBottomColor: '#2a3a2a',
  },
  title: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  connectionStatus: { color: '#4CAF50', fontSize: 12 },
  content: { flex: 1, padding: 8 },
  section: { backgroundColor: '#1a2a1a', borderRadius: 12, padding: 16, marginBottom: 12 },
  sectionTitle: { color: '#fff', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  configRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', marginBottom: 12,
  },
  configLabel: { color: '#ccc', fontSize: 14, flex: 1 },
  pickerRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 4 },
  pickerBtn: {
    backgroundColor: '#2a3a2a', paddingHorizontal: 10, paddingVertical: 6,
    borderRadius: 6,
  },
  pickerBtnActive: { backgroundColor: '#4CAF50' },
  pickerBtnText: { color: '#fff', fontSize: 11 },
  slider: { flex: 1, height: 30, marginHorizontal: 8 },
  sliderValue: { color: '#4CAF50', fontSize: 14, fontWeight: 'bold', width: 40 },
  estimateText: { color: '#888', fontSize: 12, marginTop: 4 },
  actionBtn: {
    backgroundColor: '#2a3a2a', padding: 12, borderRadius: 8, marginBottom: 8,
    alignItems: 'center',
  },
  stopBtn: { backgroundColor: '#FF5722' },
  dfuBtn: { backgroundColor: '#FF9800' },
  actionBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
  applyBtn: {
    backgroundColor: '#4CAF50', padding: 16, borderRadius: 12,
    alignItems: 'center', marginBottom: 12,
  },
  applyBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  footer: { alignItems: 'center', padding: 12 },
  footerText: { color: '#555', fontSize: 11 },
});