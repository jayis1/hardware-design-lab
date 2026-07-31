/**
 * LiveMonitorScreen.tsx — Real-time sensor and gesture visualization.
 *
 * Displays live EMG envelopes, finger curl angles, wrist orientation,
 * current gesture classification, and a scrolling MIDI event log.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { useBle } from '../ble/BleManager';
import EmgBarChart from '../components/EmgBarChart';
import FingerCurlView from '../components/FingerCurlView';
import GestureBadge from '../components/GestureBadge';
import MidiMonitor from '../components/MidiMonitor';

/**
 * LiveMonitorScreen — real-time performance dashboard.
 * Author: jayis1
 */
export default function LiveMonitorScreen() {
  const {
    isConnected,
    isScanning,
    deviceName,
    batteryLevel,
    oscData,
    midiEvents,
    connect,
    disconnect,
  } = useBle();

  const emgValues = oscData?.emgEnvelopes || [0, 0, 0, 0, 0];
  const curls = oscData?.fingerCurls || [0, 0, 0, 0, 0];
  const velocities = oscData?.fingerVelocities || [0, 0, 0, 0, 0];
  const gestureId = oscData?.gestureId ?? -1;
  const gestureConf = oscData?.gestureConfidence ?? 0;

  // Determine which finger is active (highest velocity)
  const activeFinger = velocities.indexOf(Math.max(...velocities));
  const fingerForGesture = activeFinger >= 0 && velocities[activeFinger] > 0.1
    ? activeFinger : -1;

  if (!isConnected) {
    return (
      <View style={styles.container}>
        <View style={styles.disconnectedBox}>
          <Text style={styles.disconnectedTitle}>Not Connected</Text>
          <Text style={styles.disconnectedText}>
            Connect to your Synthand glove to start playing.
          </Text>
          <TouchableOpacity
            style={styles.connectButton}
            onPress={connect}
            disabled={isScanning}
          >
            <Text style={styles.connectButtonText}>
              {isScanning ? 'Scanning...' : 'Connect to Synthand'}
            </Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      {/* Status bar */}
      <View style={styles.statusBar}>
        <Text style={styles.deviceName}>{deviceName}</Text>
        <View style={styles.statusRight}>
          {batteryLevel !== null && (
            <Text style={styles.batteryText}>
              🔋 {batteryLevel}%
            </Text>
          )}
          <TouchableOpacity
            style={styles.disconnectBtn}
            onPress={disconnect}
          >
            <Text style={styles.disconnectBtnText}>Disconnect</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Gesture badge */}
      <GestureBadge
        gestureId={gestureId}
        confidence={gestureConf}
        finger={fingerForGesture}
      />

      {/* EMG bar chart */}
      <EmgBarChart values={emgValues} />

      {/* Finger curl visualization */}
      <FingerCurlView curls={curls} velocities={velocities} />

      {/* Wrist orientation display */}
      <View style={styles.quatBox}>
        <Text style={styles.quatTitle}>Wrist Orientation</Text>
        <Text style={styles.quatValues}>
          w: {((oscData?.wristQuaternion[0] || 1)).toFixed(3)}  {'  '}
          x: {((oscData?.wristQuaternion[1] || 0)).toFixed(3)}  {'  '}
          y: {((oscData?.wristQuaternion[2] || 0)).toFixed(3)}  {'  '}
          z: {((oscData?.wristQuaternion[3] || 0)).toFixed(3)}
        </Text>
      </View>

      {/* MIDI event log */}
      <MidiMonitor events={midiEvents} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  disconnectedBox: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingTop: 100,
  },
  disconnectedTitle: {
    color: '#e94560',
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  disconnectedText: {
    color: '#888',
    fontSize: 16,
    marginBottom: 24,
    textAlign: 'center',
  },
  connectButton: {
    backgroundColor: '#e94560',
    borderRadius: 8,
    paddingHorizontal: 32,
    paddingVertical: 16,
  },
  connectButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  deviceName: {
    color: '#e94560',
    fontSize: 18,
    fontWeight: 'bold',
  },
  statusRight: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  batteryText: {
    color: '#ccc',
    fontSize: 14,
    marginRight: 12,
  },
  disconnectBtn: {
    backgroundColor: '#0f3460',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 6,
  },
  disconnectBtnText: {
    color: '#ccc',
    fontSize: 12,
  },
  quatBox: {
    backgroundColor: '#16213e',
    borderRadius: 12,
    padding: 16,
    marginVertical: 8,
  },
  quatTitle: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  quatValues: {
    color: '#aaa',
    fontFamily: 'monospace',
    fontSize: 11,
  },
});