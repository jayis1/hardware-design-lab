/**
 * ThermalPreview — Live 12×8 thermal array preview component
 *
 * Renders a visual representation of the thermal array, showing each cell's
 * target temperature as a colored tile. Warm cells are red-orange, cool cells
 * are blue, neutral cells are gray.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import type { ThermalFrame } from '../types';

interface Props {
  frame: ThermalFrame | null;
  onCellPress?: (row: number, col: number) => void;
  interactive?: boolean;
}

const COLS = 12;
const ROWS = 8;
const AMBIENT = 25;

/**
 * Map a temperature to a color.
 * Below ambient = blue (cool), above ambient = red-orange (warm).
 */
function tempToColor(tempC: number, active: boolean): string {
  if (!active || tempC === 0) return '#333333';
  const delta = tempC - AMBIENT;
  if (delta > 0) {
    // Warm: 0→dark red, max→bright orange
    const intensity = Math.min(1, delta / 10);
    const r = Math.round(180 + intensity * 75);
    const g = Math.round(60 + intensity * 100);
    const b = Math.round(20 + intensity * 20);
    return `rgb(${r}, ${g}, ${b})`;
  } else if (delta < 0) {
    // Cool: 0→dark blue, max→bright cyan
    const intensity = Math.min(1, -delta / 10);
    const r = Math.round(20 + intensity * 20);
    const g = Math.round(80 + intensity * 100);
    const b = Math.round(180 + intensity * 75);
    return `rgb(${r}, ${g}, ${b})`;
  }
  return '#555555';
}

export const ThermalPreview: React.FC<Props> = ({ frame, onCellPress, interactive = false }) => {
  const cells = frame?.cells ?? [];

  const renderCell = (row: number, col: number) => {
    const cell = cells.find(c => c.row === row && c.col === col);
    const temp = cell?.targetTempC ?? 0;
    const color = tempToColor(temp, frame?.active ?? false);

    return (
      <TouchableOpacity
        key={`${row}-${col}`}
        style={[styles.cell, { backgroundColor: color }]}
        onPress={() => interactive && onCellPress?.(row, col)}
        disabled={!interactive}
        activeOpacity={0.6}
      >
        {interactive && (
          <Text style={styles.cellText}>
            {cell?.targetTempC ? cell.targetTempC.toFixed(0) : '·'}
          </Text>
        )}
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.label}>Thermal Array (12×8 = 96 cells)</Text>
      <View style={styles.grid}>
        {Array.from({ length: ROWS }, (_, row) => (
          <View key={row} style={styles.row}>
            {Array.from({ length: COLS }, (_, col) => renderCell(row, col))}
          </View>
        ))}
      </View>
      <View style={styles.legend}>
        <View style={[styles.legendItem, { backgroundColor: 'rgb(50, 80, 200)' }]} />
        <Text style={styles.legendText}>Cool</Text>
        <View style={[styles.legendItem, { backgroundColor: '#333' }]} />
        <Text style={styles.legendText}>Neutral</Text>
        <View style={[styles.legendItem, { backgroundColor: 'rgb(255, 160, 40)' }]} />
        <Text style={styles.legendText}>Warm</Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    padding: 12,
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    marginVertical: 8,
  },
  label: {
    color: '#888',
    fontSize: 12,
    marginBottom: 8,
    fontFamily: 'monospace',
  },
  grid: {
    flexDirection: 'column',
    alignItems: 'center',
  },
  row: {
    flexDirection: 'row',
    gap: 2,
  },
  cell: {
    width: 22,
    height: 22,
    borderRadius: 3,
    justifyContent: 'center',
    alignItems: 'center',
  },
  cellText: {
    color: '#fff',
    fontSize: 8,
    fontFamily: 'monospace',
  },
  legend: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    marginTop: 10,
  },
  legendItem: {
    width: 12,
    height: 12,
    borderRadius: 2,
  },
  legendText: {
    color: '#888',
    fontSize: 11,
    marginRight: 8,
  },
});