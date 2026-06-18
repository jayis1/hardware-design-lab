// screens/DeploymentsScreen.js — logging interval, LoRa, export, deep-sleep
// Author: jayis1  License: MIT
import React, { useState } from 'react';
import { View, Text, TextInput, Button, Switch, ScrollView, StyleSheet, Alert, Share } from 'react-native';
import { theme } from '../utils/theme';
import ble from '../utils/ble';

export default function DeploymentsScreen() {
  const [periodMs, setPeriodMs] = useState('10000');
  const [pump, setPump] = useState(false);
  const [loraInterval, setLoraInterval] = useState('300');
  const [exporting, setExporting] = useState(false);

  const applyPeriod = async () => {
    const ms = parseInt(periodMs, 10);
    if (isNaN(ms) || ms < 1000 || ms > 3600000) {
      Alert.alert('Invalid period', 'Enter a value between 1000 and 3600000 ms.');
      return;
    }
    try { await ble.setPeriodMs(ms); Alert.alert('Applied', `Survey period = ${ms} ms`); }
    catch (e) { Alert.alert('Error', String(e.message || e)); }
  };

  const togglePump = async (v) => {
    setPump(v);
    try { v ? await ble.pumpOn() : await ble.pumpOff(); }
    catch (e) { Alert.alert('Error', String(e.message || e)); setPump(!v); }
  };

  const exportCsv = async () => {
    setExporting(true);
    // In a full implementation, this reads the SD log over BLE and formats
    // as CSV. Here we produce a GeoJSON template the user can paste into.
    const geojson = {
      type: 'FeatureCollection',
      features: [],   // would be populated from downloaded samples
      metadata: { device: 'HydroFluor', author: 'jayis1', exported: new Date().toISOString() },
    };
    try {
      await Share.share({ message: JSON.stringify(geojson, null, 2), title: 'HydroFluor export' });
    } catch (e) {
      Alert.alert('Export error', String(e.message || e));
    } finally {
      setExporting(false);
    }
  };

  return (
    <ScrollView style={styles.container} contentContainerStyle={{ paddingBottom: 40 }}>
      <Text style={styles.title}>Deployments</Text>
      <Text style={styles.sub}>Configure logging, pump, LoRa uplink, and export survey data.</Text>

      <Text style={styles.fieldLabel}>Survey period (ms)</Text>
      <View style={styles.row}>
        <TextInput style={styles.input} value={periodMs} onChangeText={setPeriodMs} keyboardType="numeric" />
        <Button title="Apply" onPress={applyPeriod} color={theme.colors.accent} />
      </View>

      <View style={styles.toggleRow}>
        <Text style={styles.toggleLabel}>Flow-cell pump</Text>
        <Switch value={pump} onValueChange={togglePump} trackColor={{ false: theme.colors.accentDim, true: theme.colors.good }} />
      </View>

      <Text style={styles.fieldLabel}>LoRa uplink interval (s, 0 = off)</Text>
      <TextInput style={styles.input} value={loraInterval} onChangeText={setLoraInterval} keyboardType="numeric" />

      <Text style={styles.sectionTitle}>Deep-sleep logging</Text>
      <Text style={styles.sectionBody}>
        For unattended deployments the sonde sleeps in STOP2 (~120 µA) and wakes on the
        RTC alarm every survey period to sample, log to SD, optionally LoRa-uplink, and
        return to sleep. Configure the wake interval above; the device enters sleep mode
        automatically after each sample when STOP is issued.
      </Text>

      <Text style={styles.sectionTitle}>Export</Text>
      <Text style={styles.sectionBody}>
        Download the SD log (CSV) or export the current session as GeoJSON for GIS tools.
      </Text>
      <Button title={exporting ? 'Exporting…' : 'Export GeoJSON'} onPress={exportCsv} disabled={exporting} color={theme.colors.accent} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: theme.colors.bg, padding: theme.spacing.md },
  title: { color: theme.colors.text, fontSize: 22, fontWeight: '700' },
  sub: { color: theme.colors.textDim, fontSize: 12, marginBottom: theme.spacing.md },
  fieldLabel: { color: theme.colors.textDim, fontSize: 11, textTransform: 'uppercase', marginTop: 10, marginBottom: 4 },
  row: { flexDirection: 'row', gap: 8 },
  input: { flex: 1, backgroundColor: theme.colors.surface, color: theme.colors.text, borderRadius: theme.radius.md, paddingHorizontal: 10, paddingVertical: 8, fontFamily: theme.fonts.mono },
  toggleRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', backgroundColor: theme.colors.surface, borderRadius: theme.radius.md, padding: 10, marginTop: 10 },
  toggleLabel: { color: theme.colors.text, fontSize: 14 },
  sectionTitle: { color: theme.colors.accent, fontSize: 13, fontWeight: '700', marginTop: theme.spacing.lg, marginBottom: 4, textTransform: 'uppercase' },
  sectionBody: { color: theme.colors.textDim, fontSize: 12, lineHeight: 18 },
});