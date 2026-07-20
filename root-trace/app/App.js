/**
 * App.js — RootTrace Companion App
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * React Native companion app for the RootTrace EIT device.
 * Provides:
 *   - Device connection screen (BLE scanning and pairing)
 *   - Live scan view (real-time 2D conductivity map)
 *   - 3D volumetric reconstruction (stacked depth scans)
 *   - Longitudinal tracking (growth curves over time)
 *   - Frequency-difference analysis
 *   - Calibration & diagnostics
 *   - Study management (plant assignment, interventions)
 *   - Data export (CSV, PNG, NetCDF)
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Switch,
  Slider,
  Alert,
  FlatList,
  Modal,
  TextInput,
  StatusBar,
  Dimensions,
} from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { BleProvider, useBle } from './BleContext';
import { ConductivityMapView } from './components/ConductivityMap';
import { StudyManager } from './components/StudyManager';
import { GrowthTracker } from './components/GrowthTracker';
import { ExportView } from './components/ExportView';
import { SettingsView } from './components/SettingsView';
import { CalibrationView } from './components/CalibrationView';

const { width, height } = Dimensions.get('window');

// ====================================================================
//  Color palette
// ====================================================================

const Colors = {
  bg: '#0a1628',
  card: '#162640',
  accent: '#3b82f6',
  accent2: '#10b981',
  text: '#e2e8f0',
  textDim: '#94a3b8',
  warning: '#f59e0b',
  danger: '#ef4444',
  root: '#22c55e',      // root biomass color
  soil: '#92400e',      // soil color
  moisture: '#3b82f6',  // moisture color
};

// ====================================================================
//  Connection Screen
// ====================================================================

function ConnectionScreen({ navigation }) {
  const ble = useBle();
  const [autoScan, setAutoScan] = useState(true);

  useEffect(() => {
    if (autoScan && !ble.connected && !ble.scanning) {
      ble.scanForDevices();
    }
  }, [autoScan, ble.connected, ble.scanning]);

  const handleConnect = async () => {
    if (ble.device) {
      await ble.connectDevice(ble.device);
      if (ble.connected) {
        navigation.navigate('Scan');
      }
    }
  };

  return (
    <View style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor={Colors.bg} />
      <View style={styles.header}>
        <Text style={styles.title}>RootTrace</Text>
        <Text style={styles.subtitle}>EIT Root Imaging System</Text>
        <Text style={styles.author}>by jayis1 · v1.0</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Connection</Text>
        {ble.connected ? (
          <View>
            <Text style={styles.statusText}>✓ Connected</Text>
            <Text style={styles.deviceName}>{ble.device?.name || 'RootTrace device'}</Text>
            <TouchableOpacity
              style={[styles.button, styles.primaryButton]}
              onPress={() => navigation.navigate('Scan')}>
              <Text style={styles.buttonText}>Go to Live Scan</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.button, styles.secondaryButton]}
              onPress={() => ble.disconnect()}>
              <Text style={styles.buttonText}>Disconnect</Text>
            </TouchableOpacity>
          </View>
        ) : (
          <View>
            <Text style={styles.statusText}>
              {ble.scanning ? '🔍 Scanning for devices...' : 'Not connected'}
            </Text>
            {ble.device && !ble.connected && (
              <View style={styles.deviceItem}>
                <Text style={styles.deviceName}>Found: {ble.device.name}</Text>
                <TouchableOpacity
                  style={[styles.button, styles.primaryButton]}
                  onPress={handleConnect}>
                  <Text style={styles.buttonText}>Connect</Text>
                </TouchableOpacity>
              </View>
            )}
            <TouchableOpacity
              style={[styles.button, styles.secondaryButton]}
              onPress={() => ble.scanForDevices()}>
              <Text style={styles.buttonText}>Scan Again</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>About RootTrace</Text>
        <Text style={styles.aboutText}>
          RootTrace is an open-source electrical impedance tomography (EIT)
          instrument that non-invasively images plant root systems in soil.
          It uses a 16-electrode ring probe and a regularized Gauss-Newton
          inverse solver to reconstruct 2D cross-sectional maps of subsurface
          conductivity — revealing root biomass density, water uptake zones,
          and soil moisture heterogeneity.
        </Text>
        <Text style={styles.aboutText}>
          Designed for agricultural researchers, crop breeders, and soil
          scientists who need non-destructive root phenotyping.
        </Text>
        <Text style={styles.copyText}>© 2026 jayis1 · MIT License</Text>
      </View>

      <View style={styles.autoScanRow}>
        <Text style={styles.label}>Auto-scan on startup</Text>
        <Switch
          value={autoScan}
          onValueChange={setAutoScan}
          trackColor={{ false: Colors.card, true: Colors.accent }}
        />
      </View>
    </View>
  );
}

// ====================================================================
//  Live Scan Screen
// ====================================================================

function ScanScreen({ navigation }) {
  const ble = useBle();
  const [scanInterval, setScanInterval] = useState(400); // ms
  const [selectedFreq, setSelectedFreq] = useState(0);
  const [currentUa, setCurrentUa] = useState(200);
  const [showContactOverlay, setShowContactOverlay] = useState(true);
  const [autoFrame, setAutoFrame] = useState(true);

  // Poll for status and frames
  useEffect(() => {
    if (!ble.connected) return;
    const interval = setInterval(() => {
      ble.requestStatus();
      if (autoFrame) {
        ble.requestFrame();
      }
    }, scanInterval);
    return () => clearInterval(interval);
  }, [ble.connected, scanInterval, autoFrame]);

  const handleStartScan = () => {
    ble.startScan(selectedFreq);
  };

  const handleStopScan = () => {
    ble.stopScan();
  };

  const freqLabels = ['1 kHz', '10 kHz', '50 kHz', '100 kHz'];

  return (
    <ScrollView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor={Colors.bg} />
      <View style={styles.header}>
        <Text style={styles.title}>Live Scan</Text>
        <TouchableOpacity onPress={() => navigation.navigate('Connection')}>
          <Text style={styles.link}>← Back</Text>
        </TouchableOpacity>
      </View>

      {/* Status bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Battery</Text>
          <Text style={styles.statusValue}>
            {ble.status.batteryPct}%{ble.status.charging ? ' ⚡' : ''}
          </Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Frame</Text>
          <Text style={styles.statusValue}>#{ble.status.frameSeq}</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Contacts</Text>
          <Text style={styles.statusValue}>
            {ble.status.contactMask ? `${Array.from(
              { length: 16 }, (_, i) =>
                (ble.status.contactMask >> i) & 1
              ).filter(x => x).length}/16` : '—'}
          </Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>SD Card</Text>
          <Text style={styles.statusValue}>
            {ble.status.sdOk ? '✓' : '✗'}
          </Text>
        </View>
      </View>

      {/* Conductivity map */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Reconstruction</Text>
        <ConductivityMapView
          frameData={ble.frameData}
          showContacts={showContactOverlay}
          width={width - 80}
          height={width - 80}
        />
      </View>

      {/* Environmental data */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Environmental</Text>
        <View style={styles.envRow}>
          <View style={styles.envItem}>
            <Text style={styles.envLabel}>Soil Temp</Text>
            <Text style={styles.envValue}>{ble.envData.soilTemp.toFixed(1)}°C</Text>
          </View>
          <View style={styles.envItem}>
            <Text style={styles.envLabel}>Moisture</Text>
            <Text style={styles.envValue}>{ble.envData.soilMoisture.toFixed(0)}%</Text>
          </View>
          <View style={styles.envItem}>
            <Text style={styles.envLabel}>Air Temp</Text>
            <Text style={styles.envValue}>{ble.envData.ambientTemp.toFixed(1)}°C</Text>
          </View>
          <View style={styles.envItem}>
            <Text style={styles.envLabel}>Humidity</Text>
            <Text style={styles.envValue}>{ble.envData.ambientRh.toFixed(0)}%</Text>
          </View>
        </View>
      </View>

      {/* Frequency selector */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Excitation Frequency</Text>
        <View style={styles.freqRow}>
          {freqLabels.map((label, idx) => (
            <TouchableOpacity
              key={idx}
              style={[
                styles.freqButton,
                selectedFreq === idx && styles.freqButtonActive,
              ]}
              onPress={() => {
                setSelectedFreq(idx);
                ble.setFrequency(idx);
              }}>
              <Text
                style={[
                  styles.freqButtonText,
                  selectedFreq === idx && styles.freqButtonTextActive,
                ]}>
                {label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Current control */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Excitation Current</Text>
        <Text style={styles.sliderValue}>{currentUa} µA</Text>
        <Slider
          style={styles.slider}
          minimumValue={10}
          maximumValue={500}
          step={10}
          value={currentUa}
          onValueChange={(val) => setCurrentUa(val)}
          onSlidingComplete={(val) => ble.setCurrent(val)}
          minimumTrackTintColor={Colors.accent}
          maximumTrackTintColor={Colors.card}
        />
        <Text style={styles.note}>Safe range: 10–500 µA (IEC 60601 limit)</Text>
      </View>

      {/* Scan controls */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Acquisition</Text>
        <View style={styles.controlRow}>
          <TouchableOpacity
            style={[styles.button, styles.primaryButton, { flex: 1, marginRight: 8 }]}
            onPress={handleStartScan}
            disabled={ble.status.scanning}>
            <Text style={styles.buttonText}>
              {ble.status.scanning ? 'Scanning...' : 'Start Scan'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.button, styles.dangerButton, { flex: 1, marginLeft: 8 }]}
            onPress={handleStopScan}
            disabled={!ble.status.scanning}>
            <Text style={styles.buttonText}>Stop</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.autoFrameRow}>
          <Text style={styles.label}>Auto-request frames</Text>
          <Switch
            value={autoFrame}
            onValueChange={setAutoFrame}
            trackColor={{ false: Colors.card, true: Colors.accent }}
          />
        </View>
        <View style={styles.autoFrameRow}>
          <Text style={styles.label}>Show electrode contacts</Text>
          <Switch
            value={showContactOverlay}
            onValueChange={setShowContactOverlay}
            trackColor={{ false: Colors.card, true: Colors.accent }}
          />
        </View>
      </View>

      {/* Navigation */}
      <View style={styles.navRow}>
        <TouchableOpacity
          style={[styles.navButton, styles.card]}
          onPress={() => navigation.navigate('Studies')}>
          <Text style={styles.navButtonText}>📋 Studies</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navButton, styles.card]}
          onPress={() => navigation.navigate('Growth')}>
          <Text style={styles.navButtonText}>📈 Growth</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navButton, styles.card]}
          onPress={() => navigation.navigate('Export')}>
          <Text style={styles.navButtonText}>💾 Export</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navButton, styles.card]}
          onPress={() => navigation.navigate('Calibration')}>
          <Text style={styles.navButtonText}>🔧 Calib</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.footer}>
        <Text style={styles.copyText}>RootTrace v1.0 · jayis1 · © 2026</Text>
      </View>
    </ScrollView>
  );
}

