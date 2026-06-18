/**
 * OnboardingScreen.tsx - BLE pairing & station provisioning
 *
 * Scans for nearby Pollen Scouts over BLE, lets the user pick one,
 * then provisions it with WiFi + LoRaWAN credentials.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TextInput, TouchableOpacity,
  FlatList, ActivityIndicator,
} from 'react-native';
import { useStation } from '../context/StationContext';

type Step = 'scan' | 'provision' | 'done';

export default function OnboardingScreen() {
  const { scanning, discovered, startScan, connectLocal, provision } = useStation();
  const [step, setStep] = useState<Step>('scan');
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [wifiSsid, setWifiSsid] = useState('');
  const [wifiPw, setWifiPw] = useState('');
  const [joinEui, setJoinEui] = useState('');
  const [appKey, setAppKey] = useState('');
  const [busy, setBusy] = useState(false);

  const onPick = async (id: string) => {
    setSelectedId(id);
    setBusy(true);
    try {
      await connectLocal(discovered.find((d) => d.id === id)!);
      setStep('provision');
    } finally {
      setBusy(false);
    }
  };

  const onProvision = async () => {
    setBusy(true);
    try {
      await provision(wifiSsid, wifiPw, joinEui, appKey);
      setStep('done');
    } finally {
      setBusy(false);
    }
  };

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Pair a Pollen Scout</Text>

      {step === 'scan' && (
        <>
          <TouchableOpacity style={s.btn} onPress={startScan}>
            <Text style={s.btnText}>
              {scanning ? 'Scanning…' : 'Start BLE scan'}
            </Text>
          </TouchableOpacity>
          {scanning && <ActivityIndicator style={{ margin: 12 }} />}
          <FlatList
            data={discovered}
            keyExtractor={(d) => d.id}
            renderItem={({ item }) => (
              <TouchableOpacity
                style={s.devRow}
                onPress={() => onPick(item.id)}
              >
                <View>
                  <Text style={s.devName}>{item.name}</Text>
                  <Text style={s.devId}>{item.id}</Text>
                </View>
                <Text style={s.rssi}>{item.rssi} dBm</Text>
              </TouchableOpacity>
            )}
            ListEmptyComponent={
              <Text style={s.empty}>No devices yet — tap “Start BLE scan”.</Text>
            }
            style={{ marginTop: 8 }}
          />
        </>
      )}

      {step === 'provision' && (
        <>
          <Text style={s.sectionTitle}>
            Provision {selectedId}
          </Text>
          <Text style={s.label}>WiFi SSID</Text>
          <TextInput style={s.input} value={wifiSsid} onChangeText={setWifiSsid}
                     placeholder="MyHomeWiFi" autoCapitalize="none" />
          <Text style={s.label}>WiFi Password</Text>
          <TextInput style={s.input} value={wifiPw} onChangeText={setWifiPw}
                     placeholder="••••••••" secureTextEntry autoCapitalize="none" />
          <Text style={s.label}>LoRaWAN JoinEUI</Text>
          <TextInput style={s.input} value={joinEui} onChangeText={setJoinEui}
                     placeholder="0000000000000000" autoCapitalize="none" />
          <Text style={s.label}>LoRaWAN AppKey (16 bytes hex)</Text>
          <TextInput style={s.input} value={appKey} onChangeText={setAppKey}
                     placeholder="00112233445566778899aabbccddeeff" autoCapitalize="none" />
          <TouchableOpacity style={s.btn} onPress={onProvision} disabled={busy}>
            <Text style={s.btnText}>{busy ? 'Provisioning…' : 'Provision'}</Text>
          </TouchableOpacity>
        </>
      )}

      {step === 'done' && (
        <View style={s.doneBox}>
          <Text style={s.doneTitle}>✓ Paired & provisioned</Text>
          <Text style={s.doneSub}>
            Your station is now online. Visit the Dashboard to see live data.
          </Text>
          <TouchableOpacity style={s.btn} onPress={() => setStep('scan')}>
            <Text style={s.btnText}>Pair another</Text>
          </TouchableOpacity>
        </View>
      )}
      <Text style={s.footer}>Pollen Scout · jayis1 · MIT</Text>
    </ScrollView>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 20, fontWeight: '800', color: '#2E7D32', margin: 16 },
  btn: { backgroundColor: '#2E7D32', margin: 12, padding: 14, borderRadius: 10, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: '700' },
  devRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', backgroundColor: '#fff', marginHorizontal: 12, marginVertical: 4, padding: 12, borderRadius: 8, elevation: 1 },
  devName: { fontSize: 14, fontWeight: '700', color: '#333' },
  devId:   { fontSize: 11, color: '#aaa' },
  rssi:    { fontSize: 13, color: '#2E7D32', fontWeight: '700' },
  empty:   { textAlign: 'center', color: '#999', margin: 20, fontSize: 12 },
  sectionTitle: { fontSize: 15, fontWeight: '700', color: '#333', margin: 12 },
  label: { fontSize: 12, color: '#666', marginHorizontal: 16, marginTop: 8 },
  input: { borderWidth: 1, borderColor: '#ddd', borderRadius: 8, marginHorizontal: 16, marginTop: 2, paddingHorizontal: 10, paddingVertical: 6, fontSize: 14, backgroundColor: '#fff' },
  doneBox: { alignItems: 'center', margin: 24, padding: 20, backgroundColor: '#E8F5E9', borderRadius: 12 },
  doneTitle: { fontSize: 18, fontWeight: '800', color: '#2E7D32' },
  doneSub: { fontSize: 13, color: '#555', textAlign: 'center', marginTop: 6 },
  footer: { textAlign: 'center', color: '#999', fontSize: 11, margin: 16 },
});