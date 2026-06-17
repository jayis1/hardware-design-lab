/**
 * CalibrationScreen.js — Guided 3-Axis Calibration
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Guides the user through a figure-8 motion to collect calibration
 * data. Shows real-time progress and a 3D scatter plot of raw readings.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Animated, Easing } from 'react-native';
import { useBle } from '../utils/BleContext';
import StatusBar from '../components/StatusBar';

export default function CalibrationScreen() {
  const ble = useBle();
  const [calibState, setCalibState] = useState('idle'); // idle, collecting, complete, failed
  const [progress, setProgress] = useState(0);
  const [sampleCount, setSampleCount] = useState(0);
  const [rawPoints, setRawPoints] = useState([]);
  const spinAnim = useRef(new Animated.Value(0)).start;
  const targetSamples = 2000;

  // Track progress based on received field data
  useEffect(() => {
    if (calibState === 'collecting' && ble.fieldData) {
      setSampleCount((prev) => {
        const next = prev + 1;
        setProgress(Math.min((next / targetSamples) * 100, 100));
        // Accumulate raw points for scatter plot
        setRawPoints((points) => {
          const updated = [...points, {
            x: ble.fieldData.bx,
            y: ble.fieldData.by,
            z: ble.fieldData.bz,
          }];
          if (updated.length > 500) updated.shift();
          return updated;
        });
        if (next >= targetSamples) {
          setCalibState('complete');
        }
        return next;
      });
    }
  }, [ble.fieldData, calibState]);

  const handleStartCalib = () => {
    setCalibState('collecting');
    setProgress(0);
    setSampleCount(0);
    setRawPoints([]);
    ble.startCalib();
    ble.startStream();
  };

  const handleFinish = () => {
    setCalibState('idle');
    setProgress(0);
    setSampleCount(0);
    setRawPoints([]);
    ble.stopStream();
    ble.getCalib();
  };

  // Spin animation for the figure-8 guide
  useEffect(() => {
    if (calibState === 'collecting') {
      Animated.loop(
        Animated.timing(spinAnim, {
          toValue: 1,
          duration: 3000,
          easing: Easing.linear,
          useNativeDriver: true,
        })
      ).start();
    }
  }, [calibState]);

  const spin = spinAnim.interpolate({
    inputRange: [0, 1],
    outputRange: ['0deg', '360deg'],
  });

  return (
    <ScrollView style={styles.container}>
      <StatusBar
        connectionState={ble.connectionState}
        status={ble.status}
        deviceInfo={ble.deviceInfo}
      />

      <View style={styles.header}>
        <Text style={styles.title}>3-Axis Calibration</Text>
        <Text style={styles.subtitle}>Ellipsoid Fitting</Text>
      </View>

      {calibState === 'idle' && (
        <View style={styles.instructions}>
          <Text style={styles.instructionTitle}>Before You Begin</Text>
          <Text style={styles.instructionText}>
            1. Move to an open area away from metal objects, vehicles, and buildings.
          </Text>
          <Text style={styles.instructionText}>
            2. Hold the FerroProbe device in front of you.
          </Text>
          <Text style={styles.instructionText}>
            3. When prompted, slowly rotate the device in a figure-8 motion,
            covering all orientations (front, back, left, right, up, down).
          </Text>
          <Text style={styles.instructionText}>
            4. The calibration takes approximately 30 seconds.
          </Text>
          <Text style={styles.instructionText}>
            5. Do not move near metal objects during calibration.
          </Text>

          <TouchableOpacity
            style={styles.startCalibButton}
            onPress={handleStartCalib}
            disabled={ble.connectionState !== 'connected'}
          >
            <Text style={styles.startCalibButtonText}>
              {ble.connectionState === 'connected'
                ? 'Start Calibration'
                : 'Connect First'}
            </Text>
          </TouchableOpacity>
        </View>
      )}

      {calibState === 'collecting' && (
        <View style={styles.collectingContainer}>
          {/* Figure-8 animation guide */}
          <View style={styles.figure8Container}>
            <Animated.View
              style={[
                styles.figure8Icon,
                { transform: [{ rotate: spin }] },
              ]}
            >
              <Text style={styles.figure8Text}>∞</Text>
            </Animated.View>
          </View>

          <Text style={styles.collectingText}>
            Rotate the device in a figure-8 motion
          </Text>

          {/* Progress bar */}
          <View style={styles.progressContainer}>
            <View style={styles.progressTrack}>
              <View style={[styles.progressFill, { width: `${progress}%` }]} />
            </View>
            <Text style={styles.progressText}>
              {sampleCount} / {targetSamples} samples ({progress.toFixed(0)}%)
            </Text>
          </View>

          {/* 3D scatter plot preview */}
          <View style={styles.scatterPlot}>
            <Text style={styles.scatterTitle}>Raw Readings (Ellipsoid)</Text>
            <View style={styles.scatterArea}>
              {rawPoints.slice(-100).map((p, i) => {
                const norm = (val, min, max) => {
                  const v = (val - min) / (max - min);
                  return Math.max(0, Math.min(1, v));
                };
                const left = norm(p.x, -80000, 80000) * 100;
                const top = norm(p.z, -80000, 80000) * 100;
                const color = p.y > 0 ? '#2196F3' : '#4CAF50';
                return (
                  <View
                    key={i}
                    style={[
                      styles.scatterDot,
                      { left: `${left}%`, top: `${top}%`, backgroundColor: color },
                    ]}
                  />
                );
              })}
            </View>
          </View>
        </View>
      )}

      {calibState === 'complete' && (
        <View style={styles.completeContainer}>
          <Text style={styles.completeIcon}>✓</Text>
          <Text style={styles.completeText}>Calibration Complete!</Text>

          {ble.calibData && (
            <View style={styles.calibResults}>
              <Text style={styles.resultsTitle}>Calibration Parameters</Text>
              <Text style={styles.resultLine}>
                Offset X: {ble.calibData.offset[0].toFixed(1)} nT
              </Text>
              <Text style={styles.resultLine}>
                Offset Y: {ble.calibData.offset[1].toFixed(1)} nT
              </Text>
              <Text style={styles.resultLine}>
                Offset Z: {ble.calibData.offset[2].toFixed(1)} nT
              </Text>
              <Text style={styles.resultLine}>
                Scale X: {ble.calibData.scale[0].toFixed(4)}
              </Text>
              <Text style={styles.resultLine}>
                Scale Y: {ble.calibData.scale[1].toFixed(4)}
              </Text>
              <Text style={styles.resultLine}>
                Scale Z: {ble.calibData.scale[2].toFixed(4)}
              </Text>
            </View>
          )}

          <TouchableOpacity
            style={styles.finishButton}
            onPress={handleFinish}
          >
            <Text style={styles.finishButtonText}>Done</Text>
          </TouchableOpacity>
        </View>
      )}

      {calibState === 'failed' && (
        <View style={styles.failedContainer}>
          <Text style={styles.failedIcon}>✗</Text>
          <Text style={styles.failedText}>Calibration Failed</Text>
          <Text style={styles.failedSubtext}>
            Not enough valid samples were collected. Please try again in an
            open area away from metal objects.
          </Text>
          <TouchableOpacity
            style={styles.retryButton}
            onPress={() => setCalibState('idle')}
          >
            <Text style={styles.retryButtonText}>Retry</Text>
          </TouchableOpacity>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 14, marginTop: 4 },
  instructions: {
    backgroundColor: '#1a1a2e',
    margin: 15,
    borderRadius: 12,
    padding: 20,
  },
  instructionTitle: { color: '#2196F3', fontSize: 16, fontWeight: 'bold', marginBottom: 10 },
  instructionText: { color: '#ccc', fontSize: 13, marginBottom: 8, lineHeight: 20 },
  startCalibButton: {
    backgroundColor: '#2196F3',
    paddingVertical: 16,
    borderRadius: 10,
    alignItems: 'center',
    marginTop: 20,
  },
  startCalibButtonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  collectingContainer: { padding: 20, alignItems: 'center' },
  figure8Container: { marginBottom: 20 },
  figure8Icon: { width: 80, height: 80, alignItems: 'center', justifyContent: 'center' },
  figure8Text: { fontSize: 60, color: '#2196F3' },
  collectingText: { color: '#ccc', fontSize: 16, marginBottom: 20 },
  progressContainer: { width: '100%', marginBottom: 20 },
  progressTrack: {
    height: 12,
    backgroundColor: '#2a2a3e',
    borderRadius: 6,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: '#4CAF50',
    borderRadius: 6,
  },
  progressText: { color: '#888', fontSize: 12, marginTop: 8, textAlign: 'center' },
  scatterPlot: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 15,
    width: '100%',
  },
  scatterTitle: { color: '#888', fontSize: 12, marginBottom: 10 },
  scatterArea: {
    height: 200,
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    position: 'relative',
  },
  scatterDot: {
    position: 'absolute',
    width: 4,
    height: 4,
    borderRadius: 2,
    opacity: 0.7,
  },
  completeContainer: { padding: 30, alignItems: 'center' },
  completeIcon: { fontSize: 60, color: '#4CAF50', marginBottom: 10 },
  completeText: { color: '#fff', fontSize: 22, fontWeight: 'bold', marginBottom: 20 },
  calibResults: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 20,
    width: '100%',
    marginBottom: 20,
  },
  resultsTitle: { color: '#2196F3', fontSize: 14, fontWeight: 'bold', marginBottom: 10 },
  resultLine: { color: '#ccc', fontSize: 13, fontFamily: 'monospace', marginBottom: 4 },
  finishButton: {
    backgroundColor: '#4CAF50',
    paddingVertical: 16,
    paddingHorizontal: 40,
    borderRadius: 10,
  },
  finishButtonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  failedContainer: { padding: 30, alignItems: 'center' },
  failedIcon: { fontSize: 60, color: '#F44336', marginBottom: 10 },
  failedText: { color: '#fff', fontSize: 22, fontWeight: 'bold', marginBottom: 15 },
  failedSubtext: { color: '#888', fontSize: 14, textAlign: 'center', marginBottom: 20 },
  retryButton: {
    backgroundColor: '#2196F3',
    paddingVertical: 14,
    paddingHorizontal: 40,
    borderRadius: 10,
  },
  retryButtonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
});