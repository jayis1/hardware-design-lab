/*
 * DoctorReportScreen.js — Clinical PDF report generator
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Share, Platform,
} from 'react-native';
import BleManager from '../ble/BleManager';
import AsyncStorage from '@react-native-async-storage/async-storage';
import RNFS from 'react-native-fs';
import moment from 'moment';

export default function DoctorReportScreen() {
  const [reportData, setReportData] = useState(null);
  const [generating, setGenerating] = useState(false);
  const [reportPath, setReportPath] = useState(null);

  useEffect(() => {
    generateReport();
  }, []);

  const generateReport = async () => {
    setGenerating(true);
    try {
      // Load episodes from storage
      const episodes = JSON.parse(
        await AsyncStorage.getItem('downloaded_episodes') || '[]'
      );
      const doses = JSON.parse(
        await AsyncStorage.getItem('medication_doses') || '[]'
      );

      // Compute clinical metrics (MDS-UPDRS aligned)
      const totalTremorMinutes = episodes.reduce((sum, e) => sum + e.duration / 60000, 0);
      const avgFreq = episodes.length > 0
        ? episodes.reduce((sum, e) => sum + e.frequency, 0) / episodes.length : 0;
      const avgAmplitude = episodes.length > 0
        ? episodes.reduce((sum, e) => sum + e.amplitude, 0) / episodes.length : 0;

      // Class distribution
      const classCounts = [0, 0, 0, 0, 0];
      episodes.forEach(e => { if (e.class >= 0 && e.class < 5) classCounts[e.class]++; });

      // Rest vs action tremor
      const restTremorCount = episodes.filter(e => e.context === 0).length;
      const actionTremorCount = episodes.filter(e => e.context === 2).length;

      // Medication correlation
      const medCorrelations = doses.map(dose => {
        const before = episodes.filter(e =>
          e.timestamp >= dose.timestamp - 1800000 && e.timestamp < dose.timestamp
        );
        const after = episodes.filter(e =>
          e.timestamp >= dose.timestamp && e.timestamp < dose.timestamp + 1800000
        );
        return {
          dose: dose,
          beforeMinutes: before.reduce((s, e) => s + e.duration / 60000, 0),
          afterMinutes: after.reduce((s, e) => s + e.duration / 60000, 0),
        };
      });

      const report = {
        generatedAt: new Date().toISOString(),
        dateRange: {
          start: episodes.length > 0 ? Math.min(...episodes.map(e => e.timestamp)) : Date.now(),
          end: episodes.length > 0 ? Math.max(...episodes.map(e => e.timestamp)) : Date.now(),
        },
        totalEpisodes: episodes.length,
        totalTremorMinutes,
        avgFreq,
        avgAmplitude,
        classDistribution: classCounts,
        restTremorCount,
        actionTremorCount,
        medicationCorrelations: medCorrelations,
        episodes: episodes.slice(0, 20), // Top 20 recent episodes
      };

      setReportData(report);

      // Generate PDF-like text report
      const pdfText = buildReportText(report);
      const path = `${RNFS.DocumentDirectoryPath}/tremorsense_report_${Date.now()}.txt`;
      await RNFS.writeFile(path, pdfText, 'utf8');
      setReportPath(path);
    } catch (e) {
      console.error('Report generation failed:', e);
    } finally {
      setGenerating(false);
    }
  };

  const buildReportText = (report) => {
    const classNames = ['Parkinsonian', 'Essential', 'Cerebellar', 'Physiological', 'Drug-Induced'];

    let text = '═══════════════════════════════════════════════════\n';
    text += '       TREMORSENSE CLINICAL REPORT\n';
    text += '═══════════════════════════════════════════════════\n\n';
    text += `Generated: ${moment(report.generatedAt).format('YYYY-MM-DD HH:mm')}\n`;
    text += `Period: ${moment(report.dateRange.start).format('YYYY-MM-DD')} to ${moment(report.dateRange.end).format('YYYY-MM-DD')}\n\n`;
    text += '── SUMMARY ──────────────────────────────────────\n';
    text += `Total episodes recorded:     ${report.totalEpisodes}\n`;
    text += `Total tremor duration:       ${report.totalTremorMinutes.toFixed(1)} minutes\n`;
    text += `Average tremor frequency:    ${report.avgFreq.toFixed(2)} Hz\n`;
    text += `Average tremor amplitude:    ${report.avgAmplitude.toFixed(4)} g\n\n`;
    text += '── TREMOR TYPE DISTRIBUTION ─────────────────────\n';
    report.classDistribution.forEach((count, i) => {
      const pct = report.totalEpisodes > 0 ? (count / report.totalEpisodes * 100).toFixed(1) : '0.0';
      text += `  ${classNames[i].padEnd(15)} ${String(count).padStart(4)} (${pct}%)\n`;
    });
    text += '\n';
    text += '── CONTEXT ANALYSIS ─────────────────────────────\n';
    text += `  Rest tremor episodes:       ${report.restTremorCount}\n`;
    text += `  Action tremor episodes:     ${report.actionTremorCount}\n\n`;
    text += '── MEDICATION CORRELATION ───────────────────────\n';
    if (report.medicationCorrelations.length === 0) {
      text += '  No medication doses logged.\n';
    } else {
      report.medicationCorrelations.forEach(c => {
        const improvement = c.beforeMinutes > 0
          ? ((c.beforeMinutes - c.afterMinutes) / c.beforeMinutes * 100).toFixed(0)
          : 'N/A';
        text += `  ${moment(c.dose.timestamp).format('MM/DD HH:mm')} ${c.dose.name} ${c.dose.dose}\n`;
        text += `    Before: ${c.beforeMinutes.toFixed(1)} min | After: ${c.afterMinutes.toFixed(1)} min`;
        text += ` | Improvement: ${improvement}%\n`;
      });
    }
    text += '\n';
    text += '── CLINICAL INTERPRETATION ───────────────────────\n';
    text += `  Dominant tremor type: ${classNames[report.classDistribution.indexOf(Math.max(...report.classDistribution))]}\n`;
    text += `  Frequency range: ${report.avgFreq.toFixed(1)} Hz `;
    text += report.avgFreq < 6 ? '(consistent with Parkinsonian tremor)\n' : '(consistent with Essential Tremor)\n';
    text += `  Rest/action ratio: ${(report.restTremorCount / Math.max(1, report.actionTremorCount)).toFixed(2)}\n`;
    text += '\n';
    text += '── RECENT EPISODES (TOP 20) ────────────────────\n';
    text += '  Time                Duration  Freq(Hz)  Amp(g)   Class          Conf\n';
    report.episodes.forEach(e => {
      text += `  ${moment(e.timestamp).format('MM/DD HH:mm')}   `;
      text += `${(e.duration/60000).toFixed(1).padStart(7)}m  `;
      text += `${e.frequency.toFixed(2).padStart(8)}  `;
      text += `${e.amplitude.toFixed(4).padStart(7)}  `;
      text += `${classNames[e.class].padEnd(14)}  `;
      text += `${(e.confidence*100).toFixed(0)}%\n`;
    });
    text += '\n';
    text += '═══════════════════════════════════════════════════\n';
    text += 'TremorSense by jayis1 — Copyright (c) 2026 jayis1\n';
    text += 'MIT License — This report is for clinical reference only.\n';
    text += 'It does not constitute a diagnosis. Consult a neurologist.\n';
    text += '═══════════════════════════════════════════════════\n';

    return text;
  };

  const shareReport = async () => {
    if (!reportPath) return;
    try {
      await Share.share({
        message: 'TremorSense Clinical Report',
        url: Platform.OS === 'ios' ? reportPath : `file://${reportPath}`,
        title: 'TremorSense Report',
      });
    } catch (e) {
      console.error('Share failed:', e);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Doctor Report</Text>
      <Text style={styles.subtitle}>MDS-UPDRS-aligned tremor summary for clinical visits</Text>

      {generating && <Text style={styles.loadingText}>Generating report...</Text>}

      {reportData && (
        <ScrollView style={styles.content}>
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Summary</Text>
            <View style={styles.reportRow}>
              <Text style={styles.reportLabel}>Total Episodes:</Text>
              <Text style={styles.reportValue}>{reportData.totalEpisodes}</Text>
            </View>
            <View style={styles.reportRow}>
              <Text style={styles.reportLabel}>Total Tremor:</Text>
              <Text style={styles.reportValue}>
                {reportData.totalTremorMinutes.toFixed(1)} min
              </Text>
            </View>
            <View style={styles.reportRow}>
              <Text style={styles.reportLabel}>Avg Frequency:</Text>
              <Text style={styles.reportValue}>
                {reportData.avgFreq.toFixed(2)} Hz
              </Text>
            </View>
            <View style={styles.reportRow}>
              <Text style={styles.reportLabel}>Avg Amplitude:</Text>
              <Text style={styles.reportValue}>
                {reportData.avgAmplitude.toFixed(4)} g
              </Text>
            </View>
          </View>

          <View style={styles.card}>
            <Text style={styles.cardTitle}>Tremor Types</Text>
            {['Parkinsonian', 'Essential', 'Cerebellar', 'Physiological', 'Drug'].map((name, i) => (
              <View key={i} style={styles.reportRow}>
                <Text style={styles.reportLabel}>{name}</Text>
                <Text style={styles.reportValue}>{reportData.classDistribution[i]}</Text>
              </View>
            ))}
          </View>

          <View style={styles.card}>
            <Text style={styles.cardTitle}>Medication Response</Text>
            {reportData.medicationCorrelations.length === 0 ? (
              <Text style={styles.emptyText}>No medication data</Text>
            ) : (
              reportData.medicationCorrelations.map((c, i) => {
                const improvement = c.beforeMinutes > 0
                  ? ((c.beforeMinutes - c.afterMinutes) / c.beforeMinutes * 100).toFixed(0)
                  : 'N/A';
                return (
                  <View key={i} style={styles.medRow}>
                    <Text style={styles.medTime}>
                      {moment(c.dose.timestamp).format('MM/DD HH:mm')} {c.dose.name}
                    </Text>
                    <Text style={styles.medDetail}>
                      Before: {c.beforeMinutes.toFixed(1)}m → After: {c.afterMinutes.toFixed(1)}m ({improvement}%)
                    </Text>
                  </View>
                );
              })
            )}
          </View>

          <TouchableOpacity style={styles.shareButton} onPress={shareReport}>
            <Text style={styles.shareButtonText}>📤 Share Report with Doctor</Text>
          </TouchableOpacity>

          <Text style={styles.disclaimer}>
            This report is for clinical reference only and does not constitute a diagnosis.
            Please consult a neurologist for proper interpretation.
          </Text>
        </ScrollView>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1C1C1E', padding: 20 },
  title: { color: '#FFFFFF', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#8E8E93', fontSize: 14, marginBottom: 16 },
  loadingText: { color: '#0A84FF', fontSize: 16, textAlign: 'center', padding: 20 },
  content: { flex: 1 },
  card: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: { color: '#8E8E93', fontSize: 14, marginBottom: 10, fontWeight: '600' },
  reportRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6 },
  reportLabel: { color: '#FFFFFF', fontSize: 15 },
  reportValue: { color: '#0A84FF', fontSize: 15, fontWeight: '600' },
  medRow: { paddingVertical: 6, borderBottomWidth: 0.5, borderBottomColor: '#3A3A3C' },
  medTime: { color: '#FFFFFF', fontSize: 14, fontWeight: '500' },
  medDetail: { color: '#8E8E93', fontSize: 12, marginTop: 2 },
  emptyText: { color: '#8E8E93', fontSize: 14 },
  shareButton: {
    backgroundColor: '#34C759',
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    marginBottom: 16,
  },
  shareButtonText: { color: '#FFFFFF', fontSize: 18, fontWeight: '600' },
  disclaimer: {
    color: '#8E8E93',
    fontSize: 11,
    textAlign: 'center',
    lineHeight: 16,
  },
});