/**
 * SpikeViewScreen.js — Spike analysis view for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Displays spike raster plot, sorted waveform templates, ISI histogram,
 * and inter-channel propagation map.
 */

import React, { useState, useMemo } from 'react';
import { View, Text, StyleSheet, ScrollView, FlatList, Dimensions } from 'react-native';
import { useBle } from '../utils/BleContext';
import SpikeRaster from '../components/SpikeRaster';

const SCREEN_WIDTH = Dimensions.get('window').width;

export default function SpikeViewScreen() {
  const { spikes, epoch, classLabels, classColors } = useBle();
  const [selectedChannel, setSelectedChannel] = useState(0);

  // Compute spike statistics.
  const stats = useMemo(() => {
    if (spikes.length === 0) return { total: 0, byChannel: [], meanAmp: 0, maxAmp: 0 };

    const byChannel = new Array(16).fill(0);
    let ampSum = 0, maxAmp = 0;
    for (const sp of spikes) {
      if (sp.channel < 16) byChannel[sp.channel]++;
      const absAmp = Math.abs(sp.amplitudeUv);
      ampSum += absAmp;
      if (absAmp > maxAmp) maxAmp = absAmp;
    }
    return {
      total: spikes.length,
      byChannel,
      meanAmp: ampSum / spikes.length,
      maxAmp,
    };
  }, [spikes]);

  // Compute ISI histogram for selected channel.
  const isiHistogram = useMemo(() => {
    const channelSpikes = spikes
      .filter((s) => s.channel === selectedChannel)
      .sort((a, b) => a.timestampMs - b.timestampMs);

    if (channelSpikes.length < 2) return [];

    const intervals = [];
    for (let i = 1; i < channelSpikes.length; i++) {
      intervals.push(channelSpikes[i].timestampMs - channelSpikes[i - 1].timestampMs);
    }

    // Bin into 10ms intervals up to 500ms.
    const bins = new Array(50).fill(0);
    for (const isi of intervals) {
      const binIdx = Math.min(Math.floor(isi / 10), 49);
      bins[binIdx]++;
    }
    return bins;
  }, [spikes, selectedChannel]);

  // Group spikes by template for waveform display.
  const templates = useMemo(() => {
    const byTemplate = {};
    for (const sp of spikes) {
      if (!byTemplate[sp.templateId]) byTemplate[sp.templateId] = [];
      byTemplate[sp.templateId].push(sp);
    }
    return Object.entries(byTemplate).map(([id, sps]) => ({
      templateId: parseInt(id),
      count: sps.length,
      meanAmp: sps.reduce((s, sp) => s + Math.abs(sp.amplitudeUv), 0) / sps.length,
    }));
  }, [spikes]);

  const epochLabel = epoch ? classLabels[epoch.classLabel] : '—';
  const epochColor = epoch ? classColors[epoch.classLabel] : '#666';

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Spike Analysis</Text>
        <View style={[styles.classBadge, { backgroundColor: epochColor }]}>
          <Text style={styles.classBadgeText}>{epochLabel}</Text>
        </View>
      </View>

      <View style={styles.statsRow}>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{stats.total}</Text>
          <Text style={styles.statLabel}>Total Spikes</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{Math.round(stats.meanAmp)}</Text>
          <Text style={styles.statLabel}>Mean µV</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{Math.round(stats.maxAmp)}</Text>
          <Text style={styles.statLabel}>Max µV</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statValue}>{epoch?.spikeRate || 0}</Text>
          <Text style={styles.statLabel}>Rate/s</Text>
        </View>
      </View>

      <ScrollView style={styles.scrollContent}>
        {/* Spike Raster Plot */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Spike Raster (all channels)</Text>
          <SpikeRaster spikes={spikes.slice(-500)} />
        </View>

        {/* Per-channel spike counts */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Spikes per Channel</Text>
          <View style={styles.channelBars}>
            {stats.byChannel.map((count, ch) => (
              <View key={ch} style={styles.channelBar}>
                <Text style={styles.channelLabel}>CH{ch}</Text>
                <View style={styles.barContainer}>
                  <View
                    style={[
                      styles.bar,
                      { width: `${Math.min((count / Math.max(...stats.byChannel, 1)) * 100, 100)}%` },
                    ]}
                  />
                </View>
                <Text style={styles.channelCount}>{count}</Text>
              </View>
            ))}
          </View>
        </View>

        {/* ISI Histogram for selected channel */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>
            ISI Histogram — Channel {selectedChannel}
          </Text>
          <View style={styles.channelSelector}>
            {Array.from({ length: 16 }, (_, i) => (
              <TouchableOpacity
                key={i}
                style={[styles.chSelBtn, selectedChannel === i && styles.chSelBtnActive]}
                onPress={() => setSelectedChannel(i)}
              >
                <Text style={styles.chSelBtnText}>{i}</Text>
              </TouchableOpacity>
            ))}
          </View>
          <View style={styles.histogram}>
            {isiHistogram.map((count, bin) => (
              <View
                key={bin}
                style={[
                  styles.histBar,
                  { height: Math.max((count / Math.max(...isiHistogram, 1)) * 80, 2) },
                ]}
              />
            ))}
          </View>
          <Text style={styles.histLabel}>0ms ──────────── 500ms</Text>
        </View>

        {/* Spike Templates */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Spike Templates</Text>
          <FlatList
            data={templates}
            horizontal
            keyExtractor={(item) => item.templateId.toString()}
            renderItem={({ item }) => (
              <View style={styles.templateCard}>
                <Text style={styles.templateId}>Template {item.templateId}</Text>
                <Text style={styles.templateStat}>Count: {item.count}</Text>
                <Text style={styles.templateStat}>Mean: {Math.round(item.meanAmp)}µV</Text>
              </View>
            )}
          />
        </View>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1a0a' },
  header: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', padding: 16, borderBottomWidth: 1, borderBottomColor: '#2a3a2a',
  },
  title: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  classBadge: { paddingHorizontal: 12, paddingVertical: 6, borderRadius: 12 },
  classBadgeText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  statsRow: { flexDirection: 'row', padding: 8, gap: 8 },
  statCard: {
    flex: 1, backgroundColor: '#1a2a1a', borderRadius: 8, padding: 12, alignItems: 'center',
  },
  statValue: { color: '#4CAF50', fontSize: 22, fontWeight: 'bold' },
  statLabel: { color: '#888', fontSize: 11 },
  scrollContent: { flex: 1, padding: 8 },
  section: { backgroundColor: '#1a2a1a', borderRadius: 12, padding: 12, marginBottom: 12 },
  sectionTitle: { color: '#fff', fontSize: 15, fontWeight: 'bold', marginBottom: 8 },
  channelBars: { gap: 4 },
  channelBar: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  channelLabel: { color: '#888', fontSize: 10, width: 35 },
  barContainer: { flex: 1, height: 12, backgroundColor: '#0a1a0a', borderRadius: 6 },
  bar: { height: '100%', backgroundColor: '#4CAF50', borderRadius: 6 },
  channelCount: { color: '#aaa', fontSize: 10, width: 30 },
  channelSelector: { flexDirection: 'row', flexWrap: 'wrap', gap: 4, marginBottom: 8 },
  chSelBtn: {
    width: 28, height: 28, borderRadius: 6, backgroundColor: '#2a3a2a',
    alignItems: 'center', justifyContent: 'center',
  },
  chSelBtnActive: { backgroundColor: '#4CAF50' },
  chSelBtnText: { color: '#fff', fontSize: 11 },
  histogram: { flexDirection: 'row', alignItems: 'flex-end', height: 80, gap: 2 },
  histBar: { flex: 1, backgroundColor: '#4CAF50', minHeight: 2, borderRadius: 2 },
  histLabel: { color: '#666', fontSize: 10, marginTop: 4 },
  templateCard: {
    backgroundColor: '#0a1a0a', borderRadius: 8, padding: 12,
    marginRight: 8, width: 120,
  },
  templateId: { color: '#4CAF50', fontSize: 13, fontWeight: 'bold' },
  templateStat: { color: '#aaa', fontSize: 11, marginTop: 4 },
});