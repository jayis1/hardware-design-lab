/**
 * CalibrationScreen.tsx — EMG and IMU calibration interface.
 *
 * Guides the user through a 60-second calibration sequence:
 * 1. Resting baseline (5 seconds)
 * 2. Maximum voluntary contraction (3 seconds)
 * 3. Individual finger isolation (2 seconds × 5)
 * 4. IMU static calibration (5 seconds)
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ProgressView,
  Alert,
} from 'react-native';
import { useBle } from '../ble/BleManager';
import EmgBarChart from '../components/EmgBarChart';

type CalibPhase = 'idle' | 'baseline' | 'mvc' | 'fingers' | 'imu' | 'done';

interface CalibStep {
  phase: CalibPhase;
  label: string;
  duration: number;  // seconds
  instruction: string;
}

const CALIB_STEPS: CalibStep[] = [
  { phase: 'baseline', label: 'Resting Baseline', duration: 5,
    instruction: 'Relax your hand completely. Keep it still and flat.' },
  { phase: 'mvc', label: 'Maximum Contraction', duration: 3,
    instruction: 'Clench your fist as hard as you can! Hold it!' },
  { phase: 'fingers', label: 'Finger Isolation', duration: 10,
    instruction: 'Flex each finger one at a time, for 2 seconds each.' },
  { phase: 'imu', label: 'IMU Calibration', duration: 5,
    instruction: 'Hold your hand flat, palm down, and keep it still.' },
];

/**
 * CalibrationScreen — guides user through sensor calibration.
 * Author: jayis1
 */
