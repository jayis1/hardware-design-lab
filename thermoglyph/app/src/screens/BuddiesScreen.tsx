/**
 * BuddiesScreen — LoRa buddy management and messaging
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert, RefreshControl } from 'react-native';
import { useDeviceStore } from '../store/deviceStore';
import type { GlyphCommand } from '../types';

export const BuddiesScreen: React.FC = () => {
  const { connected, buddies, refreshBuddies, sendGlyph } = useDeviceStore();
  const [refreshing, setRefreshing] = React.useState(false);

  useEffect(() => {
    refreshBuddies();
  }, []);

  const onRefresh = async () => {
    setRefreshing(true);
    refreshBuddies();
    setRefreshing(false);
  };

  const sendToBuddy = (buddyName: string, buddyId: string, cmd: GlyphCommand) => {
    Alert.alert(
      `Send to ${buddyName}`,
      `Send "${cmd.type}" via LoRa?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Send', onPress: () => {
          // In production, uses ThermoGlyphSDK.sendToBuddy()
          sendGlyph(cmd);
        }},
      ]
    );
  };

  return (
    <ScrollView
      style={styles.screen}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
    >
      <View style={styles.header}>
        <Text style={styles.title}>Buddies</Text>
        <Text style={styles.subtitle}>LoRa long-range communication</Text>
      </View>

      <View style={styles.infoCard}>
        <Text style={styles.infoText}>
          Send thermal glyphs to other ThermoGlyph devices via LoRa (up to 5 km range).
          All messages are AES-128 encrypted.
        </Text>
      </View>

      <Text style={styles.sectionTitle}>Nearby Devices ({buddies.length})</Text>
      {buddies.map(buddy => (
        <View key={buddy.id} style={styles.buddyRow}>
          <View style={styles.buddyInfo}>
            <View style={styles.buddyHeader}>
              <Text style={styles.buddyName}>{buddy.name}</Text>
              <View style={[styles.onlineDot, { backgroundColor: buddy.online ? '#4a4' : '#666' }]} />
            </View>
            <Text style={styles.buddyDetails}>
              ID: {buddy.id} · RSSI: {buddy.lastRssi} dBm
            </Text>
            <Text style={styles.buddyLastSeen}>
              Last seen: {buddy.lastSeen.toLocaleTimeString()}
            </Text>
          </View>
        </View>
      ))}

      <Text style={styles.sectionTitle}>Quick Send</Text>
      {buddies.filter(b => b.online).map(buddy => (
        <View key={`send-${buddy.id}`} style={styles.sendRow}>
          <Text style={styles.sendLabel}>{buddy.name}</Text>
          <View style={styles.sendBtns}>
            <TouchableOpacity style={styles.sendBtn} onPress={() => sendToBuddy(buddy.name, buddy.id, { type: 'arrow', direction: 'north', polarity: 'warm', intensity: 160, durationMs: 3000 })}>
              <Text style={styles.sendBtnText}>↑</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.sendBtn} onPress={() => sendToBuddy(buddy.name, buddy.id, { type: 'arrow', direction: 'east', polarity: 'warm', intensity: 160, durationMs: 3000 })}>
              <Text style={styles.sendBtnText}>→</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.sendBtn} onPress={() => sendToBuddy(buddy.name, buddy.id, { type: 'shape', shape: 'check', polarity: 'warm', intensity: 200, durationMs: 3000 })}>
              <Text style={styles.sendBtnText}>✓</Text>
            </TouchableOpacity>
            <TouchableOpacity style={[styles.sendBtn, styles.sosBtn]} onPress={() => sendToBuddy(buddy.name, buddy.id, { type: 'anim', shape: 'ring', polarity: 'warm', intensity: 255, durationMs: 10000 })}>
              <Text style={styles.sendBtnText}>SOS</Text>
            </TouchableOpacity>
          </View>
        </View>
      ))}

      {!connected && (
        <Text style={styles.warning}>Connect to device to send LoRa messages</Text>
      )}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 13, marginTop: 4 },
  infoCard: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 16, marginHorizontal: 12, marginVertical: 8 },
  infoText: { color: '#aaa', fontSize: 13, lineHeight: 20 },
  sectionTitle: { color: '#ccc', fontSize: 15, fontWeight: '600', paddingHorizontal: 16, marginTop: 16, marginBottom: 8 },
  buddyRow: { flexDirection: 'row', backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginHorizontal: 8, marginVertical: 3 },
  buddyInfo: { flex: 1 },
  buddyHeader: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  buddyName: { color: '#fff', fontSize: 15, fontWeight: '500' },
  onlineDot: { width: 8, height: 8, borderRadius: 4 },
  buddyDetails: { color: '#888', fontSize: 12, marginTop: 4, fontFamily: 'monospace' },
  buddyLastSeen: { color: '#666', fontSize: 11, marginTop: 2 },
  sendRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginHorizontal: 8, marginVertical: 3 },
  sendLabel: { color: '#fff', fontSize: 14, flex: 1 },
  sendBtns: { flexDirection: 'row', gap: 6 },
  sendBtn: { width: 36, height: 36, borderRadius: 8, backgroundColor: '#333', justifyContent: 'center', alignItems: 'center' },
  sosBtn: { backgroundColor: '#a33', width: 50 },
  sendBtnText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  warning: { color: '#a66', fontSize: 13, textAlign: 'center', paddingVertical: 20 },
});