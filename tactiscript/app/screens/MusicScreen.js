/**
 * MusicScreen.js — Music-to-haptic translation mode
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Translates music rhythm and melody contour into tactile patterns
 * on the ring. Designed primarily for hearing-impaired users to
 * experience music through touch. Includes a rhythm visualizer and
 * tempo control.
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView,
} from 'react-native';
import { BLEManager, Commands } from '../utils/ble';

export default function MusicScreen() {
  const [playing, setPlaying] = useState(false);
  const [tempo, setTempo] = useState(120); // BPM
  const [beat, setBeat] = useState(0);
  const [intensity, setIntensity] = useState(80);
  const beatTimerRef = useRef(null);

  // Beat pattern: strong-weak-strong-weak (4/4 time)
  const isStrong = beat % 2 === 0;

  const startMusic = useCallback(() => {
    BLEManager.sendCommand(Commands.MODE_MUSIC);
    BLEManager.sendCommand(Commands.RESUME);
    setPlaying(true);
  }, []);

  const stopMusic = useCallback(() => {
    BLEManager.sendCommand(Commands.PAUSE);
    setPlaying(false);
  }, []);

  // Beat visualizer timer
  useEffect(() => {
    if (!playing) {
      if (beatTimerRef.current) clearInterval(beatTimerRef.current);
      return;
    }
    const intervalMs = 60000 / tempo; // ms per beat
    beatTimerRef.current = setInterval(() => {
      setBeat(b => (b + 1) % 4);
    }, intervalMs);
    return () => {
      if (beatTimerRef.current) clearInterval(beatTimerRef.current);
    };
  }, [playing, tempo]);

  const handleTempoChange = useCallback((newTempo) => {
    setTempo(newTempo);
    // In production, send tempo to device
  }, []);

  const handleIntensityChange = useCallback((newIntensity) => {
    setIntensity(newIntensity);
    const delta = Math.round((newIntensity - intensity) / 10);
    if (delta > 0) {
      for (let i = 0; i < delta; i++) BLEManager.sendCommand(Commands.INTENSITY_UP);
    } else {
      for (let i = 0; i < -delta; i++) BLEManager.sendCommand(Commands.INTENSITY_DOWN);
    }
  }, [intensity]);

  // Preset rhythms
  const presets = [
    { name: 'Rock 4/4', tempo: 120, pattern: [1, 0, 1, 0] },
    { name: 'Waltz 3/4', tempo: 90, pattern: [1, 0, 0] },
    { name: 'March 2/4', tempo: 140, pattern: [1, 0] },
    { name: 'Slow Ballad', tempo: 60, pattern: [1, 0, 0, 0] },
  ];

  const loadPreset = useCallback((preset) => {
    setTempo(preset.tempo);
    startMusic();
  }, [startMusic]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Music Haptics</Text>
      <Text style={styles.subtitle}>Feel the rhythm and melody</Text>

      {/* Beat visualizer */}
      <View style={styles.visualizer}>
        <Text style={styles.beatLabel}>Beat:</Text>
        <View style={styles.beatDots}>
          {[0, 1, 2, 3].map(i => (
            <View
              key={i}
              style={[
                styles.beatDot,
                playing && beat === i && styles.beatDotActive,
                !playing && styles.beatDotInactive,
              ]}
            />
          ))}
        </View>
        <Text style={styles.beatInfo}>
          {playing ? (isStrong ? 'STRONG' : 'weak') : '—'}
        </Text>
      </View>

      {/* Tempo control */}
      <View style={styles.controlRow}>
        <Text style={styles.label}>Tempo: {tempo} BPM</Text>
        <View style={styles.tempoRow}>
          {[60, 90, 120, 140, 180].map(t => (
            <TouchableOpacity
              key={t}
              style={[styles.tempoBtn, tempo === t && styles.tempoBtnActive]}
              onPress={() => handleTempoChange(t)}
            >
              <Text style={styles.tempoBtnText}>{t}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Intensity control */}
      <View style={styles.controlRow}>
        <Text style={styles.label}>Intensity: {intensity}%</Text>
        <View style={styles.tempoRow}>
          {[40, 60, 80, 100].map(i => (
            <TouchableOpacity
              key={i}
              style={[styles.tempoBtn, intensity === i && styles.tempoBtnActive]}
              onPress={() => handleIntensityChange(i)}
            >
              <Text style={styles.tempoBtnText}>{i}%</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Play/Stop */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.playBtn, playing ? styles.stopBtn : styles.startBtn]}
          onPress={playing ? stopMusic : startMusic}
        >
          <Text style={styles.playBtnText}>
            {playing ? '⏸ Stop' : '▶ Play'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Preset rhythms */}
      <Text style={styles.sectionLabel}>Rhythm Presets:</Text>
      <View style={styles.presetRow}>
        {presets.map(p => (
          <TouchableOpacity
            key={p.name}
            style={styles.presetBtn}
            onPress={() => loadPreset(p)}
          >
            <Text style={styles.presetName}>{p.name}</Text>
            <Text style={styles.presetTempo}>{p.tempo} BPM</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Info */}
      <View style={styles.infoBox}>
        <Text style={styles.infoTitle}>How it works:</Text>
        <Text style={styles.infoText}>
          The ring renders rhythmic pulses on the 4×6 actuator array.
          Strong beats activate the full array; weak beats activate the
          middle rows. Melody contour is represented by which rows are
          active: high pitch = top rows, low pitch = bottom rows.
        </Text>
      </View>

      <Text style={styles.author}>Author: jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f5f5f5' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#0066CC' },
  subtitle: { fontSize: 14, color: '#666', marginBottom: 16 },
  visualizer: {
    backgroundColor: '#fff', borderRadius: 12, padding: 20,
    alignItems: 'center', marginBottom: 16,
  },
  beatLabel: { fontSize: 14, color: '#666' },
  beatDots: { flexDirection: 'row', marginVertical: 12 },
  beatDot: { width: 24, height: 24, borderRadius: 12, margin: 8, backgroundColor: '#ddd' },
  beatDotActive: { backgroundColor: '#0066CC', transform: [{ scale: 1.3 }] },
  beatDotInactive: { backgroundColor: '#eee' },
  beatInfo: { fontSize: 18, fontWeight: 'bold', color: '#0066CC' },
  controlRow: { marginBottom: 16 },
  label: { fontSize: 14, fontWeight: 'bold', color: '#333', marginBottom: 8 },
  tempoRow: { flexDirection: 'row', flexWrap: 'wrap' },
  tempoBtn: { padding: 8, margin: 4, borderRadius: 6, backgroundColor: '#e0e0e0', minWidth: 50, alignItems: 'center' },
  tempoBtnActive: { backgroundColor: '#0066CC' },
  tempoBtnText: { color: '#333', fontWeight: 'bold' },
  actionRow: { flexDirection: 'row', justifyContent: 'center', marginBottom: 16 },
  playBtn: { padding: 16, borderRadius: 12, minWidth: 200, alignItems: 'center' },
  startBtn: { backgroundColor: '#008800' },
  stopBtn: { backgroundColor: '#cc0000' },
  playBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 18 },
  sectionLabel: { fontSize: 16, fontWeight: 'bold', color: '#333', marginBottom: 8 },
  presetRow: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  presetBtn: { width: '48%', backgroundColor: '#fff', borderRadius: 8, padding: 12, marginBottom: 8, borderWidth: 1, borderColor: '#ddd' },
  presetName: { fontSize: 14, fontWeight: 'bold', color: '#333' },
  presetTempo: { fontSize: 12, color: '#666' },
  infoBox: { backgroundColor: '#e8f4f8', borderRadius: 8, padding: 12, marginTop: 16 },
  infoTitle: { fontSize: 14, fontWeight: 'bold', color: '#0066CC', marginBottom: 4 },
  infoText: { fontSize: 12, color: '#333', lineHeight: 18 },
  author: { fontSize: 10, color: '#999', marginTop: 20, textAlign: 'center' },
});