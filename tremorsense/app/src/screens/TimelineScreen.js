/*
 * TimelineScreen.js — 24-hour tremor timeline heatmap
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useContext, useCallback } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Dimensions, FlatList,
} from 'react-native';
import { TremorContext } from '../models/TremorContext';
import { COLORS } from '../utils/theme';
import EpisodeModel from '../models/EpisodeModel';

const { width } = Dimensions.get('window');
const HOUR_WIDTH = (width - 40) / 24;

export default function TimelineScreen() {
  const { deviceData } = useContext(TremorContext);
  const [episodes, setEpisodes] = useState([]);
  const [selectedDate, setSelectedDate] = useState(new Date());
  const [selectedHour, setSelectedHour] = useState(null);

  useEffect(() => {
    // Load episodes from storage
    loadEpisodes();
  }, [selectedDate]);

  const loadEpisodes = async () => {
    // In production: fetch from AsyncStorage or BLE download
    const mockEpisodes = [
      { timestamp: Date.now() - 7200000, duration: 1200, frequency: 4.5, class: 0, amplitude: 0.05, confidence: 0.92 },
      { timestamp: Date.now() - 3600000, duration: 800, frequency: 5.2, class: 1, amplitude: 0.03, confidence: 0.85 },
      { timestamp: Date.now() - 1800000, duration: 600, frequency: 4.8, class: 0, amplitude: 0.04, confidence: 0.88 },
    ];
    setEpisodes(mockEpisodes);
  };

  const getHourForEpisode = (timestamp) => {
    return new Date(timestamp).getHours();
  };

  const renderHourCell = (hour) => {
    const hourEpisodes = episodes.filter(e =>
      getHourForEpisode(e.timestamp) === hour
    );

    if (hourEpisodes.length === 0) {
      return (
        <View key={hour} style={styles.hourCell}>
          <Text style={styles.hourLabel}>{hour}</Text>
        </View>
      );
    }

    // Determine dominant class for this hour
    const classCounts = [0, 0, 0, 0, 0];
    hourEpisodes.forEach(e => { if (e.class < 5) classCounts[e.class]++; });
    const dominantClass = classCounts.indexOf(Math.max(...classCounts));
    const colors = [COLORS.parkinsonian, COLORS.essential, COLORS.cerebellar,
                    COLORS.physiological, COLORS.drug];

    return (
      <TouchableOpacity
        key={hour}
        style={[styles.hourCell, { backgroundColor: colors[dominantClass] + '40' }]}
        onPress={() => setSelectedHour(hour)}
      >
        <Text style={styles.hourLabel}>{hour}</Text>
        <View style={[styles.hourDot, { backgroundColor: colors[dominantClass] }]} />
      </TouchableOpacity>
    );
  };

  const renderEpisodeItem = ({ item }) => {
    const time = new Date(item.timestamp).toLocaleTimeString();
    const durationMin = (item.duration / 60).toFixed(1);
    const className = ['Parkinsonian', 'Essential', 'Cerebellar', 'Physiological', 'Drug'][item.class] || 'Unknown';
    const classColor = [COLORS.parkinsonian, COLORS.essential, COLORS.cerebellar,
                        COLORS.physiological, COLORS.drug][item.class] || COLORS.textSecondary;

    return (
      <TouchableOpacity
        style={styles.episodeItem}
        onPress={() => {
          // Navigate to EpisodeDetail
        }}
      >
        <View style={styles.episodeHeader}>
          <Text style={styles.episodeTime}>{time}</Text>
          <Text style={styles.episodeDuration}>{durationMin} min</Text>
        </View>
        <View style={styles.episodeDetails}>
          <Text style={[styles.episodeClass, { color: classColor }]}>{className}</Text>
          <Text style={styles.episodeFreq}>{item.frequency.toFixed(2)} Hz</Text>
          <Text style={styles.episodeAmp}>{item.amplitude.toFixed(4)} g</Text>
        </View>
        <Text style={styles.episodeConfidence}>
          Confidence: {(item.confidence * 100).toFixed(0)}%
        </Text>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Tremor Timeline</Text>

      {/* Date selector */}
      <View style={styles.dateSelector}>
        <Text style={styles.dateText}>
          {selectedDate.toLocaleDateString()}
        </Text>
      </View>

      {/* 24-hour heatmap */}
      <View style={styles.heatmapContainer}>
        <Text style={styles.sectionLabel}>24-Hour Activity</Text>
        <ScrollView horizontal showsHorizontalScrollIndicator={false}>
          <View style={styles.heatmapRow}>
            {Array.from({ length: 24 }, (_, h) => renderHourCell(h))}
          </View>
        </ScrollView>
      </View>

      {/* Legend */}
      <View style={styles.legend}>
        {['Parkinsonian', 'Essential', 'Cerebellar', 'Physiological', 'Drug'].map((name, i) => {
          const colors = [COLORS.parkinsonian, COLORS.essential, COLORS.cerebellar,
                          COLORS.physiological, COLORS.drug];
          return (
            <View key={i} style={styles.legendItem}>
              <View style={[styles.legendDot, { backgroundColor: colors[i] }]} />
              <Text style={styles.legendText}>{name}</Text>
            </View>
          );
        })}
      </View>

      {/* Episode list */}
      <Text style={styles.sectionLabel}>Episodes ({episodes.length})</Text>
      <FlatList
        data={episodes}
        keyExtractor={(item, idx) => idx.toString()}
        renderItem={renderEpisodeItem}
        style={styles.episodeList}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1C1C1E', padding: 20 },
  title: { color: '#FFFFFF', fontSize: 24, fontWeight: 'bold', marginBottom: 10 },
  dateSelector: { marginBottom: 16 },
  dateText: { color: '#0A84FF', fontSize: 16, fontWeight: '600' },
  heatmapContainer: { marginBottom: 16 },
  sectionLabel: { color: '#8E8E93', fontSize: 14, marginBottom: 8 },
  heatmapRow: { flexDirection: 'row' },
  hourCell: {
    width: HOUR_WIDTH,
    height: 50,
    borderRadius: 8,
    marginHorizontal: 2,
    backgroundColor: '#2C2C2E',
    justifyContent: 'center',
    alignItems: 'center',
  },
  hourLabel: { color: '#8E8E93', fontSize: 10 },
  hourDot: { width: 6, height: 6, borderRadius: 3, marginTop: 4 },
  legend: { flexDirection: 'row', flexWrap: 'wrap', marginBottom: 16 },
  legendItem: { flexDirection: 'row', alignItems: 'center', marginRight: 12, marginBottom: 4 },
  legendDot: { width: 8, height: 8, borderRadius: 4, marginRight: 4 },
  legendText: { color: '#8E8E93', fontSize: 11 },
  episodeList: { flex: 1 },
  episodeItem: {
    backgroundColor: '#2C2C2E',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
  },
  episodeHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  episodeTime: { color: '#FFFFFF', fontSize: 14, fontWeight: '600' },
  episodeDuration: { color: '#8E8E93', fontSize: 13 },
  episodeDetails: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 4 },
  episodeClass: { fontSize: 13, fontWeight: '600' },
  episodeFreq: { color: '#FFFFFF', fontSize: 13 },
  episodeAmp: { color: '#8E8E93', fontSize: 13 },
  episodeConfidence: { color: '#8E8E93', fontSize: 11 },
});