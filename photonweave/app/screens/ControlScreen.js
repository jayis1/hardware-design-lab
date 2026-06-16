/**
 * ControlScreen.js — Main Hologram Control Panel
 *
 * Primary control interface for the PhotonWeave CGH Engine.
 * Provides real-time hologram generation controls including:
 *   - Wavelength and color channel selection
 *   - Depth range and plane count configuration
 *   - Resolution and FFT size settings
 *   - HDMI output enable/disable
 *   - Frame processing start/stop
 *   - Live FPS and frame time display
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Switch, TextInput, ActivityIndicator, Alert, Animated,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { THEME, CMD, CGH_DEFAULTS, HDMI_4K60_TIMING } from '../App';

//=============================================================================
// Color Channel Presets
//=============================================================================
const COLOR_CHANNELS = [
  { id: 0, name: 'Red', wavelength: 638, color: '#FF4444', icon: 'circle', paramSet: 0 },
  { id: 1, name: 'Green', wavelength: 532, color: '#44FF44', icon: 'circle', paramSet: 1 },
  { id: 2, name: 'Blue', wavelength: 450, color: '#4444FF', icon: 'circle', paramSet: 2 },
];

//=============================================================================
// Propagation Model Options
//=============================================================================
const PROP_MODELS = [
  { id: 0, name: 'Fresnel', description: 'Near-field diffraction' },
  { id: 1, name: 'Angular Spectrum', description: 'Full-wave propagation (recommended)' },
  { id: 2, name: 'Fraunhofer', description: 'Far-field diffraction' },
];

//=============================================================================
// ControlScreen Component
//=============================================================================
export default function ControlScreen({ protocol, isConnected }) {
  // ── CGH Parameter State ──
  const [wavelength, setWavelength] = useState('532');
  const [colorChannel, setColorChannel] = useState(1); // Green default
  const [depthMin, setDepthMin] = useState('0');
  const [depthMax, setDepthMax] = useState('100');
  const [depthPlanes, setDepthPlanes] = useState('256');
  const [inputWidth, setInputWidth] = useState('2048');
  const [inputHeight, setInputHeight] = useState('2048');
  const [outputWidth, setOutputWidth] = useState('3840');
  const [outputHeight, setOutputHeight] = useState('2160');
  const [fftSize, setFftSize] = useState('4096');
  const [propModel, setPropModel] = useState(1); // ASM default
  const [quantBits, setQuantBits] = useState(8);
  const [pixelPitch, setPixelPitch] = useState('3.6');
  const [fillFactor, setFillFactor] = useState('92');

  // ── Pipeline State ──
  const [isProcessing, setIsProcessing] = useState(false);
  const [hdmiEnabled, setHdmiEnabled] = useState(false);
  const [fps, setFps] = useState(0);
  const [frameTime, setFrameTime] = useState(0);
  const [frameCount, setFrameCount] = useState(0);
  const [isApplying, setIsApplying] = useState(false);

  // ── Animation ──
  const pulseAnim = useRef(new Animated.Value(1)).current;
  const processingInterval = useRef(null);
  const fpsInterval = useRef(null);

  // Pulse animation when processing
  useEffect(() => {
    if (isProcessing) {
      const pulse = Animated.loop(
        Animated.sequence([
          Animated.timing(pulseAnim, {
            toValue: 0.6,
            duration: 800,
            useNativeDriver: true,
          }),
          Animated.timing(pulseAnim, {
            toValue: 1,
            duration: 800,
            useNativeDriver: true,
          }),
        ])
      );
      pulse.start();
      return () => pulse.stop();
    } else {
      pulseAnim.setValue(1);
    }
  }, [isProcessing]);

  // FPS polling when connected
  useEffect(() => {
    if (isConnected) {
      fpsInterval.current = setInterval(async () => {
        try {
          const status = await protocol.getStatus();
          setFps(status.fpsX1000 / 1000.0);
          setFrameTime(status.cghStatus & 0x04 ? status.frameTimeUs : 0);
          setFrameCount(status.frameCount);
          setIsProcessing((status.cghStatus & 0x02) !== 0); // BUSY bit
        } catch (error) {
          console.warn('FPS poll error:', error.message);
        }
      }, 500);
    }
    return () => {
      if (fpsInterval.current) clearInterval(fpsInterval.current);
    };
  }, [isConnected]);

  //===========================================================================
  // Action Handlers
  //===========================================================================

  const handleApplyParams = useCallback(async () => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Please connect to a PhotonWeave device first.');
      return;
    }

    setIsApplying(true);
    try {
      const params = {
        wavelength_nm_x1000: parseInt(wavelength) * 1000,
        depth_min_um: parseInt(depthMin),
        depth_max_um: parseInt(depthMax) * 1000, // mm → µm
        depth_planes: parseInt(depthPlanes),
        input_width: parseInt(inputWidth),
        input_height: parseInt(inputHeight),
        output_width: parseInt(outputWidth),
        output_height: parseInt(outputHeight),
        fft_size: parseInt(fftSize),
        propagation_model: propModel,
        phase_quantize_bits: quantBits,
        color_channel: colorChannel,
        pixel_pitch_um_x100: Math.round(parseFloat(pixelPitch) * 100),
        fill_factor_percent: parseInt(fillFactor),
      };

      await protocol.setCghParams(params, colorChannel);
      Alert.alert('Success', 'CGH parameters applied successfully.');
    } catch (error) {
      Alert.alert('Error', `Failed to apply parameters: ${error.message}`);
    } finally {
      setIsApplying(false);
    }
  }, [isConnected, wavelength, depthMin, depthMax, depthPlanes, inputWidth,
      inputHeight, outputWidth, outputHeight, fftSize, propModel, quantBits,
      colorChannel, pixelPitch, fillFactor]);

  const handleStartStop = useCallback(async () => {
    if (!isConnected) return;

    try {
      if (isProcessing) {
        await protocol.abortCgh();
        setIsProcessing(false);
      } else {
        await protocol.startCgh();
        setIsProcessing(true);
      }
    } catch (error) {
      Alert.alert('Error', `Pipeline control failed: ${error.message}`);
    }
  }, [isConnected, isProcessing]);

  const handleHdmiToggle = useCallback(async (value) => {
    if (!isConnected) return;

    try {
      if (value) {
        await protocol.enableHdmi(HDMI_4K60_TIMING);
      } else {
        await protocol.disableHdmi();
      }
      setHdmiEnabled(value);
    } catch (error) {
      Alert.alert('Error', `HDMI control failed: ${error.message}`);
      setHdmiEnabled(!value); // Revert
    }
  }, [isConnected]);

  const handleColorSelect = useCallback((channel) => {
    setColorChannel(channel.id);
    setWavelength(channel.wavelength.toString());
  }, []);

  const handlePreset4K60 = useCallback(() => {
    setOutputWidth('3840');
    setOutputHeight('2160');
    setFftSize('4096');
    setInputWidth('2048');
    setInputHeight('2048');
  }, []);

  const handlePreset1080p = useCallback(() => {
    setOutputWidth('1920');
    setOutputHeight('1080');
    setFftSize('2048');
    setInputWidth('1024');
    setInputHeight('1024');
  }, []);

  //===========================================================================
  // Render
  //===========================================================================

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* ── Status Bar ── */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>FPS</Text>
          <Text style={styles.statusValue}>{fps.toFixed(1)}</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Frame Time</Text>
          <Text style={styles.statusValue}>
            {frameTime > 0 ? `${(frameTime / 1000).toFixed(2)} ms` : '—'}
          </Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Frames</Text>
          <Text style={styles.statusValue}>{frameCount.toLocaleString()}</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>HDMI</Text>
          <Text style={[styles.statusValue, { color: hdmiEnabled ? THEME.success : THEME.textSecondary }]}>
            {hdmiEnabled ? 'ON' : 'OFF'}
          </Text>
        </View>
      </View>

      {/* ── Color Channel Selector ── */}
      <Text style={styles.sectionTitle}>Color Channel</Text>
      <View style={styles.colorRow}>
        {COLOR_CHANNELS.map((ch) => (
          <TouchableOpacity
            key={ch.id}
            style={[
              styles.colorChip,
              { borderColor: ch.color },
              colorChannel === ch.id && { backgroundColor: ch.color + '30' },
            ]}
            onPress={() => handleColorSelect(ch)}
          >
            <Icon name={ch.icon} size={20} color={ch.color} />
            <Text style={[styles.colorText, { color: ch.color }]}>
              {ch.name} ({ch.wavelength}nm)
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* ── Wavelength ── */}
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Wavelength (nm)</Text>
        <TextInput
          style={styles.paramInput}
          value={wavelength}
          onChangeText={setWavelength}
          keyboardType="numeric"
          placeholder="532"
        />
      </View>

      {/* ── Depth Range ── */}
      <Text style={styles.sectionTitle}>Depth Configuration</Text>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Min Depth (mm)</Text>
        <TextInput
          style={styles.paramInput}
          value={depthMin}
          onChangeText={setDepthMin}
          keyboardType="numeric"
          placeholder="0"
        />
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Max Depth (mm)</Text>
        <TextInput
          style={styles.paramInput}
          value={depthMax}
          onChangeText={setDepthMax}
          keyboardType="numeric"
          placeholder="100"
        />
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Depth Planes</Text>
        <TextInput
          style={styles.paramInput}
          value={depthPlanes}
          onChangeText={setDepthPlanes}
          keyboardType="numeric"
          placeholder="256"
        />
      </View>

      {/* ── Resolution ── */}
      <Text style={styles.sectionTitle}>Resolution</Text>
      <View style={styles.presetRow}>
        <TouchableOpacity style={styles.presetButton} onPress={handlePreset4K60}>
          <Text style={styles.presetText}>4K60</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.presetButton} onPress={handlePreset1080p}>
          <Text style={styles.presetText}>1080p</Text>
        </TouchableOpacity>
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Input (W×H)</Text>
        <View style={styles.paramRowInner}>
          <TextInput style={styles.paramInputSmall} value={inputWidth}
                     onChangeText={setInputWidth} keyboardType="numeric" />
          <Text style={styles.paramSep}>×</Text>
          <TextInput style={styles.paramInputSmall} value={inputHeight}
                     onChangeText={setInputHeight} keyboardType="numeric" />
        </View>
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Output (W×H)</Text>
        <View style={styles.paramRowInner}>
          <TextInput style={styles.paramInputSmall} value={outputWidth}
                     onChangeText={setOutputWidth} keyboardType="numeric" />
          <Text style={styles.paramSep}>×</Text>
          <TextInput style={styles.paramInputSmall} value={outputHeight}
                     onChangeText={setOutputHeight} keyboardType="numeric" />
        </View>
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>FFT Size</Text>
        <TextInput
          style={styles.paramInput}
          value={fftSize}
          onChangeText={setFftSize}
          keyboardType="numeric"
          placeholder="4096"
        />
      </View>

      {/* ── Propagation Model ── */}
      <Text style={styles.sectionTitle}>Propagation Model</Text>
      {PROP_MODELS.map((model) => (
        <TouchableOpacity
          key={model.id}
          style={[
            styles.modelRow,
            propModel === model.id && styles.modelRowActive,
          ]}
          onPress={() => setPropModel(model.id)}
        >
          <Icon
            name={propModel === model.id ? 'radiobox-marked' : 'radiobox-blank'}
            size={22}
            color={propModel === model.id ? THEME.primary : THEME.textSecondary}
          />
          <View style={styles.modelTextCol}>
            <Text style={[
              styles.modelName,
              propModel === model.id && { color: THEME.primary },
            ]}>
              {model.name}
            </Text>
            <Text style={styles.modelDesc}>{model.description}</Text>
          </View>
        </TouchableOpacity>
      ))}

      {/* ── SLM Parameters ── */}
      <Text style={styles.sectionTitle}>SLM Parameters</Text>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Pixel Pitch (µm)</Text>
        <TextInput
          style={styles.paramInput}
          value={pixelPitch}
          onChangeText={setPixelPitch}
          keyboardType="decimal-pad"
          placeholder="3.6"
        />
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Fill Factor (%)</Text>
        <TextInput
          style={styles.paramInput}
          value={fillFactor}
          onChangeText={setFillFactor}
          keyboardType="numeric"
          placeholder="92"
        />
      </View>
      <View style={styles.paramRow}>
        <Text style={styles.paramLabel}>Quantization</Text>
        <View style={styles.quantRow}>
          <TouchableOpacity
            style={[styles.quantButton, quantBits === 8 && styles.quantButtonActive]}
            onPress={() => setQuantBits(8)}
          >
            <Text style={[styles.quantText, quantBits === 8 && styles.quantTextActive]}>
              8-bit
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.quantButton, quantBits === 10 && styles.quantButtonActive]}
            onPress={() => setQuantBits(10)}
          >
            <Text style={[styles.quantText, quantBits === 10 && styles.quantTextActive]}>
              10-bit
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* ── HDMI Output ── */}
      <Text style={styles.sectionTitle}>HDMI Output</Text>
      <View style={styles.switchRow}>
        <Text style={styles.paramLabel}>Enable 4K60 Output</Text>
        <Switch
          value={hdmiEnabled}
          onValueChange={handleHdmiToggle}
          trackColor={{ false: THEME.border, true: THEME.primary + '60' }}
          thumbColor={hdmiEnabled ? THEME.primary : THEME.textSecondary}
          disabled={!isConnected}
        />
      </View>

      {/* ── Action Buttons ── */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.actionButton, styles.applyButton]}
          onPress={handleApplyParams}
          disabled={!isConnected || isApplying}
        >
          {isApplying ? (
            <ActivityIndicator color="#FFF" size="small" />
          ) : (
            <>
              <Icon name="check-circle" size={20} color="#FFF" />
              <Text style={styles.actionText}>Apply Params</Text>
            </>
          )}
        </TouchableOpacity>

        <Animated.View style={{ opacity: pulseAnim }}>
          <TouchableOpacity
            style={[
              styles.actionButton,
              isProcessing ? styles.stopButton : styles.startButton,
            ]}
            onPress={handleStartStop}
            disabled={!isConnected}
          >
            <Icon
              name={isProcessing ? 'stop-circle' : 'play-circle'}
              size={20}
              color="#FFF"
            />
            <Text style={styles.actionText}>
              {isProcessing ? 'STOP' : 'START'}
            </Text>
          </TouchableOpacity>
        </Animated.View>
      </View>

      {/* ── Connection Warning ── */}
      {!isConnected && (
        <View style={styles.warningBanner}>
          <Icon name="alert-circle" size={20} color={THEME.warning} />
          <Text style={styles.warningText}>
            Not connected to PhotonWeave device. Connect via the header button.
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

//=============================================================================
// Styles
//=============================================================================
const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.background,
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    marginBottom: 20,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  statusItem: {
    alignItems: 'center',
  },
  statusLabel: {
    color: THEME.textSecondary,
    fontSize: 11,
    fontWeight: '600',
    textTransform: 'uppercase',
    marginBottom: 4,
  },
  statusValue: {
    color: THEME.text,
    fontSize: 18,
    fontWeight: '700',
    fontVariant: ['tabular-nums'],
  },
  sectionTitle: {
    color: THEME.primary,
    fontSize: 14,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 20,
    marginBottom: 12,
  },
  colorRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  colorChip: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 10,
    paddingHorizontal: 8,
    borderRadius: 8,
    borderWidth: 2,
    marginHorizontal: 4,
  },
  colorText: {
    fontSize: 12,
    fontWeight: '600',
    marginLeft: 6,
  },
  paramRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  paramLabel: {
    color: THEME.text,
    fontSize: 14,
    fontWeight: '500',
    flex: 1,
  },
  paramInput: {
    backgroundColor: THEME.surface,
    color: THEME.text,
    fontSize: 14,
    fontWeight: '600',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    width: 100,
    textAlign: 'right',
    fontVariant: ['tabular-nums'],
  },
  paramInputSmall: {
    backgroundColor: THEME.surface,
    color: THEME.text,
    fontSize: 14,
    fontWeight: '600',
    paddingHorizontal: 10,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    width: 72,
    textAlign: 'right',
    fontVariant: ['tabular-nums'],
  },
  paramRowInner: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  paramSep: {
    color: THEME.textSecondary,
    fontSize: 16,
    marginHorizontal: 6,
  },
  presetRow: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  presetButton: {
    backgroundColor: THEME.surface,
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    marginRight: 8,
  },
  presetText: {
    color: THEME.primary,
    fontSize: 13,
    fontWeight: '600',
  },
  modelRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 10,
    paddingHorizontal: 12,
    borderRadius: 8,
    marginBottom: 6,
    backgroundColor: THEME.surface,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  modelRowActive: {
    borderColor: THEME.primary,
    backgroundColor: THEME.primary + '10',
  },
  modelTextCol: {
    marginLeft: 10,
    flex: 1,
  },
  modelName: {
    color: THEME.text,
    fontSize: 14,
    fontWeight: '600',
  },
  modelDesc: {
    color: THEME.textSecondary,
    fontSize: 12,
    marginTop: 2,
  },
  quantRow: {
    flexDirection: 'row',
  },
  quantButton: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    marginLeft: 8,
    backgroundColor: THEME.surface,
  },
  quantButtonActive: {
    borderColor: THEME.primary,
    backgroundColor: THEME.primary + '20',
  },
  quantText: {
    color: THEME.textSecondary,
    fontSize: 13,
    fontWeight: '600',
  },
  quantTextActive: {
    color: THEME.primary,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  actionRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 24,
    marginBottom: 16,
  },
  actionButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 14,
    paddingHorizontal: 24,
    borderRadius: 12,
    flex: 1,
    marginHorizontal: 6,
  },
  applyButton: {
    backgroundColor: THEME.secondary,
  },
  startButton: {
    backgroundColor: THEME.success,
  },
  stopButton: {
    backgroundColor: THEME.error,
  },
  actionText: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '700',
    marginLeft: 8,
  },
  warningBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: THEME.warning + '15',
    borderRadius: 8,
    padding: 12,
    marginTop: 8,
    borderWidth: 1,
    borderColor: THEME.warning + '40',
  },
  warningText: {
    color: THEME.warning,
    fontSize: 13,
    fontWeight: '500',
    marginLeft: 8,
    flex: 1,
  },
});
