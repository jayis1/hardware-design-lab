/**
 * HomeScreen.js — Main dashboard for BreathPrint app
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import {
  STATE_NAMES,
  STATE_COLORS,
  DEVICE_STATE_NAMES,
  formatPpm,
  formatTimestamp,
  formatDuration,
  getKetosisLevel,
  getGutFermentationLevel,
  getVocCategory,
} from '../utils/protocol';
import SensorGauge from '../components/SensorGauge';

const HomeScreen = () => {
  const {
    connected,
    scanning,
    status,
    lastResult,
    liveSample,
    history,
    error,
    startScan,
    startAnalysis,
  } = useBle();

  const [analyzing, setAnalyzing] = useState(false);

  // Track analysis state
  useEffect(() => {
    if (status.state === 1 || status.state === 2 || status.state === 4) {
      setAnalyzing(true);
    } else if (status.state === 5 || status.state === 0) {
      setAnalyzing(false);
    }
  }, [status.state]);

  const handleAnalyze = async () => {
    setAnalyzing(true);
    await startAnalysis();
  };

  const handleConnect = async () => {
    await startScan();
  };

  // Not connected state
  if (!connected) {
    return (
      <ScrollView style={styles.container} contentContainerStyle={styles.centerContent}>
        <View style={styles.header}>
          <Text style={styles.title}>BreathPrint</Text>
          <Text style={styles.subtitle}>Your metabolic fingerprint</Text>
        </View>

        <View style={styles.connectCard}>
          <Text style={styles.connectTitle}>Connect to Your Device</Text>
          <Text style={styles.connectDesc}>
            Make sure your BreathPrint device is powered on and nearby.
          </Text>

          <TouchableOpacity
            style={styles.connectButton}
            onPress={handleConnect}
            disabled={scanning}
          >
            {scanning ? (
              <ActivityIndicator color="#fff" />
            ) : (
              <Text style={styles.connectButtonText}>
                {error ? 'Retry Scan' : 'Scan for Device'}
              </Text>
            )}
          </TouchableOpacity>

          {error && <Text style={styles.errorText}>{error}</Text>}
        </View>

        <View style={styles.infoCard}>
          <Text style={styles.infoTitle}>About BreathPrint</Text>
          <Text style={styles.infoText}>
            BreathPrint analyzes volatile organic compounds in your breath
            to provide insights about your metabolic state, gut health,
            and dietary patterns.
          </Text>
          <Text style={styles.infoText}>
            • Track ketosis from breath acetone{'\n'}
            • Monitor gut fermentation (H₂, CH₄){'\n'}
            • Identify food intolerances{'\n'}
            • Correlate diet with metabolic markers
          </Text>
        </View>

        <Text style={styles.footer}>Author: jayis1 — MIT License</Text>
      </ScrollView>
    );
  }

  // Connected state
  return (
    <ScrollView style={styles.container}>
      {/* Status bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Battery</Text>
          <Text style={styles.statusValue}>{status.battery}%</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>State</Text>
          <Text style={styles.statusValue}>
            {DEVICE_STATE_NAMES[status.state] || 'Unknown'}
          </Text>
        </View>
        {status.charging && (
          <View style={styles.statusItem}>
            <Text style={styles.statusLabel}>⚡</Text>
            <Text style={styles.statusValue}>Charging</Text>
          </View>
        )}
      </View>

      {/* Analyzing state — live sensor data */}
      {analyzing && (
        <View style={styles.analyzingCard}>
          <Text style={styles.analyzingTitle}>
            {DEVICE_STATE_NAMES[status.state] || 'Analyzing...'}
          </Text>
          <ActivityIndicator size="large" color="#2E86AB" />

          {liveSample && (
            <View style={styles.liveData}>
              <Text style={styles.liveLabel}>CO₂: {liveSample.co2} ppm</Text>
              <Text style={styles.liveLabel}>H₂: {(liveSample.h2 / 10).toFixed(1)} ppm</Text>
              <Text style={styles.liveLabel}>CH₄: {liveSample.ch4} ppm</Text>
              <Text style={styles.liveLabel}>
                BME688: {liveSample.bme688_1_gas}
              </Text>
            </View>
          )}

          <Text style={styles.breathePrompt}>
            {status.state === 2
              ? 'Breathe into the mouthpiece...'
              : 'Processing your breath sample...'}
          </Text>
        </View>
      )}

      {/* Last result */}
      {lastResult && !analyzing && (
        <View style={styles.resultCard}>
          <View style={styles.resultHeader}>
            <Text style={styles.resultTitle}>Last Breath Analysis</Text>
            <Text style={styles.resultTime}>
              {formatDuration((Date.now() / 1000 - lastResult.timestamp))}
            </Text>
          </View>

          {/* Metabolic state */}
          <View
            style={[
              styles.stateBadge,
              { backgroundColor: lastResult.stateColor + '30' },
            ]}
          >
            <Text style={[styles.stateText, { color: lastResult.stateColor }]}>
              {lastResult.stateName}
            </Text>
          </View>

          {/* Quality indicator */}
          <Text style={styles.qualityText}>
            Quality: {lastResult.qualityName}
          </Text>

          {/* Key metrics */}
          <View style={styles.metricsGrid}>
            <SensorGauge
              label="Acetone"
              value={lastResult.acetonePpm.toFixed(2)}
              unit="ppm"
              color="#FF9800"
              level={getKetosisLevel(lastResult.acetonePpm).level}
            />
            <SensorGauge
              label="H₂"
              value={lastResult.h2Ppm.toFixed(1)}
              unit="ppm"
              color="#2196F3"
              level={getGutFermentationLevel(lastResult.h2Ppm, lastResult.ch4Ppm).level}
            />
          </View>

          <View style={styles.metricsGrid}>
            <SensorGauge
              label="Methane"
              value={lastResult.ch4Ppm.toFixed(1)}
              unit="ppm"
              color="#4CAF50"
            />
            <SensorGauge
              label="VOC Index"
              value={lastResult.vocIndex.toString()}
              unit=""
              color="#9C27B0"
              level={getVocCategory(lastResult.vocIndex).category}
            />
          </View>

          <View style={styles.metricsGrid}>
            <SensorGauge
              label="CO₂"
              value={lastResult.co2Ppm.toString()}
              unit="ppm"
              color="#FF5722"
            />
            <SensorGauge
              label="Isoprene"
              value={lastResult.isoprenePpb.toString()}
              unit="ppb"
              color="#3F51B5"
            />
          </View>

          {/* Environmental */}
          <View style={styles.envRow}>
            <Text style={styles.envText}>
              🌡 {lastResult.temperature.toFixed(1)}°C
            </Text>
            <Text style={styles.envText}>
              💧 {lastResult.humidity.toFixed(0)}% RH
            </Text>
            <Text style={styles.envText}>
              📊 {lastResult.pressure} hPa
            </Text>
          </View>
        </View>
      )}

      {/* Analyze button */}
      {!analyzing && (
        <TouchableOpacity
          style={styles.analyzeButton}
          onPress={handleAnalyze}
        >
          <Text style={styles.analyzeButtonText}>⚡ Analyze Breath</Text>
        </TouchableOpacity>
      )}

      {/* History summary */}
      {history.length > 0 && !analyzing && (
        <View style={styles.historyCard}>
          <Text style={styles.historyTitle}>Recent Samples</Text>
          {history.slice(-5).reverse().map((item, idx) => (
            <View key={idx} style={styles.historyItem}>
              <View
                style={[
                  styles.historyDot,
                  { backgroundColor: item.stateColor || '#9E9E9E' },
                ]}
              />
              <Text style={styles.historyState}>{item.stateName}</Text>
              <Text style={styles.historyTime}>
                {formatTimestamp(item.timestamp)}
              </Text>
            </View>
          ))}
        </View>
      )}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1e',
  },
  centerContent: {
    alignItems: 'center',
    justifyContent: 'center',
    padding: 20,
    minHeight: '100%',
  },
  header: {
    alignItems: 'center',
    marginVertical: 30,
  },
  title: {
    fontSize: 32,
    fontWeight: 'bold',
    color: '#2E86AB',
  },
  subtitle: {
    fontSize: 16,
    color: '#888',
    marginTop: 4,
  },
  connectCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 24,
    width: '100%',
    alignItems: 'center',
    marginBottom: 20,
  },
  connectTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 8,
  },
  connectDesc: {
    fontSize: 14,
    color: '#888',
    textAlign: 'center',
    marginBottom: 20,
  },
  connectButton: {
    backgroundColor: '#2E86AB',
    borderRadius: 12,
    paddingVertical: 14,
    paddingHorizontal: 32,
    minWidth: 200,
    alignItems: 'center',
  },
  connectButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  errorText: {
    color: '#F44336',
    fontSize: 14,
    marginTop: 12,
  },
  infoCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    width: '100%',
    marginBottom: 20,
  },
  infoTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 10,
  },
  infoText: {
    fontSize: 14,
    color: '#aaa',
    lineHeight: 22,
    marginBottom: 10,
  },
  footer: {
    color: '#555',
    fontSize: 12,
    marginTop: 20,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    backgroundColor: '#1a1a2e',
    paddingVertical: 12,
    paddingHorizontal: 16,
    marginHorizontal: 16,
    marginTop: 16,
    borderRadius: 12,
  },
  statusItem: {
    alignItems: 'center',
  },
  statusLabel: {
    color: '#888',
    fontSize: 12,
  },
  statusValue: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
    marginTop: 4,
  },
  analyzingCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 30,
    margin: 16,
    alignItems: 'center',
  },
  analyzingTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 16,
  },
  liveData: {
    marginTop: 16,
    alignItems: 'center',
  },
  liveLabel: {
    color: '#2E86AB',
    fontSize: 14,
    marginVertical: 4,
  },
  breathePrompt: {
    color: '#aaa',
    fontSize: 16,
    marginTop: 20,
    textAlign: 'center',
  },
  resultCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    margin: 16,
  },
  resultHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  resultTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
  },
  resultTime: {
    fontSize: 12,
    color: '#888',
  },
  stateBadge: {
    borderRadius: 12,
    paddingVertical: 10,
    paddingHorizontal: 16,
    alignSelf: 'flex-start',
    marginBottom: 8,
  },
  stateText: {
    fontSize: 18,
    fontWeight: 'bold',
  },
  qualityText: {
    color: '#888',
    fontSize: 13,
    marginBottom: 16,
  },
  metricsGrid: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  envRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 16,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: '#333',
  },
  envText: {
    color: '#aaa',
    fontSize: 13,
  },
  analyzeButton: {
    backgroundColor: '#2E86AB',
    borderRadius: 16,
    paddingVertical: 18,
    marginHorizontal: 16,
    alignItems: 'center',
  },
  analyzeButtonText: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  historyCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    margin: 16,
  },
  historyTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 12,
  },
  historyItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  historyDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 12,
  },
  historyState: {
    color: '#fff',
    fontSize: 14,
    flex: 1,
  },
  historyTime: {
    color: '#888',
    fontSize: 12,
  },
});

export default HomeScreen;