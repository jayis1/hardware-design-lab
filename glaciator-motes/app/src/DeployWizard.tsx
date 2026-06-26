// src/DeployWizard.tsx — per-mote provisioning flow over BLE
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, TextInput, Button, Alert, ActivityIndicator,
} from 'react-native';
import { Card, Title, Paragraph, RadioButton, Divider } from 'react-native-paper';

// GATT service & characteristic UUIDs for the Glaciator-Motes provisioning service
const SERVICE_UUID       = '0000f001-0000-1000-8000-00805f9b34fb';
const CHAR_NODE_ID_UUID  = '0000f002-0000-1000-8000-00805f9b34fb';
const CHAR_MESH_GROUP    = '0000f003-0000-1000-8000-00805f9b34fb';
const CHAR_RADIO_BAND    = '0000f004-0000-1000-8000-00805f9b34fb';
const CHAR_STA_LTA_THR   = '0000f005-0000-1000-8000-00805f9b34fb';
const CHAR_TEMPLATES     = '0000f006-0000-1000-8000-00805f9b34fb';
const CHAR_APPLY         = '0000f007-0000-1000-8000-00805f9b34fb';

type RadioBand = 'eu868' | 'us915';

interface ProvisioningConfig {
  nodeId: number;
  meshGroup: number;
  radioBand: RadioBand;
  staLtaThreshold: number;
  templateCount: number;
}

