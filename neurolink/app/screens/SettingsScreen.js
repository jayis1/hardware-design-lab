/**
 * NeuroLink Settings Screen — Device configuration, filter settings, BLE management
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Switch,
  Slider,
  Picker,
} from 'react-native';
import { useBle } from '../components/BleContext';
import { COMMAND_IDS, buildSetFilterCommand } from '../utils/protocol';

export default function SettingsScreen() {
  const {
    device,
    connected,
    scanning,
    error,
    startScan,
    connectToDevice,
    disconnect,
    sendCommand,
  } = useBle();

  const [filterType, setFilterType] = useState(0); // 0=bandpass
  const [filterOrder, setFilterOrder] = useState(4);
  const [filterLowHz, setFilterLowHz] = useState(0.5);
  const [filterHighHz, setFilterHighHz] = useState(45);
  const [notchEnabled, setNotchEnabled] = useState(true);
  const [biasEnabled, setBiasEnabled] = useState(true);
  const [impedanceAutoCheck, setImpedanceAutoCheck] = useState(false);
  const [autoReconnect, setAutoReconnect] = useState(true);

  const handleApplyFilter = useCallback(() => {
    if (!connected) return;
    // Convert Hz to 0.01 Hz units for protocol
    const cutoffLow = Math.round(filterLowHz * 100);
    const cutoffHigh = Math.round(filterHighHz * 100);
    const cmd = buildSetFilterCommand(filterType, filterOrder, cutoffLow, cutoffHigh);
    sendCommand(COMMAND_IDS.SET_FILTER, Array.from(cmd).slice(5));
  }, [connected, filterType, filterOrder, filterLowHz, filterHighHz, sendCommand]);

  const handleFactoryReset = useCallback(() => {
    if (!connected) return;
    sendCommand(COMMAND_IDS.RESET_DEVICE, []);
  }, [connected, sendCommand]);

  const filterTypes = [
    { label: 'Bandpass', value: 0 },
    { label: 'Notch (50/60 Hz)', value: 1 },
    { label: 'Lowpass', value: 2 },
    { label: 'Highpass', value: 3 },
  ];

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>

        {connected ? (
          <View style={styles.connectedInfo}>
            <View style={styles.connectedRow}>
              <View style={styles.statusDotActive} />
              <Text style={styles.connectedText}>
                Connected: {device?.name || 'NeuroLink Device'}
              </Text>
            </View>
            <Text style={styles.deviceId}>ID: {device?.id || 'N/A'}</Text>
            <TouchableOpacity style={styles.disconnectBtn} onPress={disconnect}>
              <Text style={styles.disconnectBtnText}>Disconnect</Text>
            </TouchableOpacity>
          </View>
        ) : (
          <View style={styles.disconnectedInfo}>
            <Text style={styles.disconnectedText}>No device connected</Text>
            <TouchableOpacity
              style={styles.scanBtn}
              onPress={() => device ? connectToDevice(device) : startScan()}
              disabled={scanning}
            >
              <Text style={styles.scanBtnText}>
                {scanning ? 'Scanning...' : 'Scan & Connect'}
              </Text>
            </TouchableOpacity>
            {error && <Text style={styles.errorText}>{error}</Text>}
          </View>
        )}

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-Reconnect</Text>
          <Switch
            value={autoReconnect}
            onValueChange={setAutoReconnect}
            trackColor={{ false: '#2A3A5C', true: '#1A5C3A' }}
            thumbColor={autoReconnect ? '#4CAF50' : '#78909C'}
          />
        </View>
      </View>

      {/* DSP Filter Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Signal Filters</Text>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Filter Type</Text>
          <View style={styles.pickerContainer}>
            {filterTypes.map((ft) => (
              <TouchableOpacity
                key={ft.value}
                style={[styles.filterTypeBtn, filterType === ft.value && styles.filterTypeBtnActive]}
                onPress={() => setFilterType(ft.value)}
              >
                <Text style={[styles.filterTypeText, filterType === ft.value && styles.filterTypeTextActive]}>
                  {ft.label}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Filter Order: {filterOrder}</Text>
          <Slider
            style={styles.slider}
            minimumValue={1}
            maximumValue={4}
            step={1}
            value={filterOrder}
            onValueChange={setFilterOrder}
            minimumTrackTintColor="#4FC3F7"
            maximumTrackTintColor="#2A3A5C"
            thumbTintColor="#4FC3F7"
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Low Cutoff: {filterLowHz.toFixed(1)} Hz</Text>
          <Slider
            style={styles.slider}
            minimumValue={0.1}
            maximumValue={100}
            step={0.1}
            value={filterLowHz}
            onValueChange={setFilterLowHz}
            minimumTrackTintColor="#4FC3F7"
            maximumTrackTintColor="#2A3A5C"
            thumbTintColor="#4FC3F7"
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>High Cutoff: {filterHighHz.toFixed(0)} Hz</Text>
          <Slider
            style={styles.slider}
            minimumValue={10}
            maximumValue={2500}
            step={1}
            value={filterHighHz}
            onValueChange={setFilterHighHz}
            minimumTrackTintColor="#4FC3F7"
            maximumTrackTintColor="#2A3A5C"
            thumbTintColor="#4FC3F7"
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>50/60 Hz Notch</Text>
          <Switch
            value={notchEnabled}
            onValueChange={setNotchEnabled}
            trackColor={{ false: '#2A3A5C', true: '#1A5C3A' }}
            thumbColor={notchEnabled ? '#4CAF50' : '#78909C'}
          />
        </View>

        <TouchableOpacity
          style={styles.applyBtn}
          onPress={handleApplyFilter}
          disabled={!connected}
        >
          <Text style={styles.applyBtnText}>Apply Filter Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Acquisition Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Acquisition</Text>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>BIAS Reference</Text>
          <Switch
            value={biasEnabled}
            onValueChange={setBiasEnabled}
            trackColor={{ false: '#2A3A5C', true: '#1A5C3A' }}
            thumbColor={biasEnabled ? '#4CAF50' : '#78909C'}
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto Impedance Check</Text>
          <Switch
            value={impedanceAutoCheck}
            onValueChange={setImpedanceAutoCheck}
            trackColor={{ false: '#2A3A5C', true: '#1A5C3A' }}
            thumbColor={impedanceAutoCheck ? '#4CAF50' : '#78909C'}
          />
        </View>
      </View>

      {/* Danger Zone */}
      <View style={[styles.section, styles.dangerSection]}>
        <Text style={[styles.sectionTitle, styles.dangerTitle]}>Danger Zone</Text>
        <TouchableOpacity style={styles.resetBtn} onPress={handleFactoryReset} disabled={!connected}>
          <Text style={styles.resetBtnText}>Factory Reset Device</Text>
        </TouchableOpacity>
      </View>

      {/* App Info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>NeuroLink Companion App v1.0.0</Text>
        <Text style={styles.aboutText}>Firmware: v1.0.0</Text>
        <Text style={styles.aboutText}>Protocol: v2 (CRC-16/CCITT)</Text>
        <Text style={styles.aboutText}>Max Channels: 32</Text>
        <Text style={styles.aboutText}>Sample Rates: 250–16000 SPS</Text>
        <Text style={styles.aboutText}>Resolution: 24-bit (ADS1299)</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F1A',
  },
  content: {
    padding: 16,
  },
  section: {
    backgroundColor: '#16213E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
  },
  sectionTitle: {
    color: '#4FC3F7',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 12,
  },
  connectedInfo: {},
  connectedRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 6,
  },
  statusDotActive: {
    width: 10,
    height: 10,
    borderRadius: 5,
    backgroundColor: '#4CAF50',
    marginRight: 8,
  },
  connectedText: {
    color: '#E8E8E8',
    fontSize: 14,
    fontWeight: '600',
  },
  deviceId: {
    color: '#607D8B',
    fontSize: 12,
    fontFamily: 'monospace',
    marginBottom: 12,
    marginLeft: 18,
  },
  disconnectBtn: {
    backgroundColor: '#3E1A1A',
    borderRadius: 8,
    paddingVertical: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#F44336',
  },
  disconnectBtnText: {
    color: '#FF8A80',
    fontSize: 14,
    fontWeight: '600',
  },
  disconnectedInfo: {
    alignItems: 'center',
  },
  disconnectedText: {
    color: '#607D8B',
    fontSize: 14,
    marginBottom: 12,
  },
  scanBtn: {
    backgroundColor: '#1A3A5C',
    borderRadius: 8,
    paddingVertical: 12,
    paddingHorizontal: 24,
    borderWidth: 1,
    borderColor: '#4FC3F7',
  },
  scanBtnText: {
    color: '#4FC3F7',
    fontSize: 14,
    fontWeight: '600',
  },
  errorText: {
    color: '#FF8A80',
    fontSize: 12,
    marginTop: 8,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1E2A45',
  },
  settingLabel: {
    color: '#B0BEC5',
    fontSize: 14,
    flex: 1,
  },
  pickerContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'flex-end',
  },
  filterTypeBtn: {
    backgroundColor: '#1E2A45',
    borderRadius: 6,
    paddingHorizontal: 8,
    paddingVertical: 4,
    marginHorizontal: 3,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  filterTypeBtnActive: {
    backgroundColor: '#1A3A5C',
    borderColor: '#4FC3F7',
  },
  filterTypeText: {
    color: '#78909C',
    fontSize: 11,
  },
  filterTypeTextActive: {
    color: '#4FC3F7',
    fontWeight: '600',
  },
  slider: {
    flex: 1,
    marginLeft: 16,
  },
  applyBtn: {
    backgroundColor: '#1A5C3A',
    borderRadius: 8,
    paddingVertical: 12,
    alignItems: 'center',
    marginTop: 12,
    borderWidth: 1,
    borderColor: '#4CAF50',
  },
  applyBtnText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  dangerSection: {
    borderColor: '#3E1A1A',
    borderWidth: 1,
  },
  dangerTitle: {
    color: '#F44336',
  },
  resetBtn: {
    backgroundColor: '#3E1A1A',
    borderRadius: 8,
    paddingVertical: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#F44336',
  },
  resetBtnText: {
    color: '#FF8A80',
    fontSize: 14,
    fontWeight: '600',
  },
  aboutText: {
    color: '#607D8B',
    fontSize: 13,
    fontFamily: 'monospace',
    lineHeight: 22,
  },
});