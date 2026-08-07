/**
 * BatchSetupScreen.js — New Batch Configuration Wizard
 *
 * Lets the user configure a new fermentation batch: select type,
 * enter batch name, set vessel volume, and define alarm thresholds.
 * Pushes the configuration to the device over BLE.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useContext, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert } from 'react-native';
import { Card, Title, TextInput, Button, List, Divider, Switch, HelperText } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { FermenTiqContext } from '../utils/ble';
import { FERM_TYPES, FERM_TYPE_LABELS } from '../utils/protocol';

export default function BatchSetupScreen() {
  const { connectionState, updateConfig, sendCommand } = useContext(FermenTiqContext);
  const existing = connectionState.config || {};

  const [batchName, setBatchName] = useState(existing.batchName || 'Batch #' + Date.now() % 1000);
  const [fermType, setFermType] = useState(existing.type || FERM_TYPES.BEER);
  const [vesselVolume, setVesselVolume] = useState(String(existing.vesselVolume || 19));
  const [tempMin, setTempMin] = useState(String(existing.tempMin || 15));
  const [tempMax, setTempMax] = useState(String(existing.tempMax || 25));
  const [phMin, setPhMin] = useState(String(existing.phMin || 3.0));
  const [phMax, setPhMax] = useState(String(existing.phMax || 7.0));
  const [active, setActive] = useState(existing.active || false);
  const [saving, setSaving] = useState(false);

  const fermTypeItems = Object.entries(FERM_TYPE_LABELS).map(([key, label]) => ({
    key: parseInt(key),
    label,
    icon: getFermIcon(parseInt(key)),
  }));

  function getFermIcon(type) {
    const icons = {
      0: 'beer', 1: 'glass-wine', 2: 'glass-cocktail', 3: 'tea',
      4: 'cup', 5: 'cup-water', 6: 'bowl-mix', 7: 'bowl-mix',
      8: 'bread-slice', 9: 'flask',
    };
    return icons[type] || 'flask';
  }

  const handleSave = async () => {
    if (!batchName.trim()) {
      Alert.alert('Error', 'Please enter a batch name');
      return;
    }

    const vol = parseFloat(vesselVolume);
    if (isNaN(vol) || vol <= 0 || vol > 1000) {
      Alert.alert('Error', 'Vessel volume must be between 0.1 and 1000 L');
      return;
    }

    setSaving(true);
    try {
      await updateConfig({
        type: fermType,
        batchName: batchName.trim(),
        vesselVolume: vol,
        tempMin: parseFloat(tempMin),
        tempMax: parseFloat(tempMax),
        phMin: parseFloat(phMin),
        phMax: parseFloat(phMax),
        active: true,
      });
      setActive(true);
      Alert.alert('Batch Started', `"${batchName}" is now being monitored.`);
    } catch (e) {
      Alert.alert('Error', 'Failed to save batch config: ' + e.message);
    }
    setSaving(false);
  };

  const handleStop = async () => {
    try {
      await updateConfig({ active: false });
      setActive(false);
      Alert.alert('Batch Stopped', 'Monitoring has been stopped.');
    } catch (e) {
      Alert.alert('Error', 'Failed to stop: ' + e.message);
    }
  };

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Batch Configuration</Title>

          <TextInput
            label="Batch Name"
            value={batchName}
            onChangeText={setBatchName}
            style={styles.input}
            theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
            maxLength={30}
          />

          <Title style={styles.subtitle}>Fermentation Type</Title>
          <View style={styles.typeGrid}>
            {fermTypeItems.map(item => (
              <List.Item
                key={item.key}
                title={item.label}
                left={props => <Icon name={item.icon} size={24} color="#D4A017" />}
                right={props => fermType === item.key ?
                  <Icon name="check-circle" size={24} color="#4CAF50" /> : null}
                onPress={() => setFermType(item.key)}
                style={[styles.typeItem, fermType === item.key && styles.typeItemSelected]}
                titleStyle={styles.typeLabel}
              />
            ))}
          </View>

          <Divider style={styles.divider} />

          <Title style={styles.subtitle}>Vessel</Title>
          <TextInput
            label="Vessel Volume (L)"
            value={vesselVolume}
            onChangeText={setVesselVolume}
            keyboardType="numeric"
            style={styles.input}
            theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
          />

          <Divider style={styles.divider} />

          <Title style={styles.subtitle}>Alarm Thresholds</Title>
          <View style={styles.thresholdRow}>
            <TextInput
              label="Temp Min (°C)"
              value={tempMin}
              onChangeText={setTempMin}
              keyboardType="numeric"
              style={styles.halfInput}
              theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
            />
            <TextInput
              label="Temp Max (°C)"
              value={tempMax}
              onChangeText={setTempMax}
              keyboardType="numeric"
              style={styles.halfInput}
              theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
            />
          </View>
          <View style={styles.thresholdRow}>
            <TextInput
              label="pH Min"
              value={phMin}
              onChangeText={setPhMin}
              keyboardType="numeric"
              style={styles.halfInput}
              theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
            />
            <TextInput
              label="pH Max"
              value={phMax}
              onChangeText={setPhMax}
              keyboardType="numeric"
              style={styles.halfInput}
              theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
            />
          </View>

          <HelperText style={styles.helperText}>
            You'll receive a push notification when any threshold is exceeded.
          </HelperText>

          <Divider style={styles.divider} />

          <View style={styles.activeRow}>
            <Text style={styles.activeLabel}>Monitoring Active</Text>
            <Switch value={active} onValueChange={setActive}
              color="#D4A017" disabled={saving} />
          </View>

          <View style={styles.buttonRow}>
            <Button
              mode="contained"
              onPress={handleSave}
              style={styles.button}
              loading={saving}
              disabled={saving}
              icon="play"
            >
              Start Batch
            </Button>
            <Button
              mode="outlined"
              onPress={handleStop}
              style={styles.button}
              disabled={saving || !active}
              icon="stop"
              textColor="#E53935"
            >
              Stop
            </Button>
          </View>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e' },
  card: { margin: 12, backgroundColor: '#16213e' },
  title: { color: '#e0e0e0', fontSize: 22, marginBottom: 16 },
  subtitle: { color: '#D4A017', fontSize: 16, marginTop: 12, marginBottom: 8 },
  input: { backgroundColor: '#0f1626', marginBottom: 8 },
  halfInput: { flex: 1, backgroundColor: '#0f1626', marginHorizontal: 4 },
  typeGrid: { flexDirection: 'row', flexWrap: 'wrap' },
  typeItem: { width: '50%', backgroundColor: 'transparent' },
  typeItemSelected: { backgroundColor: 'rgba(212, 160, 23, 0.15)' },
  typeLabel: { color: '#e0e0e0', fontSize: 14 },
  divider: { backgroundColor: '#333', marginVertical: 12 },
  thresholdRow: { flexDirection: 'row', justifyContent: 'space-between' },
  activeRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginVertical: 12 },
  activeLabel: { color: '#e0e0e0', fontSize: 16 },
  helperText: { color: '#888', fontSize: 12 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 16 },
  button: { flex: 1, marginHorizontal: 8 },
});