// ====================================================================
//  Placeholder screens for studies, growth, export, settings, calibration
//  (These would be fully implemented components in a production app)
// ====================================================================

function StudiesScreen({ navigation }) {
  return (
    <StudyManager navigation={navigation} />
  );
}

function GrowthScreen({ navigation }) {
  return (
    <GrowthTracker navigation={navigation} />
  );
}

function ExportScreen({ navigation }) {
  return (
    <ExportView navigation={navigation} />
  );
}

function SettingsScreen({ navigation }) {
  return (
    <SettingsView navigation={navigation} />
  );
}

function CalibrationScreen({ navigation }) {
  return (
    <CalibrationView navigation={navigation} />
  );
}

// ====================================================================
//  Styles
// ====================================================================

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: Colors.bg,
    padding: 16,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 20,
    marginTop: 10,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: Colors.text,
  },
  subtitle: {
    fontSize: 14,
    color: Colors.textDim,
    marginTop: 4,
  },
  author: {
    fontSize: 12,
    color: Colors.textDim,
    marginTop: 2,
  },
  card: {
    backgroundColor: Colors.card,
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: Colors.text,
    marginBottom: 12,
  },
  aboutText: {
    fontSize: 13,
    color: Colors.textDim,
    lineHeight: 20,
    marginBottom: 8,
  },
  copyText: {
    fontSize: 11,
    color: Colors.textDim,
    textAlign: 'center',
    marginTop: 8,
  },
  statusText: {
    fontSize: 14,
    color: Colors.accent2,
    marginBottom: 8,
  },
  deviceName: {
    fontSize: 16,
    color: Colors.text,
    fontWeight: '600',
    marginBottom: 12,
  },
  deviceItem: {
    marginVertical: 12,
  },
  button: {
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    marginTop: 8,
  },
  primaryButton: {
    backgroundColor: Colors.accent,
  },
  secondaryButton: {
    backgroundColor: Colors.card,
    borderWidth: 1,
    borderColor: Colors.accent,
  },
  dangerButton: {
    backgroundColor: Colors.danger,
  },
  buttonText: {
    color: Colors.text,
    fontSize: 14,
    fontWeight: '600',
  },
  link: {
    color: Colors.accent,
    fontSize: 14,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    backgroundColor: Colors.card,
    borderRadius: 12,
    padding: 12,
    marginBottom: 12,
  },
  statusItem: {
    alignItems: 'center',
  },
  statusLabel: {
    fontSize: 10,
    color: Colors.textDim,
    marginBottom: 4,
  },
  statusValue: {
    fontSize: 14,
    color: Colors.text,
    fontWeight: '600',
  },
  envRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  envItem: {
    alignItems: 'center',
  },
  envLabel: {
    fontSize: 10,
    color: Colors.textDim,
  },
  envValue: {
    fontSize: 16,
    color: Colors.text,
    fontWeight: '600',
    marginTop: 4,
  },
  freqRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  freqButton: {
    flex: 1,
    padding: 10,
    borderRadius: 8,
    backgroundColor: Colors.bg,
    marginHorizontal: 4,
    alignItems: 'center',
  },
  freqButtonActive: {
    backgroundColor: Colors.accent,
  },
  freqButtonText: {
    fontSize: 12,
    color: Colors.textDim,
  },
  freqButtonTextActive: {
    color: Colors.text,
    fontWeight: 'bold',
  },
  slider: {
    width: '100%',
    height: 40,
  },
  sliderValue: {
    fontSize: 18,
    color: Colors.accent,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  note: {
    fontSize: 11,
    color: Colors.textDim,
    marginTop: 4,
  },
  controlRow: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  autoFrameRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginVertical: 6,
  },
  label: {
    fontSize: 14,
    color: Colors.textDim,
  },
  autoScanRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: 16,
  },
  navRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  navButton: {
    flex: 1,
    marginHorizontal: 4,
    alignItems: 'center',
    padding: 16,
  },
  navButtonText: {
    fontSize: 12,
    color: Colors.text,
  },
  footer: {
    paddingVertical: 20,
  },
});

