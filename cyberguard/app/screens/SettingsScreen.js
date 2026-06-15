/**
 * SettingsScreen.js - CyberGuard Settings
 * Configure BLE parameters, security policies, fingerprint management
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Switch,
  TouchableOpacity,
  ScrollView,
  Alert,
  Vibration,
} from 'react-native';
import StatusCard from '../components/StatusCard';

export default function SettingsScreen({ navigation }) {
  const [autoConnect, setAutoConnect] = useState(true);
  const [proximityUnlock, setProximityUnlock] = useState(false);
  const [fingerprintRequired, setFingerprintRequired] = useState(true);
  const [nfcEnabled, setNfcEnabled] = useState(true);
  const [bleEnabled, setBleEnabled] = useState(true);
  const [ledNotifications, setLedNotifications] = useState(true);
  const [lockOnRemove, setLockOnRemove] = useState(true);
  const [authTimeout, setAuthTimeout] = useState(30);
  const [factoryResetConfirm, setFactoryResetConfirm] = useState(false);

  const handleFactoryReset = useCallback(() => {
    Alert.alert(
      'Factory Reset',
      'This will erase ALL credentials and fingerprints from your CyberGuard token. This action cannot be undone.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: () => {
            Alert.alert(
              'Confirm Factory Reset',
              'Type "RESET" to confirm. Are you absolutely sure?',
              [
                { text: 'Cancel', style: 'cancel' },
                {
                  text: 'Erase Everything',
                  style: 'destructive',
                  onPress: () => {
                    Vibration.vibrate([0, 200, 100, 200, 100, 200]);
                    // Send FACTORY_RESET command to device
                    Alert.alert('Reset Complete', 'All data has been erased.');
                  },
                },
              ]
            );
          },
        },
      ]
    );
  }, []);

  const handleEnrollFingerprint = useCallback(() => {
    Alert.alert(
      'Enroll Fingerprint',
      'Place your finger on the sensor. You will need to lift and place 6 times.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start Enrollment',
          onPress: () => {
            // Send ENROLL_FINGERPRINT command
            Vibration.vibrate(50);
          },
        },
      ]
    );
  }, []);

  const handleDeleteFingerprint = useCallback(() => {
    Alert.alert(
      'Delete Fingerprint',
      'Select which fingerprint slot to delete.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete All',
          style: 'destructive',
          onPress: () => {
            // Send DELETE_ALL_FINGERPRINTS command
          },
        },
      ]
    );
  }, []);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.sectionTitle}>🔒 Security</Text>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>Require Fingerprint</Text>
        <Switch
          value={fingerprintRequired}
          onValueChange={setFingerprintRequired}
          trackColor={{ false: '#333', true: '#e94560' }}
          thumbColor="#fff"
        />
      </View>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>Lock on Remove</Text>
        <Switch
          value={lockOnRemove}
          onValueChange={setLockOnRemove}
          trackColor={{ false: '#333', true: '#e94560' }}
          thumbColor="#fff"
        />
      </View>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>Proximity Unlock</Text>
        <Switch
          value={proximityUnlock}
          onValueChange={setProximityUnlock}
          trackColor={{ false: '#333', true: '#0f3460' }}
          thumbColor="#fff"
        />
      </View>

      <Text style={styles.sectionTitle}>👆 Fingerprint Management</Text>

      <TouchableOpacity
        style={[styles.settingButton, styles.enrollButton]}
        onPress={handleEnrollFingerprint}
      >
        <Text style={styles.settingButtonText}>Enroll New Fingerprint</Text>
      </TouchableOpacity>

      <TouchableOpacity
        style={[styles.settingButton, styles.deleteButton]}
        onPress={handleDeleteFingerprint}
      >
        <Text style={styles.settingButtonText}>Delete Fingerprint</Text>
      </TouchableOpacity>

      <Text style={styles.sectionTitle}>📡 Connectivity</Text>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>NFC</Text>
        <Switch
          value={nfcEnabled}
          onValueChange={setNfcEnabled}
          trackColor={{ false: '#333', true: '#0f3460' }}
          thumbColor="#fff"
        />
      </View>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>Bluetooth LE</Text>
        <Switch
          value={bleEnabled}
          onValueChange={setBleEnabled}
          trackColor={{ false: '#333', true: '#0f3460' }}
          thumbColor="#fff"
        />
      </View>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>Auto-Connect</Text>
        <Switch
          value={autoConnect}
          onValueChange={setAutoConnect}
          trackColor={{ false: '#333', true: '#0f3460' }}
          thumbColor="#fff"
        />
      </View>

      <Text style={styles.sectionTitle}>💡 Notifications</Text>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>LED Notifications</Text>
        <Switch
          value={ledNotifications}
          onValueChange={setLedNotifications}
          trackColor={{ false: '#333', true: '#533483' }}
          thumbColor="#fff"
        />
      </View>

      <View style={styles.settingRow}>
        <Text style={styles.settingLabel}>Auth Timeout (seconds)</Text>
        <Text style={styles.settingValue}>{authTimeout}s</Text>
      </View>

      <View style={styles.spacer} />

      <Text style={styles.sectionTitle}>⚠️ Danger Zone</Text>

      <TouchableOpacity
        style={[styles.settingButton, styles.dangerButton]}
        onPress={handleFactoryReset}
      >
        <Text style={[styles.settingButtonText, styles.dangerText]}>
          Factory Reset
        </Text>
      </TouchableOpacity>

      <View style={styles.spacer} />

      <Text style={styles.versionText}>CyberGuard Firmware v1.0.0</Text>
      <Text style={styles.versionText}>App v1.0.0</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
  content: {
    padding: 16,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#e94560',
    marginTop: 16,
    marginBottom: 8,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
  },
  settingLabel: {
    fontSize: 16,
    color: '#ffffff',
  },
  settingValue: {
    fontSize: 16,
    color: '#a0a0b0',
  },
  settingButton: {
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    marginBottom: 8,
  },
  enrollButton: {
    backgroundColor: '#0f3460',
  },
  deleteButton: {
    backgroundColor: '#533483',
  },
  dangerButton: {
    backgroundColor: '#3a1a1a',
    borderWidth: 1,
    borderColor: '#e94560',
  },
  settingButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },
  dangerText: {
    color: '#e94560',
  },
  spacer: {
    height: 16,
  },
  versionText: {
    fontSize: 12,
    color: '#666',
    textAlign: 'center',
  },
});