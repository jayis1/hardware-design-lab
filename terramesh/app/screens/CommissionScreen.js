/**
 * Terramesh — Commission Screen
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * NFC-based node commissioning interface. Allows field technicians to
 * tap a Terramesh node with their phone to read its ID, set burial
 * depth, assign to a site, and verify sensor readings.
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  ScrollView,
  Alert,
} from 'react-native';
import { useDevices } from '../components/DeviceContext';

const CommissionScreen = () => {
  const { updateNode } = useDevices();
  const [isScanning, setIsScanning] = useState(false);
  const [commissionedNode, setCommissionedNode] = useState(null);
  const [depth, setDepth] = useState('2.0');
  const [siteName, setSiteName] = useState('');
  const [notes, setNotes] = useState('');

  // Simulated NFC scan (in production, uses react-native-nfc-manager)
  const handleScan = async () => {
    setIsScanning(true);

    // Simulate NFC tag read
    setTimeout(() => {
      const mockNode = {
        id: `TM-${Math.floor(Math.random() * 9000 + 1000).toString()}`,
        name: `Node ${Math.floor(Math.random() * 100)}`,
        firmware: 'v1.0.0',
        hardware: 'Rev 1.0',
        serial: `SN-${Date.now().toString(36).toUpperCase()}`,
        battery: 7200,
        sensors: {
          porePressureShallow: 45.2,
          porePressureDeep: 28.7,
          moisture: 22.5,
          tiltX: 0.012,
          tiltY: -0.008,
          temperature: 18.3,
          pressure: 101325,
        },
      };

      setCommissionedNode(mockNode);
      setIsScanning(false);
    }, 2000);
  };

  const handleSave = () => {
    if (!commissionedNode || !siteName.trim()) {
      Alert.alert('Error', 'Please enter a site name');
      return;
    }

    const depthNum = parseFloat(depth);
    if (isNaN(depthNum) || depthNum < 0.5 || depthNum > 5.0) {
      Alert.alert('Error', 'Depth must be between 0.5 and 5.0 meters');
      return;
    }

    updateNode(commissionedNode.id, {
      ...commissionedNode,
      name: `${siteName} - ${depthNum}m`,
      depth: depthNum,
      site: siteName,
      notes,
      commissioned: true,
      commissionedAt: new Date().toISOString(),
    });

    Alert.alert(
      'Commissioned',
      `Node ${commissionedNode.id} commissioned at ${siteName}, depth ${depthNum}m`,
      [{ text: 'OK', onPress: () => {
        setCommissionedNode(null);
        setSiteName('');
        setNotes('');
        setDepth('2.0');
      }}]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* NFC Scan Section */}
      <View style={styles.scanCard}>
        <Text style={styles.scanTitle}>Commission New Node</Text>
        <Text style={styles.scanInstructions}>
          Hold your phone near the Terramesh node to read its NFC tag
        </Text>

        <TouchableOpacity
          style={[styles.scanButton, isScanning && styles.scanningButton]}
          onPress={handleScan}
          disabled={isScanning}
        >
          <Text style={styles.scanButtonText}>
            {isScanning ? 'Scanning...' : '📱 Tap to Scan NFC'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Commissioned Node Info */}
      {commissionedNode && (
        <View style={styles.nodeInfoCard}>
          <Text style={styles.sectionTitle}>Node Information</Text>

          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Node ID</Text>
            <Text style={styles.infoValue}>{commissionedNode.id}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Serial</Text>
            <Text style={styles.infoValue}>{commissionedNode.serial}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Firmware</Text>
            <Text style={styles.infoValue}>{commissionedNode.firmware}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Hardware</Text>
            <Text style={styles.infoValue}>{commissionedNode.hardware}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Battery</Text>
            <Text style={styles.infoValue}>
              {commissionedNode.battery} mV (100%)
            </Text>
          </View>

          {/* Sensor verification */}
          <Text style={styles.subSectionTitle}>Sensor Verification</Text>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Pore Pressure (S)</Text>
            <Text style={styles.infoValue}>
              {commissionedNode.sensors.porePressureShallow.toFixed(1)} kPa
            </Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Pore Pressure (D)</Text>
            <Text style={styles.infoValue}>
              {commissionedNode.sensors.porePressureDeep.toFixed(1)} kPa
            </Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Tilt X/Y</Text>
            <Text style={styles.infoValue}>
              {commissionedNode.sensors.tiltX.toFixed(3)}° / {commissionedNode.sensors.tiltY.toFixed(3)}°
            </Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Temperature</Text>
            <Text style={styles.infoValue}>
              {commissionedNode.sensors.temperature.toFixed(1)}°C
            </Text>
          </View>
        </View>
      )}

      {/* Configuration Form */}
      {commissionedNode && (
        <View style={styles.configCard}>
          <Text style={styles.sectionTitle}>Deployment Configuration</Text>

          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Site Name</Text>
            <TextInput
              style={styles.input}
              value={siteName}
              onChangeText={setSiteName}
              placeholder="e.g., Highway 101 Slope, Section B"
              placeholderTextColor="#4a4a5a"
            />
          </View>

          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Burial Depth (m)</Text>
            <TextInput
              style={styles.input}
              value={depth}
              onChangeText={setDepth}
              keyboardType="decimal-pad"
              placeholder="0.5 – 5.0"
              placeholderTextColor="#4a4a5a"
            />
          </View>

          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Notes</Text>
            <TextInput
              style={[styles.input, styles.textArea]}
              value={notes}
              onChangeText={setNotes}
              placeholder="Soil type, installation method, etc."
              placeholderTextColor="#4a4a5a"
              multiline
              numberOfLines={3}
            />
          </View>

          <TouchableOpacity style={styles.saveButton} onPress={handleSave}>
            <Text style={styles.saveButtonText}>✓ Save & Commission</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* Quick reference */}
      <View style={styles.referenceCard}>
        <Text style={styles.sectionTitle}>Commissioning Guide</Text>
        <Text style={styles.guideText}>
          1. Ensure the node is powered and within NFC range ({"<"} 4 cm)
        </Text>
        <Text style={styles.guideText}>
          2. Tap "Scan NFC" to read the node's identity and verify sensors
        </Text>
        <Text style={styles.guideText}>
          3. Enter the deployment site name and burial depth
        </Text>
        <Text style={styles.guideText}>
          4. Verify sensor readings are within expected ranges
        </Text>
        <Text style={styles.guideText}>
          5. Save — the node will join the mesh automatically
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
  },
  scanCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 20,
    borderWidth: 1,
    borderColor: '#2a2a3e',
    alignItems: 'center',
  },
  scanTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#e0e0e0',
    marginBottom: 8,
  },
  scanInstructions: {
    fontSize: 14,
    color: '#6c6c80',
    textAlign: 'center',
    marginBottom: 20,
    lineHeight: 20,
  },
  scanButton: {
    backgroundColor: '#00d4aa',
    paddingHorizontal: 30,
    paddingVertical: 14,
    borderRadius: 25,
  },
  scanningButton: {
    backgroundColor: '#4a4a5a',
  },
  scanButtonText: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#0f0f1a',
  },
  nodeInfoCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#e0e0e0',
    marginBottom: 12,
  },
  subSectionTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: '#e0e0e0',
    marginTop: 12,
    marginBottom: 8,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  infoLabel: {
    fontSize: 13,
    color: '#6c6c80',
  },
  infoValue: {
    fontSize: 13,
    color: '#e0e0e0',
    fontWeight: '500',
  },
  configCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  inputGroup: {
    marginBottom: 14,
  },
  inputLabel: {
    fontSize: 13,
    color: '#e0e0e0',
    marginBottom: 6,
    fontWeight: '500',
  },
  input: {
    backgroundColor: '#0f0f1a',
    borderWidth: 1,
    borderColor: '#2a2a3e',
    borderRadius: 8,
    padding: 12,
    fontSize: 14,
    color: '#e0e0e0',
  },
  textArea: {
    height: 80,
    textAlignVertical: 'top',
  },
  saveButton: {
    backgroundColor: '#00d4aa',
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    marginTop: 8,
  },
  saveButtonText: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#0f0f1a',
  },
  referenceCard: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
    marginBottom: 30,
  },
  guideText: {
    fontSize: 13,
    color: '#a0a0b0',
    lineHeight: 22,
    paddingLeft: 8,
  },
});

export default CommissionScreen;
