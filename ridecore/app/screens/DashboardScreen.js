// ============================================================================
// screens/DashboardScreen.js — Main motor control & telemetry dashboard
// ============================================================================
import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert,
} from 'react-native';
import { BleManager } from 'react-native-ble-plx';
import CircularGauge from '../components/CircularGauge';
import { buildFrame, parseFrame, CMD_IDS } from '../utils/protocol';

const BLE_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
const BLE_TX_CHAR_UUID = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E';
const BLE_RX_CHAR_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E';

export default function DashboardScreen() {
  const [bleManager] = useState(new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);

  // Motor state
  const [busVoltage, setBusVoltage] = useState(0);
  const [phaseCurrent, setPhaseCurrent] = useState(0);
  const [motorRPM, setMotorRPM] = useState(0);
  const [throttlePos, setThrottlePos] = useState(0);
  const [faultCode, setFaultCode] = useState(0);
  const [running, setRunning] = useState(false);
  const [temperature, setTemperature] = useState(25);

  // Scan for RideCore BLE devices
  const startScan = useCallback(() => {
    setScanning(true);
    bleManager.startDeviceScan(null, null, (error, scannedDevice) => {
      if (error) {
        Alert.alert('Scan Error', error.message);
        setScanning(false);
        return;
      }
      if (scannedDevice.name && scannedDevice.name.startsWith('RideCore')) {
        bleManager.stopDeviceScan();
        setScanning(false);
        connectToDevice(scannedDevice);
      }
    });
  }, [bleManager]);

  // Connect to device
  const connectToDevice = async (scannedDevice) => {
    try {
      const connectedDevice = await scannedDevice.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();
      setDevice(connectedDevice);
      setConnected(true);

      // Subscribe to notifications
      const rxChar = await connectedDevice.readCharacteristicForService(
        BLE_SERVICE_UUID, BLE_RX_CHAR_UUID
      );
      connectedDevice.monitorCharacteristicForService(
        BLE_SERVICE_UUID, BLE_RX_CHAR_UUID,
        (error, characteristic) => {
          if (characteristic?.value) {
            const data = Buffer.from(characteristic.value, 'base64');
            const frame = parseFrame(data);
            if (frame && frame.cmdId === CMD_IDS.RSP_STATUS) {
              const vbus = data.readUInt16LE(0) / 100.0;
              const iq = data.readInt16LE(2) / 10.0;
              const omega = data.readInt16LE(4) / 10.0;
              setBusVoltage(vbus);
              setPhaseCurrent(iq);
              setMotorRPM(Math.round(omega * 60 / (2 * Math.PI)));
              setFaultCode(data[6]);
              setRunning(data[7] === 1);
            }
          }
        }
      );
    } catch (err) {
      Alert.alert('Connection Error', err.message);
    }
  };

  // Send throttle command
  const sendThrottle = (value) => {
    if (!device || !connected) return;
    const throttleVal = Math.round(value * 65535);
    const payload = Buffer.alloc(2);
    payload.writeUInt16LE(throttleVal);
    const frame = buildFrame(CMD_IDS.CMD_SET_THROTTLE, payload);
    device.writeCharacteristicWithResponseForService(
      BLE_SERVICE_UUID, BLE_TX_CHAR_UUID, frame.toString('base64')
    ).catch(() => {});
  };

  // Disconnect
  const disconnect = async () => {
    if (device) {
      await device.cancelConnection();
      setDevice(null);
      setConnected(false);
    }
  };

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      bleManager.destroy();
    };
  }, []);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>RideCore</Text>
        <Text style={styles.subtitle}>48V/200A Motor Controller</Text>
      </View>

      {/* Connection */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>
        <TouchableOpacity
          style={[styles.button, connected ? styles.buttonDisconnect : styles.buttonConnect]}
          onPress={connected ? disconnect : startScan}
        >
          <Text style={styles.buttonText}>
            {scanning ? 'Scanning...' : connected ? 'Disconnect' : 'Connect BLE'}
          </Text>
        </TouchableOpacity>
        <Text style={styles.statusText}>
          {connected ? `Connected: ${device?.name}` : 'Not connected'}
        </Text>
      </View>

      {/* Gauges */}
      <View style={styles.gaugeRow}>
        <CircularGauge
          value={busVoltage}
          maxValue={72}
          unit="V"
          label="Bus Voltage"
          color="#4CAF50"
        />
        <CircularGauge
          value={Math.abs(phaseCurrent)}
          maxValue={200}
          unit="A"
          label="Phase Current"
          color="#FF6B35"
        />
      </View>

      <View style={styles.gaugeRow}>
        <CircularGauge
          value={motorRPM}
          maxValue={5000}
          unit="RPM"
          label="Motor Speed"
          color="#2196F3"
        />
        <CircularGauge
          value={temperature}
          maxValue={120}
          unit="°C"
          label="Temperature"
          color="#F44336"
        />
      </View>

      {/* Throttle slider area */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Throttle Control</Text>
        <View style={styles.throttleBar}>
          <View style={[styles.throttleFill, { width: `${throttlePos * 100}%` }]} />
        </View>
        <View style={styles.throttleButtons}>
          <TouchableOpacity style={styles.throttleBtn} onPress={() => { setThrottlePos(0); sendThrottle(0); }}>
            <Text style={styles.throttleBtnText}>0%</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.throttleBtn} onPress={() => { setThrottlePos(0.25); sendThrottle(0.25); }}>
            <Text style={styles.throttleBtnText}>25%</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.throttleBtn} onPress={() => { setThrottlePos(0.5); sendThrottle(0.5); }}>
            <Text style={styles.throttleBtnText}>50%</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.throttleBtn} onPress={() => { setThrottlePos(0.75); sendThrottle(0.75); }}>
            <Text style={styles.throttleBtnText}>75%</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.throttleBtn, styles.throttleBtnDanger]} onPress={() => { setThrottlePos(1.0); sendThrottle(1.0); }}>
            <Text style={styles.throttleBtnText}>100%</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Fault status */}
      {faultCode !== 0 && (
        <View style={styles.faultCard}>
          <Text style={styles.faultText}>⚠ FAULT: 0x{faultCode.toString(16).toUpperCase()}</Text>
        </View>
      )}

      {/* Run Status */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Status</Text>
        <Text style={styles.statusText}>
          Motor: {running ? '● RUNNING' : '○ STOPPED'}
        </Text>
        <Text style={styles.statusText}>
          Faults: {faultCode === 0 ? 'None' : `0x${faultCode.toString(16)}`}
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F23' },
  header: { padding: 20, alignItems: 'center' },
  title: { fontSize: 28, fontWeight: 'bold', color: '#FF6B35' },
  subtitle: { fontSize: 14, color: '#888', marginTop: 4 },
  card: { margin: 10, padding: 16, backgroundColor: '#1A1A2E', borderRadius: 12 },
  cardTitle: { fontSize: 16, fontWeight: '600', color: '#FFF', marginBottom: 12 },
  button: { padding: 12, borderRadius: 8, alignItems: 'center' },
  buttonConnect: { backgroundColor: '#4CAF50' },
  buttonDisconnect: { backgroundColor: '#F44336' },
  buttonText: { color: '#FFF', fontWeight: '600', fontSize: 14 },
  statusText: { color: '#AAA', marginTop: 8, fontSize: 13 },
  gaugeRow: { flexDirection: 'row', justifyContent: 'space-around', marginVertical: 8 },
  throttleBar: { height: 24, backgroundColor: '#333', borderRadius: 12, overflow: 'hidden' },
  throttleFill: { height: '100%', backgroundColor: '#FF6B35', borderRadius: 12 },
  throttleButtons: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 12 },
  throttleBtn: { padding: 10, backgroundColor: '#2A2A4A', borderRadius: 8, flex: 1, marginHorizontal: 4, alignItems: 'center' },
  throttleBtnDanger: { backgroundColor: '#F44336' },
  throttleBtnText: { color: '#FFF', fontWeight: '600', fontSize: 12 },
  faultCard: { margin: 10, padding: 16, backgroundColor: '#4A1010', borderRadius: 12, borderWidth: 1, borderColor: '#F44336' },
  faultText: { color: '#F44336', fontWeight: 'bold', fontSize: 16 },
});