/**
 * EventDetailScreen.js — Detail view for a single acoustic event
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Svg, Rect, Text as SvgText, G, Line, Circle, Path } from 'react-native-svg';
import { Dimensions } from 'react-native';

const { width } = Dimensions.get('window');

const CLASS_DETAILS = {
  0: { name: 'Root Growth', color: '#4CAF50', desc: 'Acoustic emission from root tip exudation or soil micro-fracture as roots grow through the soil matrix.' },
  1: { name: 'Root Hydraulic', color: '#66BB6A', desc: 'Nocturnal hydraulic redistribution — water moving through root xylem from moist to dry soil layers.' },
  2: { name: 'Earthworm Burrow', color: '#8D6E63', desc: 'Earthworm movement through soil, creating burrows and bioturbation. Key indicator of healthy soil biology.' },
  3: { name: 'Earthworm Cast', color: '#A1887F', desc: 'Earthworm casting and feeding activity at the soil surface or within burrows.' },
  4: { name: 'Arthropod Click', color: '#FFB74D', desc: 'Micro-arthropod (springtail, mite) movement producing short broadband clicks.' },
  5: { name: 'Grub Chew (PEST)', color: '#F44336', desc: 'Root-feeding Coleoptera larvae (e.g. Phyllophaga, Melolontha) chewing on roots. PEST ALERT.' },
  6: { name: 'Microbe Ebullition', color: '#26A69A', desc: 'CO₂ bubble release from microbial respiration accumulating in soil pores.' },
  7: { name: 'Water Infiltration', color: '#42A5F5', desc: 'Rainfall or irrigation water percolating through soil macropores.' },
  8: { name: 'Compaction Crack', color: '#FF7043', desc: 'Soil structural degradation — desiccation or compaction-induced micro-fracturing.' },
  9: { name: 'Noise', color: '#BDBDBD', desc: 'Unclassified or anthropogenic noise (traffic, machinery).' },
};

// Generate a demo mel spectrogram
function generateMelSpectrogram() {
  const data = [];
  for (let f = 0; f < 8; f++) {
    const row = [];
    for (let b = 0; b < 40; b++) {
      row.push(Math.random() * 0.7 + Math.sin(b / 5 + f / 2) * 0.3);
    }
    data.push(row);
  }
  return data;
}

export default function EventDetailScreen({ route, navigation }) {
  const { event } = route.params;
  const classInfo = CLASS_DETAILS[event?.classId] || CLASS_DETAILS[9];
  const [spectrogram] = useState(generateMelSpectrogram());

  if (!event) {
    return (
      <View style={styles.container}>
        <Text style={styles.errorText}>Event not found</Text>
      </View>
    );
  }

  // Render mel spectrogram as coloured grid
  const SG_W = width - 40;
  const SG_H = 160;
  const cellW = SG_W / 40;
  const cellH = SG_H / 8;

  const colorForValue = (v) => {
    if (v < 0.2) return '#1B5E20';
    if (v < 0.4) return '#4CAF50';
    if (v < 0.6) return '#FFEB3B';
    if (v < 0.8) return '#FF9800';
    return '#F44336';
  };

  return (
    <ScrollView style={styles.container}>
      {/* Event header */}
      <View style={[styles.header, { backgroundColor: classInfo.color }]}>
        <Text style={styles.eventClass}>{classInfo.name}</Text>
        <Text style={styles.eventTime}>{new Date(event.timestamp).toLocaleString()}</Text>
      </View>

      {/* Event details */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Description</Text>
        <Text style={styles.description}>{classInfo.desc}</Text>
      </View>

      {/* Event metadata */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Event Details</Text>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Pod:</Text>
          <Text style={styles.dataValue}>{event.podId || 'unknown'}</Text>
        </View>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Channel:</Text>
          <Text style={styles.dataValue}>{event.channel}</Text>
        </View>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Confidence:</Text>
          <Text style={styles.dataValue}>{event.confidence?.toFixed(1)}%</Text>
        </View>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Class ID:</Text>
          <Text style={styles.dataValue}>{event.classId}</Text>
        </View>
      </View>

      {/* Source localisation */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Source Localisation (Beamformer)</Text>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>X (horizontal):</Text>
          <Text style={styles.dataValue}>{event.posX?.toFixed(2)} m</Text>
        </View>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Y (horizontal):</Text>
          <Text style={styles.dataValue}>{event.posY?.toFixed(2)} m</Text>
        </View>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Z (depth):</Text>
          <Text style={styles.dataValue}>{event.posZ?.toFixed(2)} m</Text>
        </View>
        <View style={styles.dataRow}>
          <Text style={styles.dataLabel}>Residual:</Text>
          <Text style={styles.dataValue}>{event.residual?.toFixed(4)}</Text>
        </View>

        {/* Localisation diagram */}
        <View style={styles.localisationDiagram}>
          <Svg width={SG_W} height={200}>
            {/* Soil column */}
            <Rect x={0} y={0} width={SG_W} height={200} fill="#5D4037" opacity={0.2} />
            <Line x1={0} y1={20} x2={SG_W} y2={20} stroke="#4CAF50" strokeWidth={2} />
            <Text style={{ fontSize: 10, color: '#FFFFFF' }}>Surface</Text>
            <SvgText x={5} y={15} fill="#4CAF50" fontSize={10}>Surface</SvgText>

            {/* Depth markers */}
            {[20, 40, 60].map(d => {
              const y = 20 + (d / 70) * 180;
              return <Line key={d} x1={0} y1={y} x2={SG_W} y2={y} stroke="#FFFFFF" strokeWidth={0.5} opacity={0.2} />;
            })}

            {/* Source position */}
            <G>
              <Circle cx={SG_W / 2 + (event.posX || 0) * 50} cy={20 + (Math.abs(event.posZ || 0) / 70) * 180} r={8} fill={classInfo.color} opacity={0.6} />
              <Circle cx={SG_W / 2 + (event.posX || 0) * 50} cy={20 + (Math.abs(event.posZ || 0) / 70) * 180} r={3} fill={classInfo.color} />
            </G>
          </Svg>
        </View>
      </View>

      {/* Mel spectrogram */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Mel Spectrogram (8 frames × 40 bins)</Text>
        <Svg width={SG_W} height={SG_H}>
          {spectrogram.map((row, f) =>
            row.map((val, b) => (
              <Rect
                key={`${f}-${b}`}
                x={b * cellW}
                y={f * cellH}
                width={cellW}
                height={cellH}
                fill={colorForValue(val)}
              />
            ))
          )}
        </Svg>
        <View style={styles.spectrogramLegend}>
          <Text style={styles.legendText}>← Time</Text>
          <Text style={styles.legendText}>Frequency →</Text>
        </View>
      </View>

      {/* Action buttons */}
      {event.classId === 5 && (
        <View style={styles.pestAlert}>
          <Text style={styles.pestAlertTitle}>⚠️ Pest Alert</Text>
          <Text style={styles.pestAlertDesc}>
            Root-feeding larvae detected. Inspect the area around {event.podId} and consider
            targeted biological control (beneficial nematodes) within 7 days.
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { padding: 20, alignItems: 'center' },
  eventClass: { fontSize: 22, fontWeight: 'bold', color: '#FFFFFF' },
  eventTime: { fontSize: 12, color: '#FFFFFF', opacity: 0.9, marginTop: 5 },
  section: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12, elevation: 1 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#2E7D32', marginBottom: 8 },
  description: { fontSize: 14, color: '#424242', lineHeight: 20 },
  dataRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 5 },
  dataLabel: { fontSize: 13, color: '#757575' },
  dataValue: { fontSize: 13, color: '#424242', fontWeight: 'bold' },
  localisationDiagram: { marginTop: 10, alignItems: 'center' },
  spectrogramLegend: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 5 },
  legendText: { fontSize: 10, color: '#9E9E9E' },
  pestAlert: { margin: 10, padding: 15, backgroundColor: '#FFEBEE', borderRadius: 12, borderLeftWidth: 4, borderLeftColor: '#F44336' },
  pestAlertTitle: { fontSize: 16, fontWeight: 'bold', color: '#C62828', marginBottom: 5 },
  pestAlertDesc: { fontSize: 13, color: '#B71C1C' },
  errorText: { fontSize: 16, color: '#F44336', textAlign: 'center', marginTop: 50 },
});