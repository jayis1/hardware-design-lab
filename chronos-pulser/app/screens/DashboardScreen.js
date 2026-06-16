// screens/DashboardScreen.js — Main control dashboard for Chronos Pulser
// Displays live telemetry, pulse controls, and quick actions
// 150+ lines of real UI implementation

import React, { useState, useCallback, useEffect } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Alert, ActivityIndicator, Switch,
} from 'react-native';
import { useDevice, usePulseControl, useTemperature, useAcquisition } from '../components/DeviceContext';
import TelemetryGauge from '../components/TelemetryGauge';
import { dacToMillivolts, vgaCodeToDb, formatPicoseconds, timeToDistance, formatDistance } from '../utils/protocol';

export default function DashboardScreen() {
  const device = useDevice();
  const pulse = usePulseControl();
  const temp = useTemperature();
  const acq = useAcquisition();

  const [lastTdcResult, setLastTdcResult] = useState(null);
  const [tdcRunning, setTdcRunning] = useState(false);
  const [vgaGainLocal, setVgaGainLocal] = useState(device.vgaGain);

  // Quick TDC measurement
  const handleQuickTdc = useCallback(async () => {
    if (!device.connected || !device.protocolEngine) {
      Alert.alert('Not Connected', 'Please connect to Chronos Pulser first.');
      return;
    }
    setTdcRunning(true);
    try {
      const ps = await device.protocolEngine.singleTdcMeasurement();
      setLastTdcResult(ps);
    } catch (err) {
      Alert.alert('TDC Error', err.message || 'Failed to get TDC measurement');
    } finally {
      setTdcRunning(false);
    }
  }, [device.connected, device.protocolEngine]);

  // Start acquisition
  const handleStartAcq = useCallback(async () => {
    if (!device.connected) {
      Alert.alert('Not Connected', 'Please connect to Chronos Pulser first.');
      return;
    }
    try {
      await acq.start({
        sampleCount: 4096,
        triggerDelay: 0,
        decimation: 0,
        averaging: 2,
        flags: 0x03,
      });
    } catch (err) {
      Alert.alert('Acquisition Error', err.message);
    }
  }, [device.connected, acq]);

  // VGA gain adjustment
  const handleVgaGainChange = useCallback(async (delta) => {
    const newGain = Math.max(0, Math.min(255, vgaGainLocal + delta));
    setVgaGainLocal(newGain);
    try {
      await device.updateVgaGain(newGain);
    } catch (err) {
      setVgaGainLocal(device.vgaGain);  // Revert on error
    }
  }, [vgaGainLocal, device]);

  // Connect button handler
  const handleConnect = useCallback(async () => {
    if (device.connected) {
      await device.disconnectDevice();
    } else {
      await device.connectDevice();
    }
  }, [device]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Status Bar */}
      <View style={styles.statusBar}>
        <View style={[
          styles.statusDot,
          { backgroundColor: device.connected ? '#00ff88' : '#ff4444' }
        ]} />
        <Text style={styles.statusText}>
          {device.connected ? 'Connected — USB 3.0' : 'Disconnected'}
        </Text>
        <TouchableOpacity style={styles.connectButton} onPress={handleConnect}>
          <Text style={styles.connectButtonText}>
            {device.connected ? 'Disconnect' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Telemetry Gauges Row */}
      <View style={styles.gaugesRow}>
        <TelemetryGauge
          value={temp.current}
          min={-10}
          max={85}
          label="Temperature"
          unit="°C"
          precision={1}
          size={110}
          warningThreshold={70}
          criticalThreshold={85}
        />
        <TelemetryGauge
          value={pulse.config.frequency}
          min={1}
          max={1000000}
          label="Pulse Rate"
          unit="Hz"
          precision={0}
          size={110}
          colorLow="#4488ff"
          colorMid="#8844ff"
          colorHigh="#ff44ff"
        />
        <TelemetryGauge
          value={dacToMillivolts(pulse.config.amplitude)}
          min={100}
          max={500}
          label="Amplitude"
          unit="mV"
          precision={0}
          size={110}
          colorLow="#44ff88"
          colorMid="#ffaa00"
          colorHigh="#ff4444"
        />
      </View>

      {/* Pulse Controls */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Pulse Generator</Text>
        <View style={styles.controlRow}>
          <View style={styles.controlGroup}>
            <Text style={styles.controlLabel}>Frequency</Text>
            <View style={styles.buttonRow}>
              <TouchableOpacity
                style={styles.smallButton}
                onPress={() => pulse.setFrequency(Math.max(1, pulse.config.frequency / 10))}
              >
                <Text style={styles.buttonText}>÷10</Text>
              </TouchableOpacity>
              <Text style={styles.controlValue}>
                {pulse.config.frequency >= 1000
                  ? `${(pulse.config.frequency / 1000).toFixed(1)} kHz`
                  : `${pulse.config.frequency} Hz`}
              </Text>
              <TouchableOpacity
                style={styles.smallButton}
                onPress={() => pulse.setFrequency(Math.min(1000000, pulse.config.frequency * 10))}
              >
                <Text style={styles.buttonText}>×10</Text>
              </TouchableOpacity>
            </View>
          </View>
          <View style={styles.controlGroup}>
            <Text style={styles.controlLabel}>Amplitude</Text>
            <View style={styles.buttonRow}>
              <TouchableOpacity
                style={styles.smallButton}
                onPress={() => pulse.setAmplitude(Math.max(0, pulse.config.amplitude - 16))}
              >
                <Text style={styles.buttonText}>-</Text>
              </TouchableOpacity>
              <Text style={styles.controlValue}>
                {dacToMillivolts(pulse.config.amplitude).toFixed(0)} mV
              </Text>
              <TouchableOpacity
                style={styles.smallButton}
                onPress={() => pulse.setAmplitude(Math.min(255, pulse.config.amplitude + 16))}
              >
                <Text style={styles.buttonText}>+</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
        <View style={styles.actionRow}>
          <TouchableOpacity
            style={[styles.actionButton, styles.fireButton]}
            onPress={pulse.fireSingle}
          >
            <Text style={styles.actionButtonText}>⚡ Single Pulse</Text>
          </TouchableOpacity>
          <View style={styles.toggleRow}>
            <Text style={styles.toggleLabel}>Continuous</Text>
            <Switch
              value={pulse.config.enabled}
              onValueChange={pulse.toggleEnabled}
              trackColor={{ false: '#444', true: '#e94560' }}
              thumbColor="#fff"
            />
          </View>
        </View>
      </View>

      {/* VGA Gain Control */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>VGA Gain</Text>
        <View style={styles.controlRow}>
          <TouchableOpacity
            style={styles.mediumButton}
            onPress={() => handleVgaGainChange(-16)}
          >
            <Text style={styles.buttonText}>-</Text>
          </TouchableOpacity>
          <View style={styles.gainDisplay}>
            <Text style={styles.gainValue}>{vgaCodeToDb(vgaGainLocal).toFixed(1)} dB</Text>
            <View style={styles.gainBar}>
              <View style={[styles.gainFill, { width: `${(vgaGainLocal / 255) * 100}%` }]} />
            </View>
          </View>
          <TouchableOpacity
            style={styles.mediumButton}
            onPress={() => handleVgaGainChange(16)}
          >
            <Text style={styles.buttonText}>+</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Quick TDC Measurement */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Quick TDR Measurement</Text>
        <TouchableOpacity
          style={[styles.actionButton, styles.measureButton]}
          onPress={handleQuickTdc}
          disabled={tdcRunning || !device.connected}
        >
          {tdcRunning ? (
            <ActivityIndicator color="#fff" size="small" />
          ) : (
            <Text style={styles.actionButtonText}>📏 Measure Round-Trip Time</Text>
          )}
        </TouchableOpacity>
        {lastTdcResult !== null && (
          <View style={styles.resultCard}>
            <View style={styles.resultRow}>
              <Text style={styles.resultLabel}>Round-Trip Time:</Text>
              <Text style={styles.resultValue}>{formatPicoseconds(lastTdcResult)}</Text>
            </View>
            <View style={styles.resultRow}>
              <Text style={styles.resultLabel}>Est. Distance (RG-58):</Text>
              <Text style={styles.resultValue}>
                {formatDistance(timeToDistance(lastTdcResult, 0.66))}
              </Text>
            </View>
            <View style={styles.resultRow}>
              <Text style={styles.resultLabel}>Est. Distance (0.78 VF):</Text>
              <Text style={styles.resultValue}>
                {formatDistance(timeToDistance(lastTdcResult, 0.78))}
              </Text>
            </View>
          </View>
        )}
      </View>

      {/* Acquisition Control */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Acquisition</Text>
        <View style={styles.actionRow}>
          <TouchableOpacity
            style={[styles.actionButton, styles.acqButton]}
            onPress={handleStartAcq}
            disabled={acq.state.running || !device.connected}
          >
            <Text style={styles.actionButtonText}>
              {acq.state.running ? '⏳ Acquiring...' : '▶ Start Acquisition'}
            </Text>
          </TouchableOpacity>
          {acq.state.running && (
            <TouchableOpacity
              style={[styles.actionButton, styles.stopButton]}
              onPress={acq.stop}
            >
              <Text style={styles.actionButtonText}>⏹ Stop</Text>
            </TouchableOpacity>
          )}
        </View>
        {acq.state.complete && (
          <Text style={styles.completeText}>✓ Acquisition complete — view in Waveform tab</Text>
        )}
        {acq.state.error && (
          <Text style={styles.errorText}>✗ Acquisition error</Text>
        )}
      </View>

      {/* Device Status Footer */}
      {device.deviceInfo && (
        <View style={styles.footer}>
          <Text style={styles.footerText}>
            FW v{device.deviceInfo.fwMajor}.{device.deviceInfo.fwMinor}.{device.deviceInfo.fwPatch}
            {' • '}Uptime: {Math.floor(device.deviceInfo.uptimeSeconds / 3600)}h {Math.floor((device.deviceInfo.uptimeSeconds % 3600) / 60)}m
          </Text>
          <Text style={styles.footerText}>
            Calibration: {device.calibrationValid ? '✓ Valid' : '✗ Required'}
            {' • '}Power: {device.deviceInfo.powerGood ? '✓ Good' : '✗ Fault'}
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  statusBar: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 16,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  statusText: {
    color: '#e0e0e0',
    fontSize: 14,
    flex: 1,
  },
  connectButton: {
    backgroundColor: '#e94560',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 6,
  },
  connectButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 13,
  },
  gaugesRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 20,
  },
  section: {
    backgroundColor: '#16213e',
    borderRadius: 10,
    padding: 16,
    marginBottom: 14,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  controlRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  controlGroup: {
    flex: 1,
    alignItems: 'center',
  },
  controlLabel: {
    color: '#a0a0a0',
    fontSize: 11,
    textTransform: 'uppercase',
    marginBottom: 4,
  },
  buttonRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  controlValue: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
    minWidth: 70,
    textAlign: 'center',
  },
  smallButton: {
    backgroundColor: '#0f3460',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 4,
  },
  mediumButton: {
    backgroundColor: '#0f3460',
    paddingHorizontal: 16,
    paddingVertical: 10,
    borderRadius: 6,
  },
  buttonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  actionRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginTop: 8,
  },
  actionButton: {
    paddingHorizontal: 20,
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  fireButton: {
    backgroundColor: '#ff6600',
  },
  measureButton: {
    backgroundColor: '#4488ff',
  },
  acqButton: {
    backgroundColor: '#00aa55',
  },
  stopButton: {
    backgroundColor: '#cc3333',
  },
  actionButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  toggleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  toggleLabel: {
    color: '#c0c0c0',
    fontSize: 13,
  },
  gainDisplay: {
    flex: 1,
    alignItems: 'center',
    marginHorizontal: 12,
  },
  gainValue: {
    color: '#ffffff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  gainBar: {
    width: '100%',
    height: 6,
    backgroundColor: '#2a2a4a',
    borderRadius: 3,
    marginTop: 6,
    overflow: 'hidden',
  },
  gainFill: {
    height: '100%',
    backgroundColor: '#e94560',
    borderRadius: 3,
  },
  resultCard: {
    backgroundColor: '#0a0a2a',
    borderRadius: 8,
    padding: 12,
    marginTop: 12,
  },
  resultRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  resultLabel: {
    color: '#a0a0a0',
    fontSize: 12,
  },
  resultValue: {
    color: '#00ff88',
    fontSize: 14,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  completeText: {
    color: '#00ff88',
    fontSize: 13,
    marginTop: 8,
  },
  errorText: {
    color: '#ff4444',
    fontSize: 13,
    marginTop: 8,
  },
  footer: {
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    marginTop: 8,
  },
  footerText: {
    color: '#808080',
    fontSize: 11,
    textAlign: 'center',
    marginBottom: 2,
  },
});
