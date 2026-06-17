/**
 * HomeScreen.js — SonicSight Companion
 * Device status dashboard, recent scan gallery, quick-start controls.
 * @author jayis1
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView,
  ActivityIndicator, Image, FlatList,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const HomeScreen = ({ bleService, isConnected, deviceName }) => {
  const [battery, setBattery] = useState(85);
  const [temperature, setTemperature] = useState(24.5);
  const [sdFree, setSdFree] = useState('12.4 GB');
  const [recentScans, setRecentScans] = useState([
    { id: '1', date: '2026-06-17 14:32', label: 'Oak #3 — Hyde Park', thumb: null },
    { id: '2', date: '2026-06-17 11:15', label: 'Pine Beam — Barn', thumb: null },
    { id: '3', date: '2026-06-16 09:42', label: 'Glulam Joint — Bridge', thumb: null },
  ]);

  useEffect(() => {
    if (bleService && isConnected) {
      const interval = setInterval(() => {
        bleService.readStatus().then((status) => {
          if (status) {
            setBattery(status.battery);
            setTemperature(status.temperature);
            setSdFree(status.sdFree);
          }
        }).catch(() => {});
      }, 5000);
      return () => clearInterval(interval);
    }
  }, [bleService, isConnected]);

  const renderScanItem = ({ item }) => (
    <TouchableOpacity style={styles.scanItem}>
      <View style={styles.scanThumb}>
        <Icon name="image-filter-center-focus" size={32} color="#00d2ff" />
      </View>
      <View style={styles.scanInfo}>
        <Text style={styles.scanLabel}>{item.label}</Text>
        <Text style={styles.scanDate}>{item.date}</Text>
      </View>
      <Icon name="chevron-right" size={24} color="#555" />
    </TouchableOpacity>
  );

  return (
    <ScrollView style={styles.container}>
      {/* Status Card */}
      <View style={styles.statusCard}>
        <View style={styles.statusRow}>
          <Icon name={isConnected ? 'bluetooth-connect' : 'bluetooth-off'}
                size={28} color={isConnected ? '#00d2ff' : '#ff4444'} />
          <Text style={styles.statusText}>
            {isConnected ? deviceName : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.statusRow}>
          <Icon name="battery" size={24} color={battery > 20 ? '#4caf50' : '#ff4444'} />
          <Text style={styles.statusText}>{battery}%</Text>
          <Icon name="thermometer" size={24} color="#ff9800" style={{ marginLeft: 20 }} />
          <Text style={styles.statusText}>{temperature.toFixed(1)}°C</Text>
          <Icon name="sd" size={24} color="#2196f3" style={{ marginLeft: 20 }} />
          <Text style={styles.statusText}>{sdFree}</Text>
        </View>
      </View>

      {/* Quick Actions */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.actionButton} onPress={() => {
          bleService?.sendCommand('CMD=START');
        }}>
          <Icon name="play-circle" size={32} color="#4caf50" />
          <Text style={styles.actionText}>Start Scan</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionButton} onPress={() => {
          bleService?.sendCommand('CMD=CAL');
        }}>
          <Icon name="tune" size={32} color="#ff9800" />
          <Text style={styles.actionText}>Calibrate</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionButton}>
          <Icon name="file-download" size={32} color="#2196f3" />
          <Text style={styles.actionText}>Export All</Text>
        </TouchableOpacity>
      </View>

      {/* Recent Scans */}
      <Text style={styles.sectionTitle}>Recent Scans</Text>
      <FlatList
        data={recentScans}
        renderItem={renderScanItem}
        keyExtractor={(item) => item.id}
        scrollEnabled={false}
      />

      {/* Quick Start Guide */}
      <View style={styles.guideCard}>
        <Text style={styles.guideTitle}>Quick Start</Text>
        <Text style={styles.guideStep}>
          1. Place sensors evenly around object{'\n'}
          2. Apply gel couplant to each{'\n'}
          3. Connect all 32 cables{'\n'}
          4. Press 'Start Scan' to begin
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23', padding: 12 },
  statusCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12, padding: 16, marginBottom: 12,
    borderWidth: 1, borderColor: '#16213e',
  },
  statusRow: {
    flexDirection: 'row', alignItems: 'center',
    marginVertical: 4,
  },
  statusText: { color: '#fff', fontSize: 16, marginLeft: 8 },
  actionRow: {
    flexDirection: 'row', justifyContent: 'space-around',
    marginBottom: 16,
  },
  actionButton: {
    alignItems: 'center', backgroundColor: '#1a1a2e',
    borderRadius: 12, padding: 12, minWidth: 90,
    borderWidth: 1, borderColor: '#16213e',
  },
  actionText: { color: '#ccc', fontSize: 12, marginTop: 4 },
  sectionTitle: { color: '#fff', fontSize: 18, fontWeight: 'bold', marginBottom: 8 },
  scanItem: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12,
    marginBottom: 6, borderWidth: 1, borderColor: '#16213e',
  },
  scanThumb: {
    width: 48, height: 48, borderRadius: 8,
    backgroundColor: '#16213e', justifyContent: 'center',
    alignItems: 'center', marginRight: 12,
  },
  scanInfo: { flex: 1 },
  scanLabel: { color: '#fff', fontSize: 14, fontWeight: '600' },
  scanDate: { color: '#777', fontSize: 12 },
  guideCard: {
    backgroundColor: '#16213e', borderRadius: 12, padding: 16,
    marginTop: 8, marginBottom: 24,
  },
  guideTitle: { color: '#00d2ff', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  guideStep: { color: '#ccc', fontSize: 13, lineHeight: 20 },
});

export default HomeScreen;