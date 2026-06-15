/**
 * MainScreen — Primary synth control interface
 * Shows voice status, oscillator controls, filter, envelope
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Alert,
} from 'react-native';
import Slider from 'react-native-slider';
import { WaveForgeProtocol } from '../utils/protocol';
import SynthKnob from '../components/SynthKnob';

const PROTO = new WaveForgeProtocol();

export default function MainScreen({ navigation }) {
  const [connected, setConnected] = useState(false);
  const [patchName, setPatchName] = useState('Init Patch');
  const [oscMode, setOscMode] = useState(0); // 0=WT, 1=FM, 2=Noise
  const [filterCutoff, setFilterCutoff] = useState(0.8);
  const [filterResonance, setFilterResonance] = useState(0.3);
  const [attack, setAttack] = useState(0.01);
  const [decay, setDecay] = useState(0.2);
  const [sustain, setSustain] = useState(0.7);
  const [release, setRelease] = useState(0.3);
  const [volume, setVolume] = useState(0.75);
  const [fmDepth, setFmDepth] = useState(0.0);
  const [fmRatio, setFmRatio] = useState(1.0);
  const [wavetable, setWavetable] = useState(0);
  const [activeVoices, setActiveVoices] = useState(0);

  // Simulated connection status (would be BLE/USB in production)
  useEffect(() => {
    // Auto-connect attempt on mount
    const timer = setTimeout(() => {
      setConnected(true);
    }, 1000);
    return () => clearTimeout(timer);
  }, []);

  const sendParam = useCallback((command, value) => {
    if (!connected) return;
    const frame = PROTO.buildFrame(command, value);
    // In production: send via BLE or USB
    console.log('TX:', frame.toString('hex'));
  }, [connected]);

  const handleOscMode = useCallback((mode) => {
    setOscMode(mode);
    sendParam(PROTO.CMD_OSC_MODE, mode);
  }, [sendParam]);

  const handleWavetable = useCallback((idx) => {
    setWavetable(idx);
    sendParam(PROTO.CMD_WAVETABLE, idx);
  }, [sendParam]);

  const handlePlayNote = useCallback((note, velocity) => {
    if (!connected) return;
    const frame = PROTO.buildMidiNoteOn(0, note, velocity);
    console.log('TX NoteOn:', frame.toString('hex'));
  }, [connected]);

  const handleStopNote = useCallback((note) => {
    if (!connected) return;
    const frame = PROTO.buildMidiNoteOff(0, note);
    console.log('TX NoteOff:', frame.toString('hex'));
  }, [connected]);

  const oscModes = ['Wavetable', 'FM', 'Noise'];
  const wavetableNames = ['Sine', 'Saw', 'Square', 'Triangle', 'Pulse', 'Waveform6', 'Waveform7', 'Waveform8'];

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection Status */}
      <View style={styles.statusBar}>
        <View style={[styles.statusDot, { backgroundColor: connected ? '#4caf50' : '#f44336' }]} />
        <Text style={styles.statusText}>
          {connected ? 'Connected' : 'Disconnected'}
        </Text>
        <Text style={styles.voiceCount}>
          Voices: {activeVoices}/64
        </Text>
      </View>

      {/* Patch Name */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Patch</Text>
        <Text style={styles.patchName}>{patchName}</Text>
        <View style={styles.patchButtons}>
          <TouchableOpacity
            style={styles.patchButton}
            onPress={() => navigation.navigate('Settings')}
          >
            <Text style={styles.patchButtonText}>Edit</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.patchButton}
            onPress={() => Alert.alert('Save', 'Patch saved to flash')}
          >
            <Text style={styles.patchButtonText}>Save</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.patchButton}
            onPress={() => navigation.navigate('DataView')}
          >
            <Text style={styles.patchButtonText}>CV</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Oscillator Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Oscillator</Text>
        <View style={styles.oscModeRow}>
          {oscModes.map((mode, idx) => (
            <TouchableOpacity
              key={idx}
              style={[styles.oscModeButton, oscMode === idx && styles.oscModeActive]}
              onPress={() => handleOscMode(idx)}
            >
              <Text style={[styles.oscModeText, oscMode === idx && styles.oscModeTextActive]}>
                {mode}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        {oscMode === 0 && (
          <View style={styles.sliderContainer}>
            <Text style={styles.sliderLabel}>Wavetable: {wavetableNames[wavetable]}</Text>
            <Slider
              value={wavetable}
              minimumValue={0}
              maximumValue={7}
              step={1}
              onValueChange={handleWavetable}
              style={styles.slider}
              thumbStyle={styles.sliderThumb}
              trackStyle={styles.sliderTrack}
              minimumTrackTintColor="#e94560"
            />
          </View>
        )}

        {oscMode === 1 && (
          <View>
            <View style={styles.sliderContainer}>
              <Text style={styles.sliderLabel}>FM Depth: {fmDepth.toFixed(2)}</Text>
              <Slider
                value={fmDepth}
                minimumValue={0}
                maximumValue={1}
                step={0.01}
                onValueChange={(v) => { setFmDepth(v); sendParam(PROTO.CMD_FM_DEPTH, v); }}
                style={styles.slider}
                minimumTrackTintColor="#e94560"
              />
            </View>
            <View style={styles.sliderContainer}>
              <Text style={styles.sliderLabel}>FM Ratio: {fmRatio.toFixed(1)}</Text>
              <Slider
                value={fmRatio}
                minimumValue={0.5}
                maximumValue={8}
                step={0.5}
                onValueChange={(v) => { setFmRatio(v); sendParam(PROTO.CMD_FM_RATIO, v); }}
                style={styles.slider}
                minimumTrackTintColor="#e94560"
              />
            </View>
          </View>
        )}
      </View>

      {/* Filter Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Filter</Text>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Cutoff: {(filterCutoff * 20000).toFixed(0)} Hz</Text>
          <Slider
            value={filterCutoff}
            minimumValue={0}
            maximumValue={1}
            step={0.01}
            onValueChange={(v) => { setFilterCutoff(v); sendParam(PROTO.CMD_FILTER_CUTOFF, v); }}
            style={styles.slider}
            minimumTrackTintColor="#0f3460"
          />
        </View>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>Resonance: {filterResonance.toFixed(2)}</Text>
          <Slider
            value={filterResonance}
            minimumValue={0}
            maximumValue={1}
            step={0.01}
            onValueChange={(v) => { setFilterResonance(v); sendParam(PROTO.CMD_FILTER_RES, v); }}
            style={styles.slider}
            minimumTrackTintColor="#0f3460"
          />
        </View>
      </View>

      {/* Envelope Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>ADSR Envelope</Text>
        <View style={styles.adsrRow}>
          <View style={styles.adsrCol}>
            <SynthKnob label="A" value={attack} min={0.001} max={2} step={0.001}
              onChange={(v) => { setAttack(v); sendParam(PROTO.CMD_ENV_ATTACK, v); }} />
          </View>
          <View style={styles.adsrCol}>
            <SynthKnob label="D" value={decay} min={0.001} max={2} step={0.001}
              onChange={(v) => { setDecay(v); sendParam(PROTO.CMD_ENV_DECAY, v); }} />
          </View>
          <View style={styles.adsrCol}>
            <SynthKnob label="S" value={sustain} min={0} max={1} step={0.01}
              onChange={(v) => { setSustain(v); sendParam(PROTO.CMD_ENV_SUSTAIN, v); }} />
          </View>
          <View style={styles.adsrCol}>
            <SynthKnob label="R" value={release} min={0.001} max={5} step={0.001}
              onChange={(v) => { setRelease(v); sendParam(PROTO.CMD_ENV_RELEASE, v); }} />
          </View>
        </View>
      </View>

      {/* Master Volume */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Master Volume</Text>
        <View style={styles.sliderContainer}>
          <Text style={styles.sliderLabel}>{Math.round(volume * 100)}%</Text>
          <Slider
            value={volume}
            minimumValue={0}
            maximumValue={1}
            step={0.01}
            onValueChange={(v) => { setVolume(v); sendParam(PROTO.CMD_MASTER_VOL, v); }}
            style={styles.slider}
            minimumTrackTintColor="#4caf50"
          />
        </View>
      </View>

      {/* Test Keyboard (1 octave) */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Test Keyboard (C4–B4)</Text>
        <View style={styles.keyboard}>
          {['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'].map((note, idx) => (
            <TouchableOpacity
              key={idx}
              style={[
                styles.key,
                note.includes('#') ? styles.blackKey : styles.whiteKey,
              ]}
              onPressIn={() => handlePlayNote(60 + idx, 100)}
              onPressOut={() => handleStopNote(60 + idx)}
            >
              <Text style={styles.keyLabel}>{note}</Text>
            </TouchableOpacity>
          ))}
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
  statusBar: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 8,
    paddingHorizontal: 12,
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
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
  voiceCount: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
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
  patchName: {
    color: '#e0e0e0',
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  patchButtons: {
    flexDirection: 'row',
    gap: 8,
  },
  patchButton: {
    backgroundColor: '#0f3460',
    borderRadius: 8,
    paddingHorizontal: 16,
    paddingVertical: 8,
  },
  patchButtonText: {
    color: '#e0e0e0',
    fontSize: 14,
  },
  oscModeRow: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 12,
  },
  oscModeButton: {
    flex: 1,
    backgroundColor: '#0f3460',
    borderRadius: 8,
    paddingVertical: 10,
    alignItems: 'center',
  },
  oscModeActive: {
    backgroundColor: '#e94560',
  },
  oscModeText: {
    color: '#a0a0a0',
    fontSize: 14,
    fontWeight: 'bold',
  },
  oscModeTextActive: {
    color: '#ffffff',
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
  adsrRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  adsrCol: {
    alignItems: 'center',
  },
  keyboard: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 4,
  },
  key: {
    width: 28,
    height: 60,
    borderRadius: 4,
    alignItems: 'center',
    justifyContent: 'flex-end',
    paddingBottom: 4,
  },
  whiteKey: {
    backgroundColor: '#e0e0e0',
  },
  blackKey: {
    backgroundColor: '#333333',
    height: 44,
  },
  keyLabel: {
    fontSize: 10,
    color: '#666',
  },
});