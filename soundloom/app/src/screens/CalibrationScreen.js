/**
 * CalibrationScreen.js — Guided pod calibration flow
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, TextInput, ScrollView, Alert, ActivityIndicator } from 'react-native';
import BluetoothService from '../services/BluetoothService';

export default function CalibrationScreen({ navigation }) {
  const [step, setStep] = useState(0);
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connectedDevice, setConnectedDevice] = useState(null);
  const [podName, setPodName] = useState('');
  const [burialDepth, setBurialDepth] = useState('60');
  const [soilType, setSoilType] = useState('loam');
  const [tapTestRunning, setTapTestRunning] = useState(false);
  const [tapTestResult, setTapTestResult] = useState(null);
  const [baselineProgress, setBaselineProgress] = useState(0);

  const steps = [
    'Scan & Connect',
    'Pod Identification',
    'Tap Test (Acoustic Calibration)',
    'Noise Baseline (24h)',
    'Complete'
  ];

  const handleScan = async () => {
    setScanning(true);
    try {
      const found = await BluetoothService.scanForPods(10000);
      setDevices(found);
    } catch (e) {
      Alert.alert('Error', 'Bluetooth scan failed: ' + e.message);
    }
    setScanning(false);
  };

  const handleConnect = async (deviceId) => {
    try {
      await BluetoothService.connect(deviceId);
      setConnectedDevice(deviceId);
      setStep(1);
    } catch (e) {
      Alert.alert('Error', 'Connection failed: ' + e.message);
    }
  };

  const handleTapTest = async () => {
    setTapTestRunning(true);
    try {
      const result = await BluetoothService.startTapTest();
      setTapTestResult(result);
      setStep(2);
    } catch (e) {
      Alert.alert('Error', 'Tap test failed: ' + e.message);
    }
    setTapTestRunning(false);
  };

  const handleBaselineStart = () => {
    setBaselineProgress(0);
    // Simulate 24h baseline countdown (demo: 10 seconds)
    const interval = setInterval(() => {
      setBaselineProgress(p => {
        if (p >= 100) {
          clearInterval(interval);
          setStep(4);
          return 100;
        }
        return p + 10;
      });
    }, 1000);
  };

  return (
    <ScrollView style={styles.container}>
      {/* Progress indicator */}
      <View style={styles.progressContainer}>
        {steps.map((s, i) => (
          <View key={s} style={[styles.progressDot, i <= step && styles.progressDotActive]}>
            <Text style={styles.progressDotText}>{i + 1}</Text>
          </View>
        ))}
      </View>
      <Text style={styles.currentStep}>{steps[step]}</Text>

      {/* Step 0: Scan & Connect */}
      {step === 0 && (
        <View style={styles.stepContainer}>
          <Text style={styles.stepTitle}>Step 1: Find your SoundLoom pod</Text>
          <Text style={styles.stepDesc}>Ensure the pod is powered on and within 2 metres. Tap "Scan" to discover nearby pods via Bluetooth.</Text>
          <TouchableOpacity style={styles.button} onPress={handleScan} disabled={scanning}>
            {scanning ? <ActivityIndicator color="#FFFFFF" /> : <Text style={styles.buttonText}>Scan for Pods</Text>}
          </TouchableOpacity>
          {devices.map(dev => (
            <TouchableOpacity key={dev.id} style={styles.deviceItem} onPress={() => handleConnect(dev.id)}>
              <Text style={styles.deviceName}>{dev.name}</Text>
              <Text style={styles.deviceRssi}>RSSI: {dev.rssi} dBm</Text>
            </TouchableOpacity>
          ))}
        </View>
      )}

      {/* Step 1: Pod Identification */}
      {step === 1 && (
        <View style={styles.stepContainer}>
          <Text style={styles.stepTitle}>Step 2: Identify this pod</Text>
          <Text style={styles.stepDesc}>Give the pod a descriptive name and enter installation details.</Text>
          <TextInput
            style={styles.input}
            placeholder="Pod name (e.g. 'North Field A')"
            value={podName}
            onChangeText={setPodName}
          />
          <Text style={styles.inputLabel}>Burial depth (cm)</Text>
          <TextInput
            style={styles.input}
            placeholder="60"
            value={burialDepth}
            onChangeText={setBurialDepth}
            keyboardType="numeric"
          />
          <Text style={styles.inputLabel}>Soil type</Text>
          <View style={styles.soilTypeRow}>
            {['sand', 'loam', 'clay', 'silt'].map(st => (
              <TouchableOpacity
                key={st}
                style={[styles.soilTypeButton, soilType === st && styles.soilTypeButtonActive]}
                onPress={() => setSoilType(st)}
              >
                <Text style={[styles.soilTypeText, soilType === st && styles.soilTypeTextActive]}>{st}</Text>
              </TouchableOpacity>
            ))}
          </View>
          <TouchableOpacity style={styles.button} onPress={() => setStep(2)}>
            <Text style={styles.buttonText}>Continue</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* Step 2: Tap Test */}
      {step === 2 && (
        <View style={styles.stepContainer}>
          <Text style={styles.stepTitle}>Step 3: Acoustic calibration (tap test)</Text>
          <Text style={styles.stepDesc}>
            Strike the ground firmly at 1 metre from the pod, at four positions (N, E, S, W).
            This calibrates the soil-specific acoustic velocity and improves source localisation accuracy.
          </Text>
          <TouchableOpacity style={styles.button} onPress={handleTapTest} disabled={tapTestRunning}>
            {tapTestRunning ? <ActivityIndicator color="#FFFFFF" /> : <Text style={styles.buttonText}>Start Tap Test</Text>}
          </TouchableOpacity>
          {tapTestResult && (
            <View style={styles.resultBox}>
              <Text style={styles.resultTitle}>Calibration Result</Text>
              <Text style={styles.resultText}>Soil velocity: {tapTestResult.velocity || '300'} m/s</Text>
              <Text style={styles.resultText}>Localisation accuracy: ±{tapTestResult.accuracy || '12'} cm</Text>
            </View>
          )}
        </View>
      )}

      {/* Step 3: Noise Baseline */}
      {step === 3 && (
        <View style={styles.stepContainer}>
          <Text style={styles.stepTitle}>Step 4: Noise baseline</Text>
          <Text style={styles.stepDesc}>
            The pod will now run for 24 hours to establish a noise baseline and initialise the classifier's adaptive thresholds.
            During this period, avoid disturbing the area.
          </Text>
          <TouchableOpacity style={styles.button} onPress={handleBaselineStart}>
            <Text style={styles.buttonText}>Start Baseline</Text>
          </TouchableOpacity>
          {baselineProgress > 0 && (
            <View style={styles.progressBox}>
              <Text style={styles.progressText}>{baselineProgress}% complete</Text>
              <View style={styles.progressBar}>
                <View style={[styles.progressBarFill, { width: `${baselineProgress}%` }]} />
              </View>
            </View>
          )}
        </View>
      )}

      {/* Step 4: Complete */}
      {step === 4 && (
        <View style={styles.stepContainer}>
          <Text style={styles.stepTitle}>✓ Calibration Complete!</Text>
          <Text style={styles.stepDesc}>
            Pod "{podName || 'SoundLoom Pod'}" is now calibrated and running.
            SVI updates will appear in the Dashboard within 30 minutes.
          </Text>
          <View style={styles.summaryBox}>
            <Text style={styles.summaryTitle}>Installation Summary</Text>
            <Text style={styles.summaryRow}>Name: {podName || 'Unnamed'}</Text>
            <Text style={styles.summaryRow}>Depth: {burialDepth} cm</Text>
            <Text style={styles.summaryRow}>Soil: {soilType}</Text>
            <Text style={styles.summaryRow}>Velocity: {tapTestResult?.velocity || '300'} m/s</Text>
          </View>
          <TouchableOpacity style={styles.button} onPress={() => navigation.navigate('Main')}>
            <Text style={styles.buttonText}>Done — Go to Dashboard</Text>
          </TouchableOpacity>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  progressContainer: { flexDirection: 'row', justifyContent: 'center', padding: 20, backgroundColor: '#FFFFFF' },
  progressDot: { width: 36, height: 36, borderRadius: 18, backgroundColor: '#E0E0E0', margin: 5, alignItems: 'center', justifyContent: 'center' },
  progressDotActive: { backgroundColor: '#2E7D32' },
  progressDotText: { color: '#FFFFFF', fontWeight: 'bold' },
  currentStep: { textAlign: 'center', fontSize: 14, fontWeight: 'bold', color: '#424242', padding: 5 },
  stepContainer: { padding: 20, backgroundColor: '#FFFFFF', margin: 10, borderRadius: 12 },
  stepTitle: { fontSize: 18, fontWeight: 'bold', color: '#2E7D32', marginBottom: 8 },
  stepDesc: { fontSize: 14, color: '#616161', marginBottom: 15, lineHeight: 20 },
  button: { backgroundColor: '#2E7D32', padding: 14, borderRadius: 8, alignItems: 'center', marginTop: 10 },
  buttonText: { color: '#FFFFFF', fontSize: 16, fontWeight: 'bold' },
  deviceItem: { padding: 15, backgroundColor: '#F5F5F5', borderRadius: 8, marginTop: 8 },
  deviceName: { fontSize: 14, fontWeight: 'bold', color: '#424242' },
  deviceRssi: { fontSize: 12, color: '#757575', marginTop: 3 },
  input: { borderWidth: 1, borderColor: '#E0E0E0', borderRadius: 8, padding: 12, fontSize: 14, marginBottom: 10 },
  inputLabel: { fontSize: 13, color: '#757575', marginBottom: 5 },
  soilTypeRow: { flexDirection: 'row', marginBottom: 15 },
  soilTypeButton: { flex: 1, padding: 10, margin: 3, borderRadius: 8, backgroundColor: '#F5F5F5', alignItems: 'center' },
  soilTypeButtonActive: { backgroundColor: '#2E7D32' },
  soilTypeText: { fontSize: 12, color: '#616161' },
  soilTypeTextActive: { color: '#FFFFFF', fontWeight: 'bold' },
  resultBox: { padding: 15, backgroundColor: '#E8F5E9', borderRadius: 8, marginTop: 10 },
  resultTitle: { fontSize: 14, fontWeight: 'bold', color: '#2E7D32', marginBottom: 5 },
  resultText: { fontSize: 13, color: '#424242' },
  progressBox: { marginTop: 15 },
  progressText: { fontSize: 12, color: '#757575', marginBottom: 5 },
  progressBar: { height: 8, backgroundColor: '#E0E0E0', borderRadius: 4 },
  progressBarFill: { height: 8, backgroundColor: '#2E7D32', borderRadius: 4 },
  summaryBox: { padding: 15, backgroundColor: '#F5F5F5', borderRadius: 8, marginVertical: 10 },
  summaryTitle: { fontSize: 14, fontWeight: 'bold', color: '#424242', marginBottom: 8 },
  summaryRow: { fontSize: 13, color: '#616161', marginBottom: 3 },
});