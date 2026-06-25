/**
 * SoundscapeScreen.js — 2D cross-section soundscape visualiser
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Renders a colour-mapped cross-section of the soil column showing
 * acoustic activity by depth and class, with animated event markers
 * from the beamformer localisation.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Dimensions, TouchableOpacity } from 'react-native';
import { Svg, Rect, Circle, Text as SvgText, Line, G } from 'react-native-svg';
import { useSelector } from 'react-redux';

const { width } = Dimensions.get('window');
const CHART_W = width - 40;
const CHART_H = 400;
const DEPTH_MAX = 70; // cm
const WIDTH_MAX = 100; // cm relative

// Class colours
const CLASS_COLORS = {
  root_growth: '#4CAF50',
  root_hydraulic: '#66BB6A',
  earthworm_burrow: '#8D6E63',
  earthworm_cast: '#A1887F',
  arthropod_click: '#FFB74D',
  grub_chew: '#F44336',
  microbe_ebullition: '#26A69A',
  water_infiltration: '#42A5F5',
  compaction_crack: '#FF7043',
  noise: '#BDBDBD',
};

const CLASS_NAMES = [
  'root_growth', 'root_hydraulic', 'earthworm_burrow', 'earthworm_cast',
  'arthropod_click', 'grub_chew', 'microbe_ebullition', 'water_infiltration',
  'compaction_crack', 'noise'
];

// Generate demo events
function generateDemoEvents() {
  const events = [];
  for (let i = 0; i < 15; i++) {
    const classId = Math.floor(Math.random() * 9);
    events.push({
      id: i,
      classId,
      className: CLASS_NAMES[classId],
      x: Math.random() * WIDTH_MAX - WIDTH_MAX / 2,
      z: -(10 + Math.random() * 50),
      intensity: 0.3 + Math.random() * 0.7,
      timestamp: Date.now() - Math.random() * 60000,
    });
  }
  return events;
}

export default function SoundscapeScreen({ navigation }) {
  const pods = useSelector(state => state.pods.pods);
  const [selectedPod, setSelectedPod] = useState(pods[0]?.id || 'pod-001');
  const [events, setEvents] = useState([]);
  const [timeWindow, setTimeWindow] = useState('15m'); // 15m, 1h, 6h, 24h

  useEffect(() => {
    // In production: fetch events from pod via BLE or LoRa gateway
    setEvents(generateDemoEvents());
    const interval = setInterval(() => {
      setEvents(prev => [...prev, ...generateDemoEvents()].slice(-50));
    }, 5000);
    return () => clearInterval(interval);
  }, [selectedPod, timeWindow]);

  const pod = pods.find(p => p.id === selectedPod) || pods[0];

  // Map soil coordinates to screen pixels
  const toX = (cm) => (cm + WIDTH_MAX / 2) / WIDTH_MAX * CHART_W;
  const toY = (cm) => ((cm + DEPTH_MAX) / DEPTH_MAX) * CHART_H;

  return (
    <View style={styles.container}>
      {/* Pod selector */}
      <View style={styles.podSelector}>
        {pods.map(p => (
          <TouchableOpacity
            key={p.id}
            onPress={() => setSelectedPod(p.id)}
            style={[styles.podTab, selectedPod === p.id && styles.podTabActive]}
          >
            <Text style={[styles.podTabText, selectedPod === p.id && styles.podTabTextActive]}>
              {p.name || p.id}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Time window selector */}
      <View style={styles.timeSelector}>
        {['15m', '1h', '6h', '24h'].map(tw => (
          <TouchableOpacity
            key={tw}
            onPress={() => setTimeWindow(tw)}
            style={[styles.timeTab, timeWindow === tw && styles.timeTabActive]}
          >
            <Text style={[styles.timeTabText, timeWindow === tw && styles.timeTabTextActive]}>
              {tw}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Soundscape cross-section */}
      <View style={styles.chartContainer}>
        <Svg width={CHART_W} height={CHART_H}>
          {/* Background: soil layers */}
          <Rect x={0} y={0} width={CHART_W} height={CHART_H * 0.15} fill="#8D6E63" opacity={0.3} />
          <Rect x={0} y={CHART_H * 0.15} width={CHART_W} height={CHART_H * 0.25} fill="#6D4C41" opacity={0.3} />
          <Rect x={0} y={CHART_H * 0.4} width={CHART_W} height={CHART_H * 0.35} fill="#5D4037" opacity={0.3} />
          <Rect x={0} y={CHART_H * 0.75} width={CHART_W} height={CHART_H * 0.25} fill="#3E2723" opacity={0.3} />

          {/* Depth markers */}
          {[0, 10, 20, 40, 60].map(d => {
            const y = toY(-d);
            return (
              <G key={`depth-${d}`}>
                <Line x1={0} y1={y} x2={CHART_W} y2={y} stroke="#FFFFFF" strokeWidth={0.5} opacity={0.3} />
                <SvgText x={5} y={y - 3} fill="#FFFFFF" fontSize={10} opacity={0.7}>{d} cm</SvgText>
              </G>
            );
          })}

          {/* Surface line */}
          <Line x1={0} y1={toY(0)} x2={CHART_W} y2={toY(0)} stroke="#4CAF50" strokeWidth={2} />

          {/* Event markers */}
          {events.map(ev => {
            const cx = toX(ev.x);
            const cy = toY(ev.z);
            const r = 4 + ev.intensity * 10;
            const color = CLASS_COLORS[ev.className] || '#999';
            return (
              <G key={ev.id}>
                <Circle cx={cx} cy={cy} r={r} fill={color} opacity={0.4 + ev.intensity * 0.4} />
                <Circle cx={cx} cy={cy} r={r * 0.4} fill={color} opacity={0.9} />
              </G>
            );
          })}

          {/* Pod centre marker */}
          <Circle cx={CHART_W / 2} cy={toY(-5)} r={6} fill="#2E7D32" stroke="#FFFFFF" strokeWidth={2} />
        </Svg>
      </View>

      {/* Legend */}
      <View style={styles.legend}>
        <Text style={styles.legendTitle}>Event Classes:</Text>
        <View style={styles.legendGrid}>
          {CLASS_NAMES.map((name, i) => (
            <View key={name} style={styles.legendItem}>
              <View style={[styles.legendDot, { backgroundColor: CLASS_COLORS[name] }]} />
              <Text style={styles.legendText}>{name.replace(/_/g, ' ')}</Text>
            </View>
          ))}
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  podSelector: { flexDirection: 'row', padding: 10, backgroundColor: '#FFFFFF', flexWrap: 'wrap' },
  podTab: { paddingHorizontal: 12, paddingVertical: 6, margin: 4, borderRadius: 16, backgroundColor: '#E0E0E0' },
  podTabActive: { backgroundColor: '#2E7D32' },
  podTabText: { fontSize: 12, color: '#616161' },
  podTabTextActive: { color: '#FFFFFF', fontWeight: 'bold' },
  timeSelector: { flexDirection: 'row', padding: 5, backgroundColor: '#F5F5F5', justifyContent: 'center' },
  timeTab: { paddingHorizontal: 16, paddingVertical: 4, margin: 4, borderRadius: 12, backgroundColor: '#E0E0E0' },
  timeTabActive: { backgroundColor: '#388E3C' },
  timeTabText: { fontSize: 12, color: '#757575' },
  timeTabTextActive: { color: '#FFFFFF', fontWeight: 'bold' },
  chartContainer: { alignItems: 'center', padding: 10, backgroundColor: '#FFFFFF', margin: 10, borderRadius: 12, elevation: 2 },
  legend: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12 },
  legendTitle: { fontSize: 14, fontWeight: 'bold', color: '#424242', marginBottom: 8 },
  legendGrid: { flexDirection: 'row', flexWrap: 'wrap' },
  legendItem: { flexDirection: 'row', alignItems: 'center', width: '50%', marginVertical: 3 },
  legendDot: { width: 12, height: 12, borderRadius: 6, marginRight: 6 },
  legendText: { fontSize: 11, color: '#616161' },
});