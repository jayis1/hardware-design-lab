/*
 * src/screens/Commissioning.tsx — BLE commissioning + 4-point calibration wizard.
 *
 * Author: jayis1
 * License: MIT
 */
import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Alert } from 'react-native';
import { Card, IconButton, Button, TextInput, ProgressBar, ActivityIndicator } from 'react-native-paper';
import { Device } from 'react-native-ble-plx';
import {
  scanForSentinels, connectAndSubscribe, sendCalibrationCapture,
  sendThresholds, disconnect,
} from '../ble/CryoBle';
import { DewarReading, CalibrationPoint } from '../types';

type Step = 'scan' | 'connect' | 'meta' | 'cal' | 'thresholds' | 'done';

export default function Commissioning() {
  const [step, setStep] = useState<Step>('scan');
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState<Device[]>([]);
  const [connected, setConnected] = useState<Device | null>(null);
  const [live, setLive] = useState<DewarReading | null>(null);
  const [dewarSerial, setDewarSerial] = useState('');
  const [dewarLabel, setDewarLabel] = useState('');
  const [points, setPoints] = useState<CalibrationPoint[]>([]);
  const [levelWarn, setLevelWarn] = useState('35');
  const [levelCrit, setLevelCrit] = useState('20');
  const [levelRate, setLevelRate] = useState('2.0');
  const [tiltWarn, setTiltWarn] = useState('8');

  /* ---- Scan step ---- */
  const doScan = async () => {
    setScanning(true);
    const found = await scanForSentinels(8000);
    setDevices(found);
    setScanning(false);
  };
  useEffect(() => { doScan(); }, []);

  /* ---- Connect step ---- */
  const doConnect = async (dev: Device) => {
    try {
      const d = await connectAndSubscribe(dev.id, (r) => setLive(r));
      setConnected(d);
      setStep('meta');
    } catch (e: any) {
      Alert.alert('BLE connect failed', e?.message ?? String(e));
    }
  };

  /* ---- Calibration capture ---- */
  const capturePoint = async (idx: number) => {
    if (!connected) return;
    try {
      const p = await sendCalibrationCapture(connected, idx);
      setPoints((prev) => {
        const next = prev.filter((x) => x.index !== idx);
        next.push(p);
        return next.sort((a, b) => a.index - b.index);
      });
    } catch (e: any) {
      Alert.alert('Capture failed', e?.message ?? String(e));
    }
  };

  /* ---- Finish: send thresholds, disconnect ---- */
  const finish = async () => {
    if (!connected) return;
    try {
      await sendThresholds(connected,
        parseFloat(levelWarn), parseFloat(levelCrit),
        parseFloat(levelRate), parseFloat(tiltWarn));
      await disconnect(connected);
      setStep('done');
    } catch (e: any) {
      Alert.alert('Finish failed', e?.message ?? String(e));
    }
  };

  const stepIndex = ['scan', 'connect', 'meta', 'cal', 'thresholds', 'done'].indexOf(step);
  const stepLabels = ['Scan', 'Connect', 'Identify', 'Calibrate', 'Thresholds', 'Done'];

  return (
    <View style={s.wrap}>
      <Text style={s.h2}>Commissioning Wizard</Text>
      <ProgressBar progress={(stepIndex + 1) / 6} color="#0066CC" style={s.progress} />
      <Text style={s.stepLabel}>Step {stepIndex + 1} of 6 — {stepLabels[stepIndex]}</Text>

      {step === 'scan' && (
        <View>
          {scanning ? <ActivityIndicator style={s.center} /> : (
            <FlatList
              data={devices}
              keyExtractor={(d) => d.id}
              renderItem={({ item }) => (
                <TouchableOpacity onPress={() => { setStep('connect'); doConnect(item); }}>
                  <Card style={s.card}>
                    <Card.Title title={item.name ?? 'Cryo-Sentinel'}
                      subtitle={item.id}
                      left={(p) => <IconButton {...p} icon="access-point" />} />
                  </Card>
                </TouchableOpacity>
              )}
            />
          )}
          <Button mode="text" onPress={doScan} disabled={scanning}>Rescan</Button>
        </View>
      )}

      {step === 'connect' && (
        <View style={s.center}><ActivityIndicator /><Text style={s.muted}>Connecting…</Text></View>
      )}

      {step === 'meta' && (
        <View>
          <Text style={s.label}>Dewar serial number</Text>
          <TextInput value={dewarSerial} onChangeText={setDewarSerial}
            style={s.input} placeholder="e.g. TW-XC-3492" />
          <Text style={s.label}>Friendly label</Text>
          <TextInput value={dewarLabel} onChangeText={setDewarLabel}
            style={s.input} placeholder="e.g. IVF Lab A" />
          {live && (
            <Card style={s.card}>
              <Card.Title title="Live reading" subtitle={`Level raw: ${live.levelPct}%`} />
            </Card>
          )}
          <Button mode="contained" onPress={() => setStep('cal')}
            disabled={!dewarSerial}>Next: Calibrate</Button>
        </View>
      )}

      {step === 'cal' && (
        <View>
          <Text style={s.label}>Four-point calibration</Text>
          <Text style={s.help}>At each level, adjust the Dewar's LN2 to the indicated
            percentage and tap "Capture".</Text>
          {[0, 1, 2, 3].map((idx) => {
            const pct = [0, 25, 75, 100][idx];
            const p = points.find((x) => x.index === idx);
            return (
              <Card key={idx} style={s.card}>
                <Card.Title title={`Point ${idx + 1}: ${pct}%`}
                  subtitle={p ? `raw ${p.raw}` : 'not captured'}
                  right={(props) => (
                    <Button {...props} mode="contained" onPress={() => capturePoint(idx)}>
                      {p ? 'Recapture' : 'Capture'}
                    </Button>
                  )} />
              </Card>
            );
          })}
          <Button mode="contained" onPress={() => setStep('thresholds')}
            disabled={points.length < 4}>Next: Thresholds</Button>
        </View>
      )}

      {step === 'thresholds' && (
        <View>
          <Text style={s.label}>Alarm thresholds</Text>
          <TextInput label="Level WARN (%)" value={levelWarn} onChangeText={setLevelWarn}
            style={s.input} keyboardType="numeric" />
          <TextInput label="Level CRITICAL (%)" value={levelCrit} onChangeText={setLevelCrit}
            style={s.input} keyboardType="numeric" />
          <TextInput label="Level rate CRIT (%/h)" value={levelRate} onChangeText={setLevelRate}
            style={s.input} keyboardType="numeric" />
          <TextInput label="Tilt WARN (deg)" value={tiltWarn} onChangeText={setTiltWarn}
            style={s.input} keyboardType="numeric" />
          <Button mode="contained" onPress={finish}>Save & Finish</Button>
        </View>
      )}

      {step === 'done' && (
        <View style={s.center}>
          <IconButton icon="check-circle" size={64} color="#16A34A" />
          <Text style={s.h2}>Commissioned!</Text>
          <Text style={s.muted}>{dewarLabel} ({dewarSerial}) is now monitored.</Text>
        </View>
      )}
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0F172A', padding: 16 },
  center: { alignItems: 'center', justifyContent: 'center', flex: 1 },
  h2: { color: '#F1F5F9', fontSize: 22, fontWeight: 'bold', marginBottom: 12 },
  progress: { height: 6, marginBottom: 4 },
  stepLabel: { color: '#94A3B8', fontSize: 12, marginBottom: 16 },
  card: { backgroundColor: '#1E293B', marginBottom: 8, borderRadius: 8 },
  label: { color: '#F1F5F9', fontSize: 14, marginTop: 12, marginBottom: 4 },
  help: { color: '#94A3B8', fontSize: 12, marginBottom: 12 },
  input: { backgroundColor: '#1E293B', marginBottom: 8 },
  muted: { color: '#94A3B8', marginTop: 8 },
});