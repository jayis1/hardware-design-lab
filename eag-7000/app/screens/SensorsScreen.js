/**
 * EAG-7000 — Sensor Data Screen
 *
 * Displays real-time data from I2C sensors connected through
 * the TCA9548A multiplexer. Shows temperature, humidity, pressure,
 * IMU, and light sensor data across 8 mux channels.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useContext } from 'react';
import { View, ScrollView, StyleSheet } from 'react-native';
import { Text, Card, ProgressBar, Divider } from 'react-native-paper';
import { BLEContext } from '../components/BLEProvider';
import { MSG_TYPE, SENSOR_TYPE, decodeSensorData, encodeSensorRequest } from '../utils/protocol';

// Channel labels matching hardware design (Phase 2 I2C mux)
const CHANNEL_LABELS = [
  'CH0 — Temperature (CPU)',     // Channel 0: On-board temp sensor
  'CH1 — Temperature (NPU)',     // Channel 1: Hailo-8 temp sensor
  'CH2 — Temperature (Board)',   // Channel 2: Board ambient temp
  'CH3 — Humidity',              // Channel 3: Humidity sensor
  'CH4 — Pressure',             // Channel 4: Barometric pressure
  'CH5 — IMU (6-axis)',          // Channel 5: Accelerometer + Gyroscope
  'CH6 — Ambient Light',         // Channel 6: Light sensor
  'CH7 — Power Monitor',         // Channel 7: Voltage + Current
];

const initialSensorData = Array.from({ length: 8 }, () => ({
  values: {},   // { sensorType: { value, timestamp } }
  active: false,
}));

function SensorsScreen() {
  const { connected, subscribeToCharacteristic, writeCharacteristic } = useContext(BLEContext);
  const [sensorData, setSensorData] = useState(initialSensorData);

  useEffect(() => {
    if (!connected) return;

    // Subscribe to sensor data notifications
    const unsubscribe = subscribeToCharacteristic((data) => {
      const bytes = new Uint8Array(data);
      if (bytes.length < 4) return;
      if (bytes[0] !== MSG_TYPE.SENSOR_DATA) return;

      const sensor = decodeSensorData(bytes);

      setSensorData(prev => {
        const updated = [...prev];
        updated[sensor.channel] = {
          ...updated[sensor.channel],
          active: true,
          values: {
            ...updated[sensor.channel].values,
            [sensor.type]: {
              value: sensor.value,
              timestamp: sensor.timestamp,
            },
          },
        };
        return updated;
      });
    });

    // Request sensor data from all channels
    const requestAllSensors = async () => {
      for (let ch = 0; ch < 8; ch++) {
        for (const type of [SENSOR_TYPE.TEMPERATURE, SENSOR_TYPE.HUMIDITY,
                            SENSOR_TYPE.PRESSURE, SENSOR_TYPE.VOLTAGE]) {
          const packet = encodeSensorRequest(ch, type);
          await writeCharacteristic(packet);
          await new Promise(r => setTimeout(r, 50));  // Throttle requests
        }
      }
    };

    requestAllSensors();

    // Poll sensors every 2 seconds
    const interval = setInterval(requestAllSensors, 2000);

    return () => {
      if (unsubscribe) unsubscribe();
      clearInterval(interval);
    };
  }, [connected, subscribeToCharacteristic, writeCharacteristic]);

  const formatValue = (type, value) => {
    switch (type) {
      case SENSOR_TYPE.TEMPERATURE:
        return `${(value / 100).toFixed(1)} °C`;
      case SENSOR_TYPE.HUMIDITY:
        return `${(value / 10).toFixed(1)} %RH`;
      case SENSOR_TYPE.PRESSURE:
        return `${(value / 10).toFixed(1)} hPa`;
      case SENSOR_TYPE.IMU_ACCEL:
        return `${value} mg`;
      case SENSOR_TYPE.IMU_GYRO:
        return `${value} mdps`;
      case SENSOR_TYPE.LIGHT:
        return `${value} lux`;
      case SENSOR_TYPE.VOLTAGE:
        return `${(value / 1000).toFixed(2)} V`;
      case SENSOR_TYPE.CURRENT:
        return `${(value / 1000).toFixed(2)} A`;
      default:
        return `${value}`;
    }
  };

  const getTempColor = (tempX100) => {
    const tempC = tempX100 / 100;
    if (tempC < 40) return '#4CAF50';
    if (tempC < 60) return '#FFC107';
    if (tempC < 80) return '#FF9800';
    return '#F44336';
  };

  const renderChannel = (channel) => {
    const data = sensorData[channel];
    const label = CHANNEL_LABELS[channel];
    const tempData = data.values[SENSOR_TYPE.TEMPERATURE];

    return (
      <Card key={channel} style={styles.card}>
        <Card.Title
          title={`Mux Channel ${channel}`}
          subtitle={label}
          titleStyle={{ color: '#E0E0E0', fontSize: 14 }}
          subtitleStyle={{ color: '#B0B0B0', fontSize: 11 }}
        />
        <Card.Content>
          {Object.entries(data.values).map(([type, entry]) => (
            <View key={type} style={styles.sensorRow}>
              <Text style={styles.sensorType}>
                {SENSOR_TYPE_LABELS[type] || `Type ${type}`}
              </Text>
              <Text style={[
                styles.sensorValue,
                { color: Number(type) === SENSOR_TYPE.TEMPERATURE
                  ? getTempColor(entry.value)
                  : '#E0E0E0' }
              ]}>
                {formatValue(Number(type), entry.value)}
              </Text>
              <Text style={styles.sensorTime}>
                {new Date(entry.timestamp).toLocaleTimeString()}
              </Text>
            </View>
          ))}
          {!data.active && (
            <Text style={styles.noData}>No data received</Text>
          )}
        </Card.Content>
      </Card>
    );
  };

  return (
    <ScrollView style={styles.container}>
      {Array.from({ length: 8 }, (_, i) => renderChannel(i))}
    </ScrollView>
  );
}

// Sensor type labels for display
const SENSOR_TYPE_LABELS = {
  [SENSOR_TYPE.TEMPERATURE]: 'Temperature',
  [SENSOR_TYPE.HUMIDITY]: 'Humidity',
  [SENSOR_TYPE.PRESSURE]: 'Pressure',
  [SENSOR_TYPE.IMU_ACCEL]: 'Accel',
  [SENSOR_TYPE.IMU_GYRO]: 'Gyro',
  [SENSOR_TYPE.LIGHT]: 'Light',
  [SENSOR_TYPE.VOLTAGE]: 'Voltage',
  [SENSOR_TYPE.CURRENT]: 'Current',
};

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
  sensorRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 4,
  },
  sensorType: {
    color: '#B0B0B0',
    fontSize: 13,
    flex: 1,
  },
  sensorValue: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: 'bold',
    flex: 1,
    textAlign: 'right',
  },
  sensorTime: {
    color: '#757575',
    fontSize: 10,
    width: 70,
    textAlign: 'right',
  },
  noData: {
    color: '#757575',
    fontSize: 12,
    textAlign: 'center',
    paddingVertical: 8,
  },
});

export default SensorsScreen;