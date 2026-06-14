/**
 * SettingsScreen.js — Vortex-SDR settings and configuration
 * Sweep parameters, display options, data logging, device info
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  Switch,
  ScrollView,
  StyleSheet,
  Alert,
} from 'react-native';
import { useBle } from '../components/BleContext';
import { theme } from '../components/Theme';
import {
  CMD_SET_FREQ,
  CMD_SET_FFT_SIZE,
  CMD_SET_WINDOW,
  CMD_SET_ATTEN,
  CMD_SET_REF_LEVEL,
  CMD_SET_SCALE,
  CMD_SET_AGC,
  CMD_SET_MODE,
  CMD_LOG_START,
  CMD_LOG_STOP,
  CMD_RESET,
  CMD_GET_STATUS,
} from '../utils/protocol';

const FFT_OPTIONS = [
  { label: '256', value: 256 },
  { label: '512', value: 512 },
  { label: '1024', value: 1024 },
  { label: '2048', value: 2048 },
  { label: '4096', value: 4096 },
  { label: '8192', value: 8192 },
];

const WINDOW_OPTIONS = [
  { label: 'Rectangular', value: 0 },
  { label: 'Hann', value: 1 },
  { label: 'Blackman-Harris', value: 2 },
  { label: 'Flat-Top', value: 3 },
  { label: 'Kaiser', value: 4 },
];

const SWEEP_MODES = [
  { label: 'Continuous', value: 0 },
  { label: 'Single', value: 1 },
  { label: 'Zero Span', value: 2 },
  { label: 'Max Hold', value: 3 },
  { label: 'Average', value: 4 },
];

const REF_LEVELS = [0, -10, -20, -30, -40, -50, -60, -70];
const SCALES = [1, 2, 5, 10, 15, 20]; // dB/div
const ATTEN_LEVELS = [0, 10, 20, 30];

function SettingsScreen() {
  const { isConnected, deviceStatus, sendCommand, disconnectDevice } = useBle();

  const [startFreq, setStartFreq] = useState('88');
  const [stopFreq, setStopFreq] = useState('108');
  const [freqUnit, setFreqUnit] = useState('MHz'); // Hz, kHz, MHz, GHz
  const [fftIndex, setFftIndex] = useState(2);
  const [windowIndex, setWindowIndex] = useState(1);
  const [sweepMode, setSweepMode] = useState(0);
  const [attenIndex, setAttenIndex] = useState(0);
  const [refLevelIndex, setRefLevelIndex] = useState(2); // -20 dBm
  const [scaleIndex, setScaleIndex] = useState(3); // 10 dB/div
  const [isLogging, setIsLogging] = useState(false);
  const [waterfallEnabled, setWaterfallEnabled] = useState(true);
  const [peakMarkersEnabled, setPeakMarkersEnabled] = useState(true);

  // Convert user input to Hz
  const parseFreqToHz = useCallback((value, unit) => {
    const num = parseFloat(value);
    if (isNaN(num)) return 0;
    switch (unit) {
      case 'Hz': return num;
      case 'kHz': return num * 1e3;
      case 'MHz': return num * 1e6;
      case 'GHz': return num * 1e9;
      default: return num * 1e6;
    }
  }, []);

  // Apply frequency range
  const handleApplyFreq = useCallback(() => {
    if (!isConnected) return;

    const startHz = parseFreqToHz(startFreq, freqUnit);
    const stopHz = parseFreqToHz(stopFreq, freqUnit);

    if (startHz >= stopHz) {
      Alert.alert('Invalid Range', 'Start frequency must be less than stop frequency.');
      return;
    }

    if (startHz < 100000 || stopHz > 6000000000) {
      Alert.alert('Out of Range', 'Frequency range: 100 kHz — 6 GHz');
      return;
    }

    // Build payload: start_freq (4 bytes) + stop_freq (4 bytes)
    const payload = new Uint8Array(8);
    const view = new DataView(payload.buffer);
    view.setUint32(0, startHz / 1, true); // Start frequency in Hz (low 32 bits)
    view.setUint32(4, stopHz / 1, true);  // Stop frequency in Hz (low 32 bits)
    sendCommand(CMD_SET_FREQ, payload);
  }, [startFreq, stopFreq, freqUnit, isConnected, sendCommand]);

  // Apply FFT size
  const handleApplyFft = useCallback(() => {
    if (!isConnected) return;
    const payload = new Uint8Array(4);
    const view = new DataView(payload.buffer);
    view.setUint32(0, FFT_OPTIONS[fftIndex].value, true);
    sendCommand(CMD_SET_FFT_SIZE, payload);
  }, [fftIndex, isConnected, sendCommand]);

  // Apply window type
  const handleApplyWindow = useCallback(() => {
    if (!isConnected) return;
    const payload = new Uint8Array(1);
    payload[0] = WINDOW_OPTIONS[windowIndex].value;
    sendCommand(CMD_SET_WINDOW, payload);
  }, [windowIndex, isConnected, sendCommand]);

  // Apply sweep mode
  const handleApplyMode = useCallback(() => {
    if (!isConnected) return;
    const payload = new Uint8Array(1);
    payload[0] = SWEEP_MODES[sweepMode].value;
    sendCommand(CMD_SET_MODE, payload);
  }, [sweepMode, isConnected, sendCommand]);

  // Apply attenuation
  const handleApplyAtten = useCallback(() => {
    if (!isConnected) return;
    const payload = new Uint8Array(1);
    payload[0] = ATTEN_LEVELS[attenIndex];
    sendCommand(CMD_SET_ATTEN, payload);
  }, [attenIndex, isConnected, sendCommand]);

  // Toggle data logging
  const handleToggleLogging = useCallback(() => {
    if (!isConnected) return;
    if (isLogging) {
      sendCommand(CMD_LOG_STOP);
      setIsLogging(false);
    } else {
      sendCommand(CMD_LOG_START);
      setIsLogging(true);
    }
  }, [isLogging, isConnected, sendCommand]);

  // Factory reset
  const handleReset = useCallback(() => {
    Alert.alert(
      'Factory Reset',
      'Are you sure you want to reset the Vortex-SDR to factory defaults?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: () => {
            sendCommand(CMD_RESET);
          },
        },
      ]
    );
  }, [sendCommand]);

  // Request status
  const handleRefresh = useCallback(() => {
    if (!isConnected) return;
    sendCommand(CMD_GET_STATUS);
  }, [isConnected, sendCommand]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection warning */}
      {!isConnected && (
        <View style={styles.warningBanner}>
          <Text style={styles.warningText}>
            Not connected to Vortex-SDR. Connect a device to change settings.
          </Text>
        </View>
      )}

      {/* Frequency Range Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Frequency Range</Text>
        <View style={styles.row}>
          <View style={styles.inputGroup}>
            <Text style={styles.label}>Start</Text>
            <TextInput
              style={styles.input}
              value={startFreq}
              onChangeText={setStartFreq}
              keyboardType="decimal-pad"
              placeholder="88"
              placeholderTextColor={theme.colors.textDim}
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.label}>Stop</Text>
            <TextInput
              style={styles.input}
              value={stopFreq}
              onChangeText={setStopFreq}
              keyboardType="decimal-pad"
              placeholder="108"
              placeholderTextColor={theme.colors.textDim}
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.label}>Unit</Text>
            <View style={styles.unitSelector}>
              {['kHz', 'MHz', 'GHz'].map((unit) => (
                <TouchableOpacity
                  key={unit}
                  style={[
                    styles.unitButton,
                    freqUnit === unit && styles.unitButtonActive,
                  ]}
                  onPress={() => setFreqUnit(unit)}
                >
                  <Text
                    style={[
                      styles.unitButtonText,
                      freqUnit === unit && styles.unitButtonTextActive,
                    ]}
                  >
                    {unit}
                  </Text>
                </TouchableOpacity>
              ))}
            </View>
          </View>
        </View>
        <TouchableOpacity
          style={[styles.applyButton, !isConnected && styles.buttonDisabled]}
          onPress={handleApplyFreq}
          disabled={!isConnected}
        >
          <Text style={styles.applyButtonText}>Apply Frequency</Text>
        </TouchableOpacity>
      </View>

      {/* FFT Settings Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>FFT Settings</Text>

        <Text style={styles.subLabel}>FFT Size</Text>
        <View style={styles.optionRow}>
          {FFT_OPTIONS.map((opt, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.optionButton,
                fftIndex === i && styles.optionButtonActive,
              ]}
              onPress={() => setFftIndex(i)}
            >
              <Text
                style={[
                  styles.optionButtonText,
                  fftIndex === i && styles.optionButtonTextActive,
                ]}
              >
                {opt.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.subLabel}>Window Function</Text>
        <View style={styles.optionRow}>
          {WINDOW_OPTIONS.map((opt, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.optionButton,
                windowIndex === i && styles.optionButtonActive,
              ]}
              onPress={() => setWindowIndex(i)}
            >
              <Text
                style={[
                  styles.optionButtonText,
                  windowIndex === i && styles.optionButtonTextActive,
                ]}
              >
                {opt.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <TouchableOpacity
          style={[styles.applyButton, !isConnected && styles.buttonDisabled]}
          onPress={() => {
            handleApplyFft();
            handleApplyWindow();
          }}
          disabled={!isConnected}
        >
          <Text style={styles.applyButtonText}>Apply FFT Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Sweep Mode Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Sweep Mode</Text>
        <View style={styles.optionRow}>
          {SWEEP_MODES.map((mode, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.optionButton,
                sweepMode === i && styles.optionButtonActive,
              ]}
              onPress={() => setSweepMode(i)}
            >
              <Text
                style={[
                  styles.optionButtonText,
                  sweepMode === i && styles.optionButtonTextActive,
                ]}
              >
                {mode.label}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.subLabel}>Attenuation</Text>
        <View style={styles.optionRow}>
          {ATTEN_LEVELS.map((level, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.optionButton,
                attenIndex === i && styles.optionButtonActive,
              ]}
              onPress={() => setAttenIndex(i)}
            >
              <Text
                style={[
                  styles.optionButtonText,
                  attenIndex === i && styles.optionButtonTextActive,
                ]}
              >
                {level} dB
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <TouchableOpacity
          style={[styles.applyButton, !isConnected && styles.buttonDisabled]}
          onPress={() => {
            handleApplyMode();
            handleApplyAtten();
          }}
          disabled={!isConnected}
        >
          <Text style={styles.applyButtonText}>Apply Sweep Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Reference Level Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Reference Level</Text>
        <Text style={styles.subLabel}>Top of Screen (dBm)</Text>
        <View style={styles.optionRow}>
          {REF_LEVELS.map((level, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.optionButton,
                refLevelIndex === i && styles.optionButtonActive,
              ]}
              onPress={() => setRefLevelIndex(i)}
            >
              <Text
                style={[
                  styles.optionButtonText,
                  refLevelIndex === i && styles.optionButtonTextActive,
                ]}
              >
                {level}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.subLabel}>Scale (dB/div)</Text>
        <View style={styles.optionRow}>
          {SCALES.map((scale, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.optionButton,
                scaleIndex === i && styles.optionButtonActive,
              ]}
              onPress={() => setScaleIndex(i)}
            >
              <Text
                style={[
                  styles.optionButtonText,
                  scaleIndex === i && styles.optionButtonTextActive,
                ]}
              >
                {scale} dB
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Display Options Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Waterfall Display</Text>
          <Switch
            value={waterfallEnabled}
            onValueChange={setWaterfallEnabled}
            trackColor={{ false: theme.colors.border, true: theme.colors.primary }}
            thumbColor={waterfallEnabled ? '#FFFFFF' : theme.colors.textDim}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Peak Markers</Text>
          <Switch
            value={peakMarkersEnabled}
            onValueChange={setPeakMarkersEnabled}
            trackColor={{ false: theme.colors.border, true: theme.colors.primary }}
            thumbColor={peakMarkersEnabled ? '#FFFFFF' : theme.colors.textDim}
          />
        </View>
      </View>

      {/* Data Logging Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Logging</Text>
        <TouchableOpacity
          style={[
            styles.applyButton,
            isLogging ? styles.logButtonActive : null,
            !isConnected && styles.buttonDisabled,
          ]}
          onPress={handleToggleLogging}
          disabled={!isConnected}
        >
          <Text style={styles.applyButtonText}>
            {isLogging ? '■ Stop Logging' : '● Start Logging'}
          </Text>
        </TouchableOpacity>
        <Text style={styles.helpText}>
          {isLogging
            ? 'Sweep data is being logged to the Vortex-SDR flash storage.'
            : 'Log sweep data to the device flash storage for later download.'}
        </Text>
      </View>

      {/* Device Info Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Information</Text>
        <View style={styles.infoGrid}>
          <Text style={styles.infoLabel}>Firmware:</Text>
          <Text style={styles.infoValue}>v1.0.0</Text>

          <Text style={styles.infoLabel}>Hardware:</Text>
          <Text style={styles.infoValue}>Rev 1.0</Text>

          <Text style={styles.infoLabel}>MCU:</Text>
          <Text style={styles.infoValue}>STM32H743</Text>

          <Text style={styles.infoLabel}>FPGA:</Text>
          <Text style={styles.infoValue}>iCE40UP5K</Text>

          <Text style={styles.infoLabel}>Frequency:</Text>
          <Text style={styles.infoValue}>100 kHz – 6 GHz</Text>

          <Text style={styles.infoLabel}>ADC:</Text>
          <Text style={styles.infoValue}>AD9645 61.44 MSPS</Text>

          <Text style={styles.infoLabel}>PLL:</Text>
          <Text style={styles.infoValue}>ADF4351 35 MHz – 4.4 GHz</Text>

          <Text style={styles.infoLabel}>Display:</Text>
          <Text style={styles.infoValue}>ILI9341 2.8" 320×240</Text>
        </View>

        <TouchableOpacity
          style={[styles.applyButton, !isConnected && styles.buttonDisabled]}
          onPress={handleRefresh}
          disabled={!isConnected}
        >
          <Text style={styles.applyButtonText}>Refresh Status</Text>
        </TouchableOpacity>
      </View>

      {/* Danger Zone */}
      <View style={[styles.section, styles.dangerZone]}>
        <Text style={[styles.sectionTitle, styles.dangerTitle]}>Danger Zone</Text>
        <TouchableOpacity
          style={styles.dangerButton}
          onPress={handleReset}
        >
          <Text style={styles.dangerButtonText}>Factory Reset</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.dangerButton}
          onPress={disconnectDevice}
        >
          <Text style={styles.dangerButtonText}>Disconnect Device</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background,
  },
  content: {
    padding: theme.spacing.md,
    paddingBottom: 40,
  },
  warningBanner: {
    backgroundColor: theme.colors.error + '20',
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.md,
    marginBottom: theme.spacing.md,
    borderWidth: 1,
    borderColor: theme.colors.error + '40',
  },
  warningText: {
    color: theme.colors.error,
    fontSize: theme.fontSize.sm,
    textAlign: 'center',
  },
  section: {
    backgroundColor: theme.colors.surface,
    borderRadius: theme.borderRadius.lg,
    padding: theme.spacing.lg,
    marginBottom: theme.spacing.md,
  },
  sectionTitle: {
    fontSize: theme.fontSize.lg,
    fontWeight: theme.fontWeight.bold,
    color: theme.colors.text,
    marginBottom: theme.spacing.md,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: theme.spacing.md,
  },
  inputGroup: {
    flex: 1,
    marginHorizontal: theme.spacing.xs,
  },
  label: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textSecondary,
    marginBottom: theme.spacing.xs,
  },
  input: {
    backgroundColor: theme.colors.background,
    borderRadius: theme.borderRadius.sm,
    padding: theme.spacing.sm,
    color: theme.colors.text,
    fontSize: theme.fontSize.md,
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  unitSelector: {
    flexDirection: 'row',
    marginTop: theme.spacing.xs,
  },
  unitButton: {
    backgroundColor: theme.colors.surfaceLight,
    borderRadius: theme.borderRadius.sm,
    paddingHorizontal: theme.spacing.sm,
    paddingVertical: theme.spacing.xs,
    marginRight: theme.spacing.xs,
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  unitButtonActive: {
    backgroundColor: theme.colors.primary,
    borderColor: theme.colors.primary,
  },
  unitButtonText: {
    color: theme.colors.textSecondary,
    fontSize: theme.fontSize.xs,
  },
  unitButtonTextActive: {
    color: theme.colors.background,
    fontWeight: theme.fontWeight.bold,
  },
  applyButton: {
    backgroundColor: theme.colors.primary,
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.md,
    alignItems: 'center',
    marginTop: theme.spacing.sm,
  },
  applyButtonText: {
    color: theme.colors.background,
    fontSize: theme.fontSize.md,
    fontWeight: theme.fontWeight.bold,
  },
  buttonDisabled: {
    opacity: 0.4,
  },
  subLabel: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textSecondary,
    marginTop: theme.spacing.sm,
    marginBottom: theme.spacing.xs,
  },
  optionRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: theme.spacing.sm,
  },
  optionButton: {
    backgroundColor: theme.colors.surfaceLight,
    borderRadius: theme.borderRadius.sm,
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.sm,
    marginRight: theme.spacing.xs,
    marginBottom: theme.spacing.xs,
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  optionButtonActive: {
    backgroundColor: theme.colors.primary,
    borderColor: theme.colors.primary,
  },
  optionButtonText: {
    color: theme.colors.textSecondary,
    fontSize: theme.fontSize.xs,
  },
  optionButtonTextActive: {
    color: theme.colors.background,
    fontWeight: theme.fontWeight.bold,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: theme.spacing.sm,
  },
  switchLabel: {
    fontSize: theme.fontSize.md,
    color: theme.colors.text,
  },
  logButtonActive: {
    backgroundColor: theme.colors.error,
  },
  helpText: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.textDim,
    marginTop: theme.spacing.sm,
    textAlign: 'center',
  },
  infoGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  infoLabel: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textSecondary,
    width: '40%',
    marginBottom: theme.spacing.xs,
  },
  infoValue: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.text,
    fontWeight: theme.fontWeight.medium,
    width: '60%',
    marginBottom: theme.spacing.xs,
  },
  dangerZone: {
    borderColor: theme.colors.error + '40',
    borderWidth: 1,
  },
  dangerTitle: {
    color: theme.colors.error,
  },
  dangerButton: {
    backgroundColor: theme.colors.error + '20',
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.md,
    alignItems: 'center',
    marginTop: theme.spacing.sm,
    borderWidth: 1,
    borderColor: theme.colors.error + '40',
  },
  dangerButtonText: {
    color: theme.colors.error,
    fontSize: theme.fontSize.md,
    fontWeight: theme.fontWeight.bold,
  },
});

export default SettingsScreen;