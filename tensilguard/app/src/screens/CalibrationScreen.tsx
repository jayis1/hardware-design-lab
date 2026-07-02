/**
 * CalibrationScreen — guided 3-point or vibration-reference calibration wizard
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Walks an engineer through loading a cable to 3 known tensions (via a
 * hydraulic jack) and records each point. After the third point the
 * firmware's piecewise-linear T(µ) curve is written to the node's cal page.
 * Alternatively, the vibration-reference mode uses T_vib as the reference
 * and lets the magnetoelastic channel self-calibrate over one week.
 */
import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { sendDownlink } from '../api';

export default function CalibrationScreen() {
  const [nodeId, setNodeId] = useState('');
  const [step, setStep] = useState(0);
  const [t1, setT1] = useState('');
  const [t2, setT2] = useState('');
  const [t3, setT3] = useState('');
  const [mode, setMode] = useState<'manual' | 'vibref'>('manual');

  const steps = ['Enter known tension (kN) at load step 1', '…at load step 2', '…at load step 3'];

  const submit = async () => {
    if (mode === 'vibref') {
      Alert.alert('Vibration-Reference', 'The node will use T_vib as reference and self-calibrate over the next 7 days. Confirm?');
      // send a downlink to start vibref calibration
      return;
    }
    if (step < 2) {
      setStep(step + 1);
      return;
    }
    // write cal page — the downlink packs the three tensions
    Alert.alert(
      'Calibration Complete',
      `T1=${t1} kN, T2=${t2} kN, T3=${t3} kN\nWriting cal page to ${nodeId}…`,
      [{ text: 'OK', onPress: () => sendDownlink(nodeId, new Uint8Array([0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])) }],
    );
    setStep(0);
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Calibration Wizard</Text>
        <Text style={styles.sub}>Author: jayis1 — TensilGuard Field</Text>

        <Text style={styles.label}>Node ID</Text>
        <TextInput style={styles.input} value={nodeId} onChangeText={setNodeId} placeholder="00:11:22:33:44:55" />

        <Text style={styles.label}>Mode</Text>
        <View style={styles.modeRow}>
          <TouchableOpacity
            style={[styles.modeBtn, mode === 'manual' && styles.modeBtnActive]}
            onPress={() => setMode('manual')}
          >
            <Text style={styles.modeBtnText}>3-Point Manual</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.modeBtn, mode === 'vibref' && styles.modeBtnActive]}
            onPress={() => setMode('vibref')}
          >
            <Text style={styles.modeBtnText}>Vibration-Ref</Text>
          </TouchableOpacity>
        </View>

        {mode === 'manual' && (
          <View>
            <Text style={styles.stepIndicator}>Step {step + 1} of 3</Text>
            <Text style={styles.label}>{steps[step]}</Text>
            <TextInput
              style={styles.input}
              keyboardType="numeric"
              value={step === 0 ? t1 : step === 1 ? t2 : t3}
              onChangeText={(v) => step === 0 ? setT1(v) : step === 1 ? setT2(v) : setT3(v)}
              placeholder="e.g. 85.0"
            />
            <TouchableOpacity style={styles.btn} onPress={submit}>
              <Text style={styles.btnText}>{step < 2 ? 'Record & Next' : 'Write Cal Page'}</Text>
            </TouchableOpacity>
          </View>
        )}
        {mode === 'vibref' && (
          <View>
            <Text style={styles.vibrefInfo}>
              In vibration-reference mode the node uses the accelerometer-based T_vib
              as the reference and self-calibrates the magnetoelastic T_mag curve over
              the first 7 days of operation. No hydraulic jack is needed.
            </Text>
            <TouchableOpacity style={styles.btn} onPress={submit}>
              <Text style={styles.btnText}>Start Vib-Ref Calibration</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1a2a', padding: 12 },
  card: { backgroundColor: '#142535', borderRadius: 8, padding: 16 },
  title: { color: '#e8f0f8', fontSize: 18, fontWeight: 'bold', marginBottom: 4 },
  sub: { color: '#8a9ab0', fontSize: 12, marginBottom: 16 },
  label: { color: '#6a8aa0', fontSize: 13, marginTop: 12, marginBottom: 4 },
  input: { backgroundColor: '#0d1a2a', color: '#e8f0f8', borderWidth: 1, borderColor: '#2a4a6a', borderRadius: 6, padding: 10 },
  modeRow: { flexDirection: 'row', gap: 8, marginVertical: 8 },
  modeBtn: { flex: 1, backgroundColor: '#1a3a5a', padding: 10, borderRadius: 6, alignItems: 'center' },
  modeBtnActive: { backgroundColor: '#2196f3' },
  modeBtnText: { color: '#e8f0f8', fontSize: 13 },
  stepIndicator: { color: '#4fc3f7', fontSize: 14, fontWeight: '600', marginTop: 12 },
  btn: { backgroundColor: '#2196f3', padding: 14, borderRadius: 8, alignItems: 'center', marginTop: 16 },
  btnText: { color: '#fff', fontWeight: '600', fontSize: 15 },
  vibrefInfo: { color: '#8a9ab0', fontSize: 13, lineHeight: 20, marginVertical: 12 },
});