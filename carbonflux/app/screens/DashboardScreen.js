/**
 * DashboardScreen.js — Live flux, chamber state, battery, and connection.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, {useContext} from 'react';
import {View, ScrollView, Text, StyleSheet, TouchableOpacity} from 'react-native';
import BLEContext from '../components/BLEContext';
import StatusCard from '../components/StatusCard';
import FluxChart from '../components/FluxChart';

const DashboardScreen = ({navigation}) => {
  const {bleConnected, setBleConnected, deviceData} = useContext(BLEContext);

  // Mock accumulation data for demonstration (would come from BLE in production)
  const mockAccumData = Array.from({length: 90}, (_, i) =>
    400 + i * 0.8 + Math.random() * 5
  );

  return (
    <ScrollView style={styles.container}>
      {/* Connection Banner */}
      <TouchableOpacity
        style={[styles.connectionBanner, bleConnected ? styles.connected : styles.disconnected]}
        onPress={() => setBleConnected(!bleConnected)}>
        <Text style={styles.connectionText}>
          {bleConnected ? '● Connected (BLE)' : '○ Tap to Connect via BLE'}
        </Text>
      </TouchableOpacity>

      {/* Status Summary */}
      <StatusCard
        state={deviceData.state}
        batterySoc={deviceData.battery_soc}
        fluxUmol={deviceData.flux_umol}
        co2Ppm={deviceData.co2_ppm}
        measurementCount={deviceData.measurement_count}
      />

      {/* Flux Accumulation Chart */}
      <FluxChart
        data={mockAccumData}
        fluxRate={0.042}
        rSquared={0.987}
      />

      {/* Quick Actions */}
      <View style={styles.actionsRow}>
        <TouchableOpacity style={styles.actionButton} onPress={() => navigation.navigate('Config')}>
          <Text style={styles.actionIcon}>⚙️</Text>
          <Text style={styles.actionLabel}>Configure</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionButton} onPress={() => navigation.navigate('Sensors')}>
          <Text style={styles.actionIcon}>📊</Text>
          <Text style={styles.actionLabel}>Sensors</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionButton} onPress={() => navigation.navigate('Data')}>
          <Text style={styles.actionIcon}>📁</Text>
          <Text style={styles.actionLabel}>Export</Text>
        </TouchableOpacity>
      </View>

      {/* Last Measurement Summary */}
      <View style={styles.summaryBox}>
        <Text style={styles.summaryTitle}>Last Measurement</Text>
        <Text style={styles.summaryText}>
          {new Date().toLocaleString()} —{' '}
          {deviceData.flux_umol > 0
            ? `Flux: ${deviceData.flux_umol.toFixed(2)} µmol/m²/s`
            : 'No valid measurement yet'}
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  connectionBanner: {
    padding: 12,
    margin: 8,
    borderRadius: 8,
    alignItems: 'center',
  },
  connected: {
    backgroundColor: '#1b5e20',
  },
  disconnected: {
    backgroundColor: '#37474F',
  },
  connectionText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '600',
  },
  actionsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    margin: 8,
  },
  actionButton: {
    backgroundColor: '#1a1a2e',
    padding: 16,
    borderRadius: 12,
    alignItems: 'center',
    width: '30%',
    borderWidth: 1,
    borderColor: '#333',
  },
  actionIcon: {
    fontSize: 24,
    marginBottom: 4,
  },
  actionLabel: {
    color: '#e0e0e0',
    fontSize: 12,
  },
  summaryBox: {
    backgroundColor: '#1a1a2e',
    margin: 8,
    padding: 16,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  summaryTitle: {
    color: '#888',
    fontSize: 11,
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginBottom: 4,
  },
  summaryText: {
    color: '#e0e0e0',
    fontSize: 14,
    fontFamily: 'monospace',
  },
});

export default DashboardScreen;