export default function DeployWizard() {
  const [scanning, setScanning] = useState(false);
  const [connected, setConnected] = useState(false);
  const [config, setConfig] = useState<ProvisioningConfig>({
    nodeId: 1,
    meshGroup: 0,
    radioBand: 'eu868',
    staLtaThreshold: 3.0,
    templateCount: 3,
  });
  const [qrPayload, setQrPayload] = useState('');
  const [applying, setApplying] = useState(false);

  const scanForMote = useCallback(async () => {
    setScanning(true);
    // In production: use expo-ble-peripheral / react-native-ble-plx to scan
    // for the Glaciator-Motes provisioning service. Here we simulate.
    setTimeout(() => {
      setScanning(false);
      setConnected(true);
      Alert.alert('Connected', 'Found a Glaciator-Mote over BLE.');
    }, 1500);
  }, []);

  const parseQr = useCallback(() => {
    try {
      // QR encodes: node_id,group,band,thr
      const parts = qrPayload.split(',');
      if (parts.length >= 4) {
        setConfig({
          nodeId: parseInt(parts[0], 10),
          meshGroup: parseInt(parts[1], 10),
          radioBand: parts[2] as RadioBand,
          staLtaThreshold: parseFloat(parts[3]),
          templateCount: 3,
        });
      }
    } catch {
      Alert.alert('QR Error', 'Invalid QR payload.');
    }
  }, [qrPayload]);

  const applyConfig = useCallback(async () => {
    setApplying(true);
    // In production: write each characteristic via BLE, then trigger CHAR_APPLY
    const writes = [
      [CHAR_NODE_ID_UUID, new Uint8Array([config.nodeId])],
      [CHAR_MESH_GROUP,   new Uint8Array([config.meshGroup])],
      [CHAR_RADIO_BAND,   new Uint8Array([config.radioBand === 'eu868' ? 0 : 1])],
      [CHAR_STA_LTA_THR,  new Float32Array([config.staLtaThreshold]).buffer],
      [CHAR_APPLY,        new Uint8Array([0x01])],
    ];
    // Simulate writes
    setTimeout(() => {
      setApplying(false);
      Alert.alert(
        'Provisioned',
        `Mote #${config.nodeId} configured for group ${config.meshGroup}, ${config.radioBand}, thr=${config.staLtaThreshold}.`
      );
    }, 1200);
  }, [config]);

  return (
    <View style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>Step 1 — Scan QR / Connect</Title>
          <Paragraph>Power on the mote and scan its QR code or tap Connect to find it over BLE.</Paragraph>
          <TextInput
            style={styles.input}
            placeholder="QR: nodeId,group,band,thr"
            value={qrPayload}
            onChangeText={setQrPayload}
            placeholderTextColor="#888"
          />
          <View style={styles.row}>
            <Button title="Parse QR" onPress={parseQr} color="#0d6efd" />
            <Button title={scanning ? 'Scanning…' : 'Connect BLE'} onPress={scanForMote} color="#17a2b8" />
          </View>
          {scanning && <ActivityIndicator color="#17a2b8" />}
          {connected && <Text style={styles.ok}>✓ Connected to mote</Text>}
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Step 2 — Mesh Configuration</Title>
          <Divider style={styles.div} />
          <Text style={styles.label}>Node ID (1–60)</Text>
          <TextInput
            style={styles.input}
            keyboardType="numeric"
            value={String(config.nodeId)}
            onChangeText={(v) => setConfig({ ...config, nodeId: parseInt(v, 10) || 1 })}
            placeholderTextColor="#888"
          />
          <Text style={styles.label}>Mesh group (0–3)</Text>
          <TextInput
            style={styles.input}
            keyboardType="numeric"
            value={String(config.meshGroup)}
            onChangeText={(v) => setConfig({ ...config, meshGroup: parseInt(v, 10) || 0 })}
            placeholderTextColor="#888"
          />
          <Text style={styles.label}>Radio band</Text>
          <RadioButton.Group
            onValueChange={(v) => setConfig({ ...config, radioBand: v as RadioBand })}
            value={config.radioBand}
          >
            <View style={styles.row}>
              <Text style={styles.radio}>EU 868 MHz</Text>
              <RadioButton value="eu868" color="#0d6efd" />
              <Text style={styles.radio}>US 915 MHz</Text>
              <RadioButton value="us915" color="#0d6efd" />
            </View>
          </RadioButton.Group>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title>Step 3 — Seismic Trigger</Title>
          <Text style={styles.label}>STA/LTA threshold ({config.staLtaThreshold.toFixed(1)})</Text>
          <TextInput
            style={styles.input}
            keyboardType="decimal-pad"
            value={String(config.staLtaThreshold)}
            onChangeText={(v) => setConfig({ ...config, staLtaThreshold: parseFloat(v) || 3.0 })}
            placeholderTextColor="#888"
          />
          <Paragraph style={styles.hint}>
            Lower → more sensitive (more false triggers).{'\n'}
            Typical: 2.5 for quiet ice sheets, 4.0 for noisy calving fronts.
          </Paragraph>
          <Text style={styles.label}>Templates loaded: {config.templateCount}</Text>
          <Paragraph style={styles.hint}>
            Templates (basal slide, crevasse snap, calving thump) are pushed from
            the Config Sync screen. Seed templates are pre-loaded in firmware.
          </Paragraph>
        </Card.Content>
      </Card>

      <Button
        title={applying ? 'Applying…' : 'Apply & Deploy'}
        onPress={applyConfig}
        color="#28a745"
        disabled={!connected || applying}
      />
      {applying && <ActivityIndicator color="#28a745" />}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#0b1220' },
  card: { marginBottom: 12, backgroundColor: '#141b2d' },
  input: {
    backgroundColor: '#1c2540', color: '#e6eaf2',
    borderRadius: 6, paddingHorizontal: 12, paddingVertical: 8, marginVertical: 6,
  },
  row: { flexDirection: 'row', alignItems: 'center', gap: 12, marginVertical: 8 },
  radio: { color: '#e6eaf2', fontSize: 14, marginRight: 4 },
  label: { color: '#9fb0d0', fontSize: 13, marginTop: 8 },
  hint: { color: '#7a8aa8', fontSize: 12, marginTop: 4 },
  div: { marginVertical: 8, backgroundColor: '#2a3556' },
  ok: { color: '#28a745', marginTop: 8 },
});