/**
 * MapScreen.js — Main position/map screen for Chronos-RTK
 * Shows live RTK position, fix status, and satellite info
 */

import React, { useContext, useEffect, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { ChronosContext } from '../components/ChronosContext';
import StatusCard from '../components/StatusCard';

export default function MapScreen({ navigation }) {
  const { position, satellites, rtkStatus, connect, disconnect, isConnected } =
    useContext(ChronosContext);

  const [elapsed, setElapsed] = useState(0);

  useEffect(() => {
    const timer = setInterval(() => {
      setElapsed((prev) => prev + 1);
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  const formatCoord = (value, isLat) => {
    if (value === null || value === undefined) return '--.------';
    const abs = Math.abs(value);
    const deg = Math.floor(abs);
    const min = (abs - deg) * 60;
    const dir = isLat ? (value >= 0 ? 'N' : 'S') : value >= 0 ? 'E' : 'W';
    return `${deg}°${min.toFixed(4)}' ${dir}`;
  };

  const fixColor = () => {
    switch (rtkStatus) {
      case 'RTK Fixed':
        return '#4CAF50';
      case 'RTK Float':
        return '#FF9800';
      case '3D Fix':
        return '#2196F3';
      case '2D Fix':
        return '#FFC107';
      default:
        return '#F44336';
    }
  };

  return (
    <View style={styles.container}>
      {/* Status bar */}
      <View style={[styles.statusBar, { backgroundColor: fixColor() }]}>
        <Text style={styles.statusText}>
          {isConnected ? rtkStatus : 'Disconnected'}
        </Text>
        <Text style={styles.statusSubtext}>
          {isConnected ? `Sats: ${satellites.inView} | Age: ${elapsed}s` : 'Tap to connect'}
        </Text>
      </View>

      {/* Connection button */}
      <TouchableOpacity
        style={[styles.connectButton, { backgroundColor: isConnected ? '#F44336' : '#4CAF50' }]}
        onPress={() => (isConnected ? disconnect() : connect())}
      >
        <Text style={styles.connectButtonText}>
          {isConnected ? 'Disconnect' : 'Connect'}
        </Text>
      </TouchableOpacity>

      <ScrollView style={styles.scrollContainer}>
        {/* Position display */}
        <StatusCard title="Position" icon="crosshairs-gps">
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>Lat:</Text>
            <Text style={styles.coordValue}>
              {formatCoord(position?.latitude, true)}
            </Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>Lon:</Text>
            <Text style={styles.coordValue}>
              {formatCoord(position?.longitude, false)}
            </Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>Alt:</Text>
            <Text style={styles.coordValue}>
              {position?.altitude?.toFixed(2) ?? '--.--'} m
            </Text>
          </View>
        </StatusCard>

        {/* Accuracy card */}
        <StatusCard title="Accuracy" icon="target">
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>H:</Text>
            <Text style={styles.coordValue}>
              {position?.hAccuracy?.toFixed(3) ?? '---'} m
            </Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>V:</Text>
            <Text style={styles.coordValue}>
              {position?.vAccuracy?.toFixed(3) ?? '---'} m
            </Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>PDOP:</Text>
            <Text style={styles.coordValue}>
              {satellites?.pdop?.toFixed(1) ?? '--.-'}
            </Text>
          </View>
        </StatusCard>

        {/* Satellite info */}
        <StatusCard title="Satellites" icon="satellite-variant">
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>In view:</Text>
            <Text style={styles.coordValue}>{satellites.inView ?? '--'}</Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>Used:</Text>
            <Text style={styles.coordValue}>{satellites.used ?? '--'}</Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>GPS:</Text>
            <Text style={styles.coordValue}>{satellites.gps ?? '--'}</Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>GLO:</Text>
            <Text style={styles.coordValue}>{satellites.glo ?? '--'}</Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>GAL:</Text>
            <Text style={styles.coordValue}>{satellites.gal ?? '--'}</Text>
          </View>
          <View style={styles.coordRow}>
            <Text style={styles.coordLabel}>BDS:</Text>
            <Text style={styles.coordValue}>{satellites.bds ?? '--'}</Text>
          </View>
        </StatusCard>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1117',
  },
  statusBar: {
    padding: 16,
    alignItems: 'center',
  },
  statusText: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  statusSubtext: {
    color: '#fff',
    fontSize: 12,
    opacity: 0.8,
    marginTop: 4,
  },
  connectButton: {
    margin: 12,
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  connectButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  scrollContainer: {
    flex: 1,
    padding: 8,
  },
  coordRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  coordLabel: {
    color: '#8b949e',
    fontSize: 14,
    fontWeight: '600',
    width: 60,
  },
  coordValue: {
    color: '#e6edf3',
    fontSize: 14,
    fontFamily: 'Courier',
    flex: 1,
    textAlign: 'right',
  },
});