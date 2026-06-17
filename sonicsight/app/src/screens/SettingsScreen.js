/**
 * SettingsScreen.js — SonicSight Companion
 * Device configuration, BLE pairing, calibration management, firmware OTA.
 * @author jayis1
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Switch, Slider,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const SettingsScreen = ({ bleService }) => {
  const [useAutoLambda, setUseAutoLambda] = useState(true);
  const [manualLambda, setManualLambda] = useState(0.5);
  const [gridSize, setGridSize] = useState(64);
  const [tempAlpha, setTempAlpha] = useState(-0.007);
  const [tempRef, setTempRef] = useState(20);

  return (
    <ScrollView style={styles.container}>
      {/* Device Info */}
      <Text style={styles.sectionHeader}>Device</Text>
      <View style={styles.card}>
        <View style={styles.row}>
          <Text style={styles.label}>Name</Text>
          <Text style={styles.value}>SonicSight-001</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Firmware</Text>
          <Text style={styles.value}>v1.0.0</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Serial</Text>
          <Text style={styles.value}>SS-2026-0001</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>BLE Address</Text>
          <Text style={styles.value}>E4:44:A4:01:02:03</Text>
        </View>
      </View>

      {/* BLE Pairing */}
      <Text style={styles.sectionHeader}>Bluetooth</Text>
      <View style={styles.card}>
        <TouchableOpacity style={styles.actionRow}>
          <Icon name="bluetooth" size={24} color="#00d2ff" />
          <Text style={styles.actionText}>Scan for Devices</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionRow}>
          <Icon name="link-variant" size={24} color="#4caf50" />
          <Text style={styles.actionText}>SonicSight-001 (Connected)</Text>
        </TouchableOpacity>
      </View>

      {/* Reconstruction */}
      <Text style={styles.sectionHeader}>Reconstruction</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.label}>Auto Lambda</Text>
          <Switch value={useAutoLambda} onValueChange={setUseAutoLambda}
                  trackColor={{ false: '#333', true: '#00d2ff' }} />
        </View>
        {!useAutoLambda && (
          <View style={styles.settingRow}>
            <Text style={styles.label}>λ = {manualLambda.toFixed(2)}</Text>
            <Slider style={{ flex: 1 }} minimumValue={0.01} maximumValue={100}
                    value={manualLambda}
                    onValueChange={(v) => setManualLambda(v)}
                    minimumTrackTintColor="#e040fb"
                    thumbTintColor="#fff" />
          </View>
        )}
        <View style={styles.settingRow}>
          <Text style={styles.label}>Grid Size</Text>
          <View style={styles.gridSelector}>
            {[32, 64, 128].map((s) => (
              <TouchableOpacity key={s}
                style={[styles.gridBtn, gridSize === s && styles.gridActive]}
                onPress={() => { setGridSize(s); bleService?.sendCommand(`CMD=GRID:${s}`); }}>
                <Text style={[styles.gridText, gridSize === s && styles.gridTextActive]}>
                  {s}×{s}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      {/* Temperature Compensation */}
      <Text style={styles.sectionHeader}>Temperature</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.label}>α (×10⁻³/°C)</Text>
          <Text style={styles.value}>{tempAlpha.toFixed(3)}</Text>
          <Slider style={{ flex: 1, marginLeft: 8 }}
                  minimumValue={-0.015} maximumValue={-0.001}
                  value={tempAlpha}
                  onValueChange={(v) => setTempAlpha(v)}
                  minimumTrackTintColor="#ff9800"
                  thumbTintColor="#fff" />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.label}>T_ref (°C)</Text>
          <Text style={styles.value}>{tempRef} °C</Text>
          <Slider style={{ flex: 1, marginLeft: 8 }}
                  minimumValue={0} maximumValue={40}
                  value={tempRef}
                  onValueChange={(v) => setTempRef(Math.round(v))}
                  minimumTrackTintColor="#ff9800"
                  thumbTintColor="#fff" />
        </View>
      </View>

      {/* Calibration */}
      <Text style={styles.sectionHeader}>Calibration</Text>
      <View style={styles.card}>
        <TouchableOpacity style={styles.actionRow}
          onPress={() => bleService?.sendCommand('CMD=CAL')}>
          <Icon name="tune" size={24} color="#ff9800" />
          <Text style={styles.actionText}>Run Full Calibration</Text>
        </TouchableOpacity>
        <View style={styles.row}>
          <Text style={styles.label}>Last Calibrated</Text>
          <Text style={styles.value}>2026-06-17 10:30</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Temp Reference</Text>
          <Text style={styles.value}>20.0 °C</Text>
        </View>
      </View>

      {/* Firmware Update */}
      <Text style={styles.sectionHeader}>Firmware</Text>
      <View style={styles.card}>
        <TouchableOpacity style={styles.actionRow}>
          <Icon name="cloud-download" size={24} color="#2196f3" />
          <Text style={styles.actionText}>Check for Updates</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionRow}>
          <Icon name="cellphone-arrow-down" size={24} color="#e040fb" />
          <Text style={styles.actionText}>OTA DFU Update</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.aboutCard}>
        <Text style={styles.aboutText}>
          SonicSight Companion v1.0.0{'\n'}
          Author: jayis1{'\n'}
          © 2026 jayis1. MIT License.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23' },
  sectionHeader: {
    color: '#00d2ff', fontSize: 14, fontWeight: 'bold',
    textTransform: 'uppercase', letterSpacing: 1,
    marginHorizontal: 16, marginTop: 16, marginBottom: 8,
  },
  card: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 12,
    marginHorizontal: 12, marginBottom: 8,
    borderWidth: 1, borderColor: '#16213e',
  },
  row: {
    flexDirection: 'row', justifyContent: 'space-between',
    paddingVertical: 6, borderBottomWidth: 1,
    borderBottomColor: '#16213e',
  },
  label: { color: '#aaa', fontSize: 13 },
  value: { color: '#fff', fontSize: 13 },
  actionRow: {
    flexDirection: 'row', alignItems: 'center',
    paddingVertical: 10, borderBottomWidth: 1,
    borderBottomColor: '#16213e',
  },
  actionText: { color: '#fff', fontSize: 14, marginLeft: 12 },
  settingRow: {
    flexDirection: 'row', alignItems: 'center',
    paddingVertical: 8, borderBottomWidth: 1,
    borderBottomColor: '#16213e',
  },
  gridSelector: { flexDirection: 'row' },
  gridBtn: {
    backgroundColor: '#16213e', borderRadius: 4, padding: 4,
    paddingHorizontal: 10, marginLeft: 4, borderWidth: 1, borderColor: '#333',
  },
  gridActive: { borderColor: '#00d2ff', backgroundColor: '#003344' },
  gridText: { color: '#888', fontSize: 12 },
  gridTextActive: { color: '#fff' },
  aboutCard: {
    alignItems: 'center', padding: 20, marginTop: 12, marginBottom: 32,
  },
  aboutText: { color: '#555', fontSize: 12, textAlign: 'center', lineHeight: 18 },
});

export default SettingsScreen;