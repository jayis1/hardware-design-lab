// CrisperCue settings screen
// Author: jayis1
import React from 'react';
import { StyleSheet, Switch, Text, View } from 'react-native';

export default function SettingsScreen({ fanBoost, setFanBoost, notifications, setNotifications, commandPacket }) {
  return (
    <View>
      <Text style={styles.heading}>Settings and control</Text>
      <View style={styles.row}>
        <View>
          <Text style={styles.label}>Adaptive purge fan boost</Text>
          <Text style={styles.body}>Open the louver sooner when ethylene spikes.</Text>
        </View>
        <Switch value={fanBoost} onValueChange={setFanBoost} />
      </View>
      <View style={styles.row}>
        <View>
          <Text style={styles.label}>Push notifications</Text>
          <Text style={styles.body}>Send alerts when a drawer crosses into rescue mode.</Text>
        </View>
        <Switch value={notifications} onValueChange={setNotifications} />
      </View>
      <View style={styles.packetCard}>
        <Text style={styles.label}>Live command payload</Text>
        <Text style={styles.packet}>{commandPacket}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  heading: { color: '#F6FBFF', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  row: {
    backgroundColor: '#10212D',
    borderRadius: 16,
    padding: 14,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#1A3342',
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  label: { color: '#F6FBFF', fontWeight: '700', marginBottom: 4 },
  body: { color: '#9EC5D1', maxWidth: 240 },
  packetCard: { backgroundColor: '#0F1E29', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#1A3342' },
  packet: { color: '#E0F7FA', fontFamily: 'monospace', marginTop: 8 },
});
