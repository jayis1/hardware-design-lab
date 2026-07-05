// CalibrationScreen.tsx — open-sky flux calibration + orientation leveling
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect } from 'react';
import { View, Text, Button, StyleSheet, Alert } from 'react-native';
import DeviceService from '../services/DeviceService';

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0a0e27' },
  title: { color: '#e0e6ff', fontSize: 22, fontWeight: '700', marginBottom: 16 },
  section: { color: '#7c9eff', fontSize: 16, fontWeight: '600', marginTop: 12 },
  value: { color: '#e0e6ff', fontSize: 14, marginBottom: 4 },
  bubble: { width: 120, height: 120, borderRadius: 60, borderWidth: 2, borderColor: '#7c9eff',
           alignItems: 'center', justifyContent: 'center', marginVertical: 12 },
  bubbleDot: { width: 12, height: 12, borderRadius: 6, backgroundColor: '#e0e6ff' },
  steps: { color: '#5a6390', fontSize: 12, marginTop: 12, lineHeight: 18 },
});

export default function CalibrationScreen({ navigation }: any) {
  const [status, setStatus] = useState<any>(null);
  const [calibrating, setCalibrating] = useState(false);

  useEffect(() => {
    const unsub = DeviceService.onStatus((s) => setStatus(s));
    return () => unsub();
  }, []);

  const handleCalibrate = async () => {
    setCalibrating(true);
    try {
      const r = await DeviceService.startCalibration();
      Alert.alert('Calibration Started', 'Sweeping 192 SiPM channels. Takes ~30 s.');
    } catch (e: any) {
      Alert.alert('Error', e.message);
    }
    setCalibrating(false);
  };

  const roll = status ? status.roll : 0;
  const pitch = status ? status.pitch : 0;
  const level = Math.abs(roll) < 1.0 && Math.abs(pitch) < 1.0;

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Calibration</Text>

      <Text style={styles.section}>Orientation (electronic bubble level)</Text>
      <View style={styles.bubble}>
        <View style={[styles.bubbleDot, {
          transform: [{ translateX: roll * 5 }, { translateY: pitch * 5 }]
        }]} />
      </View>
      <Text style={styles.value}>Roll: {roll.toFixed(2)}°  Pitch: {pitch.toFixed(2)}°</Text>
      <Text style={styles.value}>{level ? '✓ Level' : '✗ Tilt — adjust tripod'}</Text>

      <Text style={styles.section}>SiPM Threshold Autocalibration</Text>
      <Button title={calibrating ? 'Calibrating...' : 'Start Autocalibration'}
              onPress={handleCalibrate} disabled={calibrating || !level} />
      <Text style={styles.steps}>
        Steps:{'\n'}
        1. Level the device using the bubble above.{'\n'}
        2. Start autocalibration — sweeps all 192 channels to find noise floor.{'\n'}
        3. Wait ~30 s for completion.{'\n'}
        4. Point at open sky for flux calibration (12 h recommended).{'\n'}
        5. Begin acquisition.
      </Text>

      {status && (
        <View>
          <Text style={styles.section}>Current State</Text>
          <Text style={styles.value}>SiPM Bias: {status.sipm_mv} mV</Text>
          <Text style={styles.value}>Temperature: {status.temp_c.toFixed(1)} °C</Text>
          <Text style={styles.value}>GPS: {status.gps_fix ? `FIX (${status.sats} sats)` : 'No fix'}</Text>
        </View>
      )}

      <Button title="Proceed to Acquisition" onPress={() => navigation.navigate('Acquisition')} />
    </View>
  );
}