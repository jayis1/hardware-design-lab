/**
 * AudioInspectorScreen — Live audio spectrogram over BLE
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, FlatList,
} from 'react-native';
import SpectrogramView from '../components/SpectrogramView';
import { SpectrogramData, AudioLevel, ColonyState } from '../types';

export default function AudioInspectorScreen({ route }: any) {
  const [isStreaming, setIsStreaming] = useState(false);
  const [spectrogramData, setSpectrogramData] = useState<SpectrogramData | null>(null);
  const [levels, setLevels] = useState<AudioLevel[]>([
    { channel: 0, level: 0, timestamp: 0 },
    { channel: 1, level: 0, timestamp: 0 },
    { channel: 2, level: 0, timestamp: 0 },
    { channel: 3, level: 0, timestamp: 0 },
  ]);
  const [selectedMic, setSelectedMic] = useState(0);
  const [recordedClip, setRecordedClip] = useState(false);
  const streamRef = useRef<any>(null);

  const micNames = [
    'Interior 1 (Brood)',
    'Interior 2 (Brood)',
    'Entrance L',
    'Entrance R',
  ];

  // Simulated audio stream (in production: BLE characteristic notifications)
  useEffect(() => {
    if (isStreaming) {
      streamRef.current = setInterval(() => {
        // Generate simulated FFT magnitudes (in dB, -80 to 0)
        const freqs: number[] = [];
        const mags: number[] = [];
        for (let i = 0; i < 128; i++) {
          const freq = (i * 24000) / 128;
          freqs.push(freq);
          // Simulate bee hum fundamental at ~250 Hz with harmonics
          let mag = -60 + Math.random() * 10;
          if (Math.abs(freq - 250) < 30) mag = -15 + Math.random() * 5;
          if (Math.abs(freq - 500) < 30) mag = -25 + Math.random() * 5;
          if (Math.abs(freq - 750) < 30) mag = -35 + Math.random() * 5;
          if (Math.abs(freq - 1000) < 30) mag = -45 + Math.random() * 5;
          mags.push(mag);
        }

        setSpectrogramData({
          frequencies: freqs,
          magnitudes: [mags],
          sampleRate: 48000,
          duration: 0.25,
        });

        // Update audio levels
        setLevels(prev => prev.map(l => ({
          ...l,
          level: 0.3 + Math.random() * 0.4,
          timestamp: Date.now(),
        })));
      }, 250); // 4 frames per second
    } else {
      if (streamRef.current) {
        clearInterval(streamRef.current);
        streamRef.current = null;
      }
    }

    return () => {
      if (streamRef.current) clearInterval(streamRef.current);
    };
  }, [isStreaming]);

  const handleStartStream = () => setIsStreaming(true);
  const handleStopStream = () => setIsStreaming(false);
  const handleRecord = () => {
    setRecordedClip(true);
    setTimeout(() => setRecordedClip(false), 2000);
  };

  const renderLevelBar = (level: AudioLevel) => (
    <View key={level.channel} style={styles.levelBar}>
      <Text style={styles.levelLabel}>{micNames[level.channel]}</Text>
      <View style={styles.levelTrack}>
        <View style={[styles.levelFill, { width: `${level.level * 100}%` }]} />
      </View>
      <Text style={styles.levelValue}>{(level.level * 100).toFixed(0)}</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>🎵 Audio Inspector</Text>
      <Text style={styles.subtitle}>
        {route?.params?.hiveId ? `Hive: ${route.params.hiveId}` : 'Select a hive'}
      </Text>

      {/* Microphone selector */}
      <View style={styles.micSelector}>
        {micNames.map((name, i) => (
          <TouchableOpacity
            key={i}
            style={[
              styles.micButton,
              selectedMic === i && styles.micButtonActive,
            ]}
            onPress={() => setSelectedMic(i)}
          >
            <Text style={[
              styles.micButtonText,
              selectedMic === i && styles.micButtonTextActive,
            ]}>
              {name}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Spectrogram */}
      <View style={styles.spectrogramContainer}>
        <SpectrogramView
          data={spectrogramData}
          width={350}
          height={200}
        />
        <Text style={styles.spectrogramLabel}>
          {isStreaming ? '🔴 LIVE' : '⏸ Paused'} — 0–24 kHz
        </Text>
      </View>

      {/* Audio levels */}
      <View style={styles.levelsContainer}>
        <Text style={styles.sectionTitle}>Channel Levels (RMS)</Text>
        {levels.map(renderLevelBar)}
      </View>

      {/* Controls */}
      <View style={styles.controls}>
        {!isStreaming ? (
          <TouchableOpacity style={styles.startButton} onPress={handleStartStream}>
            <Text style={styles.buttonText}>▶ Start Stream</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.stopButton} onPress={handleStopStream}>
            <Text style={styles.buttonText}>⏹ Stop</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity
          style={[styles.recordButton, recordedClip && styles.recordButtonActive]}
          onPress={handleRecord}
          disabled={!isStreaming}
        >
          <Text style={styles.buttonText}>
            {recordedClip ? '✓ Recorded!' : '● Record 30s'}
          </Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.footer}>Author: jayis1 · BLE 5.2 · OPUS codec</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E', padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: '#E8A317' },
  subtitle: { fontSize: 14, color: '#888', marginTop: 4 },
  micSelector: { flexDirection: 'row', flexWrap: 'wrap', marginTop: 12, gap: 6 },
  micButton: {
    paddingHorizontal: 10, paddingVertical: 6, borderRadius: 6,
    backgroundColor: '#1A1A2E', borderWidth: 1, borderColor: '#333',
  },
  micButtonActive: { backgroundColor: '#E8A317', borderColor: '#E8A317' },
  micButtonText: { fontSize: 11, color: '#888' },
  micButtonTextActive: { fontSize: 11, color: '#1A1A2E', fontWeight: 'bold' },
  spectrogramContainer: { alignItems: 'center', marginTop: 12 },
  spectrogramLabel: { fontSize: 12, color: '#888', marginTop: 4 },
  levelsContainer: { marginTop: 16 },
  sectionTitle: { fontSize: 14, fontWeight: 'bold', color: '#E8A317', marginBottom: 8 },
  levelBar: { flexDirection: 'row', alignItems: 'center', marginBottom: 6 },
  levelLabel: { width: 100, fontSize: 11, color: '#888' },
  levelTrack: { flex: 1, height: 8, backgroundColor: '#1A1A2E', borderRadius: 4, marginHorizontal: 8 },
  levelFill: { height: '100%', backgroundColor: '#4CAF50', borderRadius: 4 },
  levelValue: { width: 30, fontSize: 11, color: '#AAA', textAlign: 'right' },
  controls: { flexDirection: 'row', justifyContent: 'center', marginTop: 20, gap: 12 },
  startButton: { backgroundColor: '#4CAF50', paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8 },
  stopButton: { backgroundColor: '#DC3545', paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8 },
  recordButton: { backgroundColor: '#E8A317', paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8 },
  recordButtonActive: { backgroundColor: '#4CAF50' },
  buttonText: { color: '#FFF', fontSize: 14, fontWeight: 'bold' },
  footer: { fontSize: 11, color: '#444', textAlign: 'center', marginTop: 20 },
});