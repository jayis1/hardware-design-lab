/**
 * SpectrumScreen.js — Main spectrum analyzer display and controls
 * Shows live spectrum/waterfall with frequency/dB controls
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Dimensions,
} from 'react-native';
import { useBle } from '../components/BleContext';
import { theme } from '../components/Theme';
import SpectrumView from '../components/SpectrumView';
import {
  CMD_START_SWEEP,
  CMD_STOP_SWEEP,
  CMD_SET_FREQ,
  CMD_SET_FFT_SIZE,
  CMD_SET_WINDOW,
  CMD_SET_ATTEN,
  CMD_GET_PEAKS,
} from '../utils/protocol';

const FFT_SIZES = [256, 512, 1024, 2048, 4096, 8192];
const WINDOW_TYPES = ['Rectangular', 'Hann', 'Blackman-Harris', 'Flat-Top', 'Kaiser'];
const ATTEN_LEVELS = [0, 10, 20, 30];
const FREQ_PRESETS = [
  { label: 'FM', start: 88e6, stop: 108e6 },
  { label: 'AM', start: 530e3, stop: 1.7e6 },
  { label: '2.4G WiFi', start: 2.4e9, stop: 2.5e9 },
  { label: '5G WiFi', start: 5.15e9, stop: 5.85e9 },
  { label: 'Cellular', start: 700e6, stop: 2.6e9 },
  { label: 'ISM 433', start: 433e6, stop: 435e6 },
  { label: 'Full Band', start: 100e3, stop: 6e9 },
];

function SpectrumScreen() {
  const { isConnected, spectrumData, deviceStatus, sendCommand } = useBle();

  const [fftSizeIndex, setFftSizeIndex] = useState(2); // Default 1024
  const [windowIndex, setWindowIndex] = useState(1); // Default Hann
  const [attenIndex, setAttenIndex] = useState(0); // Default 0 dB
  const [isSweeping, setIsSweeping] = useState(false);
  const [showControls, setShowControls] = useState(false);

  // Start sweep
  const handleStart = useCallback(() => {
    sendCommand(CMD_START_SWEEP);
    setIsSweeping(true);
  }, [sendCommand]);

  // Stop sweep
  const handleStop = useCallback(() => {
    sendCommand(CMD_STOP_SWEEP);
    setIsSweeping(false);
  }, [sendCommand]);

  // Set frequency range
  const handleFreqPreset = useCallback(
    (preset) => {
      const payload = new Uint8Array(8);
      const view = new DataView(payload.buffer);
      view.setBigUint64(0, BigInt(preset.start), true); // little-endian
      const payload2 = new Uint8Array(8);
      const view2 = new DataView(payload2.buffer);
      view2.setBigUint64(0, BigInt(preset.stop), true);

      // Send start freq
      sendCommand(CMD_SET_FREQ, payload);
      // Send stop freq (encoded differently in full protocol)
    },
    [sendCommand]
  );

  // Set FFT size
  const handleFftSize = useCallback(() => {
    const nextIndex = (fftSizeIndex + 1) % FFT_SIZES.length;
    setFftSizeIndex(nextIndex);
    const payload = new Uint8Array(4);
    const view = new DataView(payload.buffer);
    view.setUint32(0, FFT_SIZES[nextIndex], true);
    sendCommand(CMD_SET_FFT_SIZE, payload);
  }, [fftSizeIndex, sendCommand]);

  // Set window type
  const handleWindow = useCallback(() => {
    const nextIndex = (windowIndex + 1) % WINDOW_TYPES.length;
    setWindowIndex(nextIndex);
    const payload = new Uint8Array(1);
    payload[0] = nextIndex;
    sendCommand(CMD_SET_WINDOW, payload);
  }, [windowIndex, sendCommand]);

  // Set attenuation
  const handleAtten = useCallback(() => {
    const nextIndex = (attenIndex + 1) % ATTEN_LEVELS.length;
    setAttenIndex(nextIndex);
    const payload = new Uint8Array(1);
    payload[0] = ATTEN_LEVELS[nextIndex];
    sendCommand(CMD_SET_ATTEN, payload);
  }, [attenIndex, sendCommand]);

  // Get peaks
  const handlePeaks = useCallback(() => {
    sendCommand(CMD_GET_PEAKS);
  }, [sendCommand]);

  // Format frequency for display
  const formatFreq = (hz) => {
    if (!hz) return '—';
    if (hz >= 1e9) return `${(hz / 1e9).toFixed(3)} GHz`;
    if (hz >= 1e6) return `${(hz / 1e6).toFixed(3)} MHz`;
    if (hz >= 1e3) return `${(hz / 1e3).toFixed(1)} kHz`;
    return `${hz} Hz`;
  };

  return (
    <View style={styles.container}>
      {/* Spectrum display */}
      <View style={styles.spectrumContainer}>
        <SpectrumView
          spectrumData={spectrumData}
          deviceStatus={deviceStatus}
        />
      </View>

      {/* Info bar */}
      <View style={styles.infoBar}>
        <View style={styles.infoItem}>
          <Text style={styles.infoLabel}>Center</Text>
          <Text style={styles.infoValue}>
            {formatFreq(
              deviceStatus.startFreq
                ? (deviceStatus.startFreq + deviceStatus.stopFreq) / 2
                : null
            )}
          </Text>
        </View>
        <View style={styles.infoItem}>
          <Text style={styles.infoLabel}>Span</Text>
          <Text style={styles.infoValue}>
            {formatFreq(
              deviceStatus.stopFreq && deviceStatus.startFreq
                ? deviceStatus.stopFreq - deviceStatus.startFreq
                : null
            )}
          </Text>
        </View>
        <View style={styles.infoItem}>
          <Text style={styles.infoLabel}>FFT</Text>
          <Text style={styles.infoValue}>{FFT_SIZES[fftSizeIndex]}</Text>
        </View>
        <View style={styles.infoItem}>
          <Text style={styles.infoLabel}>Window</Text>
          <Text style={styles.infoValue}>{WINDOW_TYPES[windowIndex]}</Text>
        </View>
        <View style={styles.infoItem}>
          <Text style={styles.infoLabel}>Atten</Text>
          <Text style={styles.infoValue}>{ATTEN_LEVELS[attenIndex]} dB</Text>
        </View>
      </View>

      {/* Quick controls */}
      <View style={styles.controlsRow}>
        <TouchableOpacity
          style={[
            styles.controlButton,
            isSweeping ? styles.controlButtonActive : styles.controlButtonInactive,
            !isConnected && styles.controlButtonDisabled,
          ]}
          onPress={isSweeping ? handleStop : handleStart}
          disabled={!isConnected}
        >
          <Text style={styles.controlButtonText}>
            {isSweeping ? '■ STOP' : '▶ START'}
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.controlButton, styles.controlButtonSecondary]}
          onPress={handleFftSize}
          disabled={!isConnected}
        >
          <Text style={styles.controlButtonText}>FFT</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.controlButton, styles.controlButtonSecondary]}
          onPress={handleWindow}
          disabled={!isConnected}
        >
          <Text style={styles.controlButtonText}>WIN</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.controlButton, styles.controlButtonSecondary]}
          onPress={handleAtten}
          disabled={!isConnected}
        >
          <Text style={styles.controlButtonText}>ATT</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.controlButton, styles.controlButtonSecondary]}
          onPress={handlePeaks}
          disabled={!isConnected}
        >
          <Text style={styles.controlButtonText}>PEAK</Text>
        </TouchableOpacity>
      </View>

      {/* Frequency presets */}
      <ScrollView
        horizontal
        showsHorizontalScrollIndicator={false}
        style={styles.presetsRow}
        contentContainerStyle={styles.presetsContent}
      >
        {FREQ_PRESETS.map((preset, index) => (
          <TouchableOpacity
            key={index}
            style={styles.presetButton}
            onPress={() => handleFreqPreset(preset)}
            disabled={!isConnected}
          >
            <Text style={styles.presetButtonText}>{preset.label}</Text>
          </TouchableOpacity>
        ))}
      </ScrollView>

      {/* More controls toggle */}
      <TouchableOpacity
        style={styles.moreButton}
        onPress={() => setShowControls(!showControls)}
      >
        <Text style={styles.moreButtonText}>
          {showControls ? '▼ Less Controls' : '▶ More Controls'}
        </Text>
      </TouchableOpacity>

      {/* Extended controls */}
      {showControls && (
        <View style={styles.extendedControls}>
          <Text style={styles.extendedTitle}>Frequency Bands</Text>
          <View style={styles.freqGrid}>
            {FREQ_PRESETS.map((preset, index) => (
              <TouchableOpacity
                key={index}
                style={styles.freqPreset}
                onPress={() => handleFreqPreset(preset)}
                disabled={!isConnected}
              >
                <Text style={styles.freqPresetLabel}>{preset.label}</Text>
                <Text style={styles.freqPresetRange}>
                  {formatFreq(preset.start)} — {formatFreq(preset.stop)}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background,
  },
  spectrumContainer: {
    flex: 1,
  },
  infoBar: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    backgroundColor: theme.colors.surface,
    paddingVertical: theme.spacing.sm,
    paddingHorizontal: theme.spacing.xs,
    borderTopWidth: 1,
    borderBottomWidth: 1,
    borderColor: theme.colors.border,
  },
  infoItem: {
    alignItems: 'center',
  },
  infoLabel: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.textDim,
  },
  infoValue: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.text,
    fontWeight: theme.fontWeight.medium,
  },
  controlsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    alignItems: 'center',
    paddingVertical: theme.spacing.sm,
    backgroundColor: theme.colors.surface,
  },
  controlButton: {
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.sm,
    borderRadius: theme.borderRadius.sm,
    minWidth: 60,
    alignItems: 'center',
  },
  controlButtonActive: {
    backgroundColor: theme.colors.error,
  },
  controlButtonInactive: {
    backgroundColor: theme.colors.primary,
  },
  controlButtonSecondary: {
    backgroundColor: theme.colors.surfaceLight,
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  controlButtonDisabled: {
    opacity: 0.4,
  },
  controlButtonText: {
    color: theme.colors.text,
    fontSize: theme.fontSize.sm,
    fontWeight: theme.fontWeight.bold,
  },
  presetsRow: {
    backgroundColor: theme.colors.surface,
    paddingVertical: theme.spacing.sm,
  },
  presetsContent: {
    paddingHorizontal: theme.spacing.sm,
  },
  presetButton: {
    backgroundColor: theme.colors.surfaceLight,
    borderRadius: theme.borderRadius.sm,
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.sm,
    marginRight: theme.spacing.sm,
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  presetButtonText: {
    color: theme.colors.primary,
    fontSize: theme.fontSize.sm,
    fontWeight: theme.fontWeight.medium,
  },
  moreButton: {
    backgroundColor: theme.colors.surface,
    paddingVertical: theme.spacing.sm,
    alignItems: 'center',
  },
  moreButtonText: {
    color: theme.colors.textSecondary,
    fontSize: theme.fontSize.sm,
  },
  extendedControls: {
    backgroundColor: theme.colors.surface,
    padding: theme.spacing.md,
    borderTopWidth: 1,
    borderColor: theme.colors.border,
  },
  extendedTitle: {
    fontSize: theme.fontSize.md,
    fontWeight: theme.fontWeight.semiBold,
    color: theme.colors.text,
    marginBottom: theme.spacing.sm,
  },
  freqGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  freqPreset: {
    backgroundColor: theme.colors.surfaceLight,
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.md,
    marginBottom: theme.spacing.sm,
    width: '48%',
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  freqPresetLabel: {
    fontSize: theme.fontSize.md,
    fontWeight: theme.fontWeight.bold,
    color: theme.colors.primary,
  },
  freqPresetRange: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.textSecondary,
    marginTop: 2,
  },
});

export default SpectrumScreen;