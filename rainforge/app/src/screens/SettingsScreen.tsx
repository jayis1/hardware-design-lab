/**
 * SettingsScreen.tsx — LoRaWAN keys, intervals, calibration offsets, OTA
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React, { useState } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TextInput, TouchableOpacity,
  Switch, Alert,
} from 'react-native';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';

export default function SettingsScreen() {
  const { bleStatus, sendCommand, disconnect } = useBLE();
  const [devEui, setDevEui] = useState('70B3D57ED0000001');
  const [appEui, setAppEui] = useState('0000000000000000');
  const [appKey, setAppKey] = useState('2B7E151628AED2A6ABF7158809CF4F3C');
  const [reportInterval, setReportInterval] = useState('15');
  const [heaterAuto, setHeaterAuto] = useState(true);
  const [tempOffset, setTempOffset] = useState('0.0');
  const [pressureOffset, setPressureOffset] = useState('0.0');

  const saveConfig = async () => {
    // Build config command: 0x03 + 2-byte interval (minutes, LE)
    const min = parseInt(reportInterval, 10);
    if (isNaN(min) || min < 1 || min > 360) {
      Alert.alert('Invalid interval', 'Enter 1-360 minutes.');
      return;
    }
    await sendCommand([0x03, min & 0xFF, (min >> 8) & 0xFF]);
    Alert.alert('Saved', `Report interval set to ${min} minutes.`);
  };

  const triggerHeater = async () => {
    await sendCommand([0x04]);
    Alert.alert('Heater activated', 'SHT45 heater pulsed for 1 second.');
  };

  const clearLog = async () => {
    await sendCommand([0x02]);
    Alert.alert('Event log cleared', 'FRAM event ring buffer has been reset.');
  };

  const startCalibrationMode = async () => {
    await sendCommand([0x01]);
    Alert.alert('Calibration mode', 'Device is now in calibration mode. Use the Calibrate tab.');
  };

  return (
    <ScrollView style={styles.container}>
      <Card title="Connection" accent="#4fc3f7">
        <View style={styles.row}>
          <Text style={styles.label}>Status:</Text>
          <Text style={[styles.value, { color: bleStatus.connected ? '#4caf50' : '#f44336' }]}>
            {bleStatus.connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {bleStatus.connected && (
          <>
            <View style={styles.row}>
              <Text style={styles.label}>Device:</Text>
              <Text style={styles.value}>{bleStatus.deviceName}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>RSSI:</Text>
              <Text style={styles.value}>{bleStatus.rssi} dBm</Text>
            </View>
            <TouchableOpacity style={styles.btnDanger} onPress={disconnect}>
              <Text style={styles.btnText}>Disconnect</Text>
            </TouchableOpacity>
          </>
        )}
      </Card>

      <Card title="LoRaWAN Credentials" accent="#ce93d8">
        <Text style={styles.hint}>OTAA join keys (stored in FRAM config block)</Text>
        <View style={styles.field}>
          <Text style={styles.label}>DevEUI:</Text>
          <TextInput style={styles.input} value={devEui} onChangeText={setDevEui}
            autoCapitalize="characters" />
        </View>
        <View style={styles.field}>
          <Text style={styles.label}>AppEUI:</Text>
          <TextInput style={styles.input} value={appEui} onChangeText={setAppEui}
            autoCapitalize="characters" />
        </View>
        <View style={styles.field}>
          <Text style={styles.label}>AppKey:</Text>
          <TextInput style={styles.input} value={appKey} onChangeText={setAppKey}
            autoCapitalize="characters" />
        </View>
      </Card>

      <Card title="Reporting" accent="#66bb6a">
        <View style={styles.field}>
          <Text style={styles.label}>Interval (min):</Text>
          <TextInput style={styles.input} value={reportInterval}
            onChangeText={setReportInterval} keyboardType="numeric" />
        </View>
        <TouchableOpacity style={styles.btn} onPress={saveConfig}>
          <Text style={styles.btnText}>Save to Device</Text>
        </TouchableOpacity>
      </Card>

      <Card title="Sensor Calibration Offsets" accent="#ff9800">
        <View style={styles.field}>
          <Text style={styles.label}>Temp offset (°C):</Text>
          <TextInput style={styles.input} value={tempOffset}
            onChangeText={setTempOffset} keyboardType="numeric" />
        </View>
        <View style={styles.field}>
          <Text style={styles.label}>Pressure offset (Pa):</Text>
          <TextInput style={styles.input} value={pressureOffset}
            onChangeText={setPressureOffset} keyboardType="numeric" />
        </View>
      </Card>

      <Card title="Maintenance" accent="#42a5f5">
        <View style={styles.row}>
          <Text style={styles.label}>Auto heater (condensation):</Text>
          <Switch value={heaterAuto} onValueChange={setHeaterAuto}
            trackColor={{ false: '#1a2a4a', true: '#1565c0' }}
            thumbColor={heaterAuto ? '#4fc3f7' : '#5a6a8a'} />
        </View>
        <TouchableOpacity style={styles.btnSecondary} onPress={triggerHeater}>
          <Text style={styles.btnText}>Pulse Heater Now</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnSecondary} onPress={startCalibrationMode}>
          <Text style={styles.btnText}>Enter Calibration Mode</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btnDanger} onPress={clearLog}>
          <Text style={styles.btnText}>Clear Event Log</Text>
        </TouchableOpacity>
      </Card>

      <Card title="Firmware" accent="#26c6da">
        <Text style={styles.hint}>
          Firmware OTA via LoRaWAN is supported via the Fragmented Data Block
          Transport (FDBT) specification. Maximum fragment size: 50 bytes.
        </Text>
        <TouchableOpacity style={styles.btn}>
          <Text style={styles.btnText}>Check for Updates</Text>
        </TouchableOpacity>
      </Card>

      <Card title="About" accent="#78909c">
        <Text style={styles.aboutText}>RainForge Companion App v1.0.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Copyright (C) 2026 jayis1</Text>
        <Text style={styles.aboutText}>License: MIT</Text>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1628', padding: 12 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginVertical: 4 },
  field: { flexDirection: 'row', alignItems: 'center', marginVertical: 6 },
  label: { color: '#a0b0d0', fontSize: 13, flex: 1 },
  value: { color: '#e0e0f0', fontSize: 14, fontWeight: '500' },
  input: {
    width: 180,
    backgroundColor: '#0d1e3a',
    color: '#e0e0f0',
    borderWidth: 1,
    borderColor: '#1a2a4a',
    borderRadius: 4,
    padding: 8,
    fontSize: 13,
  },
  btn: {
    backgroundColor: '#1565c0',
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 8,
  },
  btnSecondary: {
    backgroundColor: '#37474f',
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 8,
  },
  btnDanger: {
    backgroundColor: '#c62828',
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 8,
  },
  btnText: { color: '#fff', fontSize: 14, fontWeight: '600' },
  hint: { color: '#5a6a8a', fontSize: 11, fontStyle: 'italic', marginBottom: 8 },
  aboutText: { color: '#5a6a8a', fontSize: 12, marginVertical: 2 },
});