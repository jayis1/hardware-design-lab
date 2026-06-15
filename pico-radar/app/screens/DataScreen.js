/**
 * screens/DataScreen.js — IMU data view and session logging
 *
 * Shows real-time IMU readings, point cloud statistics,
 * and provides data export functionality.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  FlatList,
} from 'react-native';
import { protocol } from '../../App';
import ImuGauge from '../components/ImuGauge';

export default function DataScreen() {
  const [connected, setConnected] = useState(false);
  const [imuData, setImuData] = useState({
    accelX: 0, accelY: 0, accelZ: 9.81,
    gyroX: 0, gyroY: 0, gyroZ: 0,
    temperature: 25.0,
  });
  const [sessionStats, setSessionStats] = useState({
    totalPoints: 0,
    totalFrames: 0,
    avgPointsPerFrame: 0,
    maxRange: 0,
    maxVelocity: 0,
    duration: 0,
  });
  const [logEntries, setLogEntries] = useState([]);

  const handleImuData = useCallback((data) => {
    setImuData(data);
  }, []);

  const handleSessionStats = useCallback((stats) => {
    setSessionStats(stats);
  }, []);

  useEffect(() => {
    protocol.onImuData(handleImuData);
    protocol.onSessionStats(handleSessionStats);
    protocol.onConnectionChange((isConnected) => {
      setConnected(isConnected);
    });

    return () => {
      protocol.offImuData(handleImuData);
      protocol.offSessionStats(handleSessionStats);
    };
  }, [handleImuData, handleSessionStats]);

  const startLogging = () => {
    protocol.sendCommand(ProtocolCommands.CMD_LOG_START);
    addLogEntry('Logging started');
  };

  const stopLogging = () => {
    protocol.sendCommand(ProtocolCommands.CMD_LOG_STOP);
    addLogEntry('Logging stopped');
  };

  const exportData = () => {
    protocol.sendCommand(ProtocolCommands.CMD_EXPORT_CSV);
    addLogEntry('Export requested');
  };

  const addLogEntry = (message) => {
    const entry = {
      id: Date.now().toString(),
      time: new Date().toLocaleTimeString(),
      message,
    };
    setLogEntries((prev) => [entry, ...prev].slice(0, 50));
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection Status */}
      <View style={styles.statusCard}>
        <View style={styles.statusDot} backgroundColor={connected ? '#4CD964' : '#FF3B30'} />
        <Text style={styles.statusText}>{connected ? 'Connected' : 'Disconnected'}</Text>
      </View>

      {/* IMU Gauges */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>IMU (ICM-42688)</Text>
        <View style={styles.gaugeRow}>
          <ImuGauge label="Accel X" value={imuData.accelX} unit="g" range={16} />
          <ImuGauge label="Accel Y" value={imuData.accelY} unit="g" range={16} />
          <ImuGauge label="Accel Z" value={imuData.accelZ} unit="g" range={16} />
        </View>
        <View style={styles.gaugeRow}>
          <ImuGauge label="Gyro X" value={imuData.gyroX} unit="°/s" range={2000} />
          <ImuGauge label="Gyro Y" value={imuData.gyroY} unit="°/s" range={2000} />
          <ImuGauge label="Gyro Z" value={imuData.gyroZ} unit="°/s" range={2000} />
        </View>
        <Text style={styles.tempText}>Temperature: {imuData.temperature.toFixed(1)} °C</Text>
      </View>

      {/* Session Statistics */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Session Statistics</Text>
        <View style={styles.statGrid}>
          <View style={styles.statCell}>
            <Text style={styles.statValue}>{sessionStats.totalPoints}</Text>
            <Text style={styles.statLabel}>Total Points</Text>
          </View>
          <View style={styles.statCell}>
            <Text style={styles.statValue}>{sessionStats.totalFrames}</Text>
            <Text style={styles.statLabel}>Total Frames</Text>
          </View>
          <View style={styles.statCell}>
            <Text style={styles.statValue}>{sessionStats.avgPointsPerFrame.toFixed(1)}</Text>
            <Text style={styles.statLabel}>Avg Pts/Frame</Text>
          </View>
          <View style={styles.statCell}>
            <Text style={styles.statValue}>{sessionStats.maxRange.toFixed(2)} m</Text>
            <Text style={styles.statLabel}>Max Range</Text>
          </View>
          <View style={styles.statCell}>
            <Text style={styles.statValue}>{sessionStats.maxVelocity.toFixed(2)} m/s</Text>
            <Text style={styles.statLabel}>Max Velocity</Text>
          </View>
          <View style={styles.statCell}>
            <Text style={styles.statValue}>{Math.floor(sessionStats.duration)}s</Text>
            <Text style={styles.statLabel}>Duration</Text>
          </View>
        </View>
      </View>

      {/* Logging Controls */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Logging</Text>
        <View style={styles.logControls}>
          <TouchableOpacity style={styles.logButton} onPress={startLogging}>
            <Text style={styles.logButtonText}>Start Log</Text>
          </TouchableOpacity>
          <TouchableOpacity style={[styles.logButton, styles.logButtonStop]} onPress={stopLogging}>
            <Text style={styles.logButtonText}>Stop Log</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.logButton} onPress={exportData}>
            <Text style={styles.logButtonText}>Export CSV</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Log Entries */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Activity Log</Text>
        <FlatList
          data={logEntries}
          keyExtractor={(item) => item.id}
          renderItem={({ item }) => (
            <View style={styles.logEntry}>
              <Text style={styles.logTime}>{item.time}</Text>
              <Text style={styles.logMessage}>{item.message}</Text>
            </View>
          )}
          scrollEnabled={false}
        />
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
  statusCard: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#2C2C2E',
    borderRadius: 8,
    padding: 12,
    marginBottom: 16,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 10,
  },
  statusText: {
    color: '#FFFFFF',
    fontSize: 14,
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
  gaugeRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  tempText: {
    color: '#AAAAAA',
    fontSize: 13,
    textAlign: 'center',
    marginTop: 4,
  },
  statGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  statCell: {
    width: '30%',
    alignItems: 'center',
    paddingVertical: 8,
    marginBottom: 8,
  },
  statValue: {
    color: '#007AFF',
    fontSize: 16,
    fontWeight: '600',
  },
  statLabel: {
    color: '#888888',
    fontSize: 10,
    marginTop: 2,
  },
  logControls: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  logButton: {
    backgroundColor: '#007AFF',
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 8,
  },
  logButtonStop: {
    backgroundColor: '#FF3B30',
  },
  logButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
  logEntry: {
    flexDirection: 'row',
    paddingVertical: 4,
    borderBottomWidth: 1,
    borderBottomColor: '#3A3A3C',
  },
  logTime: {
    color: '#888888',
    fontSize: 11,
    width: 80,
  },
  logMessage: {
    color: '#CCCCCC',
    fontSize: 12,
  },
});