/**
 * Terramesh — Settings Screen
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Application settings for the Terramesh companion app. Includes
 * gateway configuration, cloud API settings, mesh parameters,
 * and data export options.
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  TextInput,
  Switch,
  Alert,
} from 'react-native';
import { useDevices } from '../components/DeviceContext';

const SettingsScreen = () => {
  const { gatewayConnected, setGatewayConnected } = useDevices();

  const [apiUrl, setApiUrl] = useState('https://api.terramesh.io/v1');
  const [apiKey, setApiKey] = useState('');
  const [loraFreq, setLoraFreq] = useState('868.5');
  const [sampleInterval, setSampleInterval] = useState('600');
  const [lteInterval, setLteInterval] = useState('21600');
  const [autoSync, setAutoSync] = useState(true);
  const [pushAlerts, setPushAlerts] = useState(true);
  const [darkMode, setDarkMode] = useState(true);

  const handleSave = () => {
    Alert.alert('Settings Saved', 'Configuration has been updated');
  };

  const handleExport = () => {
    Alert.alert(
      'Export Data',
      'Choose export format:',
      [
        { text: 'CSV', onPress: () => Alert.alert('Export', 'CSV export started') },
        { text: 'GeoJSON', onPress: () => Alert.alert('Export', 'GeoJSON export started') },
        { text: 'Cancel', style: 'cancel' },
      ]
    );
  };

  const handleFactoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will clear all local data and settings. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Reset', style: 'destructive', onPress: () => {
          Alert.alert('Reset', 'All local data cleared');
        }},
      ]
    );
  };

  const SettingSection = ({ title, children }) => (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>{title}</Text>
      {children}
    </View>
  );

  const SettingRow = ({ label, value, onPress, type }) => (
    <TouchableOpacity style={styles.settingRow} onPress={onPress} disabled={!onPress}>
      <Text style={styles.settingLabel}>{label}</Text>
      {type === 'switch' ? (
        <Switch
          value={value}
          onValueChange={onPress}
          trackColor={{ false: '#2a2a3e', true: '#003d33' }}
          thumbColor={value ? '#00d4aa' : '#6c6c80'}
        />
      ) : type === 'input' ? (
        <TextInput
          style={styles.settingInput}
          value={value}
          onChangeText={onPress}
          keyboardType={label.includes('URL') ? 'url' : 'decimal-pad'}
          placeholderTextColor="#4a4a5a"
        />
      ) : (
        <Text style={styles.settingValue}>{value}</Text>
      )}
    </TouchableOpacity>
  );

  return (
    <ScrollView style={styles.container}>
      {/* Connection */}
      <SettingSection title="Gateway Connection">
        <View style={styles.connectionStatus}>
          <View style={[styles.statusDot, {
            backgroundColor: gatewayConnected ? '#00d4aa' : '#ff4444'
          }]} />
          <Text style={styles.statusText}>
            {gatewayConnected ? 'Connected' : 'Disconnected'}
          </Text>
          <TouchableOpacity
            style={[styles.connectButton, {
              backgroundColor: gatewayConnected ? '#3d0000' : '#003d33'
            }]}
            onPress={() => setGatewayConnected(!gatewayConnected)}
          >
            <Text style={styles.connectButtonText}>
              {gatewayConnected ? 'Disconnect' : 'Connect'}
            </Text>
          </TouchableOpacity>
        </View>
      </SettingSection>

      {/* Cloud API */}
      <SettingSection title="Cloud API">
        <SettingRow
          label="API URL"
          value={apiUrl}
          type="input"
          onPress={setApiUrl}
        />
        <SettingRow
          label="API Key"
          value={apiKey ? '••••••••' : 'Not set'}
          onPress={() => Alert.alert('API Key', 'Enter your API key')}
        />
        <SettingRow
          label="Auto Sync"
          value={autoSync}
          type="switch"
          onPress={setAutoSync}
        />
      </SettingSection>

      {/* Mesh Configuration */}
      <SettingSection title="Mesh Configuration">
        <SettingRow
          label="LoRa Frequency (MHz)"
          value={loraFreq}
          type="input"
          onPress={setLoraFreq}
        />
        <SettingRow
          label="Sample Interval (s)"
          value={sampleInterval}
          type="input"
          onPress={setSampleInterval}
        />
        <SettingRow
          label="LTE Uplink Interval (s)"
          value={lteInterval}
          type="input"
          onPress={setLteInterval}
        />
      </SettingSection>

      {/* Notifications */}
      <SettingSection title="Notifications">
        <SettingRow
          label="Push Alerts"
          value={pushAlerts}
          type="switch"
          onPress={setPushAlerts}
        />
        <SettingRow
          label="Critical Alert Sound"
          value="Enabled"
          onPress={() => {}}
        />
        <SettingRow
          label="Warning Alert Sound"
          value="Enabled"
          onPress={() => {}}
        />
      </SettingSection>

      {/* Display */}
      <SettingSection title="Display">
        <SettingRow
          label="Dark Mode"
          value={darkMode}
          type="switch"
          onPress={setDarkMode}
        />
        <SettingRow
          label="Chart Time Range"
          value="Last 20 samples"
          onPress={() => {}}
        />
        <SettingRow
          label="Map Style"
          value="Satellite"
          onPress={() => {}}
        />
      </SettingSection>

      {/* Data */}
      <SettingSection title="Data Management">
        <TouchableOpacity style={styles.actionButton} onPress={handleExport}>
          <Text style={styles.actionButtonText}>📤 Export Sensor Data</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionButton} onPress={() => {
          Alert.alert('Sync', 'Manual sync initiated');
        }}>
          <Text style={styles.actionButtonText}>🔄 Force Cloud Sync</Text>
        </TouchableOpacity>
      </SettingSection>

      {/* About */}
      <SettingSection title="About">
        <SettingRow label="App Version" value="1.0.0" />
        <SettingRow label="Author" value="jayis1" />
        <SettingRow label="License" value="MIT" />
        <SettingRow label="Firmware" value="v1.0.0" />
        <SettingRow label="Protocol" value="Terramesh Mesh v1" />
      </SettingSection>

      {/* Danger Zone */}
      <SettingSection title="Danger Zone">
        <TouchableOpacity
          style={[styles.actionButton, styles.dangerButton]}
          onPress={handleFactoryReset}
        >
          <Text style={[styles.actionButtonText, styles.dangerText]}>
            ⚠ Factory Reset
          </Text>
        </TouchableOpacity>
      </SettingSection>

      <Text style={styles.footer}>
        Terramesh v1.0 — Designed by jayis1
      </Text>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
  },
  section: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  sectionTitle: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#00d4aa',
    marginBottom: 12,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  connectionStatus: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 10,
  },
  statusDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
  },
  statusText: {
    fontSize: 14,
    color: '#e0e0e0',
    flex: 1,
  },
  connectButton: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
  },
  connectButtonText: {
    fontSize: 13,
    fontWeight: '600',
    color: '#e0e0e0',
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  settingLabel: {
    fontSize: 14,
    color: '#e0e0e0',
    flex: 1,
  },
  settingValue: {
    fontSize: 14,
    color: '#6c6c80',
  },
  settingInput: {
    fontSize: 14,
    color: '#e0e0e0',
    backgroundColor: '#0f0f1a',
    borderWidth: 1,
    borderColor: '#2a2a3e',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    width: 120,
    textAlign: 'right',
  },
  actionButton: {
    backgroundColor: '#0f0f1a',
    borderWidth: 1,
    borderColor: '#2a2a3e',
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    marginBottom: 8,
  },
  actionButtonText: {
    fontSize: 14,
    color: '#e0e0e0',
    fontWeight: '500',
  },
  dangerButton: {
    borderColor: '#ff4444',
  },
  dangerText: {
    color: '#ff4444',
  },
  footer: {
    fontSize: 12,
    color: '#4a4a5a',
    textAlign: 'center',
    padding: 20,
    paddingBottom: 40,
  },
});

export default SettingsScreen;
