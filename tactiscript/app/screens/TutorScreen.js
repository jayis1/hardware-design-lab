/**
 * TutorScreen.js — Interactive Braille learning mode
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Helps users learn Braille by displaying a character on screen,
 * rendering it on the ring, and asking the user to identify it.
 * Supports letters A-Z and numbers 0-9.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView,
} from 'react-native';
import { BLEManager, Commands } from '../utils/ble';
import { charToPattern } from '../utils/braille';
import BraillePreview from '../components/BraillePreview';

const TUTOR_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';

export default function TutorScreen() {
  const [currentChar, setCurrentChar] = useState('A');
  const [showAnswer, setShowAnswer] = useState(false);
  const [score, setScore] = useState(0);
  const [attempts, setAttempts] = useState(0);
  const [mode, setMode] = useState('letter'); // 'letter' or 'number'

  const currentPattern = charToPattern(currentChar, 1);

  const nextChar = useCallback(() => {
    let newChar;
    if (mode === 'letter') {
      newChar = String.fromCharCode(65 + Math.floor(Math.random() * 26));
    } else {
      newChar = String.fromCharCode(48 + Math.floor(Math.random() * 10));
    }
    setCurrentChar(newChar);
    setShowAnswer(false);
    // Send to ring for tactile rendering
    BLEManager.sendCommand(Commands.MODE_TUTOR);
  }, [mode]);

  useEffect(() => {
    // Start tutor mode on mount
    BLEManager.sendCommand(Commands.MODE_TUTOR);
  }, []);

  const handleReveal = useCallback(() => {
    setShowAnswer(true);
    // Re-render the character on the ring
    BLEManager.sendText(currentChar);
  }, [currentChar]);

  const handleCorrect = useCallback(() => {
    setScore(s => s + 1);
    setAttempts(a => a + 1);
    nextChar();
  }, [nextChar]);

  const handleIncorrect = useCallback(() => {
    setAttempts(a => a + 1);
    nextChar();
  }, [nextChar]);

  const switchMode = useCallback((newMode) => {
    setMode(newMode);
    setShowAnswer(false);
  }, []);

  const accuracy = attempts > 0 ? Math.round((score / attempts) * 100) : 0;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Braille Tutor</Text>
      <Text style={styles.subtitle}>Learn Braille by touch</Text>

      {/* Mode switcher */}
      <View style={styles.modeRow}>
        <TouchableOpacity
          style={[styles.modeBtn, mode === 'letter' && styles.modeBtnActive]}
          onPress={() => switchMode('letter')}
        >
          <Text style={styles.modeBtnText}>Letters A-Z</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.modeBtn, mode === 'number' && styles.modeBtnActive]}
          onPress={() => switchMode('number')}
        >
          <Text style={styles.modeBtnText}>Numbers 0-9</Text>
        </TouchableOpacity>
      </View>

      {/* Score display */}
      <View style={styles.scoreBox}>
        <Text style={styles.scoreText}>Score: {score}/{attempts}</Text>
        <Text style={styles.scoreText}>Accuracy: {accuracy}%</Text>
      </View>

      {/* Current character */}
      <View style={styles.charBox}>
        <Text style={styles.prompt}>Feel the pattern on your finger, then guess:</Text>
        {showAnswer ? (
          <View style={styles.answerBox}>
            <Text style={styles.charDisplay}>{currentChar}</Text>
            <BraillePreview pattern={currentPattern} size={60} />
          </View>
        ) : (
          <Text style={styles.hiddenChar}>?</Text>
        )}
      </View>

      {/* Action buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.revealBtn} onPress={handleReveal}>
          <Text style={styles.btnText}>Reveal Answer</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.guessPrompt}>Did you guess correctly?</Text>
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.correctBtn} onPress={handleCorrect}>
          <Text style={styles.btnText}>✓ Yes</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.incorrectBtn} onPress={handleIncorrect}>
          <Text style={styles.btnText}>✗ No</Text>
        </TouchableOpacity>
      </View>

      {/* Full alphabet reference */}
      <Text style={styles.sectionLabel}>Braille Reference Chart:</Text>
      <View style={styles.chartRow}>
        {TUTOR_CHARS.split('').slice(0, 13).map(c => (
          <View key={c} style={styles.chartCell}>
            <BraillePreview pattern={charToPattern(c, 1)} size={24} />
            <Text style={styles.chartLabel}>{c}</Text>
          </View>
        ))}
      </View>
      <View style={styles.chartRow}>
        {TUTOR_CHARS.split('').slice(13, 26).map(c => (
          <View key={c} style={styles.chartCell}>
            <BraillePreview pattern={charToPattern(c, 1)} size={24} />
            <Text style={styles.chartLabel}>{c}</Text>
          </View>
        ))}
      </View>

      <Text style={styles.author}>Author: jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f5f5f5' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#0066CC' },
  subtitle: { fontSize: 14, color: '#666', marginBottom: 16 },
  modeRow: { flexDirection: 'row', justifyContent: 'center', marginBottom: 16 },
  modeBtn: { padding: 10, margin: 4, borderRadius: 8, backgroundColor: '#e0e0e0' },
  modeBtnActive: { backgroundColor: '#0066CC' },
  modeBtnText: { color: '#333', fontWeight: 'bold' },
  scoreBox: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 16 },
  scoreText: { fontSize: 16, fontWeight: 'bold', color: '#333' },
  charBox: { backgroundColor: '#fff', borderRadius: 12, padding: 20, alignItems: 'center', marginBottom: 16 },
  prompt: { fontSize: 14, color: '#666', marginBottom: 12, textAlign: 'center' },
  hiddenChar: { fontSize: 80, fontWeight: 'bold', color: '#ccc' },
  answerBox: { alignItems: 'center' },
  charDisplay: { fontSize: 48, fontWeight: 'bold', color: '#0066CC' },
  actionRow: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 12 },
  revealBtn: { backgroundColor: '#0066CC', padding: 14, borderRadius: 10, flex: 1, alignItems: 'center' },
  correctBtn: { backgroundColor: '#008800', padding: 14, borderRadius: 10, flex: 1, marginRight: 4, alignItems: 'center' },
  incorrectBtn: { backgroundColor: '#cc0000', padding: 14, borderRadius: 10, flex: 1, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  guessPrompt: { fontSize: 14, color: '#666', textAlign: 'center', marginBottom: 8 },
  sectionLabel: { fontSize: 16, fontWeight: 'bold', color: '#333', marginTop: 16, marginBottom: 8 },
  chartRow: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'center', marginBottom: 8 },
  chartCell: { alignItems: 'center', margin: 4 },
  chartLabel: { fontSize: 10, color: '#666', marginTop: 2 },
  author: { fontSize: 10, color: '#999', marginTop: 20, textAlign: 'center' },
});