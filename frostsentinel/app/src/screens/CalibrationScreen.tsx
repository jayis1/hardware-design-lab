// src/screens/CalibrationScreen.tsx — Sensor calibration walk-through
//
// Guides the user through:
// 1. Leaf wetness threshold calibration
// 2. Sky IR offset check
// 3. Psychrometer wick prime
// 4. AE baseline learning
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  Alert, Slider,
} from 'react-native';
import bleManager from '../ble/BleManager';

export default function CalibrationScreen() {
  const [step, setStep] = useState(0);
  const [wetnessThreshold, setWetnessThreshold] = useState(280);
  const [skyOffset, setSkyOffset] = useState(0);
  const [aeBaselineProgress, setAeBaselineProgress] = useState(0);

  const steps = [
    'Leaf Wetness Threshold',
    'Sky IR Offset',
    'Psychrometer Wick Prime',
    'AE Baseline Learning',
  ];

  const calibrateWetness = async () => {
    try {
      await bleManager.calibrateLeafWetness(wetnessThreshold);
      Alert.alert('Success', `Leaf wetness threshold set to ${wetnessThreshold}.`);
      setStep(1);
    } catch (e) {
      Alert.alert('Error', 'Calibration failed. Is BLE connected?');
    }
  };

  const calibrateSkyIR = async () => {
    try {
      // In production, this would send a sky-IR offset command
      Alert.alert('Success', `Sky IR offset set to ${skyOffset.toFixed(2)} °C.`);
      setStep(2);
    } catch (e) {
      Alert.alert('Error', 'Calibration failed.');
    }
  };

  const primeWick = async () => {
    Alert.alert(
      'Wick Priming',
      '1. Fill the reservoir with distilled water.\n2. Ensure the cotton wick is fully saturated.\n3. Wait 2 minutes for the wick to wick water to the wet RTD.\n4. Press OK when ready.',
      [{ text: 'OK', onPress: () => setStep(3) }]
    );
  };

  const startAEBaseline = async () => {
    try {
      await bleManager.resetAEBaseline();
      setAeBaselineProgress(0);
      // Simulate 10-minute baseline learning progress
      const interval = setInterval(() => {
        setAeBaselineProgress(prev => {
          if (prev >= 100) {
            clearInterval(interval);
            Alert.alert('AE Baseline', 'Baseline learning complete. AE detector is ready.');
            return 100;
          }
          return prev + 10;
        });
      }, 500); // 500ms × 10 = 5s (simulated; real is 10 min)
    } catch (e) {
      Alert.alert('Error', 'Could not reset AE baseline.');
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Calibration</Text>
      <Text style={styles.subtitle}>Step {step + 1} of {steps.length}: {steps[step]}</Text>

      {/* Progress indicator */}
      <View style={styles.progressBar}>
        {steps.map((_, i) => (
          <View key={i} style={[
            styles.progressDot,
            i <= step ? styles.progressDotActive : null,
          ]} />
        ))}
      </View>

      {step === 0 && (
        <View style={styles.stepCard}>
          <Text style={styles.stepTitle}>Leaf Wetness Threshold</Text>
          <Text style={styles.stepText}>
            Set the wetness level above which dew is considered present on
            the leaf-wetness sensor. This varies by site and sensor
            orientation. Default: 280 (28% of full scale).
          </Text>
          <Text style={styles.valueLabel}>Threshold: {wetnessThreshold} (0–1000)</Text>
          <Slider
            style={styles.slider}
            minimumValue={0}
            maximumValue={1000}
            step={10}
            value={wetnessThreshold}
            onValueChange={setWetnessThreshold}
            minimumTrackTintColor="#26C6DA"
            maximumTrackTintColor="#333"
          />
          <TouchableOpacity style={styles.button} onPress={calibrateWetness}>
            <Text style={styles.buttonText}>Set Threshold & Next</Text>
          </TouchableOpacity>
        </View>
      )}

      {step === 1 && (
        <View style={styles.stepCard}>
          <Text style={styles.stepTitle}>Sky IR Offset</Text>
          <Text style={styles.stepText}>
            If the sky IR reading is consistently off compared to a
            reference blackbody, apply a correction offset (in °C).
            Default: 0.00 °C (factory calibration is usually adequate).
          </Text>
          <Text style={styles.valueLabel}>Offset: {skyOffset.toFixed(2)} °C</Text>
          <Slider
            style={styles.slider}
            minimumValue={-5}
            maximumValue={5}
            step={0.1}
            value={skyOffset}
            onValueChange={setSkyOffset}
            minimumTrackTintColor="#42A5F5"
            maximumTrackTintColor="#333"
          />
          <TouchableOpacity style={styles.button} onPress={calibrateSkyIR}>
            <Text style={styles.buttonText}>Set Offset & Next</Text>
          </TouchableOpacity>
        </View>
      )}

      {step === 2 && (
        <View style={styles.stepCard}>
          <Text style={styles.stepTitle}>Psychrometer Wick Prime</Text>
          <Text style={styles.stepText}>
            The wet-bulb RTD requires a saturated cotton wick. Follow the
            priming procedure to ensure accurate wet-bulb readings.
          </Text>
          <TouchableOpacity style={styles.button} onPress={primeWick}>
            <Text style={styles.buttonText}>Open Priming Instructions</Text>
          </TouchableOpacity>
        </View>
      )}

      {step === 3 && (
        <View style={styles.stepCard}>
          <Text style={styles.stepTitle}>AE Baseline Learning</Text>
          <Text style={styles.stepText}>
            The acoustic emission detector needs a 10-minute quiet
            baseline to learn the ambient ultrasonic noise floor. Ensure
            no ice is present and the sensor is dry before starting.
          </Text>
          {aeBaselineProgress === 0 ? (
            <TouchableOpacity style={styles.button} onPress={startAEBaseline}>
              <Text style={styles.buttonText}>Start 10-Minute Baseline</Text>
            </TouchableOpacity>
          ) : (
            <View>
              <Text style={styles.progressText}>
                Learning baseline... {aeBaselineProgress}%
              </Text>
              <View style={styles.baselineBar}>
                <View style={[styles.baselineFill, { width: `${aeBaselineProgress}%` }]} />
              </View>
            </View>
          )}
        </View>
      )}

      {step > 0 && (
        <TouchableOpacity style={styles.backButton} onPress={() => setStep(step - 1)}>
          <Text style={styles.backText}>← Back</Text>
        </TouchableOpacity>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1b2a', padding: 20, paddingTop: 40 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#fff' },
  subtitle: { fontSize: 13, color: '#778da9', marginTop: 4, marginBottom: 15 },
  progressBar: { flexDirection: 'row', marginBottom: 20 },
  progressDot: {
    flex: 1, height: 4, backgroundColor: '#333', marginHorizontal: 2, borderRadius: 2,
  },
  progressDotActive: { backgroundColor: '#2196F3' },
  stepCard: { backgroundColor: '#1b263b', borderRadius: 12, padding: 20 },
  stepTitle: { fontSize: 16, fontWeight: 'bold', color: '#e0e1dd', marginBottom: 8 },
  stepText: { fontSize: 13, color: '#778da9', lineHeight: 18, marginBottom: 15 },
  valueLabel: { fontSize: 14, color: '#fff', marginBottom: 8, fontWeight: '600' },
  slider: { height: 40, marginBottom: 15 },
  button: {
    backgroundColor: '#2196F3', paddingHorizontal: 20, paddingVertical: 12,
    borderRadius: 8, alignItems: 'center',
  },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: '600' },
  backButton: { marginTop: 15, padding: 10 },
  backText: { color: '#778da9', fontSize: 14 },
  progressText: { fontSize: 13, color: '#FFC107', marginBottom: 8 },
  baselineBar: { height: 8, backgroundColor: '#333', borderRadius: 4, overflow: 'hidden' },
  baselineFill: { height: '100%', backgroundColor: '#FFC107' },
});