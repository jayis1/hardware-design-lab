/**
 * FieldMapScreen.js — Field-scale SVI heatmap with pod locations
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Dimensions } from 'react-native';
import { Svg, Circle, Rect, Text as SvgText, G, Path, RadialGradient, Stop, Defs } from 'react-native-svg';
import { useSelector } from 'react-redux';

const { width } = Dimensions.get('window');
const MAP_W = width - 20;
const MAP_H = 400;

// Demo pod positions on a relative field grid (0-1, 0-1)
const DEMO_POSITIONS = {
  'pod-001': { x: 0.2, y: 0.3, name: 'North Field A' },
  'pod-002': { x: 0.5, y: 0.5, name: 'Vineyard Row 3' },
  'pod-003': { x: 0.75, y: 0.7, name: 'Greenhouse Bed' },
};

function sviToColor(svi) {
  if (svi >= 75) return '#4CAF50';
  if (svi >= 50) return '#FFEB3B';
  if (svi >= 30) return '#FF9800';
  return '#F44336';
}

export default function FieldMapScreen({ navigation }) {
  const pods = useSelector(state => state.pods.pods);
  const [selectedPod, setSelectedPod] = useState(null);

  // Merge demo positions with pod data
  const podsWithPos = pods.map(p => ({
    ...p,
    pos: DEMO_POSITIONS[p.id] || { x: Math.random(), y: Math.random(), name: p.id },
  }));

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Field Map</Text>
        <Text style={styles.subtitle}>Soil Vitality Index distribution</Text>
      </View>

      <View style={styles.mapContainer}>
        <Svg width={MAP_W} height={MAP_H}>
          <Defs>
            {podsWithPos.map(pod => (
              <RadialGradient key={pod.id} id={`grad-${pod.id}`} cx="50%" cy="50%" r="50%">
                <Stop offset="0%" stopColor={sviToColor(pod.svi || 50)} stopOpacity="0.6" />
                <Stop offset="100%" stopColor={sviToColor(pod.svi || 50)} stopOpacity="0" />
              </RadialGradient>
            ))}
          </Defs>

          {/* Field boundary */}
          <Rect x={5} y={5} width={MAP_W - 10} height={MAP_H - 10} fill="#F1F8E9" stroke="#8BC34A" strokeWidth={2} rx={8} />

          {/* SVI heatmap circles */}
          {podsWithPos.map(pod => {
            const cx = pod.pos.x * MAP_W;
            const cy = pod.pos.y * MAP_H;
            return (
              <G key={`heat-${pod.id}`}>
                <Circle cx={cx} cy={cy} r={80} fill={`url(#grad-${pod.id})`} />
              </G>
            );
          })}

          {/* Pod markers */}
          {podsWithPos.map(pod => {
            const cx = pod.pos.x * MAP_W;
            const cy = pod.pos.y * MAP_H;
            const color = sviToColor(pod.svi || 50);
            return (
              <G key={pod.id}>
                <Circle cx={cx} cy={cy} r={12} fill={color} stroke="#FFFFFF" strokeWidth={2} />
                <SvgText x={cx} y={cy + 4} fill="#FFFFFF" fontSize={10} fontWeight="bold" textAnchor="middle">
                  {pod.svi || 0}
                </SvgText>
                <SvgText x={cx} y={cy - 18} fill="#424242" fontSize={9} textAnchor="middle">
                  {pod.pos.name}
                </SvgText>
              </G>
            );
          })}

          {/* Compass */}
          <G>
            <SvgText x={MAP_W - 20} y={20} fill="#757575" fontSize={14} textAnchor="middle">N</SvgText>
            <Path d={`M ${MAP_W - 20} 25 L ${MAP_W - 23} 35 L ${MAP_W - 17} 35 Z`} fill="#757575" />
          </G>
        </Svg>
      </View>

      {/* Legend */}
      <View style={styles.legend}>
        <Text style={styles.legendTitle}>SVI Scale:</Text>
        <View style={styles.legendBar}>
          <View style={[styles.legendSeg, { backgroundColor: '#4CAF50' }]}><Text style={styles.legendSegText}>75+</Text></View>
          <View style={[styles.legendSeg, { backgroundColor: '#FFEB3B' }]}><Text style={styles.legendSegText}>50-74</Text></View>
          <View style={[styles.legendSeg, { backgroundColor: '#FF9800' }]}><Text style={styles.legendSegText}>30-49</Text></View>
          <View style={[styles.legendSeg, { backgroundColor: '#F44336' }]}><Text style={styles.legendSegText}>0-29</Text></View>
        </View>
      </View>

      {/* Selected pod details */}
      {selectedPod && (
        <View style={styles.podDetail}>
          <Text style={styles.podDetailTitle}>{selectedPod.name}</Text>
          <Text style={styles.podDetailSvi}>SVI: {selectedPod.svi}/100</Text>
          <Text style={styles.podDetailBat}>Battery: {selectedPod.batteryPct}%</Text>
          <TouchableOpacity
            style={styles.detailButton}
            onPress={() => navigation.navigate('PodDetail', { podId: selectedPod.id })}
          >
            <Text style={styles.detailButtonText}>View Details</Text>
          </TouchableOpacity>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { padding: 15, backgroundColor: '#2E7D32', alignItems: 'center' },
  title: { fontSize: 20, fontWeight: 'bold', color: '#FFFFFF' },
  subtitle: { fontSize: 12, color: '#E8F5E9' },
  mapContainer: { alignItems: 'center', padding: 10, backgroundColor: '#FFFFFF', margin: 10, borderRadius: 12, elevation: 2 },
  legend: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12 },
  legendTitle: { fontSize: 14, fontWeight: 'bold', color: '#424242', marginBottom: 8 },
  legendBar: { flexDirection: 'row', borderRadius: 8, overflow: 'hidden' },
  legendSeg: { flex: 1, padding: 8, alignItems: 'center' },
  legendSegText: { fontSize: 10, color: '#FFFFFF', fontWeight: 'bold' },
  podDetail: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12, elevation: 2 },
  podDetailTitle: { fontSize: 16, fontWeight: 'bold', color: '#2E7D32' },
  podDetailSvi: { fontSize: 14, color: '#424242', marginTop: 5 },
  podDetailBat: { fontSize: 12, color: '#757575', marginTop: 3 },
  detailButton: { marginTop: 10, backgroundColor: '#2E7D32', padding: 8, borderRadius: 8, alignItems: 'center' },
  detailButtonText: { color: '#FFFFFF', fontSize: 14, fontWeight: 'bold' },
});