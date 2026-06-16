/**
 * ControlScreen.js — Nebula Matrix Main Control Panel
 *
 * Primary user interface for controlling the LED matrix display.
 * Features:
 *   - Input source selection (Ethernet, HDMI, SD Card, Test Pattern)
 *   - Brightness and contrast sliders
 *   - Test pattern selection and preview
 *   - Frame freeze/unfreeze
 *   - Quick actions (blackout, full white, reset)
 *   - Gamma preset selection
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  ActivityIndicator,
  Switch,
  Alert,
} from 'react-native';
import Slider from '@react-native-community/slider';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

/* =========================================================================
 * Constants
 * ========================================================================= */

const INPUT_SOURCES = [
  { id: 0x00, label: 'None (Black)', icon: 'stop-circle', color: '#333' },
  { id: 0x01, label: 'Ethernet', icon: 'ethernet', color: '#00E5FF' },
  { id: 0x02, label: 'HDMI', icon: 'hdmi-port', color: '#4CAF50' },
  { id: 0x03, label: 'SD Card', icon: 'sd', color: '#FF9800' },
  { id: 0x04, label: 'Test Pattern', icon: 'test-tube', color: '#9C27B0' },
];

const TEST_PATTERNS = [
  { id: 0, label: 'Color Bars', desc: 'SMPTE color bars' },
  { id: 1, label: 'Gradient', desc: 'Horizontal RGB gradient' },
  { id: 2, label: 'Grid', desc: '16×16 alignment grid' },
  { id: 3, label: 'White', desc: 'Full white (burn-in test)' },
];

const GAMMA_PRESETS = [
  { id: 0, label: 'Linear (1.0)', value: 1.0 },
  { id: 1, label: 'sRGB (~2.2)', value: 2.2 },
  { id: 2, label: 'Video (2.4)', value: 2.4 },
  { id: 3, label: 'DCI-P3 (2.6)', value: 2.6 },
  { id: 4, label: 'Custom', value: 0 },
];

/* =========================================================================
 * ControlScreen Component
 * ========================================================================= */

