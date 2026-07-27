/**
 * PackBuilderScreen.tsx — Multi-cell pack assembly helper.
 *
 * Lets users scan cells one by one and assign them to a series/parallel
 * pack topology. Flags mismatched cells and recommends groupings.
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
  TextInput,
  TouchableOpacity,
  FlatList,
  Alert,
  Modal,
} from 'react-native';
import { useDatabase, CellRecord } from '../db/database';
import {
  DegradationMode,
  DEGRADATION_NAMES,
  VERDICT_COLORS,
  CHEMISTRY_NAMES,
} from '../ble/protocol';

interface PackSlot {
  slot: number;
  cell: CellRecord | null;
}

export default function PackBuilderScreen() {
  const [packName, setPackName] = useState('');
  const [seriesCount, setSeriesCount] = useState('10');
  const [parallelCount, setParallelCount] = useState('4');
  const [slots, setSlots] = useState<PackSlot[]>([]);
  const [showHistory, setShowHistory] = useState(false);
  const [history, setHistory] = useState<CellRecord[]>([]);
  const [selectedSlot, setSelectedSlot] = useState(0);
  const db = useDatabase();

  const initSlots = useCallback(() => {
    const s = parseInt(seriesCount, 10) || 1;
    const p = parseInt(parallelCount, 10) || 1;
    const total = s * p;
    const newSlots: PackSlot[] = [];
    for (let i = 0; i < total; i++) {
      newSlots.push({ slot: i, cell: null });
    }
    setSlots(newSlots);
  }, [seriesCount, parallelCount]);

  const assignCell = (cell: CellRecord) => {
    setSlots((prev) => {
      const updated = [...prev];
      updated[selectedSlot] = { slot: selectedSlot, cell };
      return updated;
    });
    setShowHistory(false);
  };

  const openCellPicker = (slotIndex: number) => {
    setSelectedSlot(slotIndex);
    db.getHistory().then((h) => {
      setHistory(h);
      setShowHistory(true);
    });
  };

  // Analyze pack health and mismatch
  const analysis = () => {
    const filled = slots.filter((s) => s.cell !== null).map((s) => s.cell!);
    if (filled.length === 0) return null;

    const sohValues = filled.map((c) => c.sohScore);
    const rctValues = filled.map((c) => c.rctMohm);
    const dcirValues = filled.map((c) => c.dcirMohm);

    const sohMin = Math.min(...sohValues);
    const sohMax = Math.max(...sohValues);
    const sohSpread = sohMax - sohMin;

    const rctMin = Math.min(...rctValues);
    const rctMax = Math.max(...rctValues);
    const rctSpread = rctMax - rctMin;

    const dcirMin = Math.min(...dcirValues);
    const dcirMax = Math.max(...dcirValues);
    const dcirSpread = dcirMax - dcirMin;

    const warnings: string[] = [];
    if (sohSpread > 10) {
      warnings.push(`⚠️ SoH spread ${sohSpread}% exceeds 10% — pack imbalance risk`);
    }
    if (rctSpread > 30 && rctMin > 0) {
      warnings.push(`⚠️ Rct spread ${rctSpread} mΩ exceeds 30 mΩ — uneven aging`);
    }
    if (dcirSpread > 20) {
      warnings.push(`⚠️ DCIR spread ${dcirSpread} mΩ exceeds 20 mΩ — current imbalance`);
    }

    const faultyCells = filled.filter(
      (c) => c.degradation === DegradationMode.LithiumPlating ||
             c.degradation === DegradationMode.InternalShort
    );
    if (faultyCells.length > 0) {
      warnings.push(`🚫 ${faultyCells.length} cell(s) have dangerous degradation — remove from pack`);
    }

    const avgSoh = sohValues.reduce((a, b) => a + b, 0) / sohValues.length;
    const nominalV = 3.7 * (parseInt(seriesCount, 10) || 1);
    const nominalAh = (2500 * (parseInt(parallelCount, 10) || 1)) / 1000;
    const nominalWh = nominalV * nominalAh * (avgSoh / 100);

    return {
      filled: filled.length,
      total: slots.length,
      avgSoh: avgSoh.toFixed(1),
      sohSpread,
      rctSpread,
      dcirSpread,
      warnings,
      nominalV: nominalV.toFixed(1),
      nominalAh: nominalAh.toFixed(2),
      nominalWh: nominalWh.toFixed(0),
    };
  };

  const result = analysis();

  return (
    <View style={styles.container}>
      {/* Pack configuration */}
      <View style={styles.configSection}>
        <Text style={styles.sectionTitle}>Pack Configuration</Text>
        <TextInput
          style={styles.input}
          placeholder="Pack name"
          placeholderTextColor="#444466"
          value={packName}
          onChangeText={setPackName}
        />
        <View style={styles.configRow}>
          <View style={styles.configItem}>
            <Text style={styles.configLabel}>Series (S)</Text>
            <TextInput
              style={styles.numInput}
              keyboardType="numeric"
              value={seriesCount}
              onChangeText={setSeriesCount}
            />
          </View>
          <View style={styles.configItem}>
            <Text style={styles.configLabel}>Parallel (P)</Text>
            <TextInput
              style={styles.numInput}
              keyboardType="numeric"
              value={parallelCount}
              onChangeText={setParallelCount}
            />
          </View>
          <TouchableOpacity style={styles.initButton} onPress={initSlots}>
            <Text style={styles.buttonText}>Create</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Pack analysis */}
      {result && (
        <View style={styles.analysisSection}>
          <Text style={styles.sectionTitle}>Pack Analysis</Text>
          <View style={styles.analysisRow}>
            <Text style={styles.analysisLabel}>Cells assigned:</Text>
            <Text style={styles.analysisValue}>{result.filled}/{result.total}</Text>
          </View>
          <View style={styles.analysisRow}>
            <Text style={styles.analysisLabel}>Avg SoH:</Text>
            <Text style={styles.analysisValue}>{result.avgSoh}%</Text>
          </View>
          <View style={styles.analysisRow}>
            <Text style={styles.analysisLabel}>Nominal voltage:</Text>
            <Text style={styles.analysisValue}>{result.nominalV} V</Text>
          </View>
          <View style={styles.analysisRow}>
            <Text style={styles.analysisLabel}>Nominal capacity:</Text>
            <Text style={styles.analysisValue}>{result.nominalAh} Ah</Text>
          </View>
          <View style={styles.analysisRow}>
            <Text style={styles.analysisLabel}>Est. energy:</Text>
            <Text style={styles.analysisValue}>{result.nominalWh} Wh</Text>
          </View>
          {result.warnings.map((w, i) => (
            <Text key={i} style={styles.warningText}>{w}</Text>
          ))}
        </View>
      )}

      {/* Slot grid */}
      {slots.length > 0 && (
        <FlatList
          data={slots}
          keyExtractor={(item) => String(item.slot)}
          renderItem={({ item, index }) => (
            <TouchableOpacity
              style={styles.slotCard}
              onPress={() => openCellPicker(index)}
            >
              <Text style={styles.slotLabel}>
                Slot {item.slot + 1}
                {item.cell ? ` — SoH ${item.cell.sohScore}%` : ' — empty'}
              </Text>
              {item.cell && (
                <View style={[styles.slotBadge,
                  { backgroundColor: VERDICT_COLORS[item.cell.verdict as keyof typeof VERDICT_COLORS] }]}>
                  <Text style={styles.slotBadgeText}>
                    {DEGRADATION_NAMES[item.cell.degradation as DegradationMode]}
                  </Text>
                </View>
              )}
            </TouchableOpacity>
          )}
          contentContainerStyle={styles.slotList}
        />
      )}

      {/* Cell picker modal */}
      <Modal visible={showHistory} animationType="slide">
        <View style={styles.modalContainer}>
          <Text style={styles.modalTitle}>Select Cell for Slot {selectedSlot + 1}</Text>
          <FlatList
            data={history}
            keyExtractor={(item) => String(item.id)}
            renderItem={({ item }) => (
              <TouchableOpacity
                style={styles.historyItem}
                onPress={() => assignCell(item)}
              >
                <Text style={styles.historyItemText}>
                  SoH: {item.sohScore}% | {CHEMISTRY_NAMES[item.chemistryIdx]} | DCIR: {item.dcirMohm}mΩ
                </Text>
              </TouchableOpacity>
            )}
          />
          <TouchableOpacity
            style={styles.closeButton}
            onPress={() => setShowHistory(false)}
          >
            <Text style={styles.buttonText}>Cancel</Text>
          </TouchableOpacity>
        </View>
      </Modal>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#12122a' },
  configSection: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, margin: 12 },
  sectionTitle: { color: '#00b4ff', fontSize: 14, fontWeight: 'bold', marginBottom: 12 },
  input: {
    backgroundColor: '#222244',
    color: '#e0e0e0',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 10,
    marginBottom: 8,
    fontSize: 14,
  },
  configRow: { flexDirection: 'row', gap: 8, alignItems: 'flex-end' },
  configItem: { flex: 1 },
  configLabel: { color: '#8888aa', fontSize: 12, marginBottom: 4 },
  numInput: {
    backgroundColor: '#222244',
    color: '#e0e0e0',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 10,
    fontSize: 14,
    textAlign: 'center',
  },
  initButton: {
    backgroundColor: '#0066cc',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 6,
  },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: '600', textAlign: 'center' },
  analysisSection: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginHorizontal: 12, marginBottom: 12 },
  analysisRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  analysisLabel: { color: '#8888aa', fontSize: 13 },
  analysisValue: { color: '#e0e0e0', fontSize: 13, fontWeight: '600' },
  warningText: { color: '#ff9800', fontSize: 12, marginTop: 6 },
  slotList: { padding: 12 },
  slotCard: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 6,
    padding: 14,
    marginBottom: 6,
  },
  slotLabel: { color: '#e0e0e0', fontSize: 13 },
  slotBadge: { paddingHorizontal: 10, paddingVertical: 4, borderRadius: 10 },
  slotBadgeText: { color: '#000', fontSize: 11, fontWeight: 'bold' },
  modalContainer: { flex: 1, backgroundColor: '#12122a', padding: 16 },
  modalTitle: { color: '#e0e0e0', fontSize: 18, fontWeight: 'bold', marginBottom: 16 },
  historyItem: {
    backgroundColor: '#1a1a2e',
    borderRadius: 6,
    padding: 14,
    marginBottom: 6,
  },
  historyItemText: { color: '#e0e0e0', fontSize: 13 },
  closeButton: {
    backgroundColor: '#c62828',
    paddingVertical: 14,
    borderRadius: 8,
    marginTop: 12,
  },
});