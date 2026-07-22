/**
 * SurveySetupScreen — configure survey parameters
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React, { useState } from 'react';
import { View, Text, TextInput, Picker, Button, StyleSheet, Switch } from 'react-native';
import type { StackScreenProps } from '@react-navigation/stack';
import type { RootStackParamList, SurveyConfig } from '../../App';

type Props = StackScreenProps<RootStackParamList, 'SurveySetup'>;

export default function SurveySetupScreen({ route, navigation }: Props) {
  const [siteName, setSiteName] = useState(route.params?.siteName ?? 'New Site');
  const [gridRes, setGridRes] = useState(500);
  const [refElectrode, setRefElectrode] = useState<'CuCuSO4' | 'AgAgCl'>('CuCuSO4');
  const [hcp, setHcp] = useState(true);
  const [resistivity, setResistivity] = useState(true);
  const [cover, setCover] = useState(true);

  const startSurvey = () => {
    const config: SurveyConfig = {
      gridResMm: gridRes,
      refElectrode,
      modalityMask: (hcp ? 1 : 0) | (resistivity ? 2 : 0) | (cover ? 4 : 0),
      siteName,
    };
    navigation.navigate('Live', { config, siteName });
  };

  return (
    <View style={styles.container}>
      <Text style={styles.header}>Survey Setup</Text>

      <Text style={styles.label}>Site name</Text>
      <TextInput
        style={styles.input}
        value={siteName}
        onChangeText={setSiteName}
        placeholder="e.g., Bridge Deck 4A"
      />

      <Text style={styles.label}>Grid resolution (mm)</Text>
      <Picker
        selectedValue={gridRes}
        style={styles.picker}
        onValueChange={(v) => setGridRes(Number(v))}
      >
        <Picker.Item label="200 mm (fine)" value={200} />
        <Picker.Item label="500 mm (medium)" value={500} />
        <Picker.Item label="1000 mm (coarse)" value={1000} />
      </Picker>

      <Text style={styles.label}>Reference electrode</Text>
      <Picker
        selectedValue={refElectrode}
        style={styles.picker}
        onValueChange={(v) => setRefElectrode(v as 'CuCuSO4' | 'AgAgCl')}
      >
        <Picker.Item label="Cu/CuSO₄ (ASTM C876 default)" value="CuCuSO4" />
        <Picker.Item label="Ag/AgCl (+70 mV offset)" value="AgAgCl" />
      </Picker>

      <Text style={styles.label}>Modalities</Text>
      <View style={styles.toggleRow}>
        <Text>Half-cell potential</Text>
        <Switch value={hcp} onValueChange={setHcp} />
      </View>
      <View style={styles.toggleRow}>
        <Text>Resistivity (Wenner)</Text>
        <Switch value={resistivity} onValueChange={setResistivity} />
      </View>
      <View style={styles.toggleRow}>
        <Text>Cover depth (eddy)</Text>
        <Switch value={cover} onValueChange={setCover} />
      </View>

      <Button title="Start Survey →" onPress={startSurvey} color="#27ae60" />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 24, backgroundColor: '#1a1a2e' },
  header: { fontSize: 24, fontWeight: 'bold', color: '#ecf0f1', marginBottom: 16 },
  label: { fontSize: 14, color: '#95a5a6', marginTop: 12, marginBottom: 4 },
  input: {
    backgroundColor: '#2a2a4e',
    color: '#ecf0f1',
    padding: 10,
    borderRadius: 6,
  },
  picker: {
    backgroundColor: '#2a2a4e',
    color: '#ecf0f1',
    borderRadius: 6,
  },
  toggleRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
});