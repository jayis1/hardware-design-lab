/**
 * SkyPilot — Settings Screen
 * Connection configuration, calibration, firmware update
 */

import React, { useState, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TextInput,
  TouchableOpacity,
  Switch,
  ScrollView,
  Alert,
} from 'react-native';
import { ConnectionContext } from '../components/ConnectionContext';
import { Colors, Typography, Spacing } from '../utils/theme';
import { Commands } from '../utils/protocol';

function SettingsScreen() {
  const { connection, connect, disconnect, sendMessage } = useContext(ConnectionContext);
  const [host, setHost] = useState('skypilot.local');
  const [port, setPort] = useState('14550');
  const [imu1Range, setImu1Range] = useState('±16g / ±2000°/s');
  const [imu1Rate, setImu1Rate] = useState('8 kHz');
  const [lteApn, setLteApn] = useState('');
  const [lteServer, setLteServer] = useState('telemetry.skypilot.io');
  const [ltePort, setLtePort] = useState('14550');
  const [dshotMode, setDshotMode] = useState('DShot1200');
  const [beeperEnabled, setBeeperEnabled] = useState(true);
  const [gpsBaud, setGpsBaud] = useState('460800');
  const [firmwareVersion] = useState('1.0.0');

  const handleConnect = () => {
    if (connection.connected) {
      disconnect();
    } else {
      connect(host, parseInt(port, 10));
    }
  };

  const handleCalibrateIMU = () => {
    Alert.alert(
      'Calibrate IMU',
      'Place the drone on a flat, level surface. Hold still for 5 seconds.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start Calibration',
          onPress: () => {
            sendMessage({ cmd: Commands.CALIBRATE_IMU, payload: { imu: 'both' } });
          },
        },
      ]
    );
  };

  const handleCalibrateBaro = () => {
    Alert.alert(
      'Calibrate Barometer',
      'Calibrate barometer to current sea-level pressure?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Calibrate',
          onPress: () => {
            sendMessage({ cmd: Commands.CALIBRATE_BARO, payload: {} });
          },
        },
      ]
    );
  };

  const handleCalibrateESC = () => {
    Alert.alert(
      'ESC Calibration',
      'WARNING: Remove propellers before ESC calibration!',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start Calibration',
          style: 'destructive',
          onPress: () => {
            sendMessage({ cmd: Commands.CALIBRATE_ESC, payload: { mode: dshotMode } });
          },
        },
      ]
    );
  };

  const handleLTEConnect = () => {
    sendMessage({
      cmd: Commands.LTE_CONNECT,
      payload: {
        apn: lteApn,
        server: lteServer,
        port: parseInt(ltePort, 10),
      },
    });
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection Settings */}
      <Text style={styles.sectionTitle}>Connection</Text>
      <View style={styles.card}>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Host</Text>
          <TextInput
            style={styles.input}
            value={host}
            onChangeText={setHost}
            placeholder="skypilot.local"
            placeholderTextColor={Colors.textTertiary}
            autoCapitalize="none"
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Port</Text>
          <TextInput
            style={styles.input}
            value={port}
            onChangeText={setPort}
            placeholder="14550"
            placeholderTextColor={Colors.textTertiary}
            keyboardType="numeric"
          />
        </View>
        <TouchableOpacity
          style={[
            styles.connectButton,
            { backgroundColor: connection.connected ? Colors.error : Colors.primary },
          ]}
          onPress={handleConnect}
        >
          <Text style={styles.connectButtonText}>
            {connection.connected ? 'Disconnect' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* IMU Settings */}
      <Text style={styles.sectionTitle}>IMU Configuration</Text>
      <View style={styles.card}>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>IMU1 Range</Text>
          <Text style={styles.inputValue}>{imu1Range}</Text>
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>IMU1 Rate</Text>
          <Text style={styles.inputValue}>{imu1Rate}</Text>
        </View>
        <TouchableOpacity style={styles.actionButton} onPress={handleCalibrateIMU}>
          <Text style={styles.actionButtonText}>Calibrate IMU</Text>
        </TouchableOpacity>
      </View>

      {/* Barometer */}
      <Text style={styles.sectionTitle}>Barometer</Text>
      <View style={styles.card}>
        <TouchableOpacity style={styles.actionButton} onPress={handleCalibrateBaro}>
          <Text style={styles.actionButtonText}>Calibrate Barometer</Text>
        </TouchableOpacity>
      </View>

      {/* Motor Output */}
      <Text style={styles.sectionTitle}>Motor Output</Text>
      <View style={styles.card}>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Protocol</Text>
          <Text style={styles.inputValue}>{dshotMode}</Text>
        </View>
        <TouchableOpacity style={styles.actionButton} onPress={handleCalibrateESC}>
          <Text style={styles.actionButtonText}>ESC Calibration</Text>
        </TouchableOpacity>
      </View>

      {/* LTE Telemetry */}
      <Text style={styles.sectionTitle}>4G/LTE Telemetry</Text>
      <View style={styles.card}>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>APN</Text>
          <TextInput
            style={styles.input}
            value={lteApn}
            onChangeText={setLteApn}
            placeholder="Auto"
            placeholderTextColor={Colors.textTertiary}
            autoCapitalize="none"
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Server</Text>
          <TextInput
            style={styles.input}
            value={lteServer}
            onChangeText={setLteServer}
            placeholder="telemetry.skypilot.io"
            placeholderTextColor={Colors.textTertiary}
            autoCapitalize="none"
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Port</Text>
          <TextInput
            style={styles.input}
            value={ltePort}
            onChangeText={setLtePort}
            placeholder="14550"
            placeholderTextColor={Colors.textTertiary}
            keyboardType="numeric"
          />
        </View>
        <TouchableOpacity style={styles.actionButton} onPress={handleLTEConnect}>
          <Text style={styles.actionButtonText}>Connect LTE</Text>
        </TouchableOpacity>
      </View>

      {/* GNSS */}
      <Text style={styles.sectionTitle}>GNSS</Text>
      <View style={styles.card}>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Baud Rate</Text>
          <Text style={styles.inputValue}>{gpsBaud}</Text>
        </View>
      </View>

      {/* Misc */}
      <Text style={styles.sectionTitle}>Miscellaneous</Text>
      <View style={styles.card}>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Beeper</Text>
          <Switch
            value={beeperEnabled}
            onValueChange={setBeeperEnabled}
            trackColor={{ false: Colors.border, true: Colors.primary }}
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Firmware</Text>
          <Text style={styles.inputValue}>v{firmwareVersion}</Text>
        </View>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: Colors.background,
  },
  sectionTitle: {
    ...Typography.h4,
    color: Colors.textPrimary,
    marginTop: Spacing.md,
    marginBottom: Spacing.xs,
    paddingHorizontal: Spacing.md,
  },
  card: {
    marginHorizontal: Spacing.md,
    padding: Spacing.md,
    backgroundColor: Colors.surface,
    borderRadius: 8,
  },
  inputRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: Spacing.xs,
    borderBottomWidth: 1,
    borderBottomColor: Colors.border,
  },
  inputLabel: {
    ...Typography.body,
    color: Colors.textSecondary,
  },
  inputValue: {
    ...Typography.body,
    color: Colors.textPrimary,
  },
  input: {
    ...Typography.body,
    color: Colors.textPrimary,
    textAlign: 'right',
    flex: 1,
    marginLeft: Spacing.md,
  },
  connectButton: {
    marginTop: Spacing.md,
    paddingVertical: Spacing.md,
    borderRadius: 8,
    alignItems: 'center',
  },
  connectButtonText: {
    ...Typography.h4,
    color: '#fff',
  },
  actionButton: {
    marginTop: Spacing.sm,
    paddingVertical: Spacing.sm,
    borderRadius: 8,
    backgroundColor: Colors.primary,
    alignItems: 'center',
  },
  actionButtonText: {
    ...Typography.body,
    color: '#fff',
  },
});

export default SettingsScreen;