/**
 * SkyPilot — Flight Control Screen
 * Shows real-time attitude, motor status, armed state
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
} from 'react-native';
import { ConnectionContext } from '../components/ConnectionContext';
import { AttitudeIndicator } from '../components/AttitudeIndicator';
import { Colors, Typography, Spacing } from '../utils/theme';
import { Commands } from '../utils/protocol';

function FlightScreen() {
  const { connection, sendMessage } = useContext(ConnectionContext);
  const [armed, setArmed] = useState(false);
  const [flightMode, setFlightMode] = useState('STABILIZE');
  const [attitude, setAttitude] = useState({
    roll: 0,
    pitch: 0,
    yaw: 0,
  });
  const [motors, setMotors] = useState([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
  const [battery, setBattery] = useState({
    voltage: 0,
    current: 0,
    percentage: 100,
  });
  const [gpsFix, setGpsFix] = useState(false);
  const [lteSignal, setLteSignal] = useState(-100);

  useEffect(() => {
    if (!connection.connected) return;

    const interval = setInterval(() => {
      // Request telemetry update
      sendMessage({
        cmd: Commands.TELEMETRY_REQUEST,
        payload: { fields: ['attitude', 'motors', 'battery', 'gps', 'lte'] },
      });
    }, 100); // 10 Hz telemetry request

    return () => clearInterval(interval);
  }, [connection.connected]);

  useEffect(() => {
    if (!connection.connected) return;

    const unsubscribe = connection.onMessage((msg) => {
      switch (msg.cmd) {
        case Commands.ATTITUDE_REPORT:
          setAttitude({
            roll: msg.payload.roll_deg,
            pitch: msg.payload.pitch_deg,
            yaw: msg.payload.yaw_deg,
          });
          break;
        case Commands.MOTOR_REPORT:
          setMotors(msg.payload.motors);
          break;
        case Commands.BATTERY_REPORT:
          setBattery(msg.payload);
          break;
        case Commands.GPS_REPORT:
          setGpsFix(msg.payload.fix_type >= 3);
          break;
        case Commands.LTE_REPORT:
          setLteSignal(msg.payload.rsrp);
          break;
      }
    });

    return unsubscribe;
  }, [connection.connected]);

  const handleArm = () => {
    if (armed) {
      Alert.alert(
        'Disarm',
        'Are you sure you want to disarm the vehicle?',
        [
          { text: 'Cancel', style: 'cancel' },
          {
            text: 'Disarm',
            style: 'destructive',
            onPress: () => {
              sendMessage({ cmd: Commands.DISARM, payload: {} });
              setArmed(false);
            },
          },
        ]
      );
    } else {
      Alert.alert(
        'Arm',
        'Arm the vehicle? Ensure propellers are clear!',
        [
          { text: 'Cancel', style: 'cancel' },
          {
            text: 'Arm',
            style: 'default',
            onPress: () => {
              sendMessage({ cmd: Commands.ARM, payload: {} });
              setArmed(true);
            },
          },
        ]
      );
    }
  };

  const cycleFlightMode = () => {
    const modes = ['STABILIZE', 'ALTITUDE', 'POSITION', 'LOITER', 'AUTO', 'RTL'];
    const idx = modes.indexOf(flightMode);
    const next = modes[(idx + 1) % modes.length];
    setFlightMode(next);
    sendMessage({ cmd: Commands.SET_MODE, payload: { mode: next } });
  };

  const batteryColor = battery.percentage > 50
    ? Colors.success
    : battery.percentage > 20
    ? Colors.warning
    : Colors.error;

  return (
    <ScrollView style={styles.container}>
      {/* Connection Status Bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <View
            style={[
              styles.statusDot,
              { backgroundColor: connection.connected ? Colors.success : Colors.error },
            ]}
          />
          <Text style={styles.statusText}>
            {connection.connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusText}>
            LTE: {lteSignal} dBm
          </Text>
        </View>
        <View style={styles.statusItem}>
          <View
            style={[
              styles.statusDot,
              { backgroundColor: gpsFix ? Colors.success : Colors.inactive },
            ]}
          />
          <Text style={styles.statusText}>
            GPS {gpsFix ? '3D' : 'No Fix'}
          </Text>
        </View>
      </View>

      {/* Attitude Indicator */}
      <View style={styles.attitudeContainer}>
        <AttitudeIndicator
          roll={attitude.roll}
          pitch={attitude.pitch}
          heading={attitude.yaw}
        />
      </View>

      {/* Flight Data */}
      <View style={styles.dataRow}>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Roll</Text>
          <Text style={styles.dataValue}>{attitude.roll.toFixed(1)}°</Text>
        </View>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Pitch</Text>
          <Text style={styles.dataValue}>{attitude.pitch.toFixed(1)}°</Text>
        </View>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Yaw</Text>
          <Text style={styles.dataValue}>{attitude.yaw.toFixed(1)}°</Text>
        </View>
      </View>

      {/* Battery */}
      <View style={styles.batteryCard}>
        <View style={styles.batteryRow}>
          <Text style={styles.batteryLabel}>Battery</Text>
          <Text style={[styles.batteryValue, { color: batteryColor }]}>
            {battery.voltage.toFixed(1)}V / {battery.percentage}%
          </Text>
          <Text style={styles.batteryCurrent}>
            {battery.current.toFixed(1)}A
          </Text>
        </View>
        <View style={styles.batteryBar}>
          <View
            style={[
              styles.batteryFill,
              {
                width: `${battery.percentage}%`,
                backgroundColor: batteryColor,
              },
            ]}
          />
        </View>
      </View>

      {/* Motor Status */}
      <View style={styles.motorGrid}>
        <Text style={styles.sectionTitle}>Motors</Text>
        <View style={styles.motorRow}>
          {motors.slice(0, 4).map((throttle, i) => (
            <View key={i} style={styles.motorCell}>
              <Text style={styles.motorLabel}>M{i + 1}</Text>
              <Text style={styles.motorValue}>{throttle}%</Text>
            </View>
          ))}
        </View>
        <View style={styles.motorRow}>
          {motors.slice(4, 8).map((throttle, i) => (
            <View key={i + 4} style={styles.motorCell}>
              <Text style={styles.motorLabel}>M{i + 5}</Text>
              <Text style={styles.motorValue}>{throttle}%</Text>
            </View>
          ))}
        </View>
        <View style={styles.motorRow}>
          {motors.slice(8, 12).map((throttle, i) => (
            <View key={i + 8} style={styles.motorCell}>
              <Text style={styles.motorLabel}>M{i + 9}</Text>
              <Text style={styles.motorValue}>{throttle}%</Text>
            </View>
          ))}
        </View>
      </View>

      {/* Controls */}
      <View style={styles.controlRow}>
        <TouchableOpacity
          style={[
            styles.armButton,
            { backgroundColor: armed ? Colors.error : Colors.success },
          ]}
          onPress={handleArm}
        >
          <Text style={styles.armButtonText}>
            {armed ? 'DISARM' : 'ARM'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.modeButton}
          onPress={cycleFlightMode}
        >
          <Text style={styles.modeButtonText}>{flightMode}</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: Colors.background,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: Spacing.sm,
    backgroundColor: Colors.surface,
    borderBottomWidth: 1,
    borderBottomColor: Colors.border,
  },
  statusItem: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  statusText: {
    ...Typography.caption,
    color: Colors.textSecondary,
  },
  attitudeContainer: {
    alignItems: 'center',
    padding: Spacing.md,
  },
  dataRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    padding: Spacing.sm,
  },
  dataCard: {
    backgroundColor: Colors.surface,
    padding: Spacing.md,
    borderRadius: 8,
    alignItems: 'center',
    minWidth: 90,
  },
  dataLabel: {
    ...Typography.caption,
    color: Colors.textSecondary,
  },
  dataValue: {
    ...Typography.h3,
    color: Colors.textPrimary,
  },
  batteryCard: {
    margin: Spacing.md,
    padding: Spacing.md,
    backgroundColor: Colors.surface,
    borderRadius: 8,
  },
  batteryRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: Spacing.xs,
  },
  batteryLabel: {
    ...Typography.body,
    color: Colors.textPrimary,
  },
  batteryValue: {
    ...Typography.h3,
  },
  batteryCurrent: {
    ...Typography.caption,
    color: Colors.textSecondary,
  },
  batteryBar: {
    height: 6,
    backgroundColor: Colors.border,
    borderRadius: 3,
    overflow: 'hidden',
  },
  batteryFill: {
    height: '100%',
    borderRadius: 3,
  },
  motorGrid: {
    margin: Spacing.md,
  },
  sectionTitle: {
    ...Typography.h4,
    color: Colors.textPrimary,
    marginBottom: Spacing.sm,
  },
  motorRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: Spacing.xs,
  },
  motorCell: {
    backgroundColor: Colors.surface,
    padding: Spacing.xs,
    borderRadius: 4,
    alignItems: 'center',
    minWidth: 60,
  },
  motorLabel: {
    ...Typography.caption,
    color: Colors.textSecondary,
  },
  motorValue: {
    ...Typography.body,
    color: Colors.primary,
  },
  controlRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    padding: Spacing.lg,
  },
  armButton: {
    paddingHorizontal: Spacing.xl,
    paddingVertical: Spacing.md,
    borderRadius: 8,
    minWidth: 120,
    alignItems: 'center',
  },
  armButtonText: {
    ...Typography.h3,
    color: '#fff',
  },
  modeButton: {
    paddingHorizontal: Spacing.xl,
    paddingVertical: Spacing.md,
    borderRadius: 8,
    backgroundColor: Colors.surface,
    borderWidth: 1,
    borderColor: Colors.primary,
    minWidth: 120,
    alignItems: 'center',
  },
  modeButtonText: {
    ...Typography.h4,
    color: Colors.primary,
  },
});

export default FlightScreen;