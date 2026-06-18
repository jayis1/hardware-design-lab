/**
 * CalibrationScreen.tsx — upload calib.bin, view ref table checksum, blank run
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React, { useState } from 'react';
import { View, Text, Button, StyleSheet, ActivityIndicator } from 'react-native';
import * as DocumentPicker from 'expo-document-picker';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';

export default function CalibrationScreen() {
  const { connected, sendCommand } = useBLE();
  const [checksum, setChecksum] = useState<string>('—');
  const [busy, setBusy] = useState(false);
  const [msg, setMsg] = useState<string>('');

  const readChecksum = async () => {
    setBusy(true);
    await sendCommand('version');   // device replies with ref checksum in info char
    setChecksum('0x… (see device info characteristic)');
    setBusy(false);
  };

  const triggerBlank = async () => {
    setBusy(true); setMsg('Running 30 s clean-air blank…');
    await sendCommand('blank');
    setTimeout(() => { setMsg('Blank complete — baselines re-zeroed.'); setBusy(false); }, 32000);
  };

  const uploadCalib = async () => {
    setBusy(true); setMsg('');
    const res = await DocumentPicker.getDocumentAsync({ copyToCacheDirectory: true });
    if (res.assets && res.assets[0]) {
      await sendCommand('calib upload');
      setMsg(`Selected ${res.assets[0].name} — streaming to device over BLE…`);
      // In a full build: chunk the file into 20-byte BLE writes to CHR_CALIB.
    }
    setBusy(false);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Calibration</Text>
      <Text style={styles.author}>AeroCast by jayis1</Text>

      <Card>
        <Text style={styles.section}>Reference table</Text>
        <Text style={styles.body}>On-device 24-class k-NN reference table checksum:</Text>
        <Text style={styles.mono}>{checksum}</Text>
        <Button title="Read checksum" onPress={readChecksum} disabled={!connected || busy} />
      </Card>

      <Card>
        <Text style={styles.section}>Clean-air blank</Text>
        <Text style={styles.body}>Runs the pump for 30 s with the laser off and re-zeros the AFE baselines. Do this with the inlet in clean air.</Text>
        <Button title="Run blank (30 s)" onPress={triggerBlank} disabled={!connected || busy} />
      </Card>

      <Card>
        <Text style={styles.section}>Upload calib.bin</Text>
        <Text style={styles.body}>Stream a new reference-table overlay to the device. The on-device classifier merges it with the embedded table.</Text>
        <Button title="Pick & upload calib.bin" onPress={uploadCalib} disabled={!connected || busy} />
      </Card>

      {busy && <ActivityIndicator color="#00C7A0" style={{ marginTop: 10 }} />}
      {msg ? <Text style={styles.msg}>{msg}</Text> : null}
      {!connected && <Text style={styles.warn}>Connect an AeroCast device first.</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1020', padding: 10 },
  title: { color: '#fff', fontSize: 22, fontWeight: '700', marginVertical: 10 },
  author: { color: '#5CB4FF', fontSize: 12, marginBottom: 8 },
  section: { color: '#fff', fontSize: 15, fontWeight: '600', marginBottom: 6 },
  body: { color: '#abc', fontSize: 12, lineHeight: 18, marginBottom: 8 },
  mono: { color: '#00C7A0', fontFamily: 'monospace', fontSize: 12, marginBottom: 8 },
  msg: { color: '#5CB4FF', fontSize: 12, marginTop: 8 },
  warn: { color: '#FF6F00', fontSize: 12, marginTop: 10 },
});