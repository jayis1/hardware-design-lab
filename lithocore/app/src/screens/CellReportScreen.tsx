/**
 * CellReportScreen.tsx — Detailed cell test report screen.
 *
 * Shows the full SoH gauge, degradation mode, all Randles parameters,
 * DCIR, OCV, self-discharge rate, and the Nyquist/Bode plots.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Share,
} from 'react-native';
import { useNavigation, useRoute } from '@react-navigation/native';
import SoHGauge from '../components/SoHGauge';
import NyquistPlot from '../components/NyquistPlot';
import BodePlot from '../components/BodePlot';
import { useDatabase } from '../db/database';
import {
  DegradationMode,
  QualityVerdict,
  DEGRADATION_NAMES,
  VERDICT_NAMES,
  VERDICT_COLORS,
  CHEMISTRY_NAMES,
} from '../ble/protocol';

export default function CellReportScreen() {
  const route = useRoute();
  const navigation = useNavigation();
  const db = useDatabase();
  const [record, setRecord] = useState<any>(null);

  useEffect(() => {
    const params = route.params as { cellId?: string } | undefined;
    if (params?.cellId) {
      const id = parseInt(params.cellId, 10);
      db.getResult(id).then((r) => setRecord(r));
    }
  }, [route.params]);

  const handleShare = async () => {
    if (!record) return;
    const csv = `LithoCore Cell Report\nAuthor: jayis1\nDate: ${new Date(record.timestamp).toISOString()}\nSoH: ${record.soh_score}\nMode: ${DEGRADATION_NAMES[record.degradation as DegradationMode]}\nChemistry: ${CHEMISTRY_NAMES[record.chemistry_idx]}\nOCV: ${record.ocv_mv} mV\nDCIR: ${record.dcir_mohm} mΩ\nRs: ${record.rs_mohm} mΩ\nRsei: ${record.rsei_mohm} mΩ\nRct: ${record.rct_mohm} mΩ\nCdl: ${record.cdl_mf} mF\n`;
    await Share.share({ message: csv });
  };

  if (!record) {
    return (
      <View style={styles.container}>
        <Text style={styles.emptyText}>Loading report…</Text>
      </View>
    );
  }

  const verdict = record.verdict as QualityVerdict;
  const degradation = record.degradation as DegradationMode;
  const sweepPoints = JSON.parse(record.sweepDataJson || '[]');

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* SoH Gauge */}
      <SoHGauge score={record.soh_score} size={200} />

      {/* Verdict badge */}
      <View style={[styles.badge, { backgroundColor: VERDICT_COLORS[verdict] }]}>
        <Text style={styles.badgeText}>{VERDICT_NAMES[verdict]}</Text>
      </View>

      {/* Degradation mode */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Degradation Mode</Text>
        <Text style={styles.modeText}>{DEGRADATION_NAMES[degradation]}</Text>
      </View>

      {/* Cell measurements */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Cell Measurements</Text>
        <View style={styles.paramRow}>
          <Text style={styles.paramLabel}>Open-Circuit Voltage</Text>
          <Text style={styles.paramValue}>{(record.ocv_mv / 1000).toFixed(3)} V</Text>
        </View>
        <View style={styles.paramRow}>
          <Text style={styles.paramLabel}>Temperature</Text>
          <Text style={styles.paramValue}>{(record.temp_dc / 10).toFixed(1)} °C</Text>
        </View>
        <View style={styles.paramRow}>
          <Text style={styles.paramLabel}>DC Internal Resistance</Text>
          <Text style={styles.paramValue}>{record.dcir_mohm} mΩ</Text>
        </View>
        <View style={styles.paramRow}>
          <Text style={styles.paramLabel}>Self-Discharge Rate</Text>
          <Text style={styles.paramValue}>{record.self_discharge_uv_per_min} µV/min</Text>
        </View>
        <View style={styles.paramRow}>
          <Text style={styles.paramLabel}>Chemistry</Text>
          <Text style={styles.paramValue}>{CHEMISTRY_NAMES[record.chemistry_idx]}</Text>
        </View>
      </View>

      {/* Randles equivalent circuit parameters */}
      {record.fit_valid === 1 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Randles Equivalent Circuit</Text>
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>Rs (electrolyte)</Text>
            <Text style={styles.paramValue}>{record.rs_mohm} mΩ</Text>
          </View>
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>Rsei (SEI layer)</Text>
            <Text style={styles.paramValue}>{record.rsei_mohm} mΩ</Text>
          </View>
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>Csei (SEI capacitance)</Text>
            <Text style={styles.paramValue}>{record.csei_mf} mF</Text>
          </View>
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>Rct (charge transfer)</Text>
            <Text style={styles.paramValue}>{record.rct_mohm} mΩ</Text>
          </View>
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>Cdl (double-layer)</Text>
            <Text style={styles.paramValue}>{record.cdl_mf} mF</Text>
          </View>
          <View style={styles.paramRow}>
            <Text style={styles.paramLabel}>σ (Warburg)</Text>
            <Text style={styles.paramValue}>{record.sigma}</Text>
          </View>
        </View>
      )}

      {/* Nyquist plot */}
      <NyquistPlot points={sweepPoints} />

      {/* Bode plot */}
      <BodePlot points={sweepPoints} />

      {/* Export button */}
      <TouchableOpacity style={styles.shareButton} onPress={handleShare}>
        <Text style={styles.buttonText}>Export Report</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#12122a' },
  content: { padding: 16, alignItems: 'center' },
  emptyText: { color: '#666688', fontSize: 16, textAlign: 'center', marginTop: 40 },
  badge: {
    paddingHorizontal: 24,
    paddingVertical: 8,
    borderRadius: 20,
    marginVertical: 8,
  },
  badgeText: { color: '#000', fontSize: 16, fontWeight: 'bold' },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 16,
    width: '100%',
    marginVertical: 8,
  },
  sectionTitle: {
    color: '#00b4ff',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  modeText: {
    color: '#e0e0e0',
    fontSize: 18,
    fontWeight: 'bold',
  },
  paramRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#222244',
  },
  paramLabel: { color: '#8888aa', fontSize: 13 },
  paramValue: { color: '#e0e0e0', fontSize: 13, fontWeight: '600' },
  shareButton: {
    backgroundColor: '#0066cc',
    paddingVertical: 14,
    borderRadius: 8,
    width: '100%',
    marginTop: 12,
    marginBottom: 32,
  },
  buttonText: { color: '#fff', fontSize: 15, fontWeight: '600', textAlign: 'center' },
});