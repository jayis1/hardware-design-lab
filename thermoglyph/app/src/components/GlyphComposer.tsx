/**
 * GlyphComposer — Interactive thermal pattern editor
 *
 * Allows users to draw custom thermal patterns on a 12×8 grid by tapping
 * cells to toggle them warm/cool/neutral. The pattern can be previewed
 * and sent to the device.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  FlatList, Alert,
} from 'react-native';
import { useDeviceStore } from '../store/deviceStore';
import type { GlyphPolarity } from '../types';

const COLS = 12;
const ROWS = 8;

interface CellState {
  polarity: GlyphPolarity;
  intensity: number;
}

type Grid = (CellState | null)[][];

export const GlyphComposer: React.FC = () => {
  const { sendGlyph, saveTemplate, config } = useDeviceStore();
  const [grid, setGrid] = useState<Grid>(() =>
    Array.from({ length: ROWS }, () => Array.from({ length: COLS }, () => null))
  );
  const [brushMode, setBrushMode] = useState<GlyphPolarity>('warm');
  const [brushIntensity, setBrushIntensity] = useState(160);
  const [templateName, setTemplateName] = useState('');

  const toggleCell = useCallback((row: number, col: number) => {
    setGrid(prev => {
      const next = prev.map(r => [...r]);
      const current = next[row][col];
      if (current && current.polarity === brushMode) {
        next[row][col] = null; // toggle off if same mode
      } else {
        next[row][col] = { polarity: brushMode, intensity: brushIntensity };
      }
      return next;
    });
  }, [brushMode, brushIntensity]);

  const clearGrid = () => {
    setGrid(Array.from({ length: ROWS }, () => Array.from({ length: COLS }, () => null)));
  };

  const sendToDevice = async () => {
    // Convert grid to a series of pixel commands
    const activeCells: { row: number; col: number; cell: CellState }[] = [];
    for (let r = 0; r < ROWS; r++) {
      for (let c = 0; c < COLS; c++) {
        if (grid[r][c]) {
          activeCells.push({ row: r, col: c, cell: grid[r][c]! });
        }
      }
    }

    if (activeCells.length === 0) {
      Alert.alert('Empty', 'Draw a pattern first');
      return;
    }

    // Send each active cell as a pixel command
    for (const { row, col, cell } of activeCells) {
      await sendGlyph({
        type: 'pixel',
        polarity: cell.polarity,
        intensity: cell.intensity,
        durationMs: 3000,
        text: String.fromCharCode(((row & 0x0F) << 4) | (col & 0x0F)),
      });
    }
  };

  const saveAsTemplate = () => {
    if (!templateName.trim()) {
      Alert.alert('Name required', 'Enter a name for this template');
      return;
    }
    // Convert first active cell to a representative command
    const firstActive = grid.flat().find(c => c !== null) as CellState | null;
    if (!firstActive) {
      Alert.alert('Empty', 'Draw a pattern first');
      return;
    }
    saveTemplate({
      id: `tpl-${Date.now()}`,
      name: templateName,
      command: {
        type: 'pixel',
        polarity: firstActive.polarity,
        intensity: firstActive.intensity,
        durationMs: 3000,
      },
      createdBy: 'jayis1',
    });
    Alert.alert('Saved', `Template "${templateName}" saved`);
    setTemplateName('');
  };

  const cellColor = (cell: CellState | null): string => {
    if (!cell) return '#222';
    if (cell.polarity === 'warm') {
      const i = cell.intensity / 255;
      return `rgb(${180 + i * 75}, ${60 + i * 100}, ${20 + i * 20})`;
    } else if (cell.polarity === 'cool') {
      const i = cell.intensity / 255;
      return `rgb(${20 + i * 20}, ${80 + i * 100}, ${180 + i * 75})`;
    }
    return '#555';
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Glyph Composer</Text>
      <Text style={styles.subtitle}>Draw a custom thermal pattern</Text>

      {/* Grid */}
      <View style={styles.gridContainer}>
        {grid.map((row, r) => (
          <View key={r} style={styles.row}>
            {row.map((cell, c) => (
              <TouchableOpacity
                key={c}
                style={[styles.cell, { backgroundColor: cellColor(cell) }]}
                onPress={() => toggleCell(r, c)}
              />
            ))}
          </View>
        ))}
      </View>

      {/* Brush controls */}
      <View style={styles.brushRow}>
        <Text style={styles.label}>Brush:</Text>
        {(['warm', 'cool', 'neutral'] as GlyphPolarity[]).map(mode => (
          <TouchableOpacity
            key={mode}
            style={[
              styles.brushBtn,
              brushMode === mode && styles.brushBtnActive,
              mode === 'warm' && { backgroundColor: brushMode === mode ? '#e65' : '#421' },
              mode === 'cool' && { backgroundColor: brushMode === mode ? '#5af' : '#134' },
              mode === 'neutral' && { backgroundColor: brushMode === mode ? '#888' : '#333' },
            ]}
            onPress={() => setBrushMode(mode)}
          >
            <Text style={styles.brushBtnText}>{mode}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Intensity slider (simplified as +/- buttons) */}
      <View style={styles.intensityRow}>
        <Text style={styles.label}>Intensity: {brushIntensity}</Text>
        <TouchableOpacity style={styles.btn} onPress={() => setBrushIntensity(Math.max(0, brushIntensity - 16))}>
          <Text style={styles.btnText}>−</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btn} onPress={() => setBrushIntensity(Math.min(255, brushIntensity + 16))}>
          <Text style={styles.btnText}>+</Text>
        </TouchableOpacity>
      </View>

      {/* Template name input */}
      <TextInput
        style={styles.input}
        placeholder="Template name..."
        placeholderTextColor="#666"
        value={templateName}
        onChangeText={setTemplateName}
      />

      {/* Action buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.actionBtn} onPress={clearGrid}>
          <Text style={styles.actionBtnText}>Clear</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionBtn, styles.sendBtn]} onPress={sendToDevice}>
          <Text style={styles.actionBtnText}>Send to Device</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionBtn, styles.saveBtn]} onPress={saveAsTemplate}>
          <Text style={styles.actionBtnText}>Save</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    padding: 16,
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    marginVertical: 8,
  },
  title: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  subtitle: {
    color: '#888',
    fontSize: 13,
    marginBottom: 12,
  },
  gridContainer: {
    alignItems: 'center',
    marginVertical: 12,
  },
  row: {
    flexDirection: 'row',
    gap: 2,
  },
  cell: {
    width: 22,
    height: 22,
    borderRadius: 3,
  },
  brushRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginVertical: 8,
  },
  label: {
    color: '#aaa',
    fontSize: 13,
  },
  brushBtn: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
  },
  brushBtnActive: {
    borderWidth: 2,
    borderColor: '#fff',
  },
  brushBtnText: {
    color: '#fff',
    fontSize: 12,
    textTransform: 'capitalize',
  },
  intensityRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginVertical: 8,
  },
  btn: {
    width: 32,
    height: 32,
    borderRadius: 6,
    backgroundColor: '#333',
    justifyContent: 'center',
    alignItems: 'center',
  },
  btnText: {
    color: '#fff',
    fontSize: 18,
  },
  input: {
    backgroundColor: '#252540',
    color: '#fff',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 8,
    marginVertical: 8,
    fontSize: 14,
  },
  actionRow: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 8,
  },
  actionBtn: {
    flex: 1,
    paddingVertical: 10,
    borderRadius: 8,
    backgroundColor: '#333',
    alignItems: 'center',
  },
  sendBtn: {
    backgroundColor: '#2a6',
  },
  saveBtn: {
    backgroundColor: '#36a',
  },
  actionBtnText: {
    color: '#fff',
    fontSize: 13,
    fontWeight: '600',
  },
});