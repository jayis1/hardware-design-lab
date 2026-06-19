// CalibrationScreen.js — Guided Calibration Wizard
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert } from 'react-native';
import { useBle } from '../utils/BleContext';
import { estimateEpsR, formatDepth, MATERIAL_EPS_R } from '../utils/gprMath';

export default function CalibrationScreen() {
  const ble = useBle();
  const [step, setStep] = useState(0);
  const [knownDepth, setKnownDepth] = useState(1.0);
  const [twoWayTime, setTwoWayTime] = useState(8.16);  // ns
  const [estimatedEpsR, setEstimatedEpsR] = useState(6.0);

  if (!ble.connected) {
    return (
      <View style={styles.container}>
        <Text style={styles.empty}>Connect to a StrataScan device to calibrate.</Text>
      </View>
    );
  }

  const steps = [
    { title: 'Step 1: Shorted-Load Calibration', desc: 'Remove antenna coupling and system noise floor. The device captures a reference sweep with TX disabled. This takes ~30 seconds.' },
    { title: 'Step 2: Known-Depth Target', desc: 'Place a metal plate at a known depth below the antenna. The device measures the two-way travel time to the reflection.' },
    { title: 'Step 3: Permittivity Estimation', desc: 'Enter the known depth and measured two-way time. The app calculates the relative permittivity (ε_r) of the medium.' },
    { title: 'Step 4: Apply Calibration', desc: 'Send the calibration results to the device. The depth axis will be corrected for the measured ε_r.' },
  ];

  const handleNext = () => {
    if (step === 0) {
      ble.recalibrate();
      Alert.alert('Calibration', 'Shorted-load calibration started. Wait ~30s for completion.');
      setStep(1);
    } else if (step === 1) {
      setStep(2);
    } else if (step === 2) {
      const epsR = estimateEpsR(twoWayTime, knownDepth);
      setEstimatedEpsR(epsR);
      setStep(3);
    } else if (step === 3) {
      ble.setEpsR(estimatedEpsR);
      Alert.alert('Calibration Complete', `ε_r = ${estimatedEpsR.toFixed(2)} applied to device.`);
      setStep(0);
    }
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Calibration Wizard</Text>
        <Text style={styles.subtitle}>Guided calibration for accurate depth measurement</Text>
      </View>

      {/* Progress indicator */}
      <View style={styles.progressRow}>
        {steps.map((s, i) => (
          <View key={i} style={[styles.progressDot, i <= step && styles.progressActive]} />
        ))}
      </View>

      <View style={styles.stepCard}>
        <Text style={styles.stepTitle}>{steps[step].title}</Text>
        <Text style={styles.stepDesc}>{steps[step].desc}</Text>
      </View>

      {step === 2 && (
        <View style={styles.inputCard}>
          <Text style={styles.inputLabel}>Known target depth (meters): {knownDepth.toFixed(2)} m</Text>
          <View style={styles.depthRow}>
            {[0.25, 0.5, 1.0, 1.5, 2.0, 3.0].map(d => (
              <TouchableOpacity
                key={d}
                style={[styles.depthBtn, knownDepth === d && styles.depthSelected]}
                onPress={() => setKnownDepth(d)}
              >
                <Text style={styles.depthBtnText}>{d}m</Text>
              </TouchableOpacity>
            ))}
          </View>

          <Text style={styles.inputLabel}>Two-way travel time: {twoWayTime.toFixed(2)} ns</Text>
          <View style={styles.depthRow}>
            {[5, 8, 10, 12, 15, 20, 25].map(t => (
              <TouchableOpacity
                key={t}
                style={[styles.depthBtn, twoWayTime === t && styles.depthSelected]}
                onPress={() => setTwoWayTime(t)}
              >
                <Text style={styles.depthBtnText}>{t}ns</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      )}

      {step === 3 && (
        <View style={styles.resultCard}>
          <Text style={styles.resultTitle}>Estimated Relative Permittivity</Text>
          <Text style={styles.resultValue}>ε_r = {estimatedEpsR.toFixed(2)}</Text>
          <Text style={styles.resultInfo}>
            This value will be used to convert radar two-way travel time to
            depth: d = c × t / (2 × √ε_r).
          </Text>
          <Text style={styles.materialCompare}>
            Closest material: {Object.entries(MATERIAL_EPS_R).reduce((closest, [name, val]) => {
              const diff = Math.abs(val - estimatedEpsR);
              if (!closest || diff < closest.diff) return { name, diff };
              return closest;
            }, null)?.name || 'Unknown'}
          </Text>
        </View>
      )}

      <View style={styles.buttonRow}>
        {step > 0 && (
          <TouchableOpacity style={[styles.button, styles.backButton]} onPress={() => setStep(step - 1)}>
            <Text style={styles.buttonText}>← Back</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={[styles.button, styles.nextButton]} onPress={handleNext}>
          <Text style={styles.buttonText}>
            {step === 3 ? 'Apply & Finish' : 'Next →'}
          </Text>
        </TouchableOpacity>
      </View>

      <View style={styles.statusCard}>
        <Text style={styles.statusTitle}>Device Calibration Status</Text>
        <Text style={styles.statusRow}>
          Calibrated: {ble.status?.calibrated ? '✓ Yes' : '✗ No'}
        </Text>
        <Text style={styles.statusRow}>
          Current band: {ble.status?.bandName || '—'}
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  header: { padding: 15, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  title: { color: '#0ff', fontSize: 20, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 12, marginTop: 2 },
  empty: { color: '#888', textAlign: 'center', marginTop: 100, fontSize: 16 },
  progressRow: { flexDirection: 'row', justifyContent: 'center', gap: 10, paddingVertical: 15 },
  progressDot: {
    width: 12, height: 12, borderRadius: 6, backgroundColor: '#333',
  },
  progressActive: { backgroundColor: '#0066CC' },
  stepCard: {
    backgroundColor: '#16213e', margin: 15, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333',
  },
  stepTitle: { color: '#0ff', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  stepDesc: { color: '#ccc', fontSize: 13, lineHeight: 20 },
  inputCard: {
    backgroundColor: '#16213e', margin: 15, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333',
  },
  inputLabel: { color: '#ccc', fontSize: 13, marginBottom: 8, marginTop: 10 },
  depthRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  depthBtn: {
    backgroundColor: '#0a0a1a', paddingHorizontal: 14, paddingVertical: 6,
    borderRadius: 6, borderWidth: 1, borderColor: '#333',
  },
  depthSelected: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  depthBtnText: { color: '#ccc', fontSize: 12 },
  resultCard: {
    backgroundColor: '#16213e', margin: 15, padding: 20, borderRadius: 8,
    borderWidth: 1, borderColor: '#0066CC', alignItems: 'center',
  },
  resultTitle: { color: '#0ff', fontSize: 16, marginBottom: 10 },
  resultValue: { color: '#fff', fontSize: 36, fontWeight: 'bold', marginVertical: 10 },
  resultInfo: { color: '#888', fontSize: 12, textAlign: 'center', lineHeight: 18 },
  materialCompare: { color: '#5a5', fontSize: 13, marginTop: 10 },
  buttonRow: { flexDirection: 'row', justifyContent: 'center', gap: 10, paddingHorizontal: 15 },
  button: {
    paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8,
    borderWidth: 1, borderColor: '#333', minWidth: 120, alignItems: 'center',
  },
  backButton: { backgroundColor: '#16213e' },
  nextButton: { backgroundColor: '#1a3050', borderColor: '#0066CC' },
  buttonText: { color: '#fff', fontSize: 15, fontWeight: 'bold' },
  statusCard: {
    backgroundColor: '#16213e', margin: 15, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333', marginBottom: 30,
  },
  statusTitle: { color: '#0ff', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  statusRow: { color: '#ccc', fontSize: 12, marginVertical: 2 },
});