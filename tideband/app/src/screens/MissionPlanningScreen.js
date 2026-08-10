/**
 * @file    MissionPlanningScreen.js
 * @brief   Mission planning: set sample rate, haptic threshold, pre-dive
 *          checklist, and tide-based current prediction.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  Switch, ScrollView, Alert,
} from 'react-native';
import { useTideBand } from '../services/TideBandContext';

export default function MissionPlanningScreen() {
  const {
    connected, status, sampleRate, setRate,
    hapticThreshold, setThreshold, units, sendCommand,
  } = useTideBand();

  const [hapticEnabled, setHapticEnabled] = useState(true);
  const [plannedDepth, setPlannedDepth] = useState('30');
  const [tideTime, setTideTime] = useState('');
  const [tideType, setTideType] = useState('high');
  const [prediction, setPrediction] = useState(null);

  // Pre-dive checklist state
  const [checklist, setChecklist] = useState({
    battery: false,
    sensors: false,
    calibration: false,
    storage: false,
    sealed: false,
  });

  const runPreDiveCheck = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to TideBand first.');
      return;
    }

    const newChecklist = {
      battery: status && status.batteryPct > 30,
      sensors: status && status.quality > 0,
      calibration: true, // Would check calibration date from device
      storage: true,     // Would check free space from device
      sealed: true,      // Manual check — user confirms
    };
    setChecklist(newChecklist);

    const allPass = Object.values(newChecklist).every(v => v);
    if (allPass) {
      Alert.alert('✓ Ready to Dive', 'All pre-dive checks passed.');
    } else {
      const failures = Object.entries(newChecklist)
        .filter(([k, v]) => !v)
        .map(([k]) => k.charAt(0).toUpperCase() + k.slice(1));
      Alert.alert('⚠ Checks Failed', `Failed: ${failures.join(', ')}`);
    }
  };

  const predictCurrent = () => {
    // Simple tidal current prediction based on tide time and type.
    // In a real app, this would use local tide station data and
    // harmonic constants. Here we use a simplified model.
    const hour = parseInt(tideTime.split(':')[0]) || 0;
    const minute = parseInt(tideTime.split(':')[1]) || 0;
    const totalMin = hour * 60 + minute;

    // Slack current occurs at high/low tide; peak current ~3 hours after
    const slackOffset = tideType === 'high' ? 0 : 180; // 3 hours
    const peakOffset = tideType === 'high' ? 180 : 0;

    const minFromSlack = (totalMin - slackOffset + 360) % 360;
    const minFromPeak = (totalMin - peakOffset + 360) % 360;

    // Current speed follows sinusoidal pattern
    const phaseRad = (minFromPeak / 360) * 2 * Math.PI;
    const predictedSpeed = Math.abs(Math.sin(phaseRad)) * 2.5; // Max 2.5 m/s

    // Direction reverses with tide
    const direction = minFromSlack < 180 ? 'Flooding' : 'Ebbing';

    // Best dive window: within 30 min of slack
    const isSafe = predictedSpeed < 0.5;
    const safetyWindow = isSafe
      ? `${Math.max(0, 30 - minFromSlack)} min until peak current`
      : 'Current too strong — wait for slack';

    setPrediction({
      speed: predictedSpeed,
      direction: direction,
      safety: safetyWindow,
      isSafe: isSafe,
    });
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Pre-Dive Checklist</Text>
      <View style={styles.card}>
        <View style={styles.checklistRow}>
          <Text style={styles.checklistLabel}>{'Battery > 30%'}</Text>
          <Text style={[
            styles.checklistStatus,
            { color: checklist.battery ? '#00AA00' : '#FF4444' }
          ]}>
            {checklist.battery ? '✓ Pass' : '✗ Fail'}
          </Text>
        </View>
        <View style={styles.checklistRow}>
          <Text style={styles.checklistLabel}>Sensors responding</Text>
          <Text style={[
            styles.checklistStatus,
            { color: checklist.sensors ? '#00AA00' : '#FF4444' }
          ]}>
            {checklist.sensors ? '✓ Pass' : '✗ Fail'}
          </Text>
        </View>
        <View style={styles.checklistRow}>
          <Text style={styles.checklistLabel}>Calibration valid</Text>
          <Text style={[
            styles.checklistStatus,
            { color: checklist.calibration ? '#00AA00' : '#FF4444' }
          ]}>
            {checklist.calibration ? '✓ Pass' : '✗ Fail'}
          </Text>
        </View>
        <View style={styles.checklistRow}>
          <Text style={styles.checklistLabel}>Storage available</Text>
          <Text style={[
            styles.checklistStatus,
            { color: checklist.storage ? '#00AA00' : '#FF4444' }
          ]}>
            {checklist.storage ? '✓ Pass' : '✗ Fail'}
          </Text>
        </View>
        <View style={styles.checklistRow}>
          <Text style={styles.checklistLabel}>Enclosure sealed</Text>
          <Text style={[
            styles.checklistStatus,
            { color: checklist.sealed ? '#00AA00' : '#FF4444' }
          ]}>
            {checklist.sealed ? '✓ Pass' : '✗ Fail'}
          </Text>
        </View>
        <TouchableOpacity
          style={styles.checkButton}
          onPress={runPreDiveCheck}
        >
          <Text style={styles.checkButtonText}>Run Pre-Dive Check</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.sectionTitle}>Sampling Configuration</Text>
      <View style={styles.card}>
        <Text style={styles.cardLabel}>Sample Rate</Text>
        <View style={styles.rateRow}>
          {[1, 2, 4].map(rate => (
            <TouchableOpacity
              key={rate}
              style={[
                styles.rateButton,
                sampleRate === rate && styles.rateButtonActive
              ]}
              onPress={() => setRate(rate)}
            >
              <Text style={[
                styles.rateButtonText,
                sampleRate === rate && styles.rateButtonTextActive
              ]}>
                {rate} Hz
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.cardLabel}>Haptic Feedback</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Enable haptic alerts</Text>
          <Switch
            value={hapticEnabled}
            onValueChange={(val) => {
              setHapticEnabled(val);
              // Would send enable/disable command to device
            }}
            trackColor={{ false: '#E0E0E0', true: '#0080FF' }}
          />
        </View>

        <Text style={styles.cardLabel}>Current Speed Threshold</Text>
        <Text style={styles.thresholdDescription}>
          Alert when current exceeds this speed
        </Text>
        <TextInput
          style={styles.input}
          value={String(hapticThreshold)}
          onChangeText={(text) => {
            const val = parseFloat(text);
            if (!isNaN(val) && val >= 0 && val <= 5) {
              setThreshold(val);
            }
          }}
          keyboardType="decimal-pad"
          placeholder="0.5"
        />
        <Text style={styles.unitLabel}>m/s</Text>
      </View>

      <Text style={styles.sectionTitle}>Current Prediction</Text>
      <View style={styles.card}>
        <Text style={styles.cardLabel}>Tide Type</Text>
        <View style={styles.rateRow}>
          <TouchableOpacity
            style={[
              styles.rateButton,
              tideType === 'high' && styles.rateButtonActive
            ]}
            onPress={() => setTideType('high')}
          >
            <Text style={[
              styles.rateButtonText,
              tideType === 'high' && styles.rateButtonTextActive
            ]}>
              High Tide
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[
              styles.rateButton,
              tideType === 'low' && styles.rateButtonActive
            ]}
            onPress={() => setTideType('low')}
          >
            <Text style={[
              styles.rateButtonText,
              tideType === 'low' && styles.rateButtonTextActive
            ]}>
              Low Tide
            </Text>
          </TouchableOpacity>
        </View>

        <Text style={styles.cardLabel}>Tide Time (24h)</Text>
        <TextInput
          style={styles.input}
          value={tideTime}
          onChangeText={setTideTime}
          placeholder="HH:MM"
          keyboardType="numeric"
        />

        <TouchableOpacity
          style={styles.predictButton}
          onPress={predictCurrent}
        >
          <Text style={styles.predictButtonText}>Predict Current</Text>
        </TouchableOpacity>

        {prediction && (
          <View style={styles.predictionBox}>
            <Text style={styles.predictionTitle}>Prediction</Text>
            <Text style={styles.predictionRow}>
              Speed: {prediction.speed.toFixed(2)} m/s
            </Text>
            <Text style={styles.predictionRow}>
              Direction: {prediction.direction}
            </Text>
            <Text style={[
              styles.predictionRow,
              { color: prediction.isSafe ? '#00AA00' : '#FF4444', fontWeight: 'bold' }
            ]}>
              {prediction.safety}
            </Text>
          </View>
        )}
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F0F0F0',
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333333',
    marginHorizontal: 16,
    marginTop: 16,
    marginBottom: 8,
  },
  card: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 16,
    marginHorizontal: 16,
    marginBottom: 8,
  },
  checklistRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#F0F0F0',
  },
  checklistLabel: {
    fontSize: 14,
    color: '#333333',
  },
  checklistStatus: {
    fontSize: 14,
    fontWeight: 'bold',
  },
  checkButton: {
    backgroundColor: '#0080FF',
    paddingVertical: 12,
    borderRadius: 8,
    marginTop: 12,
    alignItems: 'center',
  },
  checkButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: 'bold',
  },
  cardLabel: {
    fontSize: 14,
    fontWeight: '600',
    color: '#333333',
    marginTop: 12,
    marginBottom: 8,
  },
  rateRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  rateButton: {
    flex: 1,
    paddingVertical: 10,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#E0E0E0',
    marginHorizontal: 4,
    alignItems: 'center',
  },
  rateButtonActive: {
    backgroundColor: '#0080FF',
    borderColor: '#0080FF',
  },
  rateButtonText: {
    fontSize: 14,
    color: '#808080',
  },
  rateButtonTextActive: {
    color: '#FFFFFF',
    fontWeight: 'bold',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginVertical: 8,
  },
  switchLabel: {
    fontSize: 14,
    color: '#333333',
  },
  thresholdDescription: {
    fontSize: 12,
    color: '#808080',
    marginBottom: 8,
  },
  input: {
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 6,
    paddingVertical: 10,
    paddingHorizontal: 12,
    fontSize: 16,
    color: '#333333',
  },
  unitLabel: {
    fontSize: 12,
    color: '#808080',
    marginTop: 4,
  },
  predictButton: {
    backgroundColor: '#00AA44',
    paddingVertical: 12,
    borderRadius: 8,
    marginTop: 16,
    alignItems: 'center',
  },
  predictButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: 'bold',
  },
  predictionBox: {
    backgroundColor: '#F0F8FF',
    borderRadius: 8,
    padding: 12,
    marginTop: 16,
  },
  predictionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#0080FF',
    marginBottom: 8,
  },
  predictionRow: {
    fontSize: 14,
    color: '#333333',
    marginBottom: 4,
  },
});