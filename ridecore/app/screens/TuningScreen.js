// ============================================================================
// screens/TuningScreen.js — FOC parameter tuning screen
// ============================================================================
import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import SliderCard from '../components/SliderCard';
import { buildFrame, CMD_IDS } from '../utils/protocol';

export default function TuningScreen({ route }) {
  // Get device reference from navigation params or context (simplified)
  const [kpD, setKpD] = useState(0.50);
  const [kiD, setKiD] = useState(0.01);
  const [kpQ, setKpQ] = useState(0.50);
  const [kiQ, setKiQ] = useState(0.01);
  const [currentLimit, setCurrentLimit] = useState(120);
  const [pwmFreq, setPwmFreq] = useState(20);
  const [deadtime, setDeadtime] = useState(500);
  const [sensorless, setSensorless] = useState(true);

  const sendConfig = () => {
    const payload = Buffer.alloc(24);
    payload.writeFloatLE(kpD, 0);
    payload.writeFloatLE(kiD, 4);
    payload.writeFloatLE(kpQ, 8);
    payload.writeFloatLE(kiQ, 12);
    payload.writeUInt16LE(currentLimit * 1000, 16);  // mA
    payload.writeUInt16LE(pwmFreq * 1000, 18);        // Hz
    payload.writeUInt16LE(deadtime, 20);               // ns
    payload.writeUInt8(sensorless ? 1 : 0, 22);
    const frame = buildFrame(CMD_IDS.CMD_SET_CONFIG, payload);
    // In production: write to BLE characteristic
    Alert.alert('Config Sent', `${frame.length} bytes transmitted`);
  };

  const saveConfig = () => {
    const frame = buildFrame(CMD_IDS.CMD_SAVE_CONFIG, Buffer.alloc(0));
    Alert.alert('Saved', 'Configuration saved to flash');
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>FOC Tuning</Text>
        <Text style={styles.subtitle}>Adjust controller parameters in real-time</Text>
      </View>

      {/* D-axis PI Controller */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>D-Axis Current Controller</Text>
        <SliderCard label="Kp (d)" value={kpD} min={0} max={2} step={0.01} onChange={setKpD} />
        <SliderCard label="Ki (d)" value={kiD} min={0} max={0.1} step={0.001} onChange={setKiD} />
      </View>

      {/* Q-axis PI Controller */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Q-Axis Current Controller</Text>
        <SliderCard label="Kp (q)" value={kpQ} min={0} max={2} step={0.01} onChange={setKpQ} />
        <SliderCard label="Ki (q)" value={kiQ} min={0} max={0.1} step={0.001} onChange={setKiQ} />
      </View>

      {/* Motor Limits */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Motor Limits</Text>
        <SliderCard label="Current Limit (A)" value={currentLimit} min={10} max={200} step={1} onChange={setCurrentLimit} />
        <SliderCard label="PWM Freq (kHz)" value={pwmFreq} min={10} max={30} step={1} onChange={setPwmFreq} />
        <SliderCard label="Dead Time (ns)" value={deadtime} min={100} max={2000} step={50} onChange={setDeadtime} />
      </View>

      {/* Mode */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Position Sensing</Text>
        <TouchableOpacity
          style={[styles.toggleButton, sensorless ? styles.toggleActive : styles.toggleInactive]}
          onPress={() => setSensorless(!sensorless)}
        >
          <Text style={styles.toggleText}>
            {sensorless ? 'Sensorless (Observer)' : 'Hall Sensors'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.applyButton} onPress={sendConfig}>
          <Text style={styles.applyText}>Apply</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.saveButton} onPress={saveConfig}>
          <Text style={styles.applyText}>Save to Flash</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F23' },
  header: { padding: 20, alignItems: 'center' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#FF6B35' },
  subtitle: { fontSize: 13, color: '#888', marginTop: 4 },
  section: { marginVertical: 8, paddingHorizontal: 16 },
  sectionTitle: { fontSize: 15, fontWeight: '600', color: '#CCC', marginBottom: 8 },
  toggleButton: { padding: 14, borderRadius: 8, alignItems: 'center' },
  toggleActive: { backgroundColor: '#2196F3' },
  toggleInactive: { backgroundColor: '#2A2A4A' },
  toggleText: { color: '#FFF', fontWeight: '600', fontSize: 14 },
  actionRow: { flexDirection: 'row', justifyContent: 'space-around', padding: 20 },
  applyButton: { backgroundColor: '#4CAF50', padding: 14, borderRadius: 8, flex: 1, marginHorizontal: 8, alignItems: 'center' },
  saveButton: { backgroundColor: '#FF6B35', padding: 14, borderRadius: 8, flex: 1, marginHorizontal: 8, alignItems: 'center' },
  applyText: { color: '#FFF', fontWeight: '700', fontSize: 16 },
});