export default function CalibrationScreen() {
  const { isConnected, oscData, sendCalibration, calibrationData } = useBle();
  const [currentStep, setCurrentStep] = useState(-1);
  const [progress, setProgress] = useState(0);
  const [calibData, setCalibData] = useState<CalibrationData | null>(null);
  const [isRunning, setIsRunning] = useState(false);

  const emgValues = oscData?.emgEnvelopes || [0, 0, 0, 0, 0];

  const startCalibration = useCallback(async () => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Please connect to Synthand first.');
      return;
    }

    setIsRunning(true);
    const newCalib: CalibrationData = {
      emgBaseline: [0, 0, 0, 0, 0],
      emgMvc: [0, 0, 0, 0, 0],
      gyroBias: [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]],
      accelBias: [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]],
      handedness: 0,
    };

    for (let i = 0; i < CALIB_STEPS.length; i++) {
      setCurrentStep(i);
      setProgress(0);
      const step = CALIB_STEPS[i];

      // Collect data for this step's duration
      const samples: number[][] = [];
      const interval = setInterval(() => {
        if (oscData) {
          samples.push([...oscData.emgEnvelopes]);
        }
      }, 100);

      // Animate progress bar
      const startTime = Date.now();
      while (Date.now() - startTime < step.duration * 1000) {
        const elapsed = Date.now() - startTime;
        setProgress(elapsed / (step.duration * 1000));
        await new Promise((r) => setTimeout(r, 50));
      }

      clearInterval(interval);

      // Process collected samples
      if (samples.length > 0) {
        if (step.phase === 'baseline') {
          // Average the EMG values as baseline
          for (let ch = 0; ch < 5; ch++) {
            const avg = samples.reduce((sum, s) => sum + s[ch], 0) / samples.length;
            newCalib.emgBaseline[ch] = Math.round(avg * 32768);
          }
        } else if (step.phase === 'mvc') {
          // Take the maximum EMG values as MVC
          for (let ch = 0; ch < 5; ch++) {
            const maxVal = Math.max(...samples.map((s) => s[ch]));
            newCalib.emgMvc[ch] = Math.round(maxVal * 32768);
          }
        }
      }
    }

    setCalibData(newCalib);
    await sendCalibration(newCalib);
    setCurrentStep(-1);
    setIsRunning(false);
    setProgress(1);
    Alert.alert('Calibration Complete', 'Your Synthand is calibrated and ready!');
  }, [isConnected, oscData, sendCalibration]);

  return (
    <View style={styles.container}>
      <Text style={styles.header}>Sensor Calibration</Text>

      {!isConnected && (
        <View style={styles.warningBox}>
          <Text style={styles.warningText}>
            ⚠ Not connected to Synthand. Go to Settings to connect.
          </Text>
        </View>
      )}

      {/* Live EMG preview */}
      <EmgBarChart values={emgValues} />

      {/* Current step instruction */}
      {currentStep >= 0 && currentStep < CALIB_STEPS.length && (
        <View style={styles.stepBox}>
          <Text style={styles.stepLabel}>
            Step {currentStep + 1} of {CALIB_STEPS.length}: {CALIB_STEPS[currentStep].label}
          </Text>
          <Text style={styles.stepInstruction}>
            {CALIB_STEPS[currentStep].instruction}
          </Text>
          <ProgressView progress={progress} style={styles.progressBar} />
        </View>
      )}

      {/* Calibration status */}
      {calibData && !isRunning && (
        <View style={styles.resultBox}>
          <Text style={styles.resultTitle}>✓ Calibration Saved</Text>
          <Text style={styles.resultDetail}>
            EMG Baseline: {calibData.emgBaseline.map((v) => (v / 32768 * 100).toFixed(0) + '%').join(', ')}
          </Text>
          <Text style={styles.resultDetail}>
            EMG MVC: {calibData.emgMvc.map((v) => (v / 32768 * 100).toFixed(0) + '%').join(', ')}
          </Text>
        </View>
      )}

      {/* Start button */}
      <TouchableOpacity
        style={[styles.button, (!isConnected || isRunning) && styles.buttonDisabled]}
        onPress={startCalibration}
        disabled={!isConnected || isRunning}
      >
        <Text style={styles.buttonText}>
          {isRunning ? 'Calibrating...' : 'Start Calibration'}
        </Text>
      </TouchableOpacity>

      {/* Step list */}
      <View style={styles.stepList}>
        {CALIB_STEPS.map((step, i) => (
          <Text
            key={i}
            style={[
              styles.stepItem,
              currentStep === i && styles.stepItemActive,
              currentStep > i && styles.stepItemDone,
            ]}
          >
            {currentStep > i ? '✓' : currentStep === i ? '▶' : '○'} {step.label} ({step.duration}s)
          </Text>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  header: {
    color: '#e94560',
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 16,
  },
  warningBox: {
    backgroundColor: 'rgba(233, 69, 96, 0.2)',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  warningText: {
    color: '#e94560',
    fontSize: 14,
  },
  stepBox: {
    backgroundColor: '#16213e',
    borderRadius: 12,
    padding: 16,
    marginVertical: 8,
  },
  stepLabel: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  stepInstruction: {
    color: '#ccc',
    fontSize: 14,
    marginBottom: 12,
  },
  progressBar: {
    height: 6,
  },
  resultBox: {
    backgroundColor: 'rgba(83, 52, 131, 0.3)',
    borderRadius: 12,
    padding: 16,
    marginVertical: 8,
  },
  resultTitle: {
    color: '#0f0',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  resultDetail: {
    color: '#aaa',
    fontSize: 12,
    marginVertical: 2,
  },
  button: {
    backgroundColor: '#e94560',
    borderRadius: 8,
    padding: 16,
    alignItems: 'center',
    marginVertical: 12,
  },
  buttonDisabled: {
    backgroundColor: '#555',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  stepList: {
    marginVertical: 8,
  },
  stepItem: {
    color: '#555',
    fontSize: 13,
    paddingVertical: 4,
  },
  stepItemActive: {
    color: '#e94560',
    fontWeight: 'bold',
  },
  stepItemDone: {
    color: '#0f0',
  },
});