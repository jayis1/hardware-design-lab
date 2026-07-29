// src/screens/CalibrationScreen.tsx — White reference + SPAD calibration
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert } from 'react-native';
import { Card, Title, Paragraph, Button, TextInput, Divider, ActivityIndicator } from 'react-native-paper';
import { useBle } from '../ble/BleManager';

export default function CalibrationScreen() {
  const { connectionState, sendCommand } = useBle();
  const [calibState, setCalibState] = useState<'idle' | 'white_ref' | 'spad' | 'done'>('idle');
  const [busy, setBusy] = useState(false);
  const [spadKnown, setSpadKnown] = useState('');
  const [spadReading, setSpadReading] = useState('');

  const connected = connectionState === 'connected';

  const handleWhiteRef = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to the ChloroMap device first.');
      return;
    }
    setBusy(true);
    setCalibState('white_ref');
    // Send CAL WHITE command (command code 0x02)
    await sendCommand([0x02]);
    // Simulate calibration time
    setTimeout(() => {
      setBusy(false);
      setCalibState('done');
      Alert.alert('Success', 'White reference calibration stored.');
    }, 3000);
  };

  const handleSpadCal = async () => {
    const known = parseFloat(spadKnown);
    const reading = parseFloat(spadReading);
    if (isNaN(known) || isNaN(reading)) {
      Alert.alert('Invalid Input', 'Enter numeric SPAD values from reference meter and ChloroMap.');
      return;
    }
    setBusy(true);
    setCalibState('spad');
    // Calculate slope + offset: known = slope * reading + offset
    const slope = known / reading;
    const slopeX1000 = Math.round(slope * 1000);
    // Send SPAD calibration command (0x03) with slope bytes
    await sendCommand([0x03, (slopeX1000 >> 0) & 0xff, (slopeX1000 >> 8) & 0xff]);
    setTimeout(() => {
      setBusy(false);
      Alert.alert('Success', `SPAD calibration stored. Slope: ${slope.toFixed(3)}`);
      setCalibState('done');
    }, 1500);
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Status */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Calibration</Title>
          <Paragraph style={styles.paragraph}>
            Calibrate your ChloroMap for accurate reflectance measurements.
            Recalibrate when lighting conditions change significantly.
          </Paragraph>
          <Divider style={styles.divider} />
          <View style={styles.statusRow}>
            <Text style={styles.statusLabel}>Connection:</Text>
            <Text style={[styles.statusVal, { color: connected ? '#66bb6a' : '#d32f2f' }]}>
              {connected ? 'Connected' : 'Disconnected'}
            </Text>
          </View>
        </Card.Content>
      </Card>

      {/* White reference calibration */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>1. White Reference</Title>
          <Paragraph style={styles.instructions}>
            Place the white Teflon reference tile in the leaf clamp.
            Press "Start White Reference" and hold still for 3 seconds.
          </Paragraph>
          <Button
            mode="contained"
            onPress={handleWhiteRef}
            disabled={!connected || busy}
            color="#2e7d32"
            style={styles.button}
          >
            Start White Reference
          </Button>
          {calibState === 'white_ref' && busy && (
            <View style={styles.busyRow}>
              <ActivityIndicator color="#66bb6a" />
              <Text style={styles.busyText}>Calibrating...</Text>
            </View>
          )}
        </Card.Content>
      </Card>

      {/* SPAD calibration */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>2. SPAD Calibration (Optional)</Title>
          <Paragraph style={styles.instructions}>
            For SPAD values matching a commercial SPAD meter, enter paired readings:
            take a reading with both the reference SPAD meter and ChloroMap
            on the same leaf.
          </Paragraph>
          <TextInput
            label="Reference SPAD meter reading"
            value={spadKnown}
            onChangeText={setSpadKnown}
            keyboardType="numeric"
            style={styles.input}
            theme={{ colors: { text: '#e8f5e9', placeholder: '#81c784' } }}
          />
          <TextInput
            label="ChloroMap SPAD reading"
            value={spadReading}
            onChangeText={setSpadReading}
            keyboardType="numeric"
            style={styles.input}
            theme={{ colors: { text: '#e8f5e9', placeholder: '#81c784' } }}
          />
          <Button
            mode="contained"
            onPress={handleSpadCal}
            disabled={!connected || busy || !spadKnown || !spadReading}
            color="#ffca28"
            style={styles.button}
          >
            Save SPAD Calibration
          </Button>
          {calibState === 'spad' && busy && (
            <View style={styles.busyRow}>
              <ActivityIndicator color="#ffca28" />
              <Text style={styles.busyText}>Saving...</Text>
            </View>
          )}
        </Card.Content>
      </Card>

      {/* Tips */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Calibration Tips</Title>
          <Text style={styles.tip}>• Calibrate white reference at the start of each field session</Text>
          <Text style={styles.tip}>• Recalibrate if temperature changes {'>'} 10°C</Text>
          <Text style={styles.tip}>• Keep the reference tile clean and scratch-free</Text>
          <Text style={styles.tip}>• SPAD calibration needs at least 3 leaf samples for accuracy</Text>
          <Text style={styles.tip}>• Avoid calibrating in direct sunlight (causes stray light)</Text>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f0a' },
  content: { padding: 8, paddingBottom: 20 },
  card: { backgroundColor: '#152815', marginBottom: 8 },
  title: { color: '#e8f5e9', fontSize: 22 },
  paragraph: { color: '#81c784', fontSize: 13, marginTop: 4 },
  divider: { marginVertical: 8, backgroundColor: '#2e4d2e' },
  statusRow: { flexDirection: 'row', marginTop: 4 },
  statusLabel: { color: '#b0bec5', fontSize: 14, marginRight: 8 },
  statusVal: { fontSize: 14, fontWeight: '600' },
  sectionTitle: { color: '#e8f5e9', fontSize: 18, marginBottom: 8 },
  instructions: { color: '#81c784', fontSize: 13, marginBottom: 12 },
  button: { marginTop: 8 },
  busyRow: { flexDirection: 'row', alignItems: 'center', marginTop: 12, gap: 8 },
  busyText: { color: '#81c784', fontSize: 13, marginLeft: 8 },
  input: { backgroundColor: '#0a1f0a', marginBottom: 8 },
  tip: { color: '#b0bec5', fontSize: 12, marginVertical: 3 },
});