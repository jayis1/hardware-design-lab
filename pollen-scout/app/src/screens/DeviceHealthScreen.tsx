/**
 * DeviceHealthScreen.tsx - Device health & OTA
 *
 * Shows battery %, solar V, flow rate, SD usage, last uplink,
 * firmware version, and an OTA update button.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useMemo } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert,
} from 'react-native';
import { useStation, makeDemoStation } from '../context/StationContext';

export default function DeviceHealthScreen() {
  const { station: live, triggerOta } = useStation();
  const station = live ?? useMemo(() => makeDemoStation(), []);
  const [otaBusy, setOtaBusy] = useState(false);
  const h = station.health;

  const onOta = async () => {
    setOtaBusy(true);
    try {
      await triggerOta();
      Alert.alert('OTA', 'Update triggered. The station will reboot when done.');
    } catch (e: any) {
      Alert.alert('OTA failed', e?.message ?? 'unknown');
    } finally {
      setOtaBusy(false);
    }
  };

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Device Health</Text>
      <View style={s.cardRow}>
        <Metric label="Battery" value={`${(h.batteryMv / 1000).toFixed(2)} V`}
                sub={`${h.chargePct}%`} good={h.chargePct > 50} />
        <Metric label="Solar"   value={`${(h.solarMv / 1000).toFixed(2)} V`}
                sub={h.charging ? 'charging' : '—'} good={h.charging} />
      </View>
      <View style={s.cardRow}>
        <Metric label="Flow"  value={`${h.flowLpm.toFixed(2)} L/min`}
                sub="target 2.00" good={Math.abs(h.flowLpm - 2) < 0.1} />
        <Metric label="SD"    value={`${h.sdUsagePct}%`}
                sub="used" good={h.sdUsagePct < 80} />
      </View>
      <View style={s.cardRow}>
        <Metric label="Last uplink"
                value={`${(h.lastUplinkMs / 1000).toFixed(0)} s ago`}
                sub="LoRa" good={h.lastUplinkMs < 120_000} />
        <Metric label="Firmware" value={h.firmwareVersion}
                sub="build" good />
      </View>
      <View style={s.cardRow}>
        <Metric label="Captures" value={`${h.captureCount}`}
                sub="frames" good />
        <Metric label="ROIs"     value={`${h.roiCountTotal}`}
                sub="classified" good />
      </View>

      <Text style={s.sectionTitle}>OTA Firmware Update</Text>
      <TouchableOpacity
        style={[s.otaBtn, otaBusy && s.otaBtnBusy]}
        onPress={onOta}
        disabled={otaBusy}
      >
        <Text style={s.otaBtnText}>
          {otaBusy ? 'Triggering…' : 'Check & Update'}
        </Text>
      </TouchableOpacity>
      <Text style={s.note}>
        The ESP32-C3 co-MCU fetches a signed manifest over WiFi/BLE and
        reflashes the STM32 via its system bootloader. Images are
        Ed25519-signed.
      </Text>
      <Text style={s.footer}>Pollen Scout · jayis1 · MIT</Text>
    </ScrollView>
  );
}

function Metric({ label, value, sub, good }: {
  label: string; value: string; sub: string; good: boolean;
}) {
  return (
    <View style={[s.metric, { borderLeftColor: good ? '#4CAF50' : '#FF9800' }]}>
      <Text style={s.metricLabel}>{label}</Text>
      <Text style={s.metricValue}>{value}</Text>
      <Text style={s.metricSub}>{sub}</Text>
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 20, fontWeight: '800', color: '#2E7D32', margin: 16 },
  cardRow: { flexDirection: 'row', justifyContent: 'space-between', marginHorizontal: 12 },
  metric: { flex: 1, backgroundColor: '#fff', margin: 4, padding: 12, borderRadius: 8, borderLeftWidth: 4, elevation: 1 },
  metricLabel: { fontSize: 11, color: '#888' },
  metricValue: { fontSize: 18, fontWeight: '800', color: '#333' },
  metricSub:   { fontSize: 11, color: '#aaa' },
  sectionTitle: { fontSize: 15, fontWeight: '700', color: '#333', margin: 12, marginBottom: 4 },
  otaBtn: { backgroundColor: '#1565C0', margin: 12, padding: 14, borderRadius: 10, alignItems: 'center' },
  otaBtnBusy: { backgroundColor: '#90A4AE' },
  otaBtnText: { color: '#fff', fontWeight: '700' },
  note: { fontSize: 11, color: '#888', marginHorizontal: 16, marginTop: 4 },
  footer: { textAlign: 'center', color: '#999', fontSize: 11, margin: 16 },
});