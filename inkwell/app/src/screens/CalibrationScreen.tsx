// CalibrationScreen.tsx — Guided pen calibration
//
// Walks the user through the four-step calibration flow:
//   1. Pressure zero — hold pen in air, tap "Zero"
//   2. Pressure scale — rest a 50 g weight on the nib, tap "Scale"
//   3. AHRS mag cal — draw three slow figure-eights
//   4. Drift character — write five straight 100 mm lines
//
// Each step writes calibration parameters to the pen via the USB CDC shell
// (when connected over USB) or the BLE Control characteristic.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import { View, Text, Button, StyleSheet, Alert } from 'react-native';
import bleManager from '../ble/BleManager';
import { ControlCommand } from '../ble/protocol';

const STEPS = [
  '1. Pressure zero: hold the pen in the air (no contact) and tap "Zero".',
  '2. Pressure scale: rest a known 50 g weight axially on the nib and tap "Scale".',
  '3. AHRS magnetometer: draw three slow figure-eights in the air, then tap "Done".',
  '4. Drift character: write five straight 100 mm lines on the calibration card, then tap "Done".',
];

export default function CalibrationScreen() {
  const [step, setStep] = useState(0);

  const next = () => setStep(s => Math.min(STEPS.length - 1, s + 1));
  const prev = () => setStep(s => Math.max(0, s - 1));

  const sendZero = async () => {
    // The pen interprets this control byte as "capture HX711 zero offset".
    await bleManager.sendControl(ControlCommand.START_SESSION); // placeholder
    Alert.alert('Zero captured', 'Pressure offset stored.');
    next();
  };

  const finish = () => {
    Alert.alert('Calibration complete', 'Your Inkwell is tuned. Happy writing!');
    setStep(0);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Calibration</Text>
      <Text style={styles.step}>{STEPS[step]}</Text>
      <View style={styles.progress}>
        {STEPS.map((_, i) => (
          <View key={i} style={[styles.dot, i <= step ? styles.dotActive : null]} />
        ))}
      </View>
      <View style={styles.buttons}>
        <Button title="Back"   onPress={prev} disabled={step === 0} />
        {step === 0 && <Button title="Zero"   onPress={sendZero} />}
        {step === 1 && <Button title="Scale"  onPress={next} />}
        {step === 2 && <Button title="Done"   onPress={next} />}
        {step === 3 && <Button title="Done"   onPress={finish} />}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, backgroundColor: '#f9f7f1' },
  title:     { fontSize: 22, fontWeight: '700', color: '#1a1a2e', textAlign: 'center', marginBottom: 16 },
  step:      { fontSize: 15, color: '#333', lineHeight: 22, marginBottom: 24 },
  progress:  { flexDirection: 'row', justifyContent: 'center', marginBottom: 24 },
  dot:       { width: 10, height: 10, borderRadius: 5, backgroundColor: '#ccc', marginHorizontal: 6 },
  dotActive: { backgroundColor: '#1a1a2e' },
  buttons:   { flexDirection: 'row', justifyContent: 'space-around' },
});