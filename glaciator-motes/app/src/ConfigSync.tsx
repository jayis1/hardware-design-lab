// src/ConfigSync.tsx — threshold/template/slot management for the array
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, Button, Alert, Switch } from 'react-native';
import { Card, Title, Paragraph, Divider, Checkbox } from 'react-native-paper';

interface TemplateEntry {
  id: number;
  name: string;
  type: string;
  enabled: boolean;
}

const INITIAL_TEMPLATES: TemplateEntry[] = [
  { id: 0, name: 'basal-slide-v1',    type: 'Basal slide',       enabled: true },
  { id: 1, name: 'crevasse-snap-v2',  type: 'Crevasse snap',     enabled: true },
  { id: 2, name: 'calving-thump-v1',  type: 'Calving thump',     enabled: true },
  { id: 3, name: 'subglacial-tremor', type: 'Subglacial tremor', enabled: false },
  { id: 4, name: 'icequake-local',    type: 'Icequake (local)',  enabled: false },
];

export default function ConfigSync() {
  const [staLta, setStaLta] = useState('3.0');
  const [memsThresh, setMemsThresh] = useState('8000');  // mg
  const [gpsDutyMin, setGpsDuty] = useState('5');
  const [beaconPeriod, setBeaconPeriod] = useState('60');
  const [templates, setTemplates] = useState<TemplateEntry[]>(INITIAL_TEMPLATES);
  const [pushing, setPushing] = useState(false);
  const [autoUpdate, setAutoUpdate] = useState(true);

  const toggleTemplate = (id: number) => {
    setTemplates(ts => ts.map(t => t.id === id ? { ...t, enabled: !t.enabled } : t));
  };

  const pushConfig = () => {
    setPushing(true);
    // In production: build a config packet and send to gateway, which
    // broadcasts it via PKT_CONFIG to all motes in the mesh.
    const cfg = {
      staLtaThreshold: parseFloat(staLta),
      memsThresholdMg: parseInt(memsThresh, 10),
      gpsDutyMinutes:  parseInt(gpsDutyMin, 10),
      beaconPeriodS:   parseInt(beaconPeriod, 10),
      templates: templates.filter(t => t.enabled).map(t => t.id),
    };
    setTimeout(() => {
      setPushing(false);
      Alert.alert(
        'Configuration pushed',
        `Sent to ${5} motes:\n` +
        `STA/LTA=${cfg.staLtaThreshold}\n` +
        `MEMS=${cfg.memsThresholdMg} mg\n` +
        `GPS=${cfg.gpsDutyMinutes} min/hr\n` +
        `Beacon=${cfg.beaconPeriodS} s\n` +
        `Templates: ${cfg.templates.length} enabled`
      );
    }, 1000);
  };

  const importTemplate = () => {
    Alert.alert('Import', 'In production: load a template file (SAC/mini-SEED) from device storage and convert to one-bit sign stream for firmware.');
  };

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Trigger thresholds</Title>
          <Divider style={styles.div} />
          <Text style={styles.label}>STA/LTA ratio</Text>
          <TextInput style={styles.input} keyboardType="decimal-pad" value={staLta} onChangeText={setStaLta} />
          <Text style={styles.label}>MEMS crevasse threshold (mg)</Text>
          <TextInput style={styles.input} keyboardType="numeric" value={memsThresh} onChangeText={setMemsThresh} />
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Mesh & timing</Title>
          <Divider style={styles.div} />
          <Text style={styles.label}>Beacon period (s)</Text>
          <TextInput style={styles.input} keyboardType="numeric" value={beaconPeriod} onChangeText={setBeaconPeriod} />
          <Text style={styles.label}>GPS duty cycle (min/hr)</Text>
          <TextInput style={styles.input} keyboardType="numeric" value={gpsDutyMin} onChangeText={setGpsDuty} />
          <View style={styles.row}>
            <Text style={styles.label}>Auto-update new motes</Text>
            <Switch value={autoUpdate} onValueChange={setAutoUpdate} thumbColor="#0d6efd" />
          </View>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Event templates ({templates.filter(t => t.enabled).length} enabled)</Title>
          <Divider style={styles.div} />
          {templates.map(t => (
            <View key={t.id} style={styles.tmplRow}>
              <Checkbox
                status={t.enabled ? 'checked' : 'unchecked'}
                onPress={() => toggleTemplate(t.id)}
                color="#0d6efd"
              />
              <View style={styles.tmplInfo}>
                <Text style={styles.tmplName}>{t.name}</Text>
                <Text style={styles.tmplType}>{t.type}</Text>
              </View>
            </View>
          ))}
          <Button title="Import template from file" onPress={importTemplate} color="#17a2b8" />
        </Card.Content>
      </Card>

      <Button
        title={pushing ? 'Pushing…' : 'Push to all motes'}
        onPress={pushConfig}
        color="#28a745"
        disabled={pushing}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1220', padding: 8 },
  card: { backgroundColor: '#141b2d', marginBottom: 8 },
  input: {
    backgroundColor: '#1c2540', color: '#e6eaf2',
    borderRadius: 6, paddingHorizontal: 12, paddingVertical: 8, marginVertical: 4,
  },
  label: { color: '#9fb0d0', fontSize: 13, marginTop: 6 },
  div: { marginVertical: 8, backgroundColor: '#2a3556' },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginVertical: 4 },
  tmplRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 6 },
  tmplInfo: { marginLeft: 8 },
  tmplName: { color: '#e6eaf2', fontSize: 14 },
  tmplType: { color: '#7a8aa8', fontSize: 12 },
});