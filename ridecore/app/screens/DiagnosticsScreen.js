// ============================================================================
// screens/DiagnosticsScreen.js — Fault codes, bus monitor, raw data
// ============================================================================
import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, FlatList } from 'react-native';

const FAULT_NAMES = {
  0x01: 'Overvoltage (VBAT > 60V)',
  0x02: 'Undervoltage (VBAT < 22V)',
  0x04: 'Overcurrent (phase > limit)',
  0x08: 'Over Temperature',
  0x10: 'DESAT Fault (gate driver)',
  0x20: 'Motor Stall',
  0x40: 'Hardware Fault',
};

export default function DiagnosticsScreen() {
  const [faultHistory, setFaultHistory] = useState([
    { id: '1', time: '12:34:56', code: 0x04, msg: 'Overcurrent 142A > 120A limit', cleared: true },
    { id: '2', time: '12:33:01', code: 0x10, msg: 'DESAT fault on Phase B gate driver', cleared: true },
  ]);
  const [activeFaults, setActiveFaults] = useState(0);
  const [busVoltage, setBusVoltage] = useState(48.2);
  const [currentA, setCurrentA] = useState(12.5);
  const [currentB, setCurrentB] = useState(11.8);
  const [currentC, setCurrentC] = useState(12.1);
  const [mcuTemp, setMcuTemp] = useState(42);
  const [mosfetTemp, setMosfetTemp] = useState(58);

  const clearFaults = () => {
    setActiveFaults(0);
    // Send clear command over BLE
  };

  const decodeFaults = (code) => {
    const faults = [];
    for (const [bit, name] of Object.entries(FAULT_NAMES)) {
      if (code & parseInt(bit)) faults.push(name);
    }
    return faults;
  };

  const renderFaultItem = ({ item }) => (
    <View style={[styles.faultItem, item.cleared ? styles.faultCleared : styles.faultActive]}>
      <Text style={styles.faultTime}>{item.time}</Text>
      <Text style={styles.faultMsg}>{item.msg}</Text>
      <Text style={styles.faultCode}>0x{item.code.toString(16).toUpperCase()}</Text>
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Diagnostics</Text>
      </View>

      {/* Active Faults */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Active Faults</Text>
        {activeFaults === 0 ? (
          <Text style={styles.noFaults}>✓ No active faults</Text>
        ) : (
          <View>
            {decodeFaults(activeFaults).map((f, i) => (
              <Text key={i} style={styles.faultText}>⚠ {f}</Text>
            ))}
            <TouchableOpacity style={styles.clearBtn} onPress={clearFaults}>
              <Text style={styles.clearBtnText}>Clear Faults</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      {/* Phase Currents */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Phase Currents</Text>
        <View style={styles.phaseRow}>
          <View style={styles.phaseItem}>
            <Text style={styles.phaseLabel}>A</Text>
            <Text style={styles.phaseValue}>{currentA.toFixed(1)} A</Text>
          </View>
          <View style={styles.phaseItem}>
            <Text style={[styles.phaseLabel, { color: '#4CAF50' }]}>B</Text>
            <Text style={styles.phaseValue}>{currentB.toFixed(1)} A</Text>
          </View>
          <View style={styles.phaseItem}>
            <Text style={[styles.phaseLabel, { color: '#2196F3' }]}>C</Text>
            <Text style={styles.phaseValue}>{currentC.toFixed(1)} A</Text>
          </View>
        </View>
      </View>

      {/* Temperatures */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Temperatures</Text>
        <View style={styles.tempRow}>
          <View style={styles.tempItem}>
            <Text style={styles.tempLabel}>MCU</Text>
            <Text style={[styles.tempValue, { color: mcuTemp > 80 ? '#F44336' : '#4CAF50' }]}>
              {mcuTemp}°C
            </Text>
          </View>
          <View style={styles.tempItem}>
            <Text style={styles.tempLabel}>MOSFET</Text>
            <Text style={[styles.tempValue, { color: mosfetTemp > 100 ? '#F44336' : '#FF6B35' }]}>
              {mosfetTemp}°C
            </Text>
          </View>
        </View>
      </View>

      {/* Bus Voltage */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Bus Voltage</Text>
        <Text style={styles.busVoltage}>{busVoltage.toFixed(1)} V</Text>
        <View style={styles.voltageBar}>
          <View style={[styles.voltageFill, { width: `${(busVoltage / 72) * 100}%`, backgroundColor: busVoltage > 58 ? '#F44336' : '#4CAF50' }]} />
        </View>
      </View>

      {/* Fault History */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Fault History</Text>
        <FlatList
          data={faultHistory}
          renderItem={renderFaultItem}
          keyExtractor={item => item.id}
          scrollEnabled={false}
        />
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F23' },
  header: { padding: 20, alignItems: 'center' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#FF6B35' },
  card: { margin: 10, padding: 16, backgroundColor: '#1A1A2E', borderRadius: 12 },
  cardTitle: { fontSize: 16, fontWeight: '600', color: '#FFF', marginBottom: 12 },
  noFaults: { color: '#4CAF50', fontSize: 14 },
  faultText: { color: '#F44336', fontSize: 14, marginVertical: 4 },
  clearBtn: { backgroundColor: '#F44336', padding: 10, borderRadius: 8, marginTop: 12, alignItems: 'center' },
  clearBtnText: { color: '#FFF', fontWeight: '600' },
  phaseRow: { flexDirection: 'row', justifyContent: 'space-around' },
  phaseItem: { alignItems: 'center' },
  phaseLabel: { color: '#FF6B35', fontSize: 18, fontWeight: 'bold' },
  phaseValue: { color: '#FFF', fontSize: 16, marginTop: 4 },
  tempRow: { flexDirection: 'row', justifyContent: 'space-around' },
  tempItem: { alignItems: 'center' },
  tempLabel: { color: '#888', fontSize: 13 },
  tempValue: { fontSize: 20, fontWeight: 'bold', marginTop: 4 },
  busVoltage: { color: '#FFF', fontSize: 32, fontWeight: 'bold', textAlign: 'center' },
  voltageBar: { height: 8, backgroundColor: '#333', borderRadius: 4, marginTop: 12, overflow: 'hidden' },
  voltageFill: { height: '100%', borderRadius: 4 },
  faultItem: { padding: 12, marginVertical: 4, borderRadius: 8, backgroundColor: '#2A2A4A' },
  faultCleared: { opacity: 0.6 },
  faultActive: { borderLeftWidth: 3, borderLeftColor: '#F44336' },
  faultTime: { color: '#888', fontSize: 11 },
  faultMsg: { color: '#DDD', fontSize: 13, marginTop: 4 },
  faultCode: { color: '#FF6B35', fontSize: 11, marginTop: 2 },
});