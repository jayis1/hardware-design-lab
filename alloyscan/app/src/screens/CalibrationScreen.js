/**
 * CalibrationScreen.js — Factory calibration assistant
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Alert, Modal,
} from 'react-native';
import { useBle } from '../ble/BleContext';
import Colors from '../constants/Colors';

const CALIBRATION_STEPS = [
  {
    title: 'Step 1: Open Circuit',
    instruction: 'Hold the probe in the air, away from any metal (at least 10 cm clearance). This records the baseline coil impedance.',
    action: 'Hold in air',
  },
  {
    title: 'Step 2: Reference Block (Contact)',
    instruction: 'Press the probe firmly against the provided 6061-T6 aluminum reference block. Ensure full contact with no gap.',
    action: 'Press on block',
  },
  {
    title: 'Step 3: Reference Block + 0.5mm Shim',
    instruction: 'Place the 0.5 mm shim on the reference block, then press the probe onto the shim. This records lift-off variation point 1.',
    action: 'Add 0.5mm shim',
  },
  {
    title: 'Step 4: Reference Block + 1.0mm Shim',
    instruction: 'Replace the shim with the 1.0 mm shim on the reference block, then press the probe onto it. This records lift-off variation point 2 and completes calibration.',
    action: 'Add 1.0mm shim',
  },
];

const CalibrationScreen = () => {
  const { connected, startCalibration } = useBle();
  const [currentStep, setCurrentStep] = useState(-1);
  const [calibrationComplete, setCalibrationComplete] = useState(false);
  const [modalVisible, setModalVisible] = useState(false);

  const beginCalibration = () => {
    Alert.alert(
      'Start Calibration?',
      'You will need the 6061 reference block and 0.5/1.0mm shims. The process takes about 2 minutes.',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Start', onPress: () => { setCurrentStep(0); setCalibrationComplete(false); } },
      ]
    );
  };

  const performStep = (stepIndex) => {
    // Send calibrate command to device
    startCalibration(stepIndex);

    // Show "measuring" modal
    setModalVisible(true);

    // Simulate measurement time
    setTimeout(() => {
      setModalVisible(false);
      if (stepIndex < CALIBRATION_STEPS.length - 1) {
        setCurrentStep(stepIndex + 1);
      } else {
        setCalibrationComplete(true);
        setCurrentStep(-1);
      }
    }, 3000);
  };

  if (!connected) {
    return (
      <View style={styles.container}>
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedTitle}>Not Connected</Text>
          <Text style={styles.disconnectedText}>
            Connect to your AlloyScan device to perform calibration.
          </Text>
        </View>
      </View>
    );
  }

  if (calibrationComplete) {
    return (
      <View style={styles.container}>
        <View style={styles.completeContainer}>
          <Text style={styles.completeIcon}>✓</Text>
          <Text style={styles.completeTitle}>Calibration Complete!</Text>
          <Text style={styles.completeText}>
            Your AlloyScan is now calibrated and ready for use.
            Recalibration is recommended every 6 months or 5,000 scans.
          </Text>
          <TouchableOpacity
            style={styles.doneButton}
            onPress={() => setCalibrationComplete(false)}
          >
            <Text style={styles.doneButtonText}>Done</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Factory Calibration</Text>
        <Text style={styles.headerSubtitle}>
          4-step process · ~2 minutes
        </Text>
      </View>

      {currentStep === -1 ? (
        // Pre-calibration overview
        <View style={styles.overview}>
          <Text style={styles.overviewTitle}>Calibration Overview</Text>
          <Text style={styles.overviewText}>
            Calibration ensures accurate alloy identification by recording
            the probe's electromagnetic baseline and lift-off characteristics.
          </Text>

          <View style={styles.stepsList}>
            {CALIBRATION_STEPS.map((step, i) => (
              <View key={i} style={styles.stepPreview}>
                <View style={styles.stepNumber}>
                  <Text style={styles.stepNumberText}>{i + 1}</Text>
                </View>
                <View style={styles.stepInfo}>
                  <Text style={styles.stepTitle}>{step.title}</Text>
                  <Text style={styles.stepInstructionPreview} numberOfLines={2}>
                    {step.instruction}
                  </Text>
                </View>
              </View>
            ))}
          </View>

          <View style={styles.requirementsBox}>
            <Text style={styles.requirementsTitle}>You will need:</Text>
            <Text style={styles.requirementItem}>• 6061-T6 aluminum reference block</Text>
            <Text style={styles.requirementItem}>• 0.5 mm precision shim</Text>
            <Text style={styles.requirementItem}>• 1.0 mm precision shim</Text>
            <Text style={styles.requirementItem}>• Clean, flat metal surface</Text>
          </View>

          <TouchableOpacity style={styles.startButton} onPress={beginCalibration}>
            <Text style={styles.startButtonText}>Start Calibration</Text>
          </TouchableOpacity>
        </View>
      ) : (
        // Active calibration step
        <View style={styles.activeStep}>
          <View style={styles.progressBar}>
            {CALIBRATION_STEPS.map((_, i) => (
              <View
                key={i}
                style={[
                  styles.progressSegment,
                  i <= currentStep && { backgroundColor: Colors.accent },
                ]}
              />
            ))}
          </View>

          <Text style={styles.stepLabel}>
            Step {currentStep + 1} of {CALIBRATION_STEPS.length}
          </Text>
          <Text style={styles.stepTitleActive}>
            {CALIBRATION_STEPS[currentStep].title}
          </Text>
          <Text style={styles.stepInstruction}>
            {CALIBRATION_STEPS[currentStep].instruction}
          </Text>

          <View style={styles.illustrationBox}>
            <Text style={styles.illustrationText}>
              {CALIBRATION_STEPS[currentStep].action}
            </Text>
          </View>

          <TouchableOpacity
            style={styles.confirmButton}
            onPress={() => performStep(currentStep)}
          >
            <Text style={styles.confirmButtonText}>Confirm & Measure</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={styles.cancelButton}
            onPress={() => setCurrentStep(-1)}
          >
            <Text style={styles.cancelButtonText}>Cancel</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* Measuring modal */}
      <Modal visible={modalVisible} transparent animationType="fade">
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Measuring...</Text>
            <Text style={styles.modalText}>
              Hold steady while the probe records the electromagnetic signature.
            </Text>
            <View style={styles.modalSpinner} />
          </View>
        </View>
      </Modal>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.dark },
  disconnected: { flex: 1, justifyContent: 'center', alignItems: 'center', padding: 40 },
  disconnectedTitle: { fontSize: 22, fontWeight: 'bold', color: Colors.white, marginBottom: 12 },
  disconnectedText: { fontSize: 14, color: Colors.gray, textAlign: 'center' },
  header: { padding: 16, borderBottomWidth: 1, borderBottomColor: Colors.darkGray },
  headerTitle: { color: Colors.white, fontSize: 20, fontWeight: 'bold' },
  headerSubtitle: { color: Colors.gray, fontSize: 13, marginTop: 4 },
  overview: { padding: 16 },
  overviewTitle: { color: Colors.white, fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  overviewText: { color: Colors.lightGray, fontSize: 14, lineHeight: 20, marginBottom: 20 },
  stepsList: { marginBottom: 20 },
  stepPreview: { flexDirection: 'row', marginBottom: 12 },
  stepNumber: {
    width: 28, height: 28, borderRadius: 14,
    backgroundColor: Colors.primaryLight,
    justifyContent: 'center', alignItems: 'center',
    marginRight: 12,
  },
  stepNumberText: { color: Colors.white, fontWeight: 'bold' },
  stepInfo: { flex: 1 },
  stepTitle: { color: Colors.white, fontSize: 15, fontWeight: 'bold', marginBottom: 2 },
  stepInstructionPreview: { color: Colors.gray, fontSize: 12 },
  requirementsBox: {
    backgroundColor: Colors.darkGray, borderRadius: 10, padding: 16, marginBottom: 20,
  },
  requirementsTitle: { color: Colors.accent, fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  requirementItem: { color: Colors.lightGray, fontSize: 13, paddingVertical: 2 },
  startButton: {
    backgroundColor: Colors.accent, borderRadius: 25, paddingVertical: 15, alignItems: 'center',
  },
  startButtonText: { color: Colors.white, fontSize: 16, fontWeight: 'bold' },
  activeStep: { padding: 16, flex: 1 },
  progressBar: { flexDirection: 'row', marginBottom: 20 },
  progressSegment: {
    flex: 1, height: 4, backgroundColor: Colors.darkGray, marginRight: 4, borderRadius: 2,
  },
  stepLabel: { color: Colors.accent, fontSize: 13, marginBottom: 4 },
  stepTitleActive: { color: Colors.white, fontSize: 22, fontWeight: 'bold', marginBottom: 12 },
  stepInstruction: { color: Colors.lightGray, fontSize: 15, lineHeight: 22, marginBottom: 20 },
  illustrationBox: {
    backgroundColor: Colors.darkGray, borderRadius: 12, padding: 30, alignItems: 'center',
    marginBottom: 20,
  },
  illustrationText: { color: Colors.cyan, fontSize: 18, fontWeight: 'bold' },
  confirmButton: {
    backgroundColor: Colors.green, borderRadius: 20, paddingVertical: 15, alignItems: 'center',
    marginBottom: 10,
  },
  confirmButtonText: { color: Colors.white, fontSize: 16, fontWeight: 'bold' },
  cancelButton: { alignItems: 'center', paddingVertical: 10 },
  cancelButtonText: { color: Colors.gray, fontSize: 14 },
  completeContainer: { flex: 1, justifyContent: 'center', alignItems: 'center', padding: 40 },
  completeIcon: { fontSize: 64, color: Colors.green, marginBottom: 16 },
  completeTitle: { fontSize: 24, fontWeight: 'bold', color: Colors.white, marginBottom: 12 },
  completeText: { color: Colors.gray, fontSize: 14, textAlign: 'center', lineHeight: 20, marginBottom: 30 },
  doneButton: {
    backgroundColor: Colors.accent, borderRadius: 25, paddingHorizontal: 40, paddingVertical: 15,
  },
  doneButtonText: { color: Colors.white, fontSize: 16, fontWeight: 'bold' },
  modalOverlay: {
    flex: 1, backgroundColor: 'rgba(0,0,0,0.8)', justifyContent: 'center', alignItems: 'center',
  },
  modalContent: {
    backgroundColor: Colors.darkGray, borderRadius: 16, padding: 30, width: '80%',
    alignItems: 'center',
  },
  modalTitle: { color: Colors.white, fontSize: 20, fontWeight: 'bold', marginBottom: 8 },
  modalText: { color: Colors.gray, fontSize: 14, textAlign: 'center', marginBottom: 20 },
  modalSpinner: {
    width: 40, height: 40, borderRadius: 20,
    borderColor: Colors.accent, borderWidth: 3, borderTopColor: 'transparent',
  },
});

export default CalibrationScreen;