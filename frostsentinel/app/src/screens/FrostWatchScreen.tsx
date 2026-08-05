// src/screens/FrostWatchScreen.tsx — Active frost watch alert view
//
// During an active frost watch, this screen shows the countdown,
// recommended mitigation action, and acknowledge/silence button.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Vibration, Alert,
} from 'react-native';
import bleManager from '../ble/BleManager';
import { rfriColor, aeStatusLabel, AE_STATUS_NUCLEATION } from '../ble/protocol';
import type { LiveData } from '../ble/BleManager';

export default function FrostWatchScreen() {
  const [live, setLive] = useState<LiveData | null>(null);
  const [acknowledged, setAcknowledged] = useState(false);
  const [watchActive, setWatchActive] = useState(false);

  useEffect(() => {
    const unsub = bleManager.on('liveData', (data: LiveData) => {
      setLive(data);
      const isCritical = data.rfri >= 0.85 || data.aeStatus === AE_STATUS_NUCLEATION;
      if (isCritical && !watchActive) {
        setWatchActive(true);
        setAcknowledged(false);
        Vibration.vibrate([500, 200, 500, 200, 500]);
      }
    });
    return unsub;
  }, [watchActive]);

  const startWatch = async () => {
    try {
      await bleManager.startFrostWatch();
      setWatchActive(true);
      setAcknowledged(false);
    } catch (e) {
      Alert.alert('Error', 'Could not start frost watch. Is BLE connected?');
    }
  };

  const stopWatch = async () => {
    try {
      await bleManager.stopFrostWatch();
      setWatchActive(false);
    } catch (e) {
      Alert.alert('Error', 'Could not stop frost watch.');
    }
  };

  const acknowledge = () => {
    setAcknowledged(true);
    Vibration.cancel();
  };

  if (!watchActive) {
    return (
      <View style={styles.container}>
        <View style={styles.idleCard}>
          <Text style={styles.idleTitle}>No Active Frost Watch</Text>
          <Text style={styles.idleText}>
            All nodes are within safe range. You can manually start a frost
            watch to increase sampling to 1-minute intervals and arm the
            acoustic emission detector.
          </Text>
          <TouchableOpacity style={styles.startButton} onPress={startWatch}>
            <Text style={styles.buttonText}>Start Frost Watch</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  const rfri = live?.rfri ?? 0;
  const twet = live?.twetC ?? 0;
  const aeStatus = live?.aeStatus ?? 0;
  const isNucleation = aeStatus === AE_STATUS_NUCLEATION;
  const color = rfriColor(rfri);

  // Recommended mitigation
  const mitigation = isNucleation
    ? 'ICE CONFIRMED. Run wind machines / helicopters NOW. Do NOT stop until T_wet > +1°C.'
    : rfri >= 0.85
    ? 'CRITICAL: Start wind machines or helicopters immediately. Consider sprinkler protection.'
    : rfri >= 0.60
    ? 'ELEVATED: Stage equipment. Monitor closely. Be ready to act within 30 minutes.'
    : 'MONITOR: Conditions developing. Check every 15 minutes.';

  return (
    <View style={[styles.container, { backgroundColor: isNucleation ? '#3d0000' : '#0d1b2a' }]}>
      <View style={[styles.alertCard, { borderLeftColor: color }]}>
        <Text style={styles.alertTitle}>
          {isNucleation ? '🧊 ICE NUCLEATION CONFIRMED' : '⚠ FROST WATCH ACTIVE'}
        </Text>
        <Text style={styles.alertNode}>Node {live?.nodeId ?? '?'}</Text>

        <View style={styles.metricRow}>
          <WatchMetric label="RFRI" value={`${(rfri * 100).toFixed(0)}%`} color={color} />
          <WatchMetric label="T_wet" value={`${twet.toFixed(1)}°C`}
                       color={twet <= 0 ? '#F44336' : '#fff'} />
          <WatchMetric label="AE" value={aeStatusLabel(aeStatus)}
                       color={isNucleation ? '#F44336' : '#FFC107'} />
        </View>

        <View style={styles.mitigationBox}>
          <Text style={styles.mitigationLabel}>Recommended Action:</Text>
          <Text style={styles.mitigationText}>{mitigation}</Text>
        </View>

        {isNucleation && (
          <View style={styles.aeBox}>
            <Text style={styles.aeText}>
              Acoustic emission has confirmed actual ice nucleation on the
              sensor surface. The cumulative AE energy indicates the severity
              of the freezing event.
            </Text>
          </View>
        )}

        <View style={styles.buttonRow}>
          {!acknowledged ? (
            <TouchableOpacity style={styles.ackButton} onPress={acknowledge}>
              <Text style={styles.buttonText}>Acknowledge & Silence Alarm</Text>
            </TouchableOpacity>
          ) : (
            <Text style={styles.acknowledgedText}>✓ Alarm acknowledged</Text>
          )}
          <TouchableOpacity style={styles.stopButton} onPress={stopWatch}>
            <Text style={styles.buttonText}>End Frost Watch</Text>
          </TouchableOpacity>
        </View>
      </View>
    </View>
  );
}

function WatchMetric({ label, value, color }: { label: string; value: string; color: string }) {
  return (
    <View style={styles.metric}>
      <Text style={styles.metricLabel}>{label}</Text>
      <Text style={[styles.metricValue, { color }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1b2a', justifyContent: 'center', padding: 20 },
  idleCard: { backgroundColor: '#1b263b', borderRadius: 12, padding: 25, alignItems: 'center' },
  idleTitle: { fontSize: 18, fontWeight: 'bold', color: '#4CAF50', marginBottom: 10 },
  idleText: { fontSize: 14, color: '#778da9', textAlign: 'center', marginBottom: 20, lineHeight: 20 },
  startButton: {
    backgroundColor: '#2196F3', paddingHorizontal: 24, paddingVertical: 12,
    borderRadius: 8,
  },
  alertCard: {
    backgroundColor: '#1b263b', borderRadius: 12, padding: 20,
    borderLeftWidth: 5,
  },
  alertTitle: { fontSize: 20, fontWeight: 'bold', color: '#F44336', marginBottom: 4 },
  alertNode: { fontSize: 14, color: '#778da9', marginBottom: 15 },
  metricRow: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 15 },
  metric: { alignItems: 'center' },
  metricLabel: { fontSize: 11, color: '#778da9' },
  metricValue: { fontSize: 18, fontWeight: 'bold', marginTop: 2 },
  mitigationBox: {
    backgroundColor: 'rgba(255,152,0,0.1)', borderRadius: 8, padding: 12, marginBottom: 12,
  },
  mitigationLabel: { fontSize: 12, color: '#FF9800', fontWeight: '600', marginBottom: 4 },
  mitigationText: { fontSize: 14, color: '#e0e1dd', lineHeight: 18 },
  aeBox: {
    backgroundColor: 'rgba(244,67,54,0.15)', borderRadius: 8, padding: 12, marginBottom: 12,
  },
  aeText: { fontSize: 12, color: '#F44336', lineHeight: 16 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-between' },
  ackButton: {
    backgroundColor: '#FF9800', paddingHorizontal: 16, paddingVertical: 10,
    borderRadius: 8, flex: 1, marginRight: 8, alignItems: 'center',
  },
  stopButton: {
    backgroundColor: '#4CAF50', paddingHorizontal: 16, paddingVertical: 10,
    borderRadius: 8, flex: 1, alignItems: 'center',
  },
  buttonText: { color: '#fff', fontSize: 13, fontWeight: '600' },
  acknowledgedText: { color: '#4CAF50', fontSize: 13, flex: 1, marginRight: 8, textAlign: 'center',
                      paddingTop: 10 },
});