// ====================================================================
//  Stack Navigator
// ====================================================================

const Stack = createStackNavigator();

function AppStack() {
  return (
    <Stack.Navigator
      screenOptions={{
        headerStyle: { backgroundColor: Colors.card },
        headerTintColor: Colors.text,
        headerTitleStyle: { fontWeight: 'bold' },
      }}>
      <Stack.Screen
        name="Connection"
        component={ConnectionScreen}
        options={{ title: 'RootTrace' }}
      />
      <Stack.Screen
        name="Scan"
        component={ScanScreen}
        options={{ title: 'Live Scan' }}
      />
      <Stack.Screen
        name="Studies"
        component={StudiesScreen}
        options={{ title: 'Studies' }}
      />
      <Stack.Screen
        name="Growth"
        component={GrowthScreen}
        options={{ title: 'Growth Tracking' }}
      />
      <Stack.Screen
        name="Export"
        component={ExportScreen}
        options={{ title: 'Data Export' }}
      />
      <Stack.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings' }}
      />
      <Stack.Screen
        name="Calibration"
        component={CalibrationScreen}
        options={{ title: 'Calibration' }}
      />
    </Stack.Navigator>
  );
}

// ====================================================================
//  Root App
// ====================================================================

export default function App() {
  return (
    <BleProvider>
      <NavigationContainer>
        <AppStack />
      </NavigationContainer>
    </BleProvider>
  );
}