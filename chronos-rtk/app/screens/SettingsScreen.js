/**
 * SettingsScreen.js — Device configuration screen for Chronos-RTK
 * Configure LoRa, logging, RTK base/rover mode, and display options
 */

import React, { useContext, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  Switch,
  TouchableOpacity,
  Picker,
} from 'react-native';
import { ChronosContext } from '../components/ChronosContext';
import StatusCard from '../components/StatusCard';

export default function SettingsScreen() {
  const { isConnected, sendCommand, config } = useContext(ChronosContext);

  const [loraFreq, setLoraFreq] = useState('868');
  const [loraSf, setLoraSf] = useState('7');
  const [loggingEnabled, setLoggingEnabled] = useState(true);
  const [rtkMode, setRtkMode] = useState('rover');
  const [nmeaRate, setNmeaRate] = useState('1');
  const [outputFormat, setOutputFormat] = useState('nmea');

  const handleApply = () => {
    if (!isConnected) return;
    sendCommand('SET_LORA_FREQ', loraFreq);
    sendCommand('SET_LORA_SF', loraSf);
    sendCommand('SET_RTK_MODE', rtkMode);
    sendCommand('SET_NMEA_RATE', nmeaRate);
    sendCommand('SET_OUTPUT', outputFormat);
    sendCommand('SET_LOGGING', loggingEnabled ? '1' : '0');
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection status */}
      <StatusCard title="Connection" icon="bluetooth-connect">
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Status</Text>
          <Text style={[styles.settingValue, { color: isConnected ? '#4CAF50' : '#F44336' }]}>
            {isConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Firmware</Text>
          <Text style={styles.settingValue}>{config?.firmware ?? 'v1.0.0'}</Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Battery</Text>
          <Text style={styles.settingValue}>{config?.battery ?? '--'}%</Text>
        </View>
      </StatusCard>

      {/* RTK Mode */}
      <StatusCard title="RTK Configuration" icon="access-point">
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Mode</Text>
          <Picker
            selectedValue={rtkMode}
            onValueChange={setRtkMode}
            style={styles.picker}
            enabled={isConnected}
          >
            <Picker.Item label="Rover (receive corrections)" value="rover" />
            <Picker.Item label="Base (send corrections)" value="base" />
            <Picker.Item label="Standalone (no RTK)" value="standalone" />
          </Picker>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Output</Text>
          <Picker
            selectedValue={outputFormat}
            onValueChange={setOutputFormat}
            style={styles.picker}
            enabled={isConnected}
          >
            <Picker.Item label="NMEA 0183" value="nmea" />
            <Picker.Item label="UBX Binary" value="ubx" />
            <Picker.Item label="RTCM v3.3" value="rtcm" />
          </Picker>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>NMEA Rate</Text>
          <Picker
            selectedValue={nmeaRate}
            onValueChange={setNmeaRate}
            style={styles.picker}
            enabled={isConnected}
          >
            <Picker.Item label="1 Hz" value="1" />
            <Picker.Item label="5 Hz" value="5" />
            <Picker.Item label="10 Hz" value="10" />
            <Picker.Item label="20 Hz" value="20" />
          </Picker>
        </View>
      </StatusCard>

      {/* LoRa Settings */}
      <StatusCard title="LoRa Radio" icon="radio-tower">
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Frequency</Text>
          <Picker
            selectedValue={loraFreq}
            onValueChange={setLoraFreq}
            style={styles.picker}
            enabled={isConnected}
          >
            <Picker.Item label="868 MHz (EU)" value="868" />
            <Picker.Item label="915 MHz (US/AU)" value="915" />
          </Picker>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Spreading Factor</Text>
          <Picker
            selectedValue={loraSf}
            onValueChange={setLoraSf}
            style={styles.picker}
            enabled={isConnected}
          >
            <Picker.Item label="SF7 (fast, short range)" value="7" />
            <Picker.Item label="SF8" value="8" />
            <Picker.Item label="SF9" value="9" />
            <Picker.Item label="SF10" value="10" />
            <Picker.Item label="SF11" value="11" />
            <Picker.Item label="SF12 (slow, long range)" value="12" />
          </Picker>
        </View>
      </StatusCard>

      {/* Logging */}
      <StatusCard title="Data Logging" icon="content-save">
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Flash Logging</Text>
          <Switch
            value={loggingEnabled}
            onValueChange={setLoggingEnabled}
            trackColor={{ false: '#767577', true: '#4CAF50' }}
            disabled={!isConnected}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Flash Used</Text>
          <Text style={styles.settingValue}>{config?.flashUsed ?? '--'}%</Text>
        </View>
        <TouchableOpacity
          style={[styles.actionButton, { opacity: isConnected ? 1 : 0.5 }]}
          onPress={() => isConnected && sendCommand('ERASE_FLASH', '1')}
          disabled={!isConnected}
        >
          <Text style={styles.actionButtonText}>Erase Flash</Text>
        </TouchableOpacity>
      </StatusCard>

      {/* Apply button */}
      <TouchableOpacity
        style={[styles.applyButton, { opacity: isConnected ? 1 : 0.5 }]}
        onPress={handleApply}
        disabled={!isConnected}
      >
        <Text style={styles.applyButtonText}>Apply Configuration</Text>
      </TouchableOpacity>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1117',
    padding: 8,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  settingLabel: {
    color: '#8b949e',
    fontSize: 14,
    fontWeight: '600',
    flex: 1,
  },
  settingValue: {
    color: '#e6edf3',
    fontSize: 14,
    flex: 1,
    textAlign: 'right',
  },
  picker: {
    color: '#e6edf3',
    flex: 1,
    height: 40,
  },
  actionButton: {
    backgroundColor: '#21262d',
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  actionButtonText: {
    color: '#F44336',
    fontSize: 14,
    fontWeight: '600',
  },
  applyButton: {
    backgroundColor: '#2196F3',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 16,
    marginBottom: 8,
  },
  applyButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
});