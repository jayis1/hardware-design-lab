/**
 * SkyPilot — Telemetry Screen
 * Shows real-time sensor data, LTE connection stats, and logs
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  FlatList,
} from 'react-native';
import { ConnectionContext } from '../components/ConnectionContext';
import { SensorCard } from '../components/SensorCard';
import { Colors, Typography, Spacing } from '../utils/theme';
import { Commands } from '../utils/protocol';

function TelemetryScreen() {
  const { connection, sendMessage } = useContext(ConnectionContext);
  const [sensors, setSensors] = useState({
    imu1: { ax: 0, ay: 0, az: 0, gx: 0, gy: 0, gz: 0, temp: 25 },
    imu2: { ax: 0, ay: 0, az: 0, gx: 0, gy: 0, gz: 0, temp: 25 },
    baro: { pressure: 1013.25, altitude: 0, temp: 25 },
    gnss: { lat: 0, lon: 0, alt: 0, satellites: 0, fix: 'None', hdop: 99 },
  });
  const [lteInfo, setLteInfo] = useState({
    connected: false,
    operator: '',
    rssi: -100,
    rsrp: -100,
    rsrq: -20,
    technology: 'No Service',
    uptime: 0,
    dataRate: { up: 0, down: 0 },
  });
  const [logs, setLogs] = useState([]);

  useEffect(() => {
    if (!connection.connected) return;

    const interval = setInterval(() => {
      sendMessage({
        cmd: Commands.TELEMETRY_REQUEST,
        payload: { fields: ['imu1', 'imu2', 'baro', 'gnss', 'lte'] },
      });
    }, 200); // 5 Hz

    return () => clearInterval(interval);
  }, [connection.connected]);

  useEffect(() => {
    if (!connection.connected) return;

    const unsubscribe = connection.onMessage((msg) => {
      const timestamp = new Date().toLocaleTimeString();

      switch (msg.cmd) {
        case Commands.IMU1_REPORT:
          setSensors((prev) => ({ ...prev, imu1: msg.payload }));
          break;
        case Commands.IMU2_REPORT:
          setSensors((prev) => ({ ...prev, imu2: msg.payload }));
          break;
        case Commands.BARO_REPORT:
          setSensors((prev) => ({ ...prev, baro: msg.payload }));
          break;
        case Commands.GPS_REPORT:
          setSensors((prev) => ({ ...prev, gnss: msg.payload }));
          break;
        case Commands.LTE_REPORT:
          setLteInfo(msg.payload);
          break;
      }

      setLogs((prev) => {
        const newLog = {
          id: Date.now().toString(),
          time: timestamp,
          cmd: msg.cmd,
          payload: msg.payload,
        };
        return [newLog, ...prev].slice(0, 100);
      });
    });

    return unsubscribe;
  }, [connection.connected]);

  return (
    <ScrollView style={styles.container}>
      {/* Primary IMU */}
      <Text style={styles.sectionTitle}>IMU 1 (ICM-42688-P)</Text>
      <View style={styles.sensorRow}>
        <SensorCard label="Accel X" value={sensors.imu1.ax.toFixed(3)} unit="g" />
        <SensorCard label="Accel Y" value={sensors.imu1.ay.toFixed(3)} unit="g" />
        <SensorCard label="Accel Z" value={sensors.imu1.az.toFixed(3)} unit="g" />
      </View>
      <View style={styles.sensorRow}>
        <SensorCard label="Gyro X" value={sensors.imu1.gx.toFixed(2)} unit="°/s" />
        <SensorCard label="Gyro Y" value={sensors.imu1.gy.toFixed(2)} unit="°/s" />
        <SensorCard label="Gyro Z" value={sensors.imu1.gz.toFixed(2)} unit="°/s" />
      </View>

      {/* Secondary IMU */}
      <Text style={styles.sectionTitle}>IMU 2 (BMI270)</Text>
      <View style={styles.sensorRow}>
        <SensorCard label="Accel X" value={sensors.imu2.ax.toFixed(3)} unit="g" />
        <SensorCard label="Accel Y" value={sensors.imu2.ay.toFixed(3)} unit="g" />
        <SensorCard label="Accel Z" value={sensors.imu2.az.toFixed(3)} unit="g" />
      </View>
      <View style={styles.sensorRow}>
        <SensorCard label="Gyro X" value={sensors.imu2.gx.toFixed(2)} unit="°/s" />
        <SensorCard label="Gyro Y" value={sensors.imu2.gy.toFixed(2)} unit="°/s" />
        <SensorCard label="Gyro Z" value={sensors.imu2.gz.toFixed(2)} unit="°/s" />
      </View>

      {/* Barometer */}
      <Text style={styles.sectionTitle}>Barometer (BMP390)</Text>
      <View style={styles.sensorRow}>
        <SensorCard label="Pressure" value={sensors.baro.pressure.toFixed(2)} unit="hPa" />
        <SensorCard label="Altitude" value={sensors.baro.altitude.toFixed(1)} unit="m" />
        <SensorCard label="Temp" value={sensors.baro.temp.toFixed(1)} unit="°C" />
      </View>

      {/* GNSS */}
      <Text style={styles.sectionTitle}>GNSS (SAM-M10Q)</Text>
      <View style={styles.sensorRow}>
        <SensorCard label="Latitude" value={sensors.gnss.lat.toFixed(6)} unit="°" />
        <SensorCard label="Longitude" value={sensors.gnss.lon.toFixed(6)} unit="°" />
        <SensorCard label="Altitude" value={sensors.gnss.alt.toFixed(1)} unit="m" />
      </View>
      <View style={styles.sensorRow}>
        <SensorCard label="Satellites" value={sensors.gnss.satellites.toString()} unit="" />
        <SensorCard label="Fix" value={sensors.gnss.fix} unit="" />
        <SensorCard label="HDOP" value={sensors.gnss.hdop.toFixed(1)} unit="" />
      </View>

      {/* LTE */}
      <Text style={styles.sectionTitle}>4G/LTE (LARA-R6)</Text>
      <View style={styles.lteCard}>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>Status</Text>
          <Text style={[
            styles.lteValue,
            { color: lteInfo.connected ? Colors.success : Colors.error },
          ]}>
            {lteInfo.connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>Operator</Text>
          <Text style={styles.lteValue}>{lteInfo.operator || 'N/A'}</Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>Technology</Text>
          <Text style={styles.lteValue}>{lteInfo.technology}</Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>RSSI</Text>
          <Text style={styles.lteValue}>{lteInfo.rssi} dBm</Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>RSRP</Text>
          <Text style={styles.lteValue}>{lteInfo.rsrp} dBm</Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>RSRQ</Text>
          <Text style={styles.lteValue}>{lteInfo.rsrq} dB</Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>Uplink</Text>
          <Text style={styles.lteValue}>{lteInfo.dataRate.up.toFixed(1)} kbps</Text>
        </View>
        <View style={styles.lteRow}>
          <Text style={styles.lteLabel}>Downlink</Text>
          <Text style={styles.lteValue}>{lteInfo.dataRate.down.toFixed(1)} kbps</Text>
        </View>
      </View>

      {/* Recent Logs */}
      <Text style={styles.sectionTitle}>Recent Telemetry</Text>
      <FlatList
        data={logs.slice(0, 20)}
        keyExtractor={(item) => item.id}
        renderItem={({ item }) => (
          <View style={styles.logItem}>
            <Text style={styles.logTime}>{item.time}</Text>
            <Text style={styles.logCmd}>CMD 0x{item.cmd.toString(16).padStart(2, '0')}</Text>
          </View>
        )}
        style={styles.logList}
      />
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
  sensorRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingHorizontal: Spacing.sm,
    marginBottom: Spacing.xs,
  },
  lteCard: {
    margin: Spacing.md,
    padding: Spacing.md,
    backgroundColor: Colors.surface,
    borderRadius: 8,
  },
  lteRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: Spacing.xs,
    borderBottomWidth: 1,
    borderBottomColor: Colors.border,
  },
  lteLabel: {
    ...Typography.body,
    color: Colors.textSecondary,
  },
  lteValue: {
    ...Typography.body,
    color: Colors.textPrimary,
  },
  logList: {
    maxHeight: 200,
    marginHorizontal: Spacing.md,
  },
  logItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: Spacing.xs,
    borderBottomWidth: 1,
    borderBottomColor: Colors.border,
  },
  logTime: {
    ...Typography.caption,
    color: Colors.textSecondary,
  },
  logCmd: {
    ...Typography.caption,
    color: Colors.primary,
    fontFamily: 'monospace',
  },
});

export default TelemetryScreen;