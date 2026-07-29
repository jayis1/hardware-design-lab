// src/screens/SettingsScreen.tsx — Device configuration
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Switch } from 'react-native';
import {
  Card, Title, Paragraph, TextInput, Button, Divider, RadioButton, List,
} from 'react-native-paper';
import { useBle } from '../ble/BleManager';

export default function SettingsScreen() {
  const { connectionState, status, deviceName, sendCommand } = useBle();
  const [bleName, setBleName] = useState('ChloroMap-001');
  const [integTime, setIntegTime] = useState('50');
  const [gpsRate, setGpsRate] = useState('10');
  const [bandMask, setBandMask] = useState('0xFFFF');
  const [darkMode, setDarkMode] = useState(true);
  const [autoLog, setAutoLog] = useState(true);
  const [vibrateOnMeas, setVibrateOnMeas] = useState(true);
  const [units, setUnits] = useState<'metric' | 'imperial'>('metric');

  const connected = connectionState === 'connected';

  const handleSave = async () => {
    if (!connected) return;
    // Send configuration commands to device
    // Command 0x10: set integration time
    await sendCommand([0x10, parseInt(integTime) & 0xff]);
    // Command 0x11: set GPS rate
    await sendCommand([0x11, parseInt(gpsRate) & 0xff]);
    // Command 0x12: set band mask (16-bit LE)
    const mask = parseInt(bandMask, 16) || 0xffff;
    await sendCommand([0x12, (mask >> 0) & 0xff, (mask >> 8) & 0xff]);
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Device info */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Device</Title>
          <View style={styles.row}>
            <Text style={styles.label}>Name:</Text>
            <Text style={styles.value}>{deviceName || 'Not connected'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Connection:</Text>
            <Text style={[styles.value, { color: connected ? '#66bb6a' : '#d32f2f' }]}>
              {connectionState}
            </Text>
          </View>
          {status && (
            <>
              <View style={styles.row}>
                <Text style={styles.label}>Battery:</Text>
                <Text style={styles.value}>{status.battMv} mV ({status.battPct}%)</Text>
              </View>
              <View style={styles.row}>
                <Text style={styles.label}>GPS Sats:</Text>
                <Text style={styles.value}>{status.sats} ({status.fixType === 3 ? '3D fix' : 'no fix'})</Text>
              </View>
            </>
          )}
        </Card.Content>
      </Card>

      {/* Measurement settings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Measurement</Title>
          <TextInput
            label="Integration time (ms)"
            value={integTime}
            onChangeText={setIntegTime}
            keyboardType="numeric"
            style={styles.input}
          />
          <TextInput
            label="Band mask (hex, e.g. 0xFFFF for all 16)"
            value={bandMask}
            onChangeText={setBandMask}
            style={styles.input}
          />
          <Paragraph style={styles.hint}>
            Band mask selects which of the 16 spectral bands to compute.
            Each bit corresponds to a band (bit 0 = 450nm, bit 15 = 1050nm).
          </Paragraph>
        </Card.Content>
      </Card>

      {/* GPS settings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>GPS</Title>
          <TextInput
            label="GPS update rate (Hz, 1–10)"
            value={gpsRate}
            onChangeText={setGpsRate}
            keyboardType="numeric"
            style={styles.input}
          />
          <Paragraph style={styles.hint}>
            Higher rates improve map accuracy but use more battery.
            10 Hz recommended for walking field rows.
          </Paragraph>
        </Card.Content>
      </Card>

      {/* App settings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>App</Title>
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Dark mode</Text>
            <Switch value={darkMode} onValueChange={setDarkMode} color="#66bb6a" />
          </View>
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Auto-log to SD</Text>
            <Switch value={autoLog} onValueChange={setAutoLog} color="#66bb6a" />
          </View>
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Vibrate on measurement</Text>
            <Switch value={vibrateOnMeas} onValueChange={setVibrateOnMeas} color="#66bb6a" />
          </View>
          <Divider style={styles.divider} />
          <Text style={styles.radioLabel}>Units</Text>
          <RadioButton.Group onValueChange={(v) => setUnits(v as 'metric' | 'imperial')} value={units}>
            <View style={styles.radioRow}>
              <RadioButton value="metric" color="#66bb6a" />
              <Text style={styles.radioText}>Metric (kg N/ha, °C)</Text>
            </View>
            <View style={styles.radioRow}>
              <RadioButton value="imperial" color="#66bb6a" />
              <Text style={styles.radioText}>Imperial (lb N/acre, °F)</Text>
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      {/* About */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>About</Title>
          <Text style={styles.aboutText}>ChloroMap v1.0</Text>
          <Text style={styles.aboutText}>Handheld Leaf Chlorophyll & Nitrogen Spectrometer</Text>
          <Text style={styles.aboutText}>Author: jayis1</Text>
          <Text style={styles.aboutText}>License: MIT (app), GPL-2.0 (firmware), CERN-OHL-S v2 (hardware)</Text>
          <Text style={styles.aboutText}>Copyright © 2026 jayis1</Text>
        </Card.Content>
      </Card>

      {/* Save button */}
      <Button
        mode="contained"
        onPress={handleSave}
        disabled={!connected}
        color="#2e7d32"
        style={styles.saveButton}
      >
        Save Settings to Device
      </Button>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f0a' },
  content: { padding: 8, paddingBottom: 20 },
  card: { backgroundColor: '#152815', marginBottom: 8 },
  title: { color: '#e8f5e9', fontSize: 22 },
  sectionTitle: { color: '#e8f5e9', fontSize: 18, marginBottom: 8 },
  row: { flexDirection: 'row', marginVertical: 4 },
  label: { color: '#b0bec5', fontSize: 14, width: 110 },
  value: { color: '#e8f5e9', fontSize: 14 },
  input: { backgroundColor: '#0a1f0a', marginBottom: 8 },
  hint: { color: '#81c784', fontSize: 12, marginTop: 4 },
  switchRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginVertical: 8 },
  switchLabel: { color: '#e8f5e9', fontSize: 14 },
  divider: { marginVertical: 8, backgroundColor: '#2e4d2e' },
  radioLabel: { color: '#e8f5e9', fontSize: 14, marginBottom: 4 },
  radioRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  radioText: { color: '#b0bec5', fontSize: 14, marginLeft: 8 },
  aboutText: { color: '#81c784', fontSize: 13, marginVertical: 2 },
  saveButton: { margin: 8, marginBottom: 20 },
});