const ControlScreen = ({ protocol, connected, deviceInfo }) => {
  /* State */
  const [inputSource, setInputSource] = useState(0x00);
  const [brightness, setBrightness] = useState(65535);
  const [contrast, setContrast] = useState(32768);
  const [testPattern, setTestPattern] = useState(0);
  const [gammaPreset, setGammaPreset] = useState(0);
  const [outputEnabled, setOutputEnabled] = useState(true);
  const [frameFrozen, setFrameFrozen] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [lastCommand, setLastCommand] = useState('');

  /* =====================================================================
   * Command Helpers
   * ===================================================================== */

  const sendCommand = useCallback(async (action, ...args) => {
    if (!connected || !protocol) {
      Alert.alert('Not Connected', 'Please connect to a Nebula Matrix device first.');
      return;
    }

    setIsLoading(true);
    setLastCommand(action);

    try {
      switch (action) {
        case 'setInputSource':
          await protocol.setInputSource(args[0]);
          setInputSource(args[0]);
          break;
        case 'setBrightness':
          await protocol.setBrightness(args[0]);
          setBrightness(args[0]);
          break;
        case 'setContrast':
          await protocol.setContrast(args[0]);
          setContrast(args[0]);
          break;
        case 'setTestPattern':
          await protocol.enableTestPattern(args[0]);
          setTestPattern(args[0]);
          setInputSource(0x04);
          break;
        case 'setGamma':
          await protocol.setGammaPreset(args[0]);
          setGammaPreset(args[0]);
          break;
        case 'toggleOutput':
          await protocol.setOutputEnable(args[0]);
          setOutputEnabled(args[0]);
          break;
        case 'freezeFrame':
          await protocol.freezeFrame(args[0]);
          setFrameFrozen(args[0]);
          break;
        case 'blackout':
          await protocol.setBrightness(0);
          setBrightness(0);
          break;
        case 'fullWhite':
          await protocol.setInputSource(0x04);
          await protocol.enableTestPattern(3);
          await protocol.setBrightness(65535);
          setInputSource(0x04);
          setTestPattern(3);
          setBrightness(65535);
          break;
        case 'reset':
          await protocol.softReset();
          setInputSource(0x00);
          setBrightness(65535);
          setContrast(32768);
          setOutputEnabled(true);
          setFrameFrozen(false);
          break;
        default:
          break;
      }
    } catch (error) {
      Alert.alert('Command Failed', `${action}: ${error.message}`);
    } finally {
      setIsLoading(false);
    }
  }, [connected, protocol]);

  /* =====================================================================
   * Load current state on connection
   * ===================================================================== */

  useEffect(() => {
    if (connected && deviceInfo) {
      setInputSource(deviceInfo.inputSource || 0x00);
      setBrightness(deviceInfo.brightness || 65535);
      setContrast(deviceInfo.contrast || 32768);
      setOutputEnabled(deviceInfo.outputEnabled !== false);
    }
  }, [connected, deviceInfo]);

  /* =====================================================================
   * Render
   * ===================================================================== */

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Status Banner */}
      {!connected && (
        <View style={styles.disconnectedBanner}>
          <Icon name="lan-disconnect" size={24} color="#FF6B6B" />
          <Text style={styles.disconnectedText}>
            Not connected to device. Use the connect button in the header.
          </Text>
        </View>
      )}

      {/* Loading indicator */}
      {isLoading && (
        <View style={styles.loadingOverlay}>
          <ActivityIndicator size="large" color="#00E5FF" />
          <Text style={styles.loadingText}>Sending: {lastCommand}</Text>
        </View>
      )}

      {/* ============================================================
       * Section: Input Source
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Input Source</Text>
      <View style={styles.sourceGrid}>
        {INPUT_SOURCES.map((src) => (
          <TouchableOpacity
            key={src.id}
            style={[
              styles.sourceButton,
              inputSource === src.id && styles.sourceButtonActive,
              inputSource === src.id && { borderColor: src.color },
            ]}
            onPress={() => sendCommand('setInputSource', src.id)}
            disabled={!connected || isLoading}
          >
            <Icon
              name={src.icon}
              size={28}
              color={inputSource === src.id ? src.color : '#666'}
            />
            <Text
              style={[
                styles.sourceLabel,
                inputSource === src.id && { color: src.color },
              ]}
            >
              {src.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* ============================================================
       * Section: Test Patterns
       * ============================================================ */}
      {inputSource === 0x04 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Test Pattern</Text>
          <View style={styles.patternGrid}>
            {TEST_PATTERNS.map((pat) => (
              <TouchableOpacity
                key={pat.id}
                style={[
                  styles.patternButton,
                  testPattern === pat.id && styles.patternButtonActive,
                ]}
                onPress={() => sendCommand('setTestPattern', pat.id)}
                disabled={!connected || isLoading}
              >
                <Text style={styles.patternLabel}>{pat.label}</Text>
                <Text style={styles.patternDesc}>{pat.desc}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      )}

      {/* ============================================================
       * Section: Brightness & Contrast
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Brightness</Text>
      <View style={styles.sliderRow}>
        <Icon name="brightness-5" size={20} color="#FFD700" />
        <Slider
          style={styles.slider}
          minimumValue={0}
          maximumValue={65535}
          step={256}
          value={brightness}
          onSlidingComplete={(val) => sendCommand('setBrightness', Math.round(val))}
          minimumTrackTintColor="#FFD700"
          maximumTrackTintColor="#333"
          thumbTintColor="#FFD700"
          disabled={!connected || isLoading}
        />
        <Text style={styles.sliderValue}>
          {Math.round(brightness / 655.35)}%
        </Text>
      </View>

      <Text style={styles.sectionTitle}>Contrast</Text>
      <View style={styles.sliderRow}>
        <Icon name="contrast-circle" size={20} color="#00E5FF" />
        <Slider
          style={styles.slider}
          minimumValue={0}
          maximumValue={65535}
          step={256}
          value={contrast}
          onSlidingComplete={(val) => sendCommand('setContrast', Math.round(val))}
          minimumTrackTintColor="#00E5FF"
          maximumTrackTintColor="#333"
          thumbTintColor="#00E5FF"
          disabled={!connected || isLoading}
        />
        <Text style={styles.sliderValue}>
          {Math.round(contrast / 655.35)}%
        </Text>
      </View>

      {/* ============================================================
       * Section: Gamma Presets
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Gamma Correction</Text>
      <View style={styles.gammaGrid}>
        {GAMMA_PRESETS.map((preset) => (
          <TouchableOpacity
            key={preset.id}
            style={[
              styles.gammaButton,
              gammaPreset === preset.id && styles.gammaButtonActive,
            ]}
            onPress={() => sendCommand('setGamma', preset.id)}
            disabled={!connected || isLoading}
          >
            <Text style={styles.gammaLabel}>{preset.label}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* ============================================================
       * Section: Output Control
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Output Control</Text>
      <View style={styles.switchRow}>
        <Icon name="led-strip-variant" size={24} color={outputEnabled ? '#4CAF50' : '#666'} />
        <Text style={styles.switchLabel}>Output Enable</Text>
        <Switch
          value={outputEnabled}
          onValueChange={(val) => sendCommand('toggleOutput', val)}
          trackColor={{ false: '#333', true: '#4CAF50' }}
          thumbColor={outputEnabled ? '#fff' : '#888'}
          disabled={!connected || isLoading}
        />
      </View>

      <View style={styles.switchRow}>
        <Icon name="pause-circle" size={24} color={frameFrozen ? '#FF9800' : '#666'} />
        <Text style={styles.switchLabel}>Freeze Frame</Text>
        <Switch
          value={frameFrozen}
          onValueChange={(val) => sendCommand('freezeFrame', val)}
          trackColor={{ false: '#333', true: '#FF9800' }}
          thumbColor={frameFrozen ? '#fff' : '#888'}
          disabled={!connected || isLoading}
        />
      </View>

      {/* ============================================================
       * Section: Quick Actions
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Quick Actions</Text>
      <View style={styles.quickActions}>
        <TouchableOpacity
          style={[styles.quickButton, styles.blackoutButton]}
          onPress={() => sendCommand('blackout')}
          disabled={!connected || isLoading}
        >
          <Icon name="brightness-1" size={20} color="#fff" />
          <Text style={styles.quickButtonText}>Blackout</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.quickButton, styles.whiteButton]}
          onPress={() => sendCommand('fullWhite')}
          disabled={!connected || isLoading}
        >
          <Icon name="brightness-7" size={20} color="#000" />
          <Text style={[styles.quickButtonText, { color: '#000' }]}>Full White</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.quickButton, styles.resetButton]}
          onPress={() => sendCommand('reset')}
          disabled={!connected || isLoading}
        >
          <Icon name="restart" size={20} color="#fff" />
          <Text style={styles.quickButtonText}>Reset</Text>
        </TouchableOpacity>
      </View>

      {/* Device info footer */}
      {deviceInfo && (
        <View style={styles.deviceInfo}>
          <Text style={styles.deviceInfoText}>
            FPGA: v{deviceInfo.version || '?.?'} | Matrix: {deviceInfo.matrixWidth || 256}×{deviceInfo.matrixHeight || 256}
          </Text>
          <Text style={styles.deviceInfoText}>
            Refresh: {deviceInfo.refreshRate || 120} Hz | Input: {INPUT_SOURCES.find(s => s.id === inputSource)?.label || 'None'}
          </Text>
        </View>
      )}
    </ScrollView>
  );
};

