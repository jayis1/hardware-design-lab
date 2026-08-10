/**
 * @file    SettingsScreen.js
 * @brief   App settings: device info, firmware version, calibration,
 *          units, BLE management, and OTA firmware update.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Switch, Alert,
  ActivityIndicator,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { useTideBand } from '../services/TideBandContext';
import { OP } from '../utils/protocol';

export default function SettingsScreen() {
  const {
    connected, device, status, disconnect, units, setUnits,
    diveCount, eraseDives, sendCommand,
  } = useTideBand();

  const [firmwareVersion, setFirmwareVersion] = useState('Unknown');
  const [calibrationDate, setCalibrationDate] = useState('Unknown');
  const [otaProgress, setOtaProgress] = useState(0);
  const [otaActive, setOtaActive] = useState(false);

  // Load saved settings
  useEffect(() => {
    loadSettings();
    if (connected) {
      // Request device info
      sendCommand(OP.GET_INFO);
    }
  }, [connected]);

  const loadSettings = async () => {
    try {
      const savedUnits = await AsyncStorage.getItem('@tideband_units');
      if (savedUnits) {
        setUnits(savedUnits);
      }
    } catch (err) {
      console.error('Failed to load settings:', err);
    }
  };

  const toggleUnits = async (isImperial) => {
    const newUnits = isImperial ? 'imperial' : 'metric';
    setUnits(newUnits);
    try {
      await AsyncStorage.setItem('@tideband_units', newUnits);
    } catch (err) {
      console.error('Failed to save units:', err);
    }
  };

  const handleEraseDives = () => {
    Alert.alert(
      'Erase All Dive Data',
      'This will permanently delete all stored dives on the device. This cannot be undone.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Erase',
          style: 'destructive',
          onPress: () => {
            eraseDives();
            Alert.alert('Done', 'All dive data has been erased.');
          },
        },
      ]
    );
  };

  const handleStartOTA = () => {
    Alert.alert(
      'Firmware Update',
      'OTA firmware update will begin. Do not disconnect during update.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start Update',
          onPress: () => {
            setOtaActive(true);
            setOtaProgress(0);
            // In a real implementation, this would:
            // 1. Send OTA_BEGIN command
            // 2. Read firmware file from app storage
            // 3. Send chunks via OTA_CHUNK
            // 4. Wait for OTA_ACK after each chunk
            // 5. Send OTA_END when complete
            simulateOTA();
          },
        },
      ]
    );
  };

  const simulateOTA = () => {
    // Simulated OTA progress
    const interval = setInterval(() => {
      setOtaProgress(prev => {
        if (prev >= 100) {
          clearInterval(interval);
          setOtaActive(false);
          Alert.alert('Update Complete', 'Firmware updated successfully.');
          return 100;
        }
        return prev + 5;
      });
    }, 200);
  };

  const handleDisconnect = () => {
    Alert.alert(
      'Disconnect',
      'Disconnect from TideBand?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Disconnect', onPress: () => disconnect() },
      ]
    );
  };

  return (
    <View style={styles.container}>
      {/* Connection Status */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Status</Text>
          <Text style={[
            styles.infoValue,
            { color: connected ? '#00AA00' : '#FF4444' }
          ]}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {device && (
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Device</Text>
            <Text style={styles.infoValue}>{device.name || 'TideBand'}</Text>
          </View>
        )}
        {connected && (
          <TouchableOpacity
            style={styles.disconnectButton}
            onPress={handleDisconnect}
          >
            <Text style={styles.disconnectButtonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Device Info */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device Information</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Firmware</Text>
          <Text style={styles.infoValue}>{firmwareVersion}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Battery</Text>
          <Text style={styles.infoValue}>
            {status ? `${status.batteryPct}%` : '—'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Dives Stored</Text>
          <Text style={styles.infoValue}>{diveCount}</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Calibration</Text>
          <Text style={styles.infoValue}>{calibrationDate}</Text>
        </View>
      </View>

      {/* Units */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Display Units</Text>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Metric (m/s, m, °C)</Text>
          <Switch
            value={false}
            onValueChange={() => toggleUnits(false)}
            trackColor={{ false: '#0080FF', true: '#E0E0E0' }}
            disabled={units === 'imperial'}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Imperial (kn, ft, °F)</Text>
          <Switch
            value={true}
            onValueChange={() => toggleUnits(true)}
            trackColor={{ false: '#E0E0E0', true: '#0080FF' }}
            disabled={units === 'metric'}
          />
        </View>
      </View>

      {/* Firmware Update */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Firmware Update (OTA)</Text>
        <Text style={styles.description}>
          Update the TideBand firmware over BLE. Ensure the device is
          charged above 30% and do not disconnect during the update.
        </Text>
        {otaActive ? (
          <View style={styles.otaProgressContainer}>
            <ActivityIndicator size="small" color="#0080FF" />
            <Text style={styles.otaProgressText}>
              Updating... {otaProgress}%
            </Text>
          </View>
        ) : (
          <TouchableOpacity
            style={[styles.actionButton,
              !connected && styles.actionButtonDisabled]}
            onPress={handleStartOTA}
            disabled={!connected}
          >
            <Text style={styles.actionButtonText}>Check for Updates</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Data Management */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Data Management</Text>
        <Text style={styles.description}>
          Erase all stored dive data from the device flash storage.
        </Text>
        <TouchableOpacity
          style={[styles.dangerButton,
            !connected && styles.actionButtonDisabled]}
          onPress={handleEraseDives}
          disabled={!connected}
        >
          <Text style={styles.dangerButtonText}>Erase All Dives</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>About</Text>
        <Text style={styles.aboutText}>TideBand Companion App</Text>
        <Text style={styles.aboutText}>Version 1.0.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>© 2026 jayis1. All rights reserved.</Text>
        <Text style={styles.aboutText}>License: MIT</Text>
        <Text style={styles.aboutLink}>
          Open hardware — schematics and firmware at github.com/jayis1/tideband
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F0F0F0',
    padding: 16,
  },
  card: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333333',
    marginBottom: 12,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  infoLabel: {
    fontSize: 14,
    color: '#808080',
  },
  infoValue: {
    fontSize: 14,
    fontWeight: '600',
    color: '#333333',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  switchLabel: {
    fontSize: 14,
    color: '#333333',
  },
  description: {
    fontSize: 13,
    color: '#808080',
    marginBottom: 12,
    lineHeight: 18,
  },
  actionButton: {
    backgroundColor: '#0080FF',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  actionButtonDisabled: {
    backgroundColor: '#CCCCCC',
  },
  actionButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: 'bold',
  },
  dangerButton: {
    backgroundColor: '#FF4444',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  dangerButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: 'bold',
  },
  disconnectButton: {
    backgroundColor: '#FF8800',
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  disconnectButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: 'bold',
  },
  otaProgressContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
  },
  otaProgressText: {
    fontSize: 14,
    color: '#0080FF',
    marginLeft: 12,
  },
  aboutText: {
    fontSize: 13,
    color: '#808080',
    paddingVertical: 2,
  },
  aboutLink: {
    fontSize: 12,
    color: '#0080FF',
    marginTop: 8,
  },
});