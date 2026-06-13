/**
 * EAG-7000 — Settings Screen
 *
 * Manages BLE connection, device configuration, and firmware updates.
 *
 * Features:
 * - BLE scan, connect, disconnect
 * - Device info display (model, firmware version, hardware revision)
 * - CAN bus configuration (bitrate, enable/disable)
 * - Firmware OTA update
 * - Factory reset
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext } from 'react';
import { View, ScrollView, StyleSheet, Alert, Switch } from 'react-native';
import { Text, Card, Button, List, Divider, TextInput, Dialog, Portal } from 'react-native-paper';
import { BLEContext } from '../components/BLEProvider';
import { MSG_TYPE, POWER_CMD, BLE, encodeMessage } from '../utils/protocol';

function SettingsScreen() {
  const {
    connected, device, scanning,
    startScan, stopScan, connect, disconnect,
    writeCharacteristic, deviceInfo,
  } = useContext(BLEContext);

  // CAN bus configuration
  const [can0Enabled, setCan0Enabled] = useState(true);
  const [can1Enabled, setCan1Enabled] = useState(true);
  const [can0Bitrate, setCan0Bitrate] = useState('500000');
  const [can1Bitrate, setCan1Bitrate] = useState('500000');

  // Firmware update state
  const [otaProgress, setOtaProgress] = useState(0);
  const [otaInProgress, setOtaInProgress] = useState(false);

  // Dialog state
  const [showResetDialog, setShowResetDialog] = useState(false);
  const [showScanDialog, setShowScanDialog] = useState(false);

  // Available devices from scan
  const [devices, setDevices] = useState([]);

  const handleScan = async () => {
    setShowScanDialog(true);
    const found = await startScan();
    setDevices(found || []);
  };

  const handleConnect = async (deviceItem) => {
    setShowScanDialog(false);
    try {
      await connect(deviceItem);
    } catch (err) {
      Alert.alert('Connection Failed', err.message);
    }
  };

  const handleDisconnect = async () => {
    try {
      await disconnect();
    } catch (err) {
      Alert.alert('Disconnection Error', err.message);
    }
  };

  const handleCanConfig = async (bus, enabled, bitrate) => {
    if (!connected) {
      Alert.alert('Not Connected', 'Please connect to gateway first');
      return;
    }

    // Send CAN config message: [CONFIG(0x09)][LEN(0x08)][DATA: bus(8) | enable(8) | bitrate(16)]
    const bitrateNum = parseInt(bitrate) || 500000;
    const packet = encodeMessage(
      MSG_TYPE.CONFIG,
      0x08,
      (bus << 8) | (enabled ? 1 : 0)
    );
    await writeCharacteristic(packet);
  };

  const handleFactoryReset = async () => {
    setShowResetDialog(false);
    if (!connected) return;

    // Send reset command: [POWER(0x06)][0x01][RESET(0x03)]
    const packet = encodeMessage(MSG_TYPE.POWER, 0x01, POWER_CMD.RESET);
    await writeCharacteristic(packet);
    Alert.alert('Reset Command Sent', 'The gateway will restart in a few seconds.');
  };

  const handleFirmwareUpdate = async () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Please connect to gateway first');
      return;
    }

    setOtaInProgress(true);
    setOtaProgress(0);

    // Send OTA start command
    const startPacket = encodeMessage(MSG_TYPE.OTA_START, 0x00, 0x0000);
    await writeCharacteristic(startPacket);

    // Simulate OTA progress (in production, actual firmware file would be sent)
    Alert.alert(
      'Firmware Update',
      'OTA update initiated. In production, the firmware binary would be transmitted over BLE in chunks.'
    );

    setOtaInProgress(false);
  };

  return (
    <ScrollView style={styles.container}>
      {/* BLE Connection */}
      <Card style={styles.card}>
        <Card.Title title="BLE Connection" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          {connected ? (
            <View>
              <List.Item
                title={device?.name || 'Unknown Device'}
                description={device?.id || ''}
                left={props => <List.Icon {...props} icon="bluetooth-connect" color="#4CAF50" />}
                titleStyle={{ color: '#E0E0E0' }}
                descriptionStyle={{ color: '#B0B0B0' }}
              />
              <Button mode="outlined" onPress={handleDisconnect} style={styles.button}
                color="#F44336">
                Disconnect
              </Button>
            </View>
          ) : (
            <View>
              <List.Item
                title="Not Connected"
                description="Tap Scan to find EAG-7000 gateway"
                left={props => <List.Icon {...props} icon="bluetooth-off" color="#757575" />}
                titleStyle={{ color: '#B0B0B0' }}
                descriptionStyle={{ color: '#757575' }}
              />
              <Button mode="contained" onPress={handleScan} style={styles.button}
                loading={scanning}>
                {scanning ? 'Scanning...' : 'Scan for Gateway'}
              </Button>
            </View>
          )}
        </Card.Content>
      </Card>

      {/* Device Info */}
      {connected && deviceInfo && (
        <Card style={styles.card}>
          <Card.Title title="Device Information" titleStyle={{ color: '#E0E0E0' }} />
          <Card.Content>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Model</Text>
              <Text style={styles.infoValue}>{deviceInfo.model || 'EAG-7000'}</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Firmware</Text>
              <Text style={styles.infoValue}>{deviceInfo.firmware || '--'}</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Hardware</Text>
              <Text style={styles.infoValue}>{deviceInfo.hardware || 'Rev A'}</Text>
            </View>
          </Card.Content>
        </Card>
      )}

      {/* CAN Bus Configuration */}
      <Card style={styles.card}>
        <Card.Title title="CAN-FD Configuration" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          <View style={styles.configRow}>
            <Switch value={can0Enabled}
              onValueChange={(v) => { setCan0Enabled(v); handleCanConfig(0, v, can0Bitrate); }} />
            <Text style={styles.configLabel}>CAN0 (Industrial Bus 1)</Text>
          </View>
          <TextInput
            style={styles.configInput}
            label="Bitrate (bps)"
            value={can0Bitrate}
            onChangeText={setCan0Bitrate}
            keyboardType="numeric"
            disabled={!can0Enabled}
            mode="outlined"
            dense
          />

          <Divider style={styles.divider} />

          <View style={styles.configRow}>
            <Switch value={can1Enabled}
              onValueChange={(v) => { setCan1Enabled(v); handleCanConfig(1, v, can1Bitrate); }} />
            <Text style={styles.configLabel}>CAN1 (Industrial Bus 2)</Text>
          </View>
          <TextInput
            style={styles.configInput}
            label="Bitrate (bps)"
            value={can1Bitrate}
            onChangeText={setCan1Bitrate}
            keyboardType="numeric"
            disabled={!can1Enabled}
            mode="outlined"
            dense
          />
        </Card.Content>
      </Card>

      {/* Firmware Update */}
      <Card style={styles.card}>
        <Card.Title title="Firmware Update" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          <Text style={styles.description}>
            Update the Cortex-M4F firmware over BLE. Ensure a stable connection
            before starting the update. The gateway will reboot after the update completes.
          </Text>
          <Button
            mode="contained"
            onPress={handleFirmwareUpdate}
            disabled={!connected || otaInProgress}
            loading={otaInProgress}
            style={styles.button}
          >
            {otaInProgress ? 'Updating...' : 'Start Firmware Update'}
          </Button>
        </Card.Content>
      </Card>

      {/* Factory Reset */}
      <Card style={styles.card}>
        <Card.Title title="Factory Reset" titleStyle={{ color: '#E0E0E0' }} />
        <Card.Content>
          <Text style={styles.description}>
            Reset the gateway to factory defaults. This will clear all configuration
            and restart the device.
          </Text>
          <Button mode="outlined" onPress={() => setShowResetDialog(true)}
            color="#F44336" style={styles.button}>
            Factory Reset
          </Button>
        </Card.Content>
      </Card>

      {/* Scan Dialog */}
      <Portal>
        <Dialog visible={showScanDialog} onDismiss={() => setShowScanDialog(false)}>
          <Dialog.Title>Available Devices</Dialog.Title>
          <Dialog.Content>
            {devices.length === 0 ? (
              <Text>Scanning for devices...</Text>
            ) : (
              devices.map((d, i) => (
                <List.Item
                  key={i}
                  title={d.name || 'Unknown'}
                  description={d.id}
                  onPress={() => handleConnect(d)}
                />
              ))
            )}
          </Dialog.Content>
          <Dialog.Actions>
            <Button onPress={() => setShowScanDialog(false)}>Cancel</Button>
          </Dialog.Actions>
        </Dialog>
      </Portal>

      {/* Reset Confirmation Dialog */}
      <Portal>
        <Dialog visible={showResetDialog} onDismiss={() => setShowResetDialog(false)}>
          <Dialog.Title>Factory Reset</Dialog.Title>
          <Dialog.Content>
            <Text>Are you sure? This will erase all configuration and restart the gateway.</Text>
          </Dialog.Content>
          <Dialog.Actions>
            <Button onPress={() => setShowResetDialog(false)}>Cancel</Button>
            <Button onPress={handleFactoryReset} color="#F44336">Reset</Button>
          </Dialog.Actions>
        </Dialog>
      </Portal>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F23',
  },
  card: {
    margin: 8,
    backgroundColor: '#1A1A2E',
    borderRadius: 8,
  },
  button: {
    marginVertical: 8,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  infoLabel: {
    color: '#B0B0B0',
    fontSize: 14,
  },
  infoValue: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: 'bold',
  },
  configRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  configLabel: {
    color: '#E0E0E0',
    fontSize: 14,
    marginLeft: 8,
    flex: 1,
  },
  configInput: {
    marginVertical: 4,
  },
  divider: {
    marginVertical: 8,
    backgroundColor: '#333',
  },
  description: {
    color: '#B0B0B0',
    fontSize: 13,
    lineHeight: 18,
    marginBottom: 8,
  },
});

export default SettingsScreen;