/* =========================================================================
 * Styles
 * ========================================================================= */

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F3460',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  disconnectedBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3A1A1A',
    borderRadius: 8,
    padding: 12,
    marginBottom: 16,
    borderWidth: 1,
    borderColor: '#FF6B6B33',
  },
  disconnectedText: {
    color: '#FF6B6B',
    marginLeft: 10,
    flex: 1,
    fontSize: 13,
  },
  loadingOverlay: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 10,
    marginBottom: 12,
  },
  loadingText: {
    color: '#00E5FF',
    marginLeft: 10,
    fontSize: 13,
  },
  sectionTitle: {
    color: '#00E5FF',
    fontSize: 14,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 20,
    marginBottom: 10,
  },
  section: {
    marginBottom: 8,
  },
  sourceGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  sourceButton: {
    width: '30%',
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 14,
    alignItems: 'center',
    borderWidth: 2,
    borderColor: '#333',
    marginBottom: 4,
  },
  sourceButtonActive: {
    backgroundColor: '#1A2A4A',
  },
  sourceLabel: {
    color: '#888',
    fontSize: 11,
    fontWeight: '600',
    marginTop: 6,
    textAlign: 'center',
  },
  patternGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  patternButton: {
    width: '47%',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    borderWidth: 1,
    borderColor: '#333',
  },
  patternButtonActive: {
    borderColor: '#9C27B0',
    backgroundColor: '#1A1A3A',
  },
  patternLabel: {
    color: '#E0E0E0',
    fontSize: 13,
    fontWeight: '600',
  },
  patternDesc: {
    color: '#888',
    fontSize: 11,
    marginTop: 2,
  },
  sliderRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  slider: {
    flex: 1,
    marginHorizontal: 10,
  },
  sliderValue: {
    color: '#E0E0E0',
    fontSize: 12,
    fontWeight: '600',
    width: 40,
    textAlign: 'right',
  },
  gammaGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  gammaButton: {
    backgroundColor: '#16213E',
    borderRadius: 8,
    paddingVertical: 10,
    paddingHorizontal: 14,
    borderWidth: 1,
    borderColor: '#333',
  },
  gammaButtonActive: {
    borderColor: '#00E5FF',
    backgroundColor: '#1A2A4A',
  },
  gammaLabel: {
    color: '#E0E0E0',
    fontSize: 12,
    fontWeight: '600',
  },
  switchRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  switchLabel: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: '500',
    flex: 1,
    marginLeft: 10,
  },
  quickActions: {
    flexDirection: 'row',
    gap: 10,
  },
  quickButton: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    borderRadius: 8,
    paddingVertical: 12,
    paddingHorizontal: 8,
    gap: 6,
  },
  blackoutButton: {
    backgroundColor: '#333',
  },
  whiteButton: {
    backgroundColor: '#FFD700',
  },
  resetButton: {
    backgroundColor: '#FF6B6B',
  },
  quickButtonText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '700',
  },
  deviceInfo: {
    marginTop: 24,
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    borderWidth: 1,
    borderColor: '#333',
  },
  deviceInfoText: {
    color: '#888',
    fontSize: 11,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    marginBottom: 2,
  },
});

export default ControlScreen;
