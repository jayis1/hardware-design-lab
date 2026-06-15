/**
 * screens/SettingsScreen.js — Device configuration
 *
 * Configure radar chirp profiles, WiFi, BLE, display, and system.
 * Sends configuration commands via binary wire protocol.
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Switch,
  Slider,
  TouchableOpacity,
  TextInput,
  ScrollView,
  Alert,
} from 'react-native';
import { protocol } from '../../App';

const CHIRP_PROFILES = [
  { id: 0, name: 'Short Range (4m)', bw: 4000, chirps: 128, frameMs: 50 },
  { id: 1, name: 'Medium Range (12m)', bw: 4000, chirps: 256, frameMs: 50 },
  { id: 2, name: 'Long Range (50m)', bw: 4000, chirps: 512, frameMs: 100 },
  { id: 3, name: 'High Speed (20fps)', bw: 4000, chirps: 128, frameMs: 50 },
  { id: 4, name: 'Vital Signs', bw: 4000, chirps: 256, frameMs: 33 },
];

export default function SettingsScreen() {
  const [connected, setConnected] = useState(false);
  const [selectedProfile, setSelectedProfile] = useState(1);
  const [contrast, setContrast] = useState(207);
  const [oledOn, setOledOn] = useState(true);
  const [wifiEnabled, setWifiEnabled] = useState(true);
  const [wifiSsid, setWifiSsid] = useState('');
  const [wifiPass, setWifiPass] = useState('');
  const [ethEnabled, setEthEnabled] = useState(true);
  const [mqttBroker, setMqttBroker] = useState('mqtt://broker.hivemq.com');
  const [imuOdr, setImuOdr] = useState(6); // 800 Hz
  const [streamingEnabled, setStreamingEnabled] = useState(true);

  useEffect(() => {
    protocol.onConnectionChange((isConnected) => {
      setConnected(isConnected);
    });
  }, []);

  const applyProfile = (profileId) => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to PicoRadar first.');
      return;
    }
    setSelectedProfile(profileId);
    const profile = CHIRP_PROFILES[profileId];
    protocol.sendCommand(ProtocolCommands.CMD_SET_PROFILE, {
      profileId: profile.id,
      bandwidth: profile.bw,
      chirps: profile.chirps,
      framePeriodMs: profile.frameMs,
    });
    Alert.alert('Profile Applied', `Switched to: ${profile.name}`);
  };

  const applyOledSettings = () => {
    if (!connected) return;
    protocol.sendCommand(ProtocolCommands.CMD_SET_OLED, {
      enabled: oledOn,
      contrast: contrast,
    });
  };

  const applyWifiSettings = () => {
    if (!connected) return;
    protocol.sendCommand(ProtocolCommands.CMD_SET_WIFI, {
      enabled: wifiEnabled,
      ssid: wifiSsid,
      password: wifiPass,
    });
  };

  const applyImuSettings = () => {
    if (!connected) return;
    protocol.sendCommand(ProtocolCommands.CMD_SET_IMU_ODR, { odr: imuOdr });
  };

  const rebootDevice = () => {
    if (!connected) return;
    Alert.alert('Reboot Device', 'Are you sure?', [
      { text: 'Cancel', style: 'cancel' },
      {
        text: 'Reboot',
        style: 'destructive',
        onPress: () => protocol.sendCommand(ProtocolCommands.CMD_REBOOT),
      },
    ]);
  };

  const firmwareUpdate = () => {
    if (!connected) return;
    Alert.alert('OTA Update', 'Check for firmware updates? (placeholder)');
  };

  return (
    <ScrollView style={styles.container}>
      {/* Chirp Profile */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Chirp Profile</Text>
        {CHIRP_PROFILES.map((profile) => (
          <TouchableOpacity
            key={profile.id}
            style={[
              styles.profileItem,
              selectedProfile === profile.id && styles.profileItemSelected,
            ]}
            onPress={() => applyProfile(profile.id)}
          >
            <Text
              style={[
                styles.profileText,
                selectedProfile === profile.id && styles.profileTextSelected,
              ]}
            >
              {profile.name}
            </Text>
            <Text style={styles.profileDetail}>
              BW={profile.bw}MHz Chirps={profile.chirps} Frame={profile.frameMs}ms
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Display Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display</Text>
        <View style={styles.row}>
          <Text style={styles.label}>OLED Enabled</Text>
          <Switch value={oledOn} onValueChange={setOledOn} />
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Contrast: {contrast}</Text>
        </View>
        <Slider
          minimumValue={0}
          maximumValue={255}
          step={1}
          value={contrast}
          onValueChange={setContrast}
          onSlidingComplete={applyOledSettings}
        />
      </View>

      {/* WiFi Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>WiFi</Text>
        <View style={styles.row}>
          <Text style={styles.label}>WiFi Enabled</Text>
          <Switch value={wifiEnabled} onValueChange={setWifiEnabled} />
        </View>
        <TextInput
          style={styles.input}
          placeholder="SSID"
          placeholderTextColor="#666"
          value={wifiSsid}
          onChangeText={setWifiSsid}
          autoCapitalize="none"
        />
        <TextInput
          style={styles.input}
          placeholder="Password"
          placeholderTextColor="#666"
          value={wifiPass}
          onChangeText={setWifiPass}
          secureTextEntry
          autoCapitalize="none"
        />
        <TouchableOpacity style={styles.applyButton} onPress={applyWifiSettings}>
          <Text style={styles.applyButtonText}>Apply WiFi</Text>
        </TouchableOpacity>
      </View>

      {/* Ethernet */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Ethernet / MQTT</Text>
        <View style={styles.row}>
          <Text style={styles.label}>Ethernet Enabled</Text>
          <Switch value={ethEnabled} onValueChange={setEthEnabled} />
        </View>
        <TextInput
          style={styles.input}
          placeholder="MQTT Broker URL"
          placeholderTextColor="#666"
          value={mqttBroker}
          onChangeText={setMqttBroker}
          autoCapitalize="none"
        />
      </View>

      {/* IMU */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>IMU (ICM-42688)</Text>
        <Text style={styles.label}>ODR: {imuOdr === 6 ? '800 Hz' : imuOdr === 5 ? '400 Hz' : '200 Hz'}</Text>
        <Slider
          minimumValue={3}
          maximumValue={6}
          step={1}
          value={imuOdr}
          onValueChange={setImuOdr}
          onSlidingComplete={applyImuSettings}
        />
      </View>

      {/* System */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>System</Text>
        <View style={styles.row}>
          <Text style={styles.label}>Data Streaming</Text>
          <Switch value={streamingEnabled} onValueChange={setStreamingEnabled} />
        </View>
        <TouchableOpacity style={styles.dangerButton} onPress={rebootDevice}>
          <Text style={styles.dangerButtonText}>Reboot Device</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.applyButton} onPress={firmwareUpdate}>
          <Text style={styles.applyButtonText}>Check for OTA Update</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#1C1C1E',
    padding: 16,
  },
  section: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
  },
  sectionTitle: {
    color: '#FFFFFF',
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 12,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  label: {
    color: '#CCCCCC',
    fontSize: 14,
  },
  profileItem: {
    padding: 12,
    borderRadius: 8,
    backgroundColor: '#3A3A3C',
    marginBottom: 8,
  },
  profileItemSelected: {
    backgroundColor: '#007AFF',
  },
  profileText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '500',
  },
  profileTextSelected: {
    color: '#FFFFFF',
  },
  profileDetail: {
    color: '#AAAAAA',
    fontSize: 11,
    marginTop: 2,
  },
  input: {
    backgroundColor: '#3A3A3C',
    color: '#FFFFFF',
    borderRadius: 8,
    padding: 10,
    marginVertical: 4,
    fontSize: 14,
  },
  applyButton: {
    backgroundColor: '#007AFF',
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  applyButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
  dangerButton: {
    backgroundColor: '#FF3B30',
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  dangerButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
});