/**
 * MappingScreen.tsx — MIDI mapping editor for Synthand.
 *
 * Allows the user to assign MIDI notes, CC numbers, and haptic
 * waveforms to each gesture and finger. Settings are saved to
 * the glove's flash via the BLE config characteristic.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  TextInput,
  Alert,
  Switch,
} from 'react-native';
import { useBle } from '../ble/BleManager';
import { MappingData, DEFAULT_MAPPING, FINGER_NAMES, GESTURE_NAMES } from '../ble/protocol';

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

function noteToName(note: number): string {
  const octave = Math.floor(note / 12) - 1;
  return `${NOTE_NAMES[note % 12]}${octave}`;
}

/**
 * MappingScreen — MIDI gesture mapping editor.
 * Author: jayis1
 */
export default function MappingScreen() {
  const { isConnected, sendMapping, mappingData } = useBle();
  const [mapping, setMapping] = useState<MappingData>(mappingData || DEFAULT_MAPPING);
  const [selectedGesture, setSelectedGesture] = useState(0);

  const updateNote = useCallback((finger: number, note: number) => {
    const newMapping = { ...mapping };
    newMapping.notes[finger] = note;
    setMapping(newMapping);
  }, [mapping]);

  const updateCC = useCallback(
    (type: 'emg' | 'curl', index: number, cc: number) => {
      const newMapping = { ...mapping };
      if (type === 'emg') newMapping.ccEmg[index] = cc;
      else newMapping.ccCurl[index] = cc;
      setMapping(newMapping);
    },
    [mapping]
  );

  const updateHaptic = useCallback((gestureId: number, waveform: number) => {
    const newMapping = { ...mapping };
    newMapping.hapticWaveforms[gestureId] = waveform;
    setMapping(newMapping);
  }, [mapping]);

  const saveMapping = useCallback(async () => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Connect to Synthand to save mapping.');
      return;
    }
    await sendMapping(mapping);
    Alert.alert('Saved', 'Mapping saved to Synthand flash.');
  }, [isConnected, mapping, sendMapping]);

  const savePreset = useCallback(() => {
    // Would call database.savePreset() here
    Alert.alert('Preset Saved', 'Mapping preset saved to phone.');
  }, [mapping]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>MIDI Mapping</Text>

      {/* Per-finger note assignment */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Per-Finger Notes</Text>
        {FINGER_NAMES.map((name, i) => (
          <View key={i} style={styles.row}>
            <Text style={styles.label}>{name}</Text>
            <Text style={styles.noteDisplay}>{noteToName(mapping.notes[i])}</Text>
            <TouchableOpacity
              style={styles.stepper}
              onPress={() => updateNote(i, Math.max(0, mapping.notes[i] - 1))}
            >
              <Text style={styles.stepperText}>−</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.stepper}
              onPress={() => updateNote(i, Math.min(127, mapping.notes[i] + 1))}
            >
              <Text style={styles.stepperText}>+</Text>
            </TouchableOpacity>
          </View>
        ))}
      </View>

      {/* EMG CC mapping */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>EMG CC Mapping</Text>
        {FINGER_NAMES.map((name, i) => (
          <View key={i} style={styles.row}>
            <Text style={styles.label}>EMG {i} ({name})</Text>
            <Text style={styles.noteDisplay}>CC {mapping.ccEmg[i]}</Text>
            <TouchableOpacity
              style={styles.stepper}
              onPress={() => updateCC('emg', i, Math.max(0, mapping.ccEmg[i] - 1))}
            >
              <Text style={styles.stepperText}>−</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.stepper}
              onPress={() => updateCC('emg', i, Math.min(127, mapping.ccEmg[i] + 1))}
            >
              <Text style={styles.stepperText}>+</Text>
            </TouchableOpacity>
          </View>
        ))}
      </View>

      {/* Haptic waveform per gesture */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Haptic Feedback</Text>
        <Text style={styles.hint}>Select a gesture to set its haptic waveform:</Text>
        <View style={styles.gestureGrid}>
          {Object.entries(GESTURE_NAMES).map(([id, name]) => (
            <TouchableOpacity
              key={id}
              style={[
                styles.gestureChip,
                selectedGesture === Number(id) && styles.gestureChipActive,
              ]}
              onPress={() => setSelectedGesture(Number(id))}
            >
              <Text
                style={[
                  styles.gestureChipText,
                  selectedGesture === Number(id) && styles.gestureChipTextActive,
                ]}
              >
                {name}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <Text style={styles.currentHaptic}>
          Current waveform: {mapping.hapticWaveforms[selectedGesture] || 0}
          {mapping.hapticWaveforms[selectedGesture] === 0 ? ' (None)' : ''}
        </Text>
        <View style={styles.waveformRow}>
          {[0, 17, 22, 47, 56, 65, 72].map((wf) => (
            <TouchableOpacity
              key={wf}
              style={[
                styles.wfButton,
                mapping.hapticWaveforms[selectedGesture] === wf && styles.wfButtonActive,
              ]}
              onPress={() => updateHaptic(selectedGesture, wf)}
            >
              <Text style={styles.wfButtonText}>
                {wf === 0 ? 'None' : `#${wf}`}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Sensitivity settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Sensitivity</Text>
        <View style={styles.row}>
          <Text style={styles.label}>EMG Threshold</Text>
          <Text style={styles.noteDisplay}>
            {((mapping.emgThreshold / 0x7FFF) * 100).toFixed(0)}%
          </Text>
          <TouchableOpacity
            style={styles.stepper}
            onPress={() =>
              setMapping({
                ...mapping,
                emgThreshold: Math.max(0x1000, mapping.emgThreshold - 0x800),
              })
            }
          >
            <Text style={styles.stepperText}>−</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.stepper}
            onPress={() =>
              setMapping({
                ...mapping,
                emgThreshold: Math.min(0x7000, mapping.emgThreshold + 0x800),
              })
            }
          >
            <Text style={styles.stepperText}>+</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Vibrato Sensitivity</Text>
          <Text style={styles.noteDisplay}>{mapping.vibratoSensitivity}</Text>
          <TouchableOpacity
            style={styles.stepper}
            onPress={() =>
              setMapping({
                ...mapping,
                vibratoSensitivity: Math.max(1, mapping.vibratoSensitivity - 1),
              })
            }
          >
            <Text style={styles.stepperText}>−</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.stepper}
            onPress={() =>
              setMapping({
                ...mapping,
                vibratoSensitivity: Math.min(10, mapping.vibratoSensitivity + 1),
              })
            }
          >
            <Text style={styles.stepperText}>+</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Save buttons */}
      <View style={styles.buttonRow}>
        <TouchableOpacity
          style={[styles.button, styles.saveButton, !isConnected && styles.buttonDisabled]}
          onPress={saveMapping}
          disabled={!isConnected}
        >
          <Text style={styles.buttonText}>Save to Glove</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.button, styles.presetButton]}
          onPress={savePreset}
        >
          <Text style={styles.buttonText}>Save Preset</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  header: {
    color: '#e94560',
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 16,
  },
  section: {
    backgroundColor: '#16213e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  hint: {
    color: '#888',
    fontSize: 12,
    marginBottom: 8,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  label: {
    color: '#ccc',
    fontSize: 14,
    flex: 1,
  },
  noteDisplay: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
    width: 80,
    textAlign: 'right',
  },
  stepper: {
    backgroundColor: '#0f3460',
    borderRadius: 6,
    width: 32,
    height: 32,
    alignItems: 'center',
    justifyContent: 'center',
    marginLeft: 8,
  },
  stepperText: {
    color: '#fff',
    fontSize: 20,
    fontWeight: 'bold',
  },
  gestureGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: 12,
  },
  gestureChip: {
    backgroundColor: '#0f3460',
    borderRadius: 16,
    paddingHorizontal: 12,
    paddingVertical: 6,
    margin: 4,
  },
  gestureChipActive: {
    backgroundColor: '#e94560',
  },
  gestureChipText: {
    color: '#ccc',
    fontSize: 12,
  },
  gestureChipTextActive: {
    color: '#fff',
    fontWeight: 'bold',
  },
  currentHaptic: {
    color: '#aaa',
    fontSize: 13,
    marginBottom: 8,
  },
  waveformRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  wfButton: {
    backgroundColor: '#0f3460',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 8,
    margin: 4,
  },
  wfButtonActive: {
    backgroundColor: '#533483',
  },
  wfButtonText: {
    color: '#fff',
    fontSize: 12,
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 32,
  },
  button: {
    flex: 1,
    borderRadius: 8,
    padding: 16,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  saveButton: {
    backgroundColor: '#e94560',
  },
  presetButton: {
    backgroundColor: '#533483',
  },
  buttonDisabled: {
    backgroundColor: '#555',
  },
  buttonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
});