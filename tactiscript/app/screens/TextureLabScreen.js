/**
 * TextureLabScreen.js — Custom tactile texture designer
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Lets users design custom 4×6 tactile patterns by tapping on a grid
 * and sending them to the ring. Includes preset textures and an
 * animation toggle for traveling-wave effects.
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView,
} from 'react-native';
import { BLEManager, Commands } from '../utils/ble';

const ROWS = 4;
const COLS = 6;
const TOTAL = ROWS * COLS;

// Preset textures (4×6 = 24 bytes, values 0-255)
const PRESETS = {
  grid: [
    255, 0, 255, 0, 255, 0,
    0, 255, 0, 255, 0, 255,
    255, 0, 255, 0, 255, 0,
    0, 255, 0, 255, 0, 255,
  ],
  wave: [
    50, 100, 150, 200, 150, 100,
    100, 150, 200, 255, 200, 150,
    150, 200, 255, 200, 150, 100,
    200, 255, 200, 150, 100, 50,
  ],
  diagonal: [
    255, 200, 150, 100, 50, 0,
    200, 255, 200, 150, 100, 50,
    150, 200, 255, 200, 150, 100,
    100, 150, 200, 255, 200, 150,
  ],
  checker: [
    255, 0, 255, 0, 255, 0,
    255, 0, 255, 0, 255, 0,
    255, 0, 255, 0, 255, 0,
    255, 0, 255, 0, 255, 0,
  ],
  gradient: [
    255, 255, 255, 255, 255, 255,
    200, 200, 200, 200, 200, 200,
    100, 100, 100, 100, 100, 100,
    50, 50, 50, 50, 50, 50,
  ],
  ripple: [
    0, 50, 100, 100, 50, 0,
    50, 150, 200, 200, 150, 50,
    100, 200, 255, 255, 200, 100,
    50, 150, 200, 200, 150, 50,
  ],
};

export default function TextureLabScreen() {
  // Grid state: array of 24 intensity values (0-255)
  const [grid, setGrid] = useState(new Array(TOTAL).fill(0));
  const [brushValue, setBrushValue] = useState(255);
  const [animating, setAnimating] = useState(false);

  const handleCellPress = useCallback((index) => {
    setGrid(prev => {
      const newGrid = [...prev];
      newGrid[index] = newGrid[index] === brushValue ? 0 : brushValue;
      return newGrid;
    });
  }, [brushValue]);

  const handleSend = useCallback(() => {
    BLEManager.sendCommand(Commands.MODE_TEXTURE);
    BLEManager.sendTexture(grid);
  }, [grid]);

  const handleClear = useCallback(() => {
    setGrid(new Array(TOTAL).fill(0));
  }, []);

  const handleLoadPreset = useCallback((presetName) => {
    const preset = PRESETS[presetName];
    if (preset) {
      setGrid([...preset]);
    }
  }, []);

  const handleAnimate = useCallback(() => {
    setAnimating(!animating);
    BLEManager.sendCommand(Commands.MODE_TEXTURE);
    // In a full implementation, we'd stream animated frames here
  }, [animating]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Texture Lab</Text>
      <Text style={styles.subtitle}>Design custom tactile patterns</Text>

      {/* 4×6 grid editor */}
      <View style={styles.gridContainer}>
        <Text style={styles.label}>Tap cells to toggle (4×6 array):</Text>
        <View style={styles.grid}>
          {Array.from({ length: ROWS }).map((_, row) => (
            <View key={row} style={styles.gridRow}>
              {Array.from({ length: COLS }).map((_, col) => {
                const idx = row * COLS + col;
                const value = grid[idx];
                const intensity = value / 255;
                return (
                  <TouchableOpacity
                    key={col}
                    style={[
                      styles.cell,
                      {
                        backgroundColor: `rgba(0, 102, 204, ${intensity})`,
                        borderWidth: value > 0 ? 1 : 0.5,
                      },
                    ]}
                    onPress={() => handleCellPress(idx)}
                  />
                );
              })}
            </View>
          ))}
        </View>
      </View>

      {/* Brush intensity selector */}
      <View style={styles.brushRow}>
        <Text style={styles.label}>Brush Intensity:</Text>
        <View style={styles.brushOptions}>
          {[64, 128, 192, 255].map(v => (
            <TouchableOpacity
              key={v}
              style={[
                styles.brushBtn,
                brushValue === v && styles.brushBtnActive,
                { backgroundColor: `rgba(0, 102, 204, ${v / 255})` },
              ]}
              onPress={() => setBrushValue(v)}
            >
              <Text style={styles.brushBtnText}>{Math.round(v / 255 * 100)}%</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Action buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.sendBtn} onPress={handleSend}>
          <Text style={styles.btnText}>Send to Ring</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.animBtn, animating && styles.animBtnActive]}
          onPress={handleAnimate}
        >
          <Text style={styles.btnText}>{animating ? 'Stop Anim' : 'Animate'}</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.clearBtn} onPress={handleClear}>
          <Text style={styles.btnText}>Clear</Text>
        </TouchableOpacity>
      </View>

      {/* Preset textures */}
      <Text style={styles.sectionLabel}>Preset Textures:</Text>
      <View style={styles.presetRow}>
        {Object.keys(PRESETS).map(name => (
          <TouchableOpacity
            key={name}
            style={styles.presetBtn}
            onPress={() => handleLoadPreset(name)}
          >
            <Text style={styles.presetName}>{name}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Cell values display (for debugging) */}
      <Text style={styles.sectionLabel}>Raw Values:</Text>
      <View style={styles.rawBox}>
        <Text style={styles.rawText}>{grid.join(', ')}</Text>
      </View>

      <Text style={styles.author}>Author: jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f5f5f5' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#0066CC' },
  subtitle: { fontSize: 14, color: '#666', marginBottom: 16 },
  gridContainer: { backgroundColor: '#fff', borderRadius: 12, padding: 16, marginBottom: 16 },
  label: { fontSize: 14, fontWeight: 'bold', color: '#333', marginBottom: 8 },
  grid: { alignItems: 'center' },
  gridRow: { flexDirection: 'row' },
  cell: {
    width: 40, height: 40, margin: 2, borderRadius: 6,
    borderColor: '#0066CC',
  },
  brushRow: { marginBottom: 16 },
  brushOptions: { flexDirection: 'row', justifyContent: 'space-around' },
  brushBtn: { padding: 10, borderRadius: 8, minWidth: 60, alignItems: 'center', borderWidth: 2, borderColor: '#ddd' },
  brushBtnActive: { borderColor: '#0066CC', borderWidth: 3 },
  brushBtnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  actionRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 16 },
  sendBtn: { backgroundColor: '#008800', padding: 14, borderRadius: 10, flex: 1, marginRight: 4, alignItems: 'center' },
  animBtn: { backgroundColor: '#0066CC', padding: 14, borderRadius: 10, flex: 1, marginRight: 4, alignItems: 'center' },
  animBtnActive: { backgroundColor: '#cc8800' },
  clearBtn: { backgroundColor: '#cc0000', padding: 14, borderRadius: 10, flex: 1, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: 'bold' },
  sectionLabel: { fontSize: 16, fontWeight: 'bold', color: '#333', marginBottom: 8 },
  presetRow: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between', marginBottom: 16 },
  presetBtn: { width: '31%', backgroundColor: '#fff', borderRadius: 8, padding: 10, marginBottom: 8, borderWidth: 1, borderColor: '#ddd', alignItems: 'center' },
  presetName: { fontSize: 12, fontWeight: 'bold', color: '#0066CC' },
  rawBox: { backgroundColor: '#f0f0f0', borderRadius: 8, padding: 8, marginBottom: 16 },
  rawText: { fontSize: 10, color: '#666', fontFamily: 'monospace' },
  author: { fontSize: 10, color: '#999', marginTop: 20, textAlign: 'center' },
});