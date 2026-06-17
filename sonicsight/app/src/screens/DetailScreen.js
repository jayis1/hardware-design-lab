/**
 * DetailScreen.js — SonicSight Companion
 * Full-size tomogram view with crosshair, anomaly overlay, and export options.
 * @author jayis1
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView, Dimensions,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const SCREEN_WIDTH = Dimensions.get('window').width;

const DetailScreen = ({ route }) => {
  const scan = route?.params?.scan || {};

  const [overlay, setOverlay] = useState('velocity'); /* velocity | tof | anomaly */

  return (
    <ScrollView style={styles.container}>
      {/* Tomogram Placeholder */}
      <View style={styles.tomogramBox}>
        <Icon name="image-filter-center-focus" size={80} color="#00d2ff" />
        <Text style={styles.tomogramHint}>Tomogram Preview</Text>
        {/* Overlay toggle */}
        <View style={styles.overlayRow}>
          {['velocity', 'tof', 'anomaly'].map((mode) => (
            <TouchableOpacity key={mode}
              style={[styles.overlayBtn, overlay === mode && styles.overlayActive]}
              onPress={() => setOverlay(mode)}>
              <Text style={styles.overlayText}>{mode}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Scan Info Card */}
      <View style={styles.infoCard}>
        <Text style={styles.infoTitle}>{scan.label || 'Scan Detail'}</Text>
        <Text style={styles.infoDate}>{scan.date} {scan.time}</Text>
        <View style={styles.metricGrid}>
          <View style={styles.metric}>
            <Text style={styles.metricValue}>{scan.velMean || 0}</Text>
            <Text style={styles.metricLabel}>Mean Vel (m/s)</Text>
          </View>
          <View style={styles.metric}>
            <Text style={[styles.metricValue, { color: '#ff9800' }]}>
              {scan.decayPct || 0}%
            </Text>
            <Text style={styles.metricLabel}>Decay Est.</Text>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricValue}>{scan.sensors || 0}</Text>
            <Text style={styles.metricLabel}>Sensors</Text>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricValue}>{scan.id || '—'}</Text>
            <Text style={styles.metricLabel}>Scan ID</Text>
          </View>
        </View>
      </View>

      {/* Anomaly List */}
      <Text style={styles.sectionTitle}>Detected Anomalies</Text>
      <View style={styles.anomalyItem}>
        <View style={[styles.anomalyDot, { backgroundColor: '#f44336' }]} />
        <View style={styles.anomalyInfo}>
          <Text style={styles.anomalyLabel}>Zone A — Advanced decay</Text>
          <Text style={styles.anomalyDetail}>12.4 cm², v=980 m/s, depth 3–8 cm</Text>
        </View>
      </View>
      <View style={styles.anomalyItem}>
        <View style={[styles.anomalyDot, { backgroundColor: '#ff9800' }]} />
        <View style={styles.anomalyInfo}>
          <Text style={styles.anomalyLabel}>Zone B — Moderate decay</Text>
          <Text style={styles.anomalyDetail}>8.1 cm², v=1850 m/s, depth 5–12 cm</Text>
        </View>
      </View>

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.actionBtn}>
          <Icon name="file-pdf-box" size={24} color="#f44336" />
          <Text style={styles.actionBtnText}>PDF Report</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn}>
          <Icon name="file-delimited" size={24} color="#4caf50" />
          <Text style={styles.actionBtnText}>CSV ToF</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn}>
          <Icon name="share-variant" size={24} color="#2196f3" />
          <Text style={styles.actionBtnText}>Share</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23' },
  tomogramBox: {
    alignItems: 'center', justifyContent: 'center',
    backgroundColor: '#000', margin: 12, borderRadius: 12,
    height: SCREEN_WIDTH - 24, borderWidth: 2, borderColor: '#16213e',
  },
  tomogramHint: { color: '#555', fontSize: 14, marginTop: 8 },
  overlayRow: { flexDirection: 'row', marginTop: 12 },
  overlayBtn: {
    backgroundColor: '#1a1a2e', borderRadius: 4, padding: 6,
    paddingHorizontal: 12, marginHorizontal: 4,
    borderWidth: 1, borderColor: '#333',
  },
  overlayActive: { borderColor: '#00d2ff', backgroundColor: '#003344' },
  overlayText: { color: '#ccc', fontSize: 11, textTransform: 'capitalize' },
  infoCard: {
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16,
    marginHorizontal: 12, marginBottom: 12,
    borderWidth: 1, borderColor: '#16213e',
  },
  infoTitle: { color: '#fff', fontSize: 18, fontWeight: 'bold' },
  infoDate: { color: '#777', fontSize: 12, marginBottom: 12 },
  metricGrid: { flexDirection: 'row', flexWrap: 'wrap' },
  metric: { width: '50%', marginBottom: 8 },
  metricValue: { color: '#00d2ff', fontSize: 22, fontWeight: 'bold' },
  metricLabel: { color: '#888', fontSize: 11 },
  sectionTitle: {
    color: '#fff', fontSize: 16, fontWeight: 'bold',
    marginHorizontal: 12, marginBottom: 8,
  },
  anomalyItem: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', marginHorizontal: 12,
    borderRadius: 8, padding: 12, marginBottom: 6,
    borderWidth: 1, borderColor: '#16213e',
  },
  anomalyDot: {
    width: 12, height: 12, borderRadius: 6, marginRight: 12,
  },
  anomalyInfo: { flex: 1 },
  anomalyLabel: { color: '#fff', fontSize: 14, fontWeight: '600' },
  anomalyDetail: { color: '#888', fontSize: 12 },
  actionRow: {
    flexDirection: 'row', justifyContent: 'space-around',
    marginHorizontal: 12, marginTop: 12, marginBottom: 24,
  },
  actionBtn: {
    alignItems: 'center', backgroundColor: '#1a1a2e',
    borderRadius: 8, padding: 12, minWidth: 80,
    borderWidth: 1, borderColor: '#16213e',
  },
  actionBtnText: { color: '#ccc', fontSize: 11, marginTop: 4 },
});

export default DetailScreen;