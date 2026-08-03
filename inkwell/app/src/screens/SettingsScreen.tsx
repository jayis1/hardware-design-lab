// SettingsScreen.tsx — App settings
//
// Toggle handwriting recognition (opt-in, on-device TFLite or cloud), toggle
// signature dynamics signing, configure BLE connection parameters, and show
// the about / credit panel.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import { View, Text, Switch, StyleSheet, ScrollView, Linking } from 'react-native';

export default function SettingsScreen() {
  const [recognize, setRecognize] = useState(false);
  const [signSigs, setSignSigs] = useState(false);
  const [darkBg, setDarkBg] = useState(true);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.section}>Recognition</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Handwriting recognition</Text>
        <Switch value={recognize} onValueChange={setRecognize} />
      </View>
      <Text style={styles.hint}>When enabled, recognized text is stored as an
        annotation on the page; the original strokes are always preserved.</Text>

      <Text style={styles.section}>Forensics</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Cryptographically sign signatures</Text>
        <Switch value={signSigs} onValueChange={setSignSigs} />
      </View>
      <Text style={styles.hint}>Records the full stroke dynamics (timing,
        pressure, velocity) and signs the record for notary-grade evidence.</Text>

      <Text style={styles.section}>Appearance</Text>
      <View style={styles.row}>
        <Text style={styles.label}>Dark canvas background</Text>
        <Switch value={darkBg} onValueChange={setDarkBg} />
      </View>

      <Text style={styles.section}>About</Text>
      <Text style={styles.about}>Inkwell v1.0.0</Text>
      <Text style={styles.about}>Author: jayis1</Text>
      <Text style={styles.about}>© 2026 jayis1 — CERN-OHL-S v2 / GPL-3.0 / MIT</Text>
      <Text style={styles.link}
            onPress={() => Linking.openURL('https://github.com/jayis1/inkwell')}>
        github.com/jayis1/inkwell
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f9f7f1' },
  section:   { fontSize: 13, fontWeight: '700', color: '#1a1a2e',
               marginTop: 20, marginLeft: 16, marginBottom: 4,
               textTransform: 'uppercase', letterSpacing: 1 },
  row:       { flexDirection: 'row', justifyContent: 'space-between',
               alignItems: 'center', paddingVertical: 8, paddingHorizontal: 16 },
  label:     { fontSize: 15, color: '#222' },
  hint:      { fontSize: 12, color: '#777', paddingHorizontal: 16, marginTop: 2 },
  about:     { fontSize: 13, color: '#444', paddingHorizontal: 16, paddingVertical: 2 },
  link:      { fontSize: 13, color: '#0066cc', paddingHorizontal: 16, paddingVertical: 4 },
});