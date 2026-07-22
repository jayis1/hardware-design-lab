/**
 * ReportScreen — ASTM-compliant inspection report export
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity, Share, Alert,
} from 'react-native';
import type { StackScreenProps } from '@react-navigation/stack';
import type { RootStackParamList } from '../../App';
import { getPoints, getSurvey, PointRecord, SurveyRecord } from '../db/database';
import {
  hcpClassLabel, resistivityClassLabel,
} from '../ble/protocol';

type Props = StackScreenProps<RootStackParamList, 'Report'>;

export default function ReportScreen({ route }: Props) {
  const { surveyId } = route.params;
  const [points, setPoints] = useState<PointRecord[]>([]);
  const [survey, setSurvey] = useState<SurveyRecord | null>(null);

  useEffect(() => {
    (async () => {
      const pts = await getPoints(surveyId);
      setPoints(pts);
      const surv = await getSurvey(surveyId);
      setSurvey(surv);
    })();
  }, [surveyId]);

  /* Summary statistics */
  const n = points.length;
  const activeCount = points.filter((p) => p.hcp_class === 2).length;
  const uncertainCount = points.filter((p) => p.hcp_class === 1).length;
  const noCorrCount = points.filter((p) => p.hcp_class === 0).length;
  const avgCover = n ? points.reduce((a, p) => a + p.cover_mm, 0) / n : 0;
  const minCover = n ? Math.min(...points.map((p) => p.cover_mm)) : 0;
  const avgRho = n ? points.reduce((a, p) => a + p.rho_ohm_m, 0) / n : 0;

  const buildReportText = (): string => {
    const date = survey ? new Date(survey.created_at).toISOString() : '';
    const lines: string[] = [];
    lines.push('================================================');
    lines.push('  RebarScope — Concrete Corrosion Inspection Report');
    lines.push('  Author: jayis1 — © 2026 jayis1');
    lines.push('  License: MIT (app) / CERN-OHL-S (HW) / GPL-3 (FW)');
    lines.push('================================================');
    lines.push('');
    lines.push(`Site:          ${survey?.site_name ?? 'Unknown'}`);
    lines.push(`Date:          ${date}`);
    lines.push(`Survey ID:     ${surveyId}`);
    lines.push(`Total points:  ${n}`);
    lines.push('');
    lines.push('--- ASTM C876 Half-Cell Potential Summary ---');
    lines.push(`Active corrosion (< -350 mV):  ${activeCount} (${n ? ((activeCount / n) * 100).toFixed(1) : 0}%)`);
    lines.push(`Uncertain (-200 to -350 mV):    ${uncertainCount} (${n ? ((uncertainCount / n) * 100).toFixed(1) : 0}%)`);
    lines.push(`No corrosion (> -200 mV):        ${noCorrCount} (${n ? ((noCorrCount / n) * 100).toFixed(1) : 0}%)`);
    lines.push('');
    lines.push('--- Resistivity Summary (Wenner 4-probe) ---');
    lines.push(`Average resistivity: ${(avgRho * 100).toFixed(1)} kΩ·cm`);
    lines.push('');
    lines.push('--- Cover Depth Summary (eddy-current) ---');
    lines.push(`Average cover: ${avgCover.toFixed(1)} mm`);
    lines.push(`Minimum cover:  ${minCover.toFixed(1)} mm`);
    lines.push('');
    lines.push('--- Tamper-Evidence ---');
    lines.push(`Hash chain: ${survey?.hash_chain ?? '(none)'}`);
    lines.push('');
    lines.push('--- Hotspot Locations (active corrosion) ---');
    points.filter((p) => p.hcp_class === 2).forEach((p, i) => {
      lines.push(`  ${i + 1}. (${(p.x_mm / 1000).toFixed(2)}, ${(p.y_mm / 1000).toFixed(2)}) m  ` +
        `HCP=${p.hcp_mv.toFixed(0)} mV  ρ=${(p.rho_ohm_m * 100).toFixed(0)} kΩ·cm  d=${p.cover_mm.toFixed(0)} mm`);
    });
    lines.push('');
    lines.push('Disclaimer: RebarScope is a screening tool; flagged hotspots');
    lines.push('must be confirmed by coring / chloride profiling.');
    lines.push('================================================');
    return lines.join('\n');
  };

  const handleShare = async () => {
    const text = buildReportText();
    try {
      await Share.share({ message: text, title: `RebarScope Report — ${survey?.site_name ?? ''}` });
    } catch (e: any) {
      Alert.alert('Share failed', e?.message ?? 'Unknown error');
    }
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Inspection Report</Text>
      <Text style={styles.site}>{survey?.site_name ?? ''}</Text>
      <Text style={styles.date}>
        {survey ? new Date(survey.created_at).toLocaleString() : ''}
      </Text>

      <View style={styles.summaryCard}>
        <Text style={styles.cardHeader}>Summary</Text>
        <Text style={styles.line}>Total points: {n}</Text>
        <Text style={[styles.line, { color: '#e74c3c' }]}>
          Active corrosion: {activeCount} ({n ? ((activeCount / n) * 100).toFixed(1) : 0}%)
        </Text>
        <Text style={[styles.line, { color: '#f39c12' }]}>
          Uncertain: {uncertainCount} ({n ? ((uncertainCount / n) * 100).toFixed(1) : 0}%)
        </Text>
        <Text style={[styles.line, { color: '#2ecc71' }]}>
          No corrosion: {noCorrCount} ({n ? ((noCorrCount / n) * 100).toFixed(1) : 0}%)
        </Text>
        <Text style={styles.line}>Avg cover: {avgCover.toFixed(1)} mm  (min: {minCover.toFixed(1)} mm)</Text>
        <Text style={styles.line}>Avg resistivity: {(avgRho * 100).toFixed(1)} kΩ·cm</Text>
      </View>

      <View style={styles.hashCard}>
        <Text style={styles.cardHeader}>Tamper-Evidence Hash Chain</Text>
        <Text style={styles.hashText}>{survey?.hash_chain ?? '(none)'}</Text>
      </View>

      <View style={styles.hotspotCard}>
        <Text style={styles.cardHeader}>Hotspots ({activeCount})</Text>
        {points.filter((p) => p.hcp_class === 2).map((p, i) => (
          <Text key={i} style={styles.hotspotLine}>
            ({(p.x_mm / 1000).toFixed(2)}, {(p.y_mm / 1000).toFixed(2)}) m  •  HCP {p.hcp_mv.toFixed(0)} mV  •  ρ {(p.rho_ohm_m * 100).toFixed(0)} kΩ·cm  •  d {p.cover_mm.toFixed(0)} mm
          </Text>
        ))}
      </View>

      <TouchableOpacity style={styles.shareBtn} onPress={handleShare}>
        <Text style={styles.shareText}>Share Report</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 16 },
  header: { fontSize: 24, fontWeight: 'bold', color: '#ecf0f1' },
  site: { fontSize: 18, color: '#bdc3c7' },
  date: { fontSize: 12, color: '#95a5a6', marginBottom: 16 },
  summaryCard: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 14, marginBottom: 12 },
  hashCard: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 14, marginBottom: 12 },
  hotspotCard: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 14, marginBottom: 12 },
  cardHeader: { fontSize: 16, fontWeight: 'bold', color: '#ecf0f1', marginBottom: 8 },
  line: { color: '#bdc3c7', fontSize: 13, paddingVertical: 2 },
  hashText: { color: '#7f8c8d', fontSize: 10, fontFamily: 'monospace' },
  hotspotLine: { color: '#e74c3c', fontSize: 11, paddingVertical: 2 },
  shareBtn: { backgroundColor: '#27ae60', padding: 14, borderRadius: 8, alignItems: 'center', marginBottom: 32 },
  shareText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
});