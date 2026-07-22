/**
 * ConnectScreen — scan and connect to RebarScope device
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React, { useState, useEffect } from 'react';
import { View, Text, Button, StyleSheet, ActivityIndicator } from 'react-native';
import BleManager from '../ble/BleManager';
import { parseStatus } from '../ble/protocol';
import type { RootStackParamList } from '../../App';
import type { StackScreenProps } from '@react-navigation/stack';

type Props = StackScreenProps<RootStackParamList, 'Connect'>;

export default function ConnectScreen({ navigation }: Props) {
  const [scanning, setScanning] = useState(false);
  const [connected, setConnected] = useState(false);
  const [status, setStatus] = useState<string | null>(null);
  const [battery, setBattery] = useState<number | null>(null);
  const [firmware, setFirmware] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);

  const handleScan = async () => {
    setScanning(true);
    setError(null);
    try {
      const device = await BleManager.scanAndConnect(10000);
      if (device) {
        setConnected(true);
        const ok = await BleManager.ping();
        if (ok) {
          const raw = await BleManager.getStatus();
          if (raw) {
            const s = parseStatus(raw);
            setBattery(s.batteryPct);
            setFirmware(s.firmwareVersion);
            setStatus(`State: ${s.state}, Ring: ${s.ringHead}`);
          }
        }
      } else {
        setError('No RebarScope device found.');
      }
    } catch (e: any) {
      setError(e?.message ?? 'Scan failed');
    }
    setScanning(false);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>RebarScope</Text>
      <Text style={styles.subtitle}>Concrete Corrosion NDT Scanner</Text>
      <Text style={styles.author}>Author: jayis1 — © 2026</Text>

      {!connected && (
        <Button
          title={scanning ? 'Scanning…' : 'Scan & Connect'}
          onPress={handleScan}
          disabled={scanning}
        />
      )}
      {scanning && <ActivityIndicator size="large" color="#3498db" />}

      {connected && (
        <View style={styles.statusBox}>
          <Text style={styles.statusText}>✅ Connected</Text>
          {battery !== null && <Text>Battery: {battery}%</Text>}
          {firmware && <Text>Firmware: v{firmware}</Text>}
          {status && <Text>{status}</Text>}
          <Button
            title="Set Up Survey →"
            onPress={() => navigation.navigate('SurveySetup', { siteName: 'New Site' })}
            color="#27ae60"
          />
        </View>
      )}

      {error && <Text style={styles.error}>{error}</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    padding: 24,
    backgroundColor: '#1a1a2e',
  },
  title: { fontSize: 32, fontWeight: 'bold', color: '#ecf0f1' },
  subtitle: { fontSize: 16, color: '#95a5a6', marginBottom: 8 },
  author: { fontSize: 12, color: '#7f8c8d', marginBottom: 32 },
  statusBox: { alignItems: 'center', gap: 6, marginTop: 24 },
  statusText: { fontSize: 18, fontWeight: 'bold', color: '#2ecc71' },
  error: { color: '#e74c3c', marginTop: 16 },
});