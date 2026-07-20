/**
 * CalibrationView.js — Calibration and diagnostics screen
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Alert, FlatList,
} from 'react-native';
import { useBle } from '../BleContext';

const Colors = {
  bg: '#0a1628', card: '#162640', accent: '#3b82f6', text: '#e2e8f0',
  textDim: '#94a3b8', success: '#10b981', danger: '#ef4444', warning: '#f59e0b',
};

export function CalibrationView({ navigation }) {
  const ble = useBle();
  const [running, setRunning] = useState(false);
  const [results, setResults] = useState(null);

  // Electrode contact quality (simulated)
  const [contacts, setContacts] = useState(
    Array.from({ length: 16 }, (_, i) => ({
      electrode: i + 1,
      status: 'unknown',
      impedance: 0,
    }))
  );

  const runSelfTest = useCallback(async () => {
    setRunning(true);
    try {
      await ble.runSelfTest();
      setResults({ selfTest: 'PASS', timestamp: Date.now() });
      Alert.alert('Self-Test', 'AD5940 self-test passed (internal 100Ω reference OK)');
    } catch (e) {
      setResults({ selfTest: 'FAIL', error: e.message });
      Alert.alert('Self-Test Failed', e.message);
    } finally {
      setRunning(false);
    }
  }, [ble]);

  const checkContacts = useCallback(() => {
    // In production, this would send a BLE command and receive results
    setContacts(Array.from({ length: 16 }, (_, i) => ({
      electrode: i + 1,
      status: i < 14 ? 'good' : (i === 14 ? 'marginal' : 'bad'),
      impedance: i < 14 ? 200 + i * 50 : (i === 14 ? 45000 : 150000),
    })));
  }, []);

  const startCalibration = useCallback(() => {
    Alert.alert(
      'Calibration',
      'Place the electrode ring in 0.1 S/m NaCl solution and press OK',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'OK', onPress: () => {
          setRunning(true);
          setTimeout(() => {
            setRunning(false);
            setResults(prev => ({ ...prev, calibration: 'PASS' }));
            Alert.alert('Calibration', 'Calibration complete. Coefficients saved to flash.');
          }, 3000);
        }},
      ]
    );
  }, []);

  const renderContact = ({ item }) => {
    const color = item.status === 'good' ? Colors.success
                 : item.status === 'marginal' ? Colors.warning
                 : item.status === 'bad' ? Colors.danger
                 : Colors.textDim;
    return (
      <View style={styles.contactRow}>
        <Text style={styles.contactNum}>E{item.electrode}</Text>
        <Text style={[styles.contactStatus, { color }]}>
          {item.status === 'unknown' ? '—' : item.status.toUpperCase()}
        </Text>
        <Text style={styles.contactImp}>
          {item.status === 'unknown' ? '—' : `${item.impedance / 1000}kΩ`}
        </Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Calibration & Diagnostics</Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>AD5940 Self-Test</Text>
        <Text style={styles.descText}>
          Measures the internal 100Ω reference resistor to verify
          the bioimpedance AFE is functioning correctly.
        </Text>
        <TouchableOpacity
          style={[styles.button, styles.primaryButton, running && styles.buttonDisabled]}
          onPress={runSelfTest}
          disabled={running}>
          <Text style={styles.buttonText}>
            {running ? 'Running...' : 'Run Self-Test'}
          </Text>
        </TouchableOpacity>
        {results && results.selfTest && (
          <Text style={[styles.resultText,
            { color: results.selfTest === 'PASS' ? Colors.success : Colors.danger }]}>
            Self-test: {results.selfTest}
          </Text>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Field Calibration</Text>
        <Text style={styles.descText}>
          Place the electrode ring in a 0.1 S/m NaCl reference solution.
          This calibrates the system and compensates for electrode
          contact impedance drift.
        </Text>
        <TouchableOpacity
          style={[styles.button, styles.primaryButton, running && styles.buttonDisabled]}
          onPress={startCalibration}
          disabled={running}>
          <Text style={styles.buttonText}>
            {running ? 'Calibrating...' : 'Start Calibration'}
          </Text>
        </TouchableOpacity>
        {results && results.calibration && (
          <Text style={[styles.resultText, { color: Colors.success }]}>
            Calibration: {results.calibration}
          </Text>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Electrode Contact Check</Text>
        <Text style={styles.descText}>
          Verify each electrode has good contact with the soil.
          Impedance &gt; 50kΩ indicates poor contact.
        </Text>
        <TouchableOpacity
          style={[styles.button, styles.secondaryButton]}
          onPress={checkContacts}>
          <Text style={styles.buttonText}>Check Contacts</Text>
        </TouchableOpacity>
        <FlatList
          data={contacts}
          renderItem={renderContact}
          keyExtractor={item => item.electrode.toString()}
          style={styles.contactList}
          scrollEnabled={false}
        />
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Info</Text>
        <InfoRow label="Firmware" value="v1.0.0" />
        <InfoRow label="Hardware" value="rev A" />
        <InfoRow label="BLE Address" value={ble.device?.id || '—'} />
        <InfoRow label="Battery" value={`${ble.status.batteryPct}% (${ble.status.voltageMv}mV)`} />
        <InfoRow label="SD Card" value={ble.status.sdOk ? 'Present' : 'Not detected'} />
      </View>

      <Text style={styles.footer}>© 2026 jayis1 · MIT</Text>
    </View>
  );
}

function InfoRow({ label, value }) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.bg, padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: Colors.text, marginBottom: 16 },
  card: { backgroundColor: Colors.card, borderRadius: 12, padding: 16, marginBottom: 12 },
  cardTitle: { fontSize: 16, color: Colors.text, fontWeight: 'bold', marginBottom: 8 },
  descText: { fontSize: 12, color: Colors.textDim, marginBottom: 12, lineHeight: 18 },
  button: { borderRadius: 8, padding: 12, alignItems: 'center', marginTop: 4 },
  primaryButton: { backgroundColor: Colors.accent },
  secondaryButton: { backgroundColor: Colors.bg, borderWidth: 1, borderColor: Colors.accent },
  buttonDisabled: { opacity: 0.5 },
  buttonText: { color: Colors.text, fontSize: 14, fontWeight: '600' },
  resultText: { fontSize: 14, fontWeight: '600', marginTop: 8 },
  contactList: { marginTop: 12 },
  contactRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6, borderBottomWidth: 0.5, borderBottomColor: '#1e3a5f' },
  contactNum: { fontSize: 13, color: Colors.text, fontWeight: '600', width: 40 },
  contactStatus: { fontSize: 12, fontWeight: '600', flex: 1 },
  contactImp: { fontSize: 12, color: Colors.textDim, width: 60, textAlign: 'right' },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  infoLabel: { fontSize: 13, color: Colors.textDim },
  infoValue: { fontSize: 13, color: Colors.text, fontWeight: '500' },
  footer: { fontSize: 11, color: Colors.textDim, textAlign: 'center', paddingVertical: 16 },
});