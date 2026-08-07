/**
 * SettingsScreen.js — Device Settings & WiFi/MQTT Configuration
 *
 * Manages BLE connection, WiFi credentials, MQTT broker URL,
 * device information, firmware updates, and data export.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useContext, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert, Share } from 'react-native';
import { Card, Title, Paragraph, TextInput, Button, List, Divider, Switch, ActivityIndicator } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { FermenTiqContext } from '../utils/ble';

export default function SettingsScreen() {
  const { bleManager, connectionState, connectToDevice, sendCommand } = useContext(FermenTiqContext);
  const [scanning, setScanning] = useState(false);
  const [foundDevices, setFoundDevices] = useState([]);
  const [wifiSsid, setWifiSsid] = useState('');
  const [wifiPass, setWifiPass] = useState('');
  const [mqttBroker, setMqttBroker] = useState('mqtt://homeassistant.local');
  const [autoExport, setAutoExport] = useState(false);

  const handleScan = () => {
    setScanning(true);
    setFoundDevices([]);
    bleManager.startScan((device) => {
      setFoundDevices(prev => {
        if (prev.find(d => d.id === device.id)) return prev;
        return [...prev, device];
      });
    });
    setTimeout(() => {
      bleManager.stopScan();
      setScanning(false);
    }, 10000);
  };

  const handleConnect = async (deviceId) => {
    try {
      await connectToDevice(deviceId);
      Alert.alert('Connected', 'FermenTiq device connected successfully');
    } catch (e) {
      Alert.alert('Connection Failed', e.message);
    }
  };

  const handleSaveWifi = async () => {
    if (!wifiSsid || !wifiPass) {
      Alert.alert('Error', 'Please enter WiFi SSID and password');
      return;
    }
    // Send WiFi config to device via BLE command
    await sendCommand(`wifi:${wifiSsid}:${wifiPass}:${mqttBroker}`);
    Alert.alert('Saved', 'WiFi credentials sent to device. It will reconnect to MQTT shortly.');
  };

  const handleExport = async () => {
    try {
      await sendCommand('export');
      Alert.alert('Export Started', 'Batch data is being exported to SD card.');
    } catch (e) {
      Alert.alert('Export Failed', e.message);
    }
  };

  const handleFirmwareUpdate = () => {
    Alert.alert(
      'Firmware Update',
      'Check for firmware updates over WiFi? The device will reboot after installation.',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Check & Update', onPress: () => sendCommand('ota_check') },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* BLE Connection */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="bluetooth" size={24} color="#D4A017" /> BLE Connection
          </Title>
          {connectionState.connected ? (
            <View>
              <List.Item
                title={connectionState.deviceName || 'FermenTiq Device'}
                description={`ID: ${connectionState.deviceId}`}
                left={props => <Icon name="check-circle" size={24} color="#4CAF50" />}
                titleStyle={styles.listTitle}
                descriptionStyle={styles.listDesc}
              />
              <Button mode="outlined" onPress={() => bleManager.disconnect()} textColor="#E53935" icon="bluetooth-off">
                Disconnect
              </Button>
            </View>
          ) : (
            <View>
              <Button mode="contained" onPress={handleScan} loading={scanning} icon="radar" style={styles.button}>
                {scanning ? 'Scanning...' : 'Scan for Devices'}
              </Button>
              {foundDevices.map(device => (
                <List.Item
                  key={device.id}
                  title={device.name}
                  description={`RSSI: ${device.rssi} dBm`}
                  left={props => <Icon name="chip" size={24} color="#888" />}
                  right={props => <Icon name="link" size={24} color="#D4A017" />}
                  onPress={() => handleConnect(device.id)}
                  titleStyle={styles.listTitle}
                  descriptionStyle={styles.listDesc}
                />
              ))}
            </View>
          )}
        </Card.Content>
      </Card>

      {/* WiFi & MQTT */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="wifi" size={24} color="#42A5F5" /> WiFi & MQTT
          </Title>
          <Paragraph style={styles.desc}>
            Connect FermenTiq to your WiFi for MQTT integration with
            Home Assistant and cloud sync.
          </Paragraph>
          <TextInput
            label="WiFi SSID"
            value={wifiSsid}
            onChangeText={setWifiSsid}
            style={styles.input}
            theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
          />
          <TextInput
            label="WiFi Password"
            value={wifiPass}
            onChangeText={setWifiPass}
            secureTextEntry
            style={styles.input}
            theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
          />
          <TextInput
            label="MQTT Broker URL"
            value={mqttBroker}
            onChangeText={setMqttBroker}
            style={styles.input}
            theme={{ colors: { text: '#e0e0e0', primary: '#D4A017' } }}
          />
          <Button mode="contained" onPress={handleSaveWifi} style={styles.button} icon="content-save">
            Save WiFi Config
          </Button>
        </Card.Content>
      </Card>

      {/* Data & Export */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="database-export" size={24} color="#66BB6A" /> Data & Export
          </Title>
          <List.Item
            title="Export Batch Data (CSV)"
            description="Save to SD card or share via email"
            left={props => <Icon name="file-export" size={24} color="#66BB6A" />}
            right={props => <Icon name="chevron-right" size={24} color="#555" />}
            onPress={handleExport}
            titleStyle={styles.listTitle}
            descriptionStyle={styles.listDesc}
          />
          <Divider style={styles.divider} />
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Auto-export on batch end</Text>
            <Switch value={autoExport} onValueChange={setAutoExport} color="#D4A017" />
          </View>
        </Card.Content>
      </Card>

      {/* Firmware */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="update" size={24} color="#FF9800" /> Firmware
          </Title>
          <List.Item
            title="Version"
            description="1.0.0 (Aug 7, 2026)"
            left={props => <Icon name="information" size={24} color="#888" />}
            titleStyle={styles.listTitle}
            descriptionStyle={styles.listDesc}
          />
          <List.Item
            title="Check for Updates"
            description="OTA update over WiFi"
            left={props => <Icon name="cloud-download" size={24} color="#FF9800" />}
            right={props => <Icon name="chevron-right" size={24} color="#555" />}
            onPress={handleFirmwareUpdate}
            titleStyle={styles.listTitle}
            descriptionStyle={styles.listDesc}
          />
        </Card.Content>
      </Card>

      {/* About */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="information" size={24} color="#888" /> About
          </Title>
          <Paragraph style={styles.aboutText}>
            FermenTiq — Smart Multi-Modal Fermentation Bioreactor Monitor
          </Paragraph>
          <Paragraph style={styles.aboutText}>Version 1.0.0</Paragraph>
          <Paragraph style={styles.aboutText}>Author: jayis1</Paragraph>
          <Paragraph style={styles.aboutText}>License: GPL-3.0 (firmware), MIT (app)</Paragraph>
          <Paragraph style={styles.aboutText}>Copyright © 2026 jayis1</Paragraph>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e' },
  card: { margin: 12, backgroundColor: '#16213e' },
  title: { color: '#e0e0e0', fontSize: 18, flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  desc: { color: '#999', fontSize: 14, marginBottom: 12 },
  input: { backgroundColor: '#0f1626', marginBottom: 8 },
  button: { marginTop: 8 },
  listTitle: { color: '#e0e0e0', fontSize: 15 },
  listDesc: { color: '#888', fontSize: 13 },
  divider: { backgroundColor: '#333', marginVertical: 8 },
  switchRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 8 },
  switchLabel: { color: '#e0e0e0', fontSize: 15 },
  aboutText: { color: '#999', fontSize: 14, marginBottom: 4 },
});