/**
 * SettingsScreen — Patch editor and global settings
 * Configure oscillator, filter, envelope, effects, MIDI routing
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  TextInput,
  Switch,
  Alert,
} from 'react-native';
import Slider from 'react-native-slider';
import { WaveForgeProtocol } from '../utils/protocol';

const PROTO = new WaveForgeProtocol();

export default function SettingsScreen({ navigation }) {
  const [patchName, setPatchName] = useState('Init Patch');
  const [patchSlot, setPatchSlot] = useState(0);
  const [midiChannel, setMidiChannel] = useState(1);
  const [midiThru, setMidiThru] = useState(true);
  const [cvInput1Mode, setCvInput1Mode] = useState(0); // 0=Pitch, 1=Filter, 2=Volume, 3=LFO
  const [cvInput2Mode, setCvInput2Mode] = useState(1);
  const [cvInput3Mode, setCvInput3Mode] = useState(2);
  const [cvInput4Mode, setCvInput4Mode] = useState(3);
  const [sampleRate, setSampleRate] = useState(0); // 0=48k, 1=96k
  const [bitDepth, setBitDepth] = useState(0); // 0=24bit, 1=16bit
  const [reverbMix, setReverbMix] = useState(0.3);
  const [delayTime, setDelayTime] = useState(0.3);
  const [delayFeedback, setDelayFeedback] = useState(0.4);
  const [chorusDepth, setChorusDepth] = useState(0.0);
  const [chorusRate, setChorusRate] = useState(0.5);

  const sendParam = useCallback((command, value) => {
    const frame = PROTO.buildFrame(command, value);
    console.log('TX:', frame.toString('hex'));
  }, []);

  const handleSavePatch = useCallback(() => {
    const frame = PROTO.buildFrame(PROTO.CMD_SAVE_PATCH, patchSlot);
    console.log('TX Save:', frame.toString('hex'));
    Alert.alert('Patch Saved', `Patch "${patchName}" saved to slot ${patchSlot}`);
  }, [patchSlot, patchName]);

  const handleLoadPatch = useCallback(() => {
    const frame = PROTO.buildFrame(PROTO.CMD_LOAD_PATCH, patchSlot);
    console.log('TX Load:', frame.toString('hex'));
    Alert.alert('Patch Loaded', `Loaded patch from slot ${patchSlot}`);
  }, [patchSlot]);

  const handleFactoryReset = useCallback(() => {
    Alert.alert(
      'Factory Reset',
      'This will erase all user patches. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Reset', style: 'destructive', onPress: () => {
          const frame = PROTO.buildFrame(PROTO.CMD_FACTORY_RESET, 0);
          console.log('TX Reset:', frame.toString('hex'));
        }},
      ]
    );
  }, []);

  const cvModeNames = ['Pitch', 'Filter', 'Volume', 'LFO Rate', 'LFO Depth', 'FM Depth', 'Pan'];

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Patch Management */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Patch Management</Text>
        <TextInput
          style={styles.input}
          value={patchName}
          onChangeText={setPatchName}
          placeholder="Patch Name"
          placeholderTextColor="#666"
        />
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Slot: {patchSlot}</Text>
          <Slider
            value={patchSlot}
            minimumValue={0}
            maximumValue={255}
            step={1}
            onValueChange={setPatchSlot}
            style={styles.slider}
            minimumTrackTintColor="#e94560"
          />
        </View>
        <View style={styles.buttonRow}>
          <TouchableOpacity style={styles.button} onPress={handleSavePatch}>
            <Text style={styles.buttonText}>Save</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.button} onPress={handleLoadPatch}>
            <Text style={styles.buttonText}>Load</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.button, styles.buttonDanger]} onPress={handleFactoryReset}>
            <Text style={styles.buttonText}>Factory Reset</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* MIDI Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>MIDI Settings</Text>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Channel: {midiChannel}</Text>
          <Slider
            value={midiChannel}
            minimumValue={1}
            maximumValue={16}
            step={1}
            onValueChange={(v) => { setMidiChannel(v); sendParam(PROTO.CMD_MIDI_CHANNEL, v - 1); }}
            style={styles.slider}
            minimumTrackTintColor="#0f3460"
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>MIDI Thru</Text>
          <Switch
            value={midiThru}
            onValueChange={(v) => { setMidiThru(v); sendParam(PROTO.CMD_MIDI_THRU, v ? 1 : 0); }}
            trackColor={{ false: '#333', true: '#e94560' }}
            thumbColor="#fff"
          />
        </View>
      </View>

      {/* CV Input Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CV Input Assignment</Text>
        {[
          { label: 'CV1', mode: cvInput1Mode, setMode: setCvInput1Mode },
          { label: 'CV2', mode: cvInput2Mode, setMode: setCvInput2Mode },
          { label: 'CV3', mode: cvInput3Mode, setMode: setCvInput3Mode },
          { label: 'CV4', mode: cvInput4Mode, setMode: setCvInput4Mode },
        ].map(({ label, mode, setMode }, idx) => (
          <View key={label} style={styles.cvRow}>
            <Text style={styles.cvLabel}>{label}:</Text>
            <View style={styles.cvButtons}>
              {cvModeNames.slice(0, 5).map((name, mIdx) => (
                <TouchableOpacity
                  key={mIdx}
                  style={[styles.cvButton, mode === mIdx && styles.cvButtonActive]}
                  onPress={() => { setMode(mIdx); sendParam(PROTO.CMD_CV1_MODE + idx, mIdx); }}
                >
                  <Text style={[styles.cvButtonText, mode === mIdx && styles.cvButtonTextActive]}>
                    {name}
                  </Text>
                </TouchableOpacity>
              ))}
            </View>
          </View>
        ))}
      </View>

      {/* Effects Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Effects</Text>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Reverb: {Math.round(reverbMix * 100)}%</Text>
          <Slider
            value={reverbMix}
            minimumValue={0}
            maximumValue={1}
            step={0.01}
            onValueChange={(v) => { setReverbMix(v); sendParam(PROTO.CMD_FX_REVERB, v); }}
            style={styles.slider}
            minimumTrackTintColor="#9c27b0"
          />
        </View>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Delay Time: {Math.round(delayTime * 1000)}ms</Text>
          <Slider
            value={delayTime}
            minimumValue={0}
            maximumValue={1}
            step={0.01}
            onValueChange={(v) => { setDelayTime(v); sendParam(PROTO.CMD_FX_DELAY_TIME, v); }}
            style={styles.slider}
            minimumTrackTintColor="#ff9800"
          />
        </View>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Delay Feedback: {Math.round(delayFeedback * 100)}%</Text>
          <Slider
            value={delayFeedback}
            minimumValue={0}
            maximumValue={0.95}
            step={0.01}
            onValueChange={(v) => { setDelayFeedback(v); sendParam(PROTO.CMD_FX_DELAY_FB, v); }}
            style={styles.slider}
            minimumTrackTintColor="#ff9800"
          />
        </View>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Chorus Depth: {Math.round(chorusDepth * 100)}%</Text>
          <Slider
            value={chorusDepth}
            minimumValue={0}
            maximumValue={1}
            step={0.01}
            onValueChange={(v) => { setChorusDepth(v); sendParam(PROTO.CMD_FX_CHORUS_DEPTH, v); }}
            style={styles.slider}
            minimumTrackTintColor="#2196f3"
          />
        </View>
      </View>

      {/* Audio Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Audio Settings</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Sample Rate: {sampleRate === 0 ? '48 kHz' : '96 kHz'}</Text>
          <Switch
            value={sampleRate === 1}
            onValueChange={(v) => { setSampleRate(v ? 1 : 0); sendParam(PROTO.CMD_SAMPLE_RATE, v ? 1 : 0); }}
            trackColor={{ false: '#333', true: '#4caf50' }}
            thumbColor="#fff"
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Bit Depth: {bitDepth === 0 ? '24-bit' : '16-bit'}</Text>
          <Switch
            value={bitDepth === 1}
            onValueChange={(v) => { setBitDepth(v ? 1 : 0); sendParam(PROTO.CMD_BIT_DEPTH, v ? 1 : 0); }}
            trackColor={{ false: '#333', true: '#4caf50' }}
            thumbColor="#fff"
          />
        </View>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  input: {
    backgroundColor: '#0f3460',
    borderRadius: 8,
    padding: 12,
    color: '#e0e0e0',
    fontSize: 16,
    marginBottom: 12,
  },
  sliderContainer: {
    marginBottom: 12,
  },
  sliderLabel: {
    color: '#a0a0a0',
    fontSize: 13,
    marginBottom: 4,
  },
  slider: {
    height: 30,
  },
  buttonRow: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 8,
  },
  button: {
    flex: 1,
    backgroundColor: '#0f3460',
    borderRadius: 8,
    paddingVertical: 10,
    alignItems: 'center',
  },
  buttonDanger: {
    backgroundColor: '#b71c1c',
  },
  buttonText: {
    color: '#e0e0e0',
    fontSize: 14,
    fontWeight: 'bold',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  switchLabel: {
    color: '#e0e0e0',
    fontSize: 14,
  },
  cvRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  cvLabel: {
    color: '#e0e0e0',
    fontSize: 14,
    width: 40,
  },
  cvButtons: {
    flexDirection: 'row',
    flex: 1,
    gap: 4,
  },
  cvButton: {
    flex: 1,
    backgroundColor: '#0f3460',
    borderRadius: 6,
    paddingVertical: 6,
    alignItems: 'center',
  },
  cvButtonActive: {
    backgroundColor: '#e94560',
  },
  cvButtonText: {
    color: '#a0a0a0',
    fontSize: 10,
  },
  cvButtonTextActive: {
    color: '#ffffff',
  },
});