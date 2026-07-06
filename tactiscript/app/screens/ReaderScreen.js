/**
 * ReaderScreen.js — Text-to-Braille streaming reader
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * The Reader screen lets users type or paste text and stream it
 * to the TactiScript ring for Braille rendering. It also intercepts
 * clipboard content and screen-reader text for accessibility.
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet,
  ScrollView, Switch, Alert,
} from 'react-native';
import { BLEManager, Commands } from '../utils/ble';
import { translateText } from '../utils/braille';
import BraillePreview from '../components/BraillePreview';

export default function ReaderScreen() {
  const [text, setText] = useState('');
  const [streaming, setStreaming] = useState(false);
  const [grade, setGrade] = useState(1); // 1 or 2
  const [speed, setSpeed] = useState(5); // 1-10
  const [intensity, setIntensity] = useState(80); // 0-100

  const patterns = translateText(text, grade);

  const handleSend = useCallback(() => {
    if (!text.trim()) {
      Alert.alert('No Text', 'Please enter some text to send.');
      return;
    }
    BLEManager.sendCommand(grade === 2 ? Commands.BRAILLE_GRADE2 : Commands.BRAILLE_GRADE1);
    BLEManager.sendText(text);
    BLEManager.sendCommand(Commands.MODE_READER);
    BLEManager.sendCommand(Commands.RESUME);
    setStreaming(true);
  }, [text, grade]);

  const handlePause = useCallback(() => {
    BLEManager.sendCommand(Commands.PAUSE);
    setStreaming(false);
  }, []);

  const handleResume = useCallback(() => {
    BLEManager.sendCommand(Commands.RESUME);
    setStreaming(true);
  }, []);

  const handleClear = useCallback(() => {
    BLEManager.sendCommand(Commands.CLEAR);
    setText('');
    setStreaming(false);
  }, []);

  const handleSpeedChange = useCallback((newSpeed) => {
    setSpeed(newSpeed);
    // Send multiple speed up/down commands to reach desired speed
    // (simplified: just send the delta)
    if (newSpeed > speed) {
      for (let i = 0; i < newSpeed - speed; i++) {
        BLEManager.sendCommand(Commands.SPEED_UP);
      }
    } else {
      for (let i = 0; i < speed - newSpeed; i++) {
        BLEManager.sendCommand(Commands.SPEED_DOWN);
      }
    }
  }, [speed]);

  const handleIntensityChange = useCallback((newIntensity) => {
    const delta = Math.round((newIntensity - intensity) / 10);
    setIntensity(newIntensity);
    if (delta > 0) {
      for (let i = 0; i < delta; i++) {
        BLEManager.sendCommand(Commands.INTENSITY_UP);
      }
    } else {
      for (let i = 0; i < -delta; i++) {
        BLEManager.sendCommand(Commands.INTENSITY_DOWN);
      }
    }
  }, [intensity]);

  // Show first 6 patterns as preview
  const previewPatterns = patterns.slice(0, 6);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Reader Mode</Text>
      <Text style={styles.subtitle}>Stream text as Braille to the ring</Text>

      {/* Text input */}
      <TextInput
        style={styles.textInput}
        value={text}
        onChangeText={setText}
        placeholder="Type or paste text here..."
        multiline
        numberOfLines={4}
        textAlignVertical="top"
      />

      {/* Braille preview */}
      <View style={styles.previewSection}>
        <Text style={styles.label}>Braille Preview ({patterns.length} cells):</Text>
        <ScrollView horizontal style={styles.previewScroll}>
          {previewPatterns.length > 0 ? (
            previewPatterns.map((p, i) => (
              <BraillePreview key={i} pattern={p} size={36} />
            ))
          ) : (
            <Text style={styles.hint}>Type text to see Braille preview</Text>
          )}
        </ScrollView>
      </View>

      {/* Grade toggle */}
      <View style={styles.row}>
        <Text style={styles.label}>UEB Grade 2:</Text>
        <Switch
          value={grade === 2}
          onValueChange={(v) => setGrade(v ? 2 : 1)}
        />
        <Text style={styles.gradeLabel}>{grade === 2 ? 'Grade 2 (contracted)' : 'Grade 1 (uncontracted)'}</Text>
      </View>

      {/* Speed slider */}
      <View style={styles.controlRow}>
        <Text style={styles.label}>Scroll Speed: {speed}</Text>
        <View style={styles.buttonRow}>
          {[1, 3, 5, 7, 10].map((s) => (
            <TouchableOpacity
              key={s}
              style={[styles.speedBtn, speed === s && styles.speedBtnActive]}
              onPress={() => handleSpeedChange(s)}
            >
              <Text style={styles.speedBtnText}>{s}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Intensity slider */}
      <View style={styles.controlRow}>
        <Text style={styles.label}>Drive Intensity: {intensity}%</Text>
        <View style={styles.buttonRow}>
          {[40, 60, 80, 100].map((i) => (
            <TouchableOpacity
              key={i}
              style={[styles.speedBtn, intensity === i && styles.speedBtnActive]}
              onPress={() => handleIntensityChange(i)}
            >
              <Text style={styles.speedBtnText}>{i}%</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Action buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.sendBtn} onPress={handleSend}>
          <Text style={styles.btnText}>Send & Stream</Text>
        </TouchableOpacity>
        {streaming ? (
          <TouchableOpacity style={styles.pauseBtn} onPress={handlePause}>
            <Text style={styles.btnText}>Pause</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.resumeBtn} onPress={handleResume}>
            <Text style={styles.btnText}>Resume</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={styles.clearBtn} onPress={handleClear}>
          <Text style={styles.btnText}>Clear</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.author}>Author: jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f5f5f5' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#0066CC' },
  subtitle: { fontSize: 14, color: '#666', marginBottom: 16 },
  textInput: {
    borderWidth: 1, borderColor: '#ccc', borderRadius: 8,
    padding: 12, fontSize: 16, minHeight: 100, backgroundColor: '#fff',
    marginBottom: 16,
  },
  previewSection: { marginBottom: 16 },
  label: { fontSize: 14, fontWeight: 'bold', color: '#333', marginBottom: 8 },
  previewScroll: { flexDirection: 'row', paddingVertical: 8 },
  hint: { fontSize: 14, color: '#999', fontStyle: 'italic' },
  row: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  gradeLabel: { fontSize: 12, color: '#666', marginLeft: 8 },
  controlRow: { marginBottom: 16 },
  buttonRow: { flexDirection: 'row', marginTop: 8 },
  speedBtn: {
    padding: 8, margin: 4, borderRadius: 6, backgroundColor: '#e0e0e0',
    minWidth: 40, alignItems: 'center',
  },
  speedBtnActive: { backgroundColor: '#0066CC' },
  speedBtnText: { color: '#333', fontSize: 14, fontWeight: 'bold' },
  actionRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 16 },
  sendBtn: { backgroundColor: '#008800', padding: 12, borderRadius: 8, flex: 1, marginRight: 4, alignItems: 'center' },
  pauseBtn: { backgroundColor: '#cc8800', padding: 12, borderRadius: 8, flex: 1, marginRight: 4, alignItems: 'center' },
  resumeBtn: { backgroundColor: '#0066CC', padding: 12, borderRadius: 8, flex: 1, marginRight: 4, alignItems: 'center' },
  clearBtn: { backgroundColor: '#cc0000', padding: 12, borderRadius: 8, flex: 1, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: 'bold' },
  author: { fontSize: 10, color: '#999', marginTop: 20, textAlign: 'center' },
});