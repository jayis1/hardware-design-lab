/**
 * LiveSweepScreen.tsx — Real-time sweep visualization screen.
 *
 * Shows the live Nyquist and Bode plots as data points arrive from
 * the device. Also displays connection status and sweep progress.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
  Alert,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import bleManager from '../ble/BleManager';
import {
  SweepPoint,
  CellResult,
  DeviceStatus,
  DegradationMode,
  DEGRADATION_NAMES,
  CHEMISTRY_NAMES,
} from '../ble/protocol';
import NyquistPlot from '../components/NyquistPlot';
import BodePlot from '../components/BodePlot';
import { useDatabase } from '../db/database';

export default function LiveSweepScreen() {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [sweeping, setSweeping] = useState(false);
  const [progress, setProgress] = useState(0);
  const [points, setPoints] = useState<SweepPoint[]>([]);
  const [result, setResult] = useState<CellResult | null>(null);
  const [status, setStatus] = useState<DeviceStatus | null>(null);
  const navigation = useNavigation();
  const db = useDatabase();

  useEffect(() => {
    // Register BLE callbacks
    bleManager.onConnectionChange((conn) => setConnected(conn));
    bleManager.onSweepPoint((point) => {
      setPoints((prev) => [...prev, point]);
    });
    bleManager.onResult((res) => {
      setResult(res);
      setSweeping(false);
      // Save to database
      db.saveResult({
        timestamp: res.timestamp,
        cellLabel: '',
        sohScore: res.sohScore,
        degradation: res.degradation,
        verdict: res.verdict,
        chemistryIdx: res.chemistryIdx,
        ocvMv: res.ocvMv,
        tempDc: res.tempDc,
        dcirMohm: res.dcirMohm,
        selfDischargeUvPerMin: res.selfDischargeUvPerMin,
        rsMohm: res.randles?.rsMohm || 0,
        rseiMohm: res.randles?.rseiMohm || 0,
        cseimF: res.randles?.cseimF || 0,
        rctMohm: res.randles?.rctMohm || 0,
        cdlmF: res.randles?.cdlmF || 0,
        sigma: res.randles?.sigma || 0,
        fitValid: res.fitValid ? 1 : 0,
        sweepDataJson: JSON.stringify(points),
      });
    });
    bleManager.onStatus((stat) => {
      setStatus(stat);
      setProgress(stat.progress);
      if (stat.state === 2 || stat.state === 3) {
        setSweeping(true);
      } else if (stat.state === 0) {
        setSweeping(false);
      }
    });
  }, []);

  const handleScan = useCallback(() => {
    setScanning(true);
    bleManager.startScan((device) => {
      bleManager.stopScan();
      setScanning(false);
      bleManager.connect(device).then((ok) => {
        if (!ok) {
          Alert.alert('Connection Failed', 'Could not connect to LithoCore device.');
        }
      });
    });
  }, []);

  const handleFastSweep = useCallback(() => {
    setPoints([]);
    setResult(null);
    setSweeping(true);
    setProgress(0);
    bleManager.startFastSweep();
  }, []);

  const handleFullSweep = useCallback(() => {
    setPoints([]);
    setResult(null);
    setSweeping(true);
    setProgress(0);
    bleManager.startFullSweep();
  }, []);

  const handleAbort = useCallback(() => {
    bleManager.abortSweep();
    setSweeping(false);
  }, []);

  const handleViewReport = useCallback(() => {
    if (result) {
      navigation.navigate('CellReport', { cellId: undefined });
    }
  }, [result, navigation]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection status */}
      <View style={styles.statusBar}>
        <View style={[styles.statusDot, { backgroundColor: connected ? '#00e676' : '#f44336' }]} />
        <Text style={styles.statusText}>
          {connected ? 'Connected' : (scanning ? 'Scanning…' : 'Disconnected')}
        </Text>
        {!connected && (
          <TouchableOpacity style={styles.connectButton} onPress={handleScan}>
            {scanning ? (
              <ActivityIndicator color="#fff" size="small" />
            ) : (
              <Text style={styles.buttonText}>Connect</Text>
            )}
          </TouchableOpacity>
        )}
      </View>

      {/* Sweep controls */}
      {connected && (
        <View style={styles.controls}>
          <TouchableOpacity
            style={[styles.sweepButton, styles.fastButton]}
            onPress={handleFastSweep}
            disabled={sweeping}
          >
            <Text style={styles.buttonText}>Fast Sweep (20s)</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.sweepButton, styles.fullButton]}
            onPress={handleFullSweep}
            disabled={sweeping}
          >
            <Text style={styles.buttonText}>Full Sweep (12min)</Text>
          </TouchableOpacity>
          {sweeping && (
            <TouchableOpacity
              style={[styles.sweepButton, styles.abortButton]}
              onPress={handleAbort}
            >
              <Text style={styles.buttonText}>Abort</Text>
            </TouchableOpacity>
          )}
        </View>
      )}

      {/* Progress bar */}
      {sweeping && (
        <View style={styles.progressContainer}>
          <View style={styles.progressBar}>
            <View style={[styles.progressFill, { width: `${progress}%` }]} />
          </View>
          <Text style={styles.progressText}>{progress}% — {points.length} points</Text>
        </View>
      )}

      {/* Live Nyquist plot */}
      <NyquistPlot points={points} />

      {/* Live Bode plot */}
      <BodePlot points={points} />

      {/* Quick result summary */}
      {result && (
        <View style={styles.resultSummary}>
          <Text style={styles.resultTitle}>Sweep Complete</Text>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>SoH:</Text>
            <Text style={styles.resultValue}>{result.sohScore}%</Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Mode:</Text>
            <Text style={styles.resultValue}>
              {DEGRADATION_NAMES[result.degradation] || 'Unknown'}
            </Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>Chemistry:</Text>
            <Text style={styles.resultValue}>
              {CHEMISTRY_NAMES[result.chemistryIdx] || 'Unknown'}
            </Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>OCV:</Text>
            <Text style={styles.resultValue}>{(result.ocvMv / 1000).toFixed(3)} V</Text>
          </View>
          <View style={styles.resultRow}>
            <Text style={styles.resultLabel}>DCIR:</Text>
            <Text style={styles.resultValue}>{result.dcirMohm} mΩ</Text>
          </View>
          <TouchableOpacity style={styles.reportButton} onPress={handleViewReport}>
            <Text style={styles.buttonText}>View Full Report</Text>
          </TouchableOpacity>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#12122a' },
  content: { padding: 16 },
  statusBar: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  statusDot: { width: 10, height: 10, borderRadius: 5, marginRight: 8 },
  statusText: { color: '#e0e0e0', fontSize: 14, flex: 1 },
  connectButton: {
    backgroundColor: '#0066cc',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 6,
  },
  controls: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
    marginBottom: 12,
  },
  sweepButton: {
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderRadius: 8,
    flex: 1,
    minWidth: 140,
  },
  fastButton: { backgroundColor: '#0066cc' },
  fullButton: { backgroundColor: '#6a1b9a' },
  abortButton: { backgroundColor: '#c62828' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: '600', textAlign: 'center' },
  progressContainer: { marginBottom: 12 },
  progressBar: {
    height: 8,
    backgroundColor: '#222244',
    borderRadius: 4,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: '#00b4ff',
    borderRadius: 4,
  },
  progressText: {
    color: '#8888aa',
    fontSize: 12,
    marginTop: 4,
    textAlign: 'center',
  },
  resultSummary: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 16,
    marginTop: 12,
  },
  resultTitle: {
    color: '#e0e0e0',
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 12,
    textAlign: 'center',
  },
  resultRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#222244',
  },
  resultLabel: { color: '#8888aa', fontSize: 14 },
  resultValue: { color: '#e0e0e0', fontSize: 14, fontWeight: '600' },
  reportButton: {
    backgroundColor: '#0066cc',
    paddingVertical: 12,
    borderRadius: 8,
    marginTop: 12,
  },
});