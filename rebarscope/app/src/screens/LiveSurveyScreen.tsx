/**
 * LiveSurveyScreen — real-time survey readout + trigger
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, Button, StyleSheet, Vibration,
} from 'react-native';
import Svg, { Circle, Polyline, Rect, Text as SvgText } from 'react-native-svg';
import type { StackScreenProps } from '@react-navigation/stack';
import type { RootStackParamList } from '../../App';
import BleManager from '../ble/BleManager';
import {
  RESP,
  parseSurveyPoint,
  hcpClassLabel,
  hcpClassColor,
  resistivityClassLabel,
  resistivityClassColor,
  SurveyPoint,
} from '../ble/protocol';
import { createSurvey, addPoint, updateHashChain } from '../db/database';

type Props = StackScreenProps<RootStackParamList, 'Live'>;

export default function LiveSurveyScreen({ route, navigation }: Props) {
  const { config, siteName } = route.params;
  const [points, setPoints] = useState<SurveyPoint[]>([]);
  const [lastPoint, setLastPoint] = useState<SurveyPoint | null>(null);
  const [surveyId, setSurveyId] = useState<number | null>(null);
  const [recording, setRecording] = useState(false);
  const [battery, setBattery] = useState<number | null>(null);
  const initialized = useRef(false);

  useEffect(() => {
    if (initialized.current) return;
    initialized.current = true;
    (async () => {
      const id = await createSurvey(siteName, JSON.stringify(config));
      setSurveyId(id);
      await BleManager.startSurvey();
      setRecording(true);
    })();

    BleManager.setResponseHandler((respId, payload) => {
      if (respId === RESP.POINT && payload.length >= 84) {
        const p = parseSurveyPoint(payload);
        setPoints((prev) => [...prev, p]);
        setLastPoint(p);
        if (surveyId) {
          addPoint({
            survey_id: surveyId,
            x_mm: p.x_mm,
            y_mm: p.y_mm,
            heading_deg: p.heading_deg,
            hcp_mv: p.hcp_mv,
            hcp_class: p.hcp_class,
            rho_ohm_m: p.rho_ohm_m,
            rho_class: p.rho_class,
            cover_mm: p.cover_mm,
            rebar_diam_mm: p.rebar_diam_mm,
            timestamp_ms: p.timestamp_ms,
            hash_hex: Array.from(p.hash).map((b) => b.toString(16).padStart(2, '0')).join(''),
          });
        }
        /* Haptic feedback on active corrosion detection */
        if (p.hcp_class === 2) Vibration.vibrate(200);
      }
    });
  }, [config, siteName, surveyId]);

  const handleTrigger = async () => {
    await BleManager.triggerPoint();
  };

  const handleStop = async () => {
    setRecording(false);
    await BleManager.stopSurvey();
    const hash = await BleManager.getHash();
    if (hash && surveyId) {
      const hex = Array.from(hash).map((b) => b.toString(16).padStart(2, '0')).join('');
      await updateHashChain(surveyId, hex);
    }
    if (surveyId) navigation.navigate('Heatmap', { surveyId });
  };

  const handleLpr = () => {
    if (surveyId) navigation.navigate('Lpr', { surveyId });
  };

  /* Compute bounding box for breadcrumb trail */
  const allX = points.map((p) => p.x_mm);
  const allY = points.map((p) => p.y_mm);
  const minX = allX.length ? Math.min(...allX) : 0;
  const maxX = allX.length ? Math.max(...allX) : 1000;
  const minY = allY.length ? Math.min(...allY) : 0;
  const maxY = allY.length ? Math.max(...allY) : 1000;
  const rangeX = Math.max(maxX - minX, 1);
  const rangeY = Math.max(maxY - minY, 1);
  const W = 320;
  const H = 240;
  const scale = Math.min(W / rangeX, H / rangeY);
  const offsetX = (W - rangeX * scale) / 2;
  const offsetY = (H - rangeY * scale) / 2;
  const pathStr = points
    .map((p, i) => `${i === 0 ? 'M' : 'L'} ${(p.x_mm - minX) * scale + offsetX} ${H - ((p.y_mm - minY) * scale + offsetY)}`)
    .join(' ');

  return (
    <View style={styles.container}>
      <Text style={styles.header}>{siteName}</Text>
      <Text style={styles.subheader}>{recording ? '● Recording' : '■ Stopped'}</Text>

      {/* Live readout card */}
      {lastPoint && (
        <View style={styles.readoutCard}>
          <View style={styles.row}>
            <Text style={styles.metricLabel}>HCP</Text>
            <Text style={[styles.metricValue, { color: hcpClassColor(lastPoint.hcp_class) }]}>
              {lastPoint.hcp_mv.toFixed(1)} mV
            </Text>
            <Text style={styles.classLabel}>{hcpClassLabel(lastPoint.hcp_class)}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.metricLabel}>Resistivity</Text>
            <Text style={[styles.metricValue, { color: resistivityClassColor(lastPoint.rho_class) }]}>
              {(lastPoint.rho_ohm_m * 100).toFixed(1)} kΩ·cm
            </Text>
            <Text style={styles.classLabel}>{resistivityClassLabel(lastPoint.rho_class)}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.metricLabel}>Cover depth</Text>
            <Text style={styles.metricValue}>{lastPoint.cover_mm.toFixed(1)} mm</Text>
            <Text style={styles.classLabel}>Ø {lastPoint.rebar_diam_mm.toFixed(0)} mm</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.metricLabel}>Position</Text>
            <Text style={styles.metricValue}>
              ({(lastPoint.x_mm / 1000).toFixed(2)}, {(lastPoint.y_mm / 1000).toFixed(2)}) m
            </Text>
          </View>
        </View>
      )}

      {/* Breadcrumb map */}
      <Svg width={W} height={H} style={styles.map}>
        <Rect x={0} y={0} width={W} height={H} fill="#0d0d1a" rx={8} />
        {points.map((p, i) => (
          <Circle
            key={i}
            cx={(p.x_mm - minX) * scale + offsetX}
            cy={H - ((p.y_mm - minY) * scale + offsetY)}
            r={3}
            fill={hcpClassColor(p.hcp_class)}
          />
        ))}
        {points.length > 1 && <Polyline points={pathStr} fill="none" stroke="#3498db" strokeWidth={1} />}
        <SvgText x={8} y={16} fill="#95a5a6" fontSize={10}>
          {points.length} points
        </SvgText>
      </Svg>

      {/* Buttons */}
      <View style={styles.buttonRow}>
        <Button title="Trigger Point" onPress={handleTrigger} color="#3498db" />
        <Button title="Run LPR" onPress={handleLpr} color="#9b59b6" />
        <Button title="Stop & View Heatmap" onPress={handleStop} color="#e74c3c" />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#1a1a2e' },
  header: { fontSize: 20, fontWeight: 'bold', color: '#ecf0f1' },
  subheader: { fontSize: 12, color: '#95a5a6', marginBottom: 8 },
  readoutCard: {
    backgroundColor: '#2a2a4e',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 4 },
  metricLabel: { color: '#95a5a6', fontSize: 14, flex: 1 },
  metricValue: { fontSize: 16, fontWeight: 'bold', color: '#ecf0f1', flex: 1, textAlign: 'center' },
  classLabel: { fontSize: 11, color: '#7f8c8d', flex: 1, textAlign: 'right' },
  map: { marginBottom: 12, alignSelf: 'center' },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between', gap: 8 },
});