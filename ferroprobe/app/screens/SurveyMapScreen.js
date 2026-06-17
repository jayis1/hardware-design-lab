/**
 * SurveyMapScreen.js — GPS-Tagged Magnetic Survey Map
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Displays a map with color-coded magnetic field data points overlaid,
 * showing anomalies and survey tracks. Uses react-native-maps.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Platform, Alert } from 'react-native';
import MapView, { Marker, Polyline, Circle } from 'react-native-maps';
import { useBle } from '../utils/BleContext';
import StatusBar from '../components/StatusBar';

export default function SurveyMapScreen() {
  const ble = useBle();
  const [surveyPoints, setSurveyPoints] = useState([]);
  const [isRecording, setIsRecording] = useState(false);
  const [region, setRegion] = useState({
    latitude: 47.6064,
    longitude: -122.3320,
    latitudeDelta: 0.01,
    longitudeDelta: 0.01,
  });
  const mapRef = useRef(null);

  // Record survey points from field data stream
  useEffect(() => {
    if (isRecording && ble.fieldData && ble.fieldData.hasGpsFix) {
      // In a real implementation, GPS coordinates would come from the
      // device's GPS.  For now, we use the field data flags.
      const point = {
        latitude: region.latitude + (Math.random() - 0.5) * 0.001,
        longitude: region.longitude + (Math.random() - 0.5) * 0.001,
        bTotal: ble.fieldData.bTotal,
        isAnomaly: ble.fieldData.isAnomaly,
        timestamp: ble.fieldData.timestamp,
      };
      setSurveyPoints((prev) => [...prev, point]);
    }
  }, [ble.fieldData, isRecording]);

  const handleStartSurvey = () => {
    setIsRecording(true);
    ble.startSurvey();
  };

  const handleStopSurvey = () => {
    setIsRecording(false);
    ble.stopSurvey();
  };

  const handleClearSurvey = () => {
    Alert.alert(
      'Clear Survey',
      'Remove all recorded data points?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Clear', onPress: () => setSurveyPoints([]) },
      ]
    );
  };

  // Color-code points based on field strength deviation from 50 µT
  const getPointColor = (bTotalNt) => {
    const deviation = Math.abs(bTotalNt - 50000);
    if (deviation < 200) return '#4CAF50';      // Green: normal
    if (deviation < 1000) return '#FFC107';     // Yellow: mild anomaly
    if (deviation < 5000) return '#FF9800';     // Orange: moderate
    return '#F44336';                             // Red: strong anomaly
  };

  // Build polyline from survey points
  const surveyPath = surveyPoints.map((p) => ({
    latitude: p.latitude,
    longitude: p.longitude,
  }));

  return (
    <View style={styles.container}>
      <StatusBar
        connectionState={ble.connectionState}
        status={ble.status}
        deviceInfo={ble.deviceInfo}
      />

      <MapView
        ref={mapRef}
        style={styles.map}
        region={region}
        onRegionChangeComplete={setRegion}
        showsUserLocation={true}
        showsCompass={true}
      >
        {/* Survey track polyline */}
        {surveyPath.length > 1 && (
          <Polyline
            coordinates={surveyPath}
            strokeColor="#2196F3"
            strokeWidth={3}
            strokeColors={['#2196F3', '#4CAF50', '#FFC107', '#FF9800', '#F44336']}
          />
        )}

        {/* Data point markers */}
        {surveyPoints.map((point, index) => (
          <Marker
            key={index}
            coordinate={{ latitude: point.latitude, longitude: point.longitude }}
            title={`Point ${index + 1}`}
            description={`|B| = ${(point.bTotal / 1000).toFixed(2)} µT${point.isAnomaly ? ' ⚠' : ''}`}
          >
            <View style={[
              styles.pointMarker,
              { backgroundColor: getPointColor(point.bTotal) },
              point.isAnomaly && styles.anomalyMarker,
            ]} />
          </Marker>
        ))}
      </MapView>

      {/* Survey control bar */}
      <View style={styles.controlBar}>
        <View style={styles.statsContainer}>
          <Text style={styles.statLabel}>Points</Text>
          <Text style={styles.statValue}>{surveyPoints.length}</Text>
        </View>
        <View style={styles.statsContainer}>
          <Text style={styles.statLabel}>Anomalies</Text>
          <Text style={styles.statValue}>
            {surveyPoints.filter((p) => p.isAnomaly).length}
          </Text>
        </View>
        <View style={styles.statsContainer}>
          <Text style={styles.statLabel}>Status</Text>
          <Text style={[styles.statValue, { color: isRecording ? '#4CAF50' : '#888' }]}>
            {isRecording ? 'REC' : 'STOP'}
          </Text>
        </View>
      </View>

      {/* Button row */}
      <View style={styles.buttonRow}>
        {!isRecording ? (
          <TouchableOpacity
            style={[styles.button, styles.startButton]}
            onPress={handleStartSurvey}
          >
            <Text style={styles.buttonText}>▶ Start Survey</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity
            style={[styles.button, styles.stopButton]}
            onPress={handleStopSurvey}
          >
            <Text style={styles.buttonText}>■ Stop Survey</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity
          style={[styles.button, styles.clearButton]}
          onPress={handleClearSurvey}
        >
          <Text style={styles.buttonText}>Clear</Text>
        </TouchableOpacity>
      </View>

      {/* Legend */}
      <View style={styles.legend}>
        <Text style={styles.legendTitle}>Anomaly Scale</Text>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: '#4CAF50' }]} />
          <Text style={styles.legendText}>Normal (&lt;200 nT)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: '#FFC107' }]} />
          <Text style={styles.legendText}>Mild (&lt;1 µT)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: '#FF9800' }]} />
          <Text style={styles.legendText}>Moderate (&lt;5 µT)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: '#F44336' }]} />
          <Text style={styles.legendText}>Strong (&gt;5 µT)</Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  map: {
    flex: 1,
  },
  controlBar: {
    position: 'absolute',
    top: Platform.OS === 'ios' ? 100 : 80,
    left: 15,
    right: 15,
    flexDirection: 'row',
    backgroundColor: 'rgba(26,26,46,0.9)',
    borderRadius: 10,
    padding: 10,
    justifyContent: 'space-around',
  },
  statsContainer: {
    alignItems: 'center',
  },
  statLabel: {
    color: '#888',
    fontSize: 11,
  },
  statValue: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  buttonRow: {
    position: 'absolute',
    bottom: 170,
    left: 15,
    right: 15,
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  button: {
    flex: 1,
    paddingVertical: 14,
    borderRadius: 10,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  startButton: { backgroundColor: '#4CAF50' },
  stopButton: { backgroundColor: '#F44336' },
  clearButton: { backgroundColor: '#333' },
  buttonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  pointMarker: {
    width: 12,
    height: 12,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#fff',
  },
  anomalyMarker: {
    width: 16,
    height: 16,
    borderRadius: 8,
    borderWidth: 2,
    borderColor: '#FFEB3B',
  },
  legend: {
    position: 'absolute',
    bottom: 15,
    left: 15,
    right: 15,
    backgroundColor: 'rgba(26,26,46,0.9)',
    borderRadius: 10,
    padding: 12,
  },
  legendTitle: {
    color: '#888',
    fontSize: 12,
    marginBottom: 8,
  },
  legendRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginVertical: 3,
  },
  legendDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  legendText: {
    color: '#ccc',
    fontSize: 11,
  },
});