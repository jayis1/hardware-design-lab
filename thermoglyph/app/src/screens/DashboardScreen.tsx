/**
 * DashboardScreen — Main device overview
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert } from 'react-native';
import { useDeviceStore } from '../store/deviceStore';
import { ThermalPreview } from '../components/ThermalPreview';
import { BatteryWidget } from '../components/BatteryWidget';

export const DashboardScreen: React.FC = () => {
  const {
    connected, connecting, connect, disconnect,
    telemetry, thermalFrame, lastGesture,
    sendArrow, sendSOS, clearGlyph, currentGlyph,
    templates, sendGlyph,
  } = useDeviceStore();

  useEffect(() => {
    if (!connected && !connecting) {
      connect().catch(() => {});
    }
  }, []);

  const handleConnect = async () => {
    try {
      await connect();
    } catch (e: any) {
      Alert.alert('Connection Failed', e?.message ?? 'Could not connect to ThermoGlyph');
    }
  };

  return (
    <ScrollView style={styles.screen}>
      <View style={styles.header}>
        <Text style={styles.title}>ThermoGlyph</Text>
        <Text style={styles.author}>by jayis1</Text>
        <View style={[styles.statusDot,
          { backgroundColor: connected ? '#4a4' : connecting ? '#aa4' : '#a44' }]} />
        <Text style={styles.statusText}>
          {connected ? 'Connected' : connecting ? 'Connecting...' : 'Disconnected'}
        </Text>
      </View>

      {!connected ? (
        <View style={styles.centerContent}>
          <Text style={styles.hint}>Connect to your ThermoGlyph device</Text>
          <TouchableOpacity style={styles.connectBtn} onPress={handleConnect}>
            <Text style={styles.connectBtnText}>Connect</Text>
          </TouchableOpacity>
        </View>
      ) : (
        <>
          {/* Thermal Array Preview */}
          <Text style={styles.sectionTitle}>Live Thermal Array</Text>
          <ThermalPreview frame={thermalFrame} />

          {/* Current Glyph */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Current Glyph</Text>
            <Text style={styles.cardValue}>
              {currentGlyph ? `${currentGlyph.type}${currentGlyph.direction ? ` (${currentGlyph.direction})` : ''}` : 'None'}
            </Text>
            <Text style={styles.cardSub}>Last gesture: {lastGesture}</Text>
            <TouchableOpacity style={styles.clearBtn} onPress={clearGlyph}>
              <Text style={styles.clearBtnText}>Clear Glyph</Text>
            </TouchableOpacity>
          </View>

          {/* Telemetry */}
          <Text style={styles.sectionTitle}>Telemetry</Text>
          <BatteryWidget telemetry={telemetry} />

          {/* Quick Actions */}
          <Text style={styles.sectionTitle}>Quick Actions</Text>
          <View style={styles.quickActions}>
            <TouchableOpacity style={styles.actionCard} onPress={() => sendArrow('north')}>
              <Text style={styles.actionIcon}>↑</Text>
              <Text style={styles.actionLabel}>North</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.actionCard} onPress={() => sendArrow('east')}>
              <Text style={styles.actionIcon}>→</Text>
              <Text style={styles.actionLabel}>East</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.actionCard} onPress={() => sendArrow('south')}>
              <Text style={styles.actionIcon}>↓</Text>
              <Text style={styles.actionLabel}>South</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.actionCard} onPress={() => sendArrow('west')}>
              <Text style={styles.actionIcon}>←</Text>
              <Text style={styles.actionLabel}>West</Text>
            </TouchableOpacity>
          </View>

          {/* SOS */}
          <TouchableOpacity style={styles.sosBtn} onPress={() => {
            Alert.alert('SOS', 'Send emergency SOS via LoRa?', [
              { text: 'Cancel', style: 'cancel' },
              { text: 'Send', style: 'destructive', onPress: () => sendSOS() },
            ]);
          }}>
            <Text style={styles.sosBtnText}>EMERGENCY SOS</Text>
          </TouchableOpacity>

          {/* Templates */}
          <Text style={styles.sectionTitle}>Saved Glyphs</Text>
          {templates.map(tpl => (
            <TouchableOpacity
              key={tpl.id}
              style={styles.templateRow}
              onPress={() => sendGlyph(tpl.command)}
            >
              <Text style={styles.templateName}>{tpl.name}</Text>
              <Text style={styles.templateType}>{tpl.command.type}</Text>
            </TouchableOpacity>
          ))}

          {/* Disconnect */}
          <TouchableOpacity style={styles.disconnectBtn} onPress={disconnect}>
            <Text style={styles.disconnectBtnText}>Disconnect</Text>
          </TouchableOpacity>
        </>
      )}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 28, fontWeight: 'bold' },
  author: { color: '#888', fontSize: 13, marginTop: 2 },
  statusDot: { width: 10, height: 10, borderRadius: 5, marginTop: 12 },
  statusText: { color: '#aaa', fontSize: 13, marginTop: 4 },
  centerContent: { alignItems: 'center', paddingVertical: 60 },
  hint: { color: '#aaa', fontSize: 15, marginBottom: 20 },
  connectBtn: { backgroundColor: '#2a6', paddingHorizontal: 32, paddingVertical: 14, borderRadius: 12 },
  connectBtnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
  sectionTitle: { color: '#ccc', fontSize: 15, fontWeight: '600', paddingHorizontal: 16, marginTop: 16, marginBottom: 4 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, marginHorizontal: 8, marginVertical: 4 },
  cardTitle: { color: '#888', fontSize: 12 },
  cardValue: { color: '#fff', fontSize: 18, fontWeight: '600', marginTop: 4 },
  cardSub: { color: '#aaa', fontSize: 12, marginTop: 4 },
  clearBtn: { marginTop: 12, backgroundColor: '#444', paddingVertical: 8, borderRadius: 8, alignItems: 'center' },
  clearBtnText: { color: '#fff', fontSize: 13 },
  quickActions: { flexDirection: 'row', justifyContent: 'space-around', paddingHorizontal: 8 },
  actionCard: { backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, alignItems: 'center', flex: 1, marginHorizontal: 4 },
  actionIcon: { fontSize: 28, color: '#fa4' },
  actionLabel: { color: '#aaa', fontSize: 12, marginTop: 4 },
  sosBtn: { backgroundColor: '#a33', paddingVertical: 16, borderRadius: 12, marginHorizontal: 16, marginTop: 16, alignItems: 'center' },
  sosBtnText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  templateRow: { flexDirection: 'row', justifyContent: 'space-between', backgroundColor: '#1a1a2e', borderRadius: 8, padding: 14, marginHorizontal: 8, marginVertical: 2 },
  templateName: { color: '#fff', fontSize: 14 },
  templateType: { color: '#888', fontSize: 12 },
  disconnectBtn: { backgroundColor: '#333', paddingVertical: 12, borderRadius: 8, marginHorizontal: 16, marginVertical: 24, alignItems: 'center' },
  disconnectBtnText: { color: '#a66', fontSize: 14 },
});