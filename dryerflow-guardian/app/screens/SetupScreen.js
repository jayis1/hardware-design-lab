/**
 * SetupScreen.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
import React from 'react';
import { ScrollView, View, Text, StyleSheet } from 'react-native';

const steps = [
  'Clamp DryerFlow Guardian to a straight 4-inch duct section at least 120 mm from the dryer outlet.',
  'Attach the two silicone tubes to the vent collar ports labelled upstream and downstream.',
  'Select dryer type: electric or gas. Gas mode enables CO-sensitive alert weighting.',
  'Run one empty warm cycle to capture fan and duct baseline.',
  'Run one typical laundry cycle to capture thermal and humidity baseline.',
  'Name the installation and store the learned envelope.'
];

export default function SetupScreen() {
  return (
    <ScrollView style={styles.screen} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Installation Wizard</Text>
      <Text style={styles.subtitle}>Commissioning workflow for DryerFlow Guardian • Author: jayis1</Text>
      {steps.map((step, index) => (
        <View key={step} style={styles.stepCard}>
          <Text style={styles.stepNumber}>{index + 1}</Text>
          <Text style={styles.stepText}>{step}</Text>
        </View>
      ))}
      <View style={styles.noteCard}>
        <Text style={styles.noteTitle}>Why baseline learning matters</Text>
        <Text style={styles.noteBody}>Vent geometry differs dramatically across homes. DryerFlow Guardian compares each future cycle against your own installed baseline instead of a generic factory threshold, improving obstruction sensitivity while reducing nuisance alerts.</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#030712' },
  content: { padding: 16, paddingBottom: 24 },
  title: { color: '#f8fafc', fontSize: 24, fontWeight: '700' },
  subtitle: { color: '#94a3b8', marginTop: 6, marginBottom: 16 },
  stepCard: { flexDirection: 'row', backgroundColor: '#111827', borderRadius: 16, padding: 14, marginBottom: 10 },
  stepNumber: { color: '#38bdf8', fontSize: 18, fontWeight: '700', width: 28 },
  stepText: { color: '#e5e7eb', flex: 1, lineHeight: 20 },
  noteCard: { backgroundColor: '#0f172a', borderRadius: 16, padding: 16, marginTop: 12 },
  noteTitle: { color: '#f8fafc', fontWeight: '700', marginBottom: 8 },
  noteBody: { color: '#cbd5e1', lineHeight: 20 }
});
