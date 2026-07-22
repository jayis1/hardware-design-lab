/**
 * HeatmapScreen — 2D corrosion-probability heatmap
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
} from 'react-native';
import Svg, { Circle, Rect, Text as SvgText, G } from 'react-native-svg';
import type { StackScreenProps } from '@react-navigation/stack';
import type { RootStackParamList } from '../../App';
import { getPoints, getSurvey, PointRecord } from '../db/database';
import {
  hcpClassColor, hcpClassLabel,
  resistivityClassColor, resistivityClassLabel,
} from '../ble/protocol';

type Props = StackScreenProps<RootStackParamList, 'Heatmap'>;

type Layer = 'hcp' | 'resistivity' | 'cover';

export default function HeatmapScreen({ route, navigation }: Props) {
  const { surveyId } = route.params;
  const [points, setPoints] = useState<PointRecord[]>([]);
  const [survey, setSurvey] = useState<{ site_name: string; created_at: number } | null>(null);
  const [layer, setLayer] = useState<Layer>('hcp');
  const [selected, setSelected] = useState<PointRecord | null>(null);

  useEffect(() => {
    (async () => {
      const pts = await getPoints(surveyId);
      setPoints(pts);
      const surv = await getSurvey(surveyId);
      if (surv) setSurvey({ site_name: surv.site_name, created_at: surv.created_at });
    })();
  }, [surveyId]);

  const W = 320;
  const H = 360;
  const allX = points.map((p) => p.x_mm);
  const allY = points.map((p) => p.y_mm);
  const minX = allX.length ? Math.min(...allX) : 0;
  const maxX = allX.length ? Math.max(...allX) : 1000;
  const minY = allY.length ? Math.min(...allY) : 0;
  const maxY = allY.length ? Math.max(...allY) : 1000;
  const rangeX = Math.max(maxX - minX, 1);
  const rangeY = Math.max(maxY - minY, 1);
  const scale = Math.min(W / rangeX, H / rangeY);
  const offsetX = (W - rangeX * scale) / 2;
  const offsetY = (H - rangeY * scale) / 2;

  const colorFor = (p: PointRecord): string => {
    if (layer === 'hcp') return hcpClassColor(p.hcp_class);
    if (layer === 'resistivity') return resistivityClassColor(p.rho_class);
    /* cover depth: scale 0-80 mm → blue→red */
    const t = Math.min(p.cover_mm / 80, 1);
    const r = Math.round(46 + (231 - 46) * t);
    const g = Math.round(204 + (76 - 204) * t);
    const b = Math.round(113 + (60 - 113) * t);
    return `rgb(${r},${g},${b})`;
  };

  const legend = () => {
    if (layer === 'hcp') {
      return [
        { c: hcpClassColor(0), label: hcpClassLabel(0) },
        { c: hcpClassColor(1), label: hcpClassLabel(1) },
        { c: hcpClassColor(2), label: hcpClassLabel(2) },
      ];
    }
    if (layer === 'resistivity') {
      return [
        { c: resistivityClassColor(0), label: resistivityClassLabel(0) },
        { c: resistivityClassColor(1), label: resistivityClassLabel(1) },
        { c: resistivityClassColor(2), label: resistivityClassLabel(2) },
        { c: resistivityClassColor(3), label: resistivityClassLabel(3) },
      ];
    }
    return [
      { c: 'rgb(46,204,113)', label: '0 mm (shallow)' },
      { c: 'rgb(138,140,86)', label: '40 mm' },
      { c: 'rgb(231,76,60)', label: '80 mm (deep)' },
    ];
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>{survey?.site_name ?? 'Survey'}</Text>
      <Text style={styles.date}>
        {survey ? new Date(survey.created_at).toLocaleString() : ''}
      </Text>

      {/* Layer selector */}
      <View style={styles.layerRow}>
        {(['hcp', 'resistivity', 'cover'] as Layer[]).map((l) => (
          <TouchableOpacity
            key={l}
            onPress={() => setLayer(l)}
            style={[styles.layerBtn, layer === l && styles.layerBtnActive]}
          >
            <Text style={{ color: layer === l ? '#fff' : '#95a5a6' }}>
              {l === 'hcp' ? 'HCP' : l === 'resistivity' ? 'Resistivity' : 'Cover'}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <Svg width={W} height={H} style={styles.map}>
        <Rect x={0} y={0} width={W} height={H} fill="#0d0d1a" rx={8} />
        {points.map((p, i) => (
          <Circle
            key={i}
            cx={(p.x_mm - minX) * scale + offsetX}
            cy={H - ((p.y_mm - minY) * scale + offsetY)}
            r={Math.max(4, 12)}
            fill={colorFor(p)}
            opacity={0.8}
            onPress={() => setSelected(p)}
          />
        ))}
        <SvgText x={8} y={16} fill="#95a5a6" fontSize={10}>
          {points.length} pts
        </SvgText>
      </Svg>

      {/* Legend */}
      <View style={styles.legendBox}>
        {legend().map((item, i) => (
          <View key={i} style={styles.legendItem}>
            <View style={[styles.legendSwatch, { backgroundColor: item.c }]} />
            <Text style={styles.legendText}>{item.label}</Text>
          </View>
        ))}
      </View>

      {/* Selected point detail */}
      {selected && (
        <View style={styles.detailBox}>
          <Text style={styles.detailHeader}>Selected Point</Text>
          <Text>HCP: {selected.hcp_mv.toFixed(1)} mV ({hcpClassLabel(selected.hcp_class)})</Text>
          <Text>Resistivity: {(selected.rho_ohm_m * 100).toFixed(1)} kΩ·cm ({resistivityClassLabel(selected.rho_class)})</Text>
          <Text>Cover: {selected.cover_mm.toFixed(1)} mm  Ø {selected.rebar_diam_mm.toFixed(0)} mm</Text>
          <Text>Pos: ({(selected.x_mm / 1000).toFixed(2)}, {(selected.y_mm / 1000).toFixed(2)}) m</Text>
          <Text>Hash: {selected.hash_hex.substring(0, 16)}…</Text>
        </View>
      )}

      <View style={styles.actions}>
        <TouchableOpacity
          style={styles.actionBtn}
          onPress={() => navigation.navigate('Lpr', { surveyId })}
        >
          <Text style={styles.actionText}>LPR Detail</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.actionBtn}
          onPress={() => navigation.navigate('Report', { surveyId })}
        >
          <Text style={styles.actionText}>Export Report</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e', padding: 16 },
  header: { fontSize: 22, fontWeight: 'bold', color: '#ecf0f1' },
  date: { fontSize: 12, color: '#95a5a6', marginBottom: 12 },
  layerRow: { flexDirection: 'row', gap: 8, marginBottom: 12 },
  layerBtn: {
    padding: 8, borderRadius: 6, backgroundColor: '#2a2a4e',
  },
  layerBtnActive: { backgroundColor: '#3498db' },
  map: { alignSelf: 'center', marginBottom: 12 },
  legendBox: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 12, marginBottom: 12 },
  legendItem: { flexDirection: 'row', alignItems: 'center', paddingVertical: 4 },
  legendSwatch: { width: 16, height: 16, borderRadius: 3, marginRight: 8 },
  legendText: { color: '#ecf0f1', fontSize: 12 },
  detailBox: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 12, marginBottom: 12 },
  detailHeader: { fontSize: 14, fontWeight: 'bold', color: '#ecf0f1', marginBottom: 6 },
  actions: { flexDirection: 'row', gap: 8, marginBottom: 24 },
  actionBtn: { flex: 1, backgroundColor: '#3498db', padding: 12, borderRadius: 8, alignItems: 'center' },
  actionText: { color: '#fff', fontWeight: 'bold' },
});