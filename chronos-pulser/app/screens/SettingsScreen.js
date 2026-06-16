// screens/SettingsScreen.js — Settings and calibration screen for Chronos Pulser
// Pulse defaults, calibration management, display preferences
// 150+ lines of real settings UI

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Switch, Alert, ActivityIndicator, TextInput,
} from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { dacToMillivolts, millivoltsToDac, vgaCodeToDb, dbToVgaCode } from '../utils/protocol';

export default function SettingsScreen() {
  const device = useDevice();

  // Pulse defaults
  const [defaultFreq, setDefaultFreq] = useState('1000');
  const [defaultAmplitudeMv, setDefaultAmplitudeMv] = useState('250');
  const [defaultVgaDb, setDefaultVgaDb] = useState('10.0');

  // Calibration state
  const [calRunning, setCalRunning] = useState(false);
  const [calResult, setCalResult] = useState(null);

  // Display preferences
  const [velocityFactor, setVelocityFactor] = useState('0.66');
  const [impedanceRef, setImpedanceRef] = useState('50');
  const [darkMode, setDarkMode] = useState(true);
  const [autoConnect, setAutoConnect] = useState(false);

  // Run TDC calibration
  const handleRunCalibration = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Connect device to run calibration.');
      return;
    }
    setCalRunning(true);
    setCalResult(null);
    try {
      const result = await device.runTdcCalibration();
      setCalResult(result);
      Alert.alert('Calibration Complete', 'TDC calibration completed successfully.');
    } catch (err) {
      Alert.alert('Calibration Failed', err.message || 'Unknown error during calibration.');
    } finally {
      setCalRunning(false);
    }
  }, [device]);

  // Apply default pulse settings
  const handleApplyDefaults = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Connect device to apply settings.');
      return;
    }
    try {
      const freq = parseInt(defaultFreq, 10);
      const ampDac = millivoltsToDac(parseFloat(defaultAmplitudeMv));
      const vgaCode = dbToVgaCode(parseFloat(defaultVgaDb));

      if (isNaN(freq) || freq < 1 || freq > 1000000) {
        Alert.alert('Invalid', 'Frequency must be 1–1,000,000 Hz');
        return;
      }

      await device.updatePulseConfig({
        frequency: freq,
        amplitude: ampDac,
        continuous: false,
        burstCount: 0,
        externalTrigger: false,
      });
      await device.updateVgaGain(vgaCode);

      Alert.alert('Applied', 'Default settings applied to device.');
    } catch (err) {
      Alert.alert('Error', 'Failed to apply settings.');
    }
  }, [device, defaultFreq, defaultAmplitudeMv, defaultVgaDb]);

  // Reset device
  const handleResetDevice = useCallback(() => {
    Alert.alert(
      'Reset Device',
      'This will reboot the Chronos Pulser. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: async () => {
            try {
              if (device.protocolEngine) {
                await device.protocolEngine.resetDevice();
              }
            } catch (err) {
              Alert.alert('Error', 'Reset command failed.');
            }
          },
        },
      ]
    );
  }, [device.protocolEngine]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Pulse Defaults */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Pulse Defaults</Text>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Default Frequency (Hz)</Text>
          <TextInput
            style={styles.input}
            value={defaultFreq}
            onChangeText={setDefaultFreq}
            keyboardType="numeric"
            placeholder="1000"
            placeholderTextColor="#555"
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Default Amplitude (mV)</Text>
          <TextInput
            style={styles.input}
            value={defaultAmplitudeMv}
            onChangeText={setDefaultAmplitudeMv}
            keyboardType="numeric"
            placeholder="250"
            placeholderTextColor="#555"
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Default VGA Gain (dB)</Text>
          <TextInput
            style={styles.input}
            value={defaultVgaDb}
            onChangeText={setDefaultVgaDb}
            keyboardType="numeric"
            placeholder="10.0"
            placeholderTextColor="#555"
          />
        </View>

        <TouchableOpacity style={styles.applyButton} onPress={handleApplyDefaults}>
          <Text style={styles.applyButtonText}>Apply to Device</Text>
        </TouchableOpacity>
      </View>

      {/* Calibration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Calibration</Text>

        <View style={styles.calStatus}>
          <Text style={styles.calLabel}>TDC Calibration:</Text>
          <Text style={[
            styles.calValue,
            { color: device.calibrationValid ? '#00ff88' : '#ffaa00' }
          ]}>
            {device.calibrationValid ? '✓ Valid' : '⚠ Required'}
          </Text>
        </View>

        <TouchableOpacity
          style={[styles.calButton, calRunning && styles.calButtonDisabled]}
          onPress={handleRunCalibration}
          disabled={calRunning || !device.connected}
        >
          {calRunning ? (
            <ActivityIndicator color="#fff" size="small" />
          ) : (
            <Text style={styles.calButtonText}>Run TDC Calibration</Text>
          )}
        </TouchableOpacity>

        {calResult && (
          <View style={styles.calResultCard}>
            <Text style={styles.calResultTitle}>Calibration Result</Text>
            <Text style={styles.calResultText}>
              Temperature: {calResult.temperatureC?.toFixed(1)}°C
            </Text>
            <Text style={styles.calResultText}>
              Bins calibrated: {calResult.binWidthFs?.length || 256}
            </Text>
            <Text style={styles.calResultText}>
              CRC: 0x{calResult.crc?.toString(16).padStart(8, '0')}
            </Text>
          </View>
        )}

        <Text style={styles.calHint}>
          Calibration requires an uncorrelated pulse source connected to the DUT port.
          Run with the port open for best results.
        </Text>
      </View>

      {/* Display Preferences */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display Preferences</Text>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Velocity Factor</Text>
          <TextInput
            style={styles.input}
            value={velocityFactor}
            onChangeText={setVelocityFactor}
            keyboardType="decimal-pad"
            placeholder="0.66"
            placeholderTextColor="#555"
          />
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Reference Impedance (Ω)</Text>
          <TextInput
            style={styles.input}
            value={impedanceRef}
            onChangeText={setImpedanceRef}
            keyboardType="numeric"
            placeholder="50"
            placeholderTextColor="#555"
          />
        </View>

        <View style={styles.toggleRow}>
          <Text style={styles.settingLabel}>Dark Mode</Text>
          <Switch
            value={darkMode}
            onValueChange={setDarkMode}
            trackColor={{ false: '#444', true: '#e94560' }}
            thumbColor="#fff"
          />
        </View>

        <View style={styles.toggleRow}>
          <Text style={styles.settingLabel}>Auto-Connect on Launch</Text>
          <Switch
            value={autoConnect}
            onValueChange={setAutoConnect}
            trackColor={{ false: '#444', true: '#e94560' }}
            thumbColor="#fff"
          />
        </View>
      </View>

      {/* Advanced */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Advanced</Text>

        <TouchableOpacity
          style={styles.dangerButton}
          onPress={handleResetDevice}
        >
          <Text style={styles.dangerButtonText}>Reset Device</Text>
        </TouchableOpacity>

        <Text style={styles.warningText}>
          This will reboot the Chronos Pulser. Any ongoing acquisition will be lost.
        </Text>
      </View>

      {/* About */}
      <View style={styles.aboutCard}>
        <Text style={styles.aboutTitle}>Chronos Pulser</Text>
        <Text style={styles.aboutText}>Companion App v1.0.0</Text>
        <Text style={styles.aboutText}>Precision Time-Domain Reflectometer</Text>
        <Text style={styles.aboutText}>jayis1 — Hardware Design Lab</Text>
        <Text style={styles.aboutText}>© 2026 — Open Source Hardware</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  section: {
    backgroundColor: '#16213e',
    borderRadius: 10,
    padding: 16,
    marginBottom: 14,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  settingLabel: {
    color: '#c0c0c0',
    fontSize: 13,
    flex: 1,
  },
  input: {
    backgroundColor: '#0a0a2a',
    color: '#ffffff',
    fontSize: 14,
    fontWeight: 'bold',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 6,
    width: 100,
    textAlign: 'center',
    fontFamily: 'monospace',
  },
  toggleRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  applyButton: {
    backgroundColor: '#00aa55',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  applyButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  calStatus: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  calLabel: {
    color: '#c0c0c0',
    fontSize: 14,
  },
  calValue: {
    fontSize: 14,
    fontWeight: 'bold',
  },
  calButton: {
    backgroundColor: '#4488ff',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 12,
  },
  calButtonDisabled: {
    backgroundColor: '#334466',
  },
  calButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  calResultCard: {
    backgroundColor: '#0a0a2a',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  calResultTitle: {
    color: '#00ff88',
    fontSize: 13,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  calResultText: {
    color: '#a0a0a0',
    fontSize: 12,
    fontFamily: 'monospace',
    marginBottom: 2,
  },
  calHint: {
    color: '#666',
    fontSize: 11,
    fontStyle: 'italic',
    lineHeight: 16,
  },
  dangerButton: {
    backgroundColor: '#cc3333',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 8,
  },
  dangerButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  warningText: {
    color: '#ff8888',
    fontSize: 11,
    fontStyle: 'italic',
  },
  aboutCard: {
    backgroundColor: '#16213e',
    borderRadius: 10,
    padding: 16,
    alignItems: 'center',
    marginTop: 8,
  },
  aboutTitle: {
    color: '#e94560',
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  aboutText: {
    color: '#808080',
    fontSize: 12,
    marginBottom: 2,
  },
});
