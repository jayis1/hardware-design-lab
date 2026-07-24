/**
 * LiveScanScreen.js — Real-time scan result display
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Animated, Vibration } from 'react-native';
import { useBle } from '../ble/BleContext';
import Colors from '../constants/Colors';
import ImpedancePlot from '../components/ImpedancePlot';
import ConfidenceBar from '../components/ConfidenceBar';

const LiveScanScreen = ({ navigation }) => {
  const { connected, scanning, scanResult, error, startScan, triggerScan } = useBle();
  const [fadeAnim] = useState(new Animated.Value(0));

  useEffect(() => {
    if (scanResult) {
      Animated.timing(fadeAnim, {
        toValue: 1,
        duration: 300,
        useNativeDriver: true,
      }).start();

      // Vibrate on high confidence
      if (scanResult.confidence >= 0.7) {
        Vibration.vibrate(100);
      }
    } else {
      fadeAnim.setValue(0);
    }
  }, [scanResult]);

  const getConfidenceColor = (conf) => {
    if (conf >= 0.7) return Colors.highConfidence;
    if (conf >= 0.4) return Colors.mediumConfidence;
    return Colors.lowConfidence;
  };

  const getFamilyColor = (alloy) => {
    if (!alloy) return Colors.gray;
    const fam = alloy.split(' ')[0];
    const familyMap = {
      'Carbon': Colors.family.Carbon,
      'Alloy': Colors.family.Alloy,
      '304': Colors.family.Austenitic,
      '316': Colors.family.Austenitic,
      'SS': Colors.family.Austenitic,
      '410': Colors.family.Ferritic,
      '420': Colors.family.Ferritic,
      '430': Colors.family.Ferritic,
      '440': Colors.family.Ferritic,
      '17-4': Colors.family.Ferritic,
      '2205': Colors.family.Duplex,
      '2507': Colors.family.Duplex,
      'Al': Colors.family.Aluminum,
      'Cu': Colors.family.Copper,
      'C': Colors.family.Copper,
      'Ti': Colors.family.Titanium,
      'Inconel': Colors.family.Nickel,
      'Monel': Colors.family.Nickel,
      'Hast': Colors.family.Nickel,
      'Zn': Colors.family.Zinc,
      'AZ': Colors.family.Zinc,
    };
    for (const key of Object.keys(familyMap)) {
      if (alloy.startsWith(key)) return familyMap[key];
    }
    return Colors.family.Other;
  };

  if (!connected) {
    return (
      <View style={styles.container}>
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedTitle}>Not Connected</Text>
          <Text style={styles.disconnectedText}>
            Connect to your AlloyScan device to start scanning.
          </Text>
          <TouchableOpacity style={styles.connectButton} onPress={startScan}>
            <Text style={styles.connectButtonText}>
              {scanning ? 'Searching...' : 'Connect to Device'}
            </Text>
          </TouchableOpacity>
          {error && <Text style={styles.errorText}>{error}</Text>}
        </View>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      {/* Header with connection status */}
      <View style={styles.header}>
        <View style={[styles.statusDot, { backgroundColor: Colors.green }]} />
        <Text style={styles.statusText}>Connected</Text>
      </View>

      {!scanResult ? (
        // Idle state — prompt to scan
        <View style={styles.idleContainer}>
          <Text style={styles.idleTitle}>Ready to Scan</Text>
          <Text style={styles.idleSubtitle}>
            Press the trigger on your AlloyScan device or tap below.
          </Text>
          <TouchableOpacity style={styles.scanButton} onPress={triggerScan}>
            <Text style={styles.scanButtonText}>Trigger Scan</Text>
          </TouchableOpacity>
        </View>
      ) : (
        // Result display
        <Animated.View style={[styles.resultContainer, { opacity: fadeAnim }]}>
          {/* Alloy name and family */}
          <View style={styles.alloyHeader}>
            <Text style={styles.alloyName}>{scanResult.alloy || 'Unknown'}</Text>
            <View
              style={[
                styles.familyBadge,
                { backgroundColor: getFamilyColor(scanResult.alloy) },
              ]}
            >
              <Text style={styles.familyText}>Identified</Text>
            </View>
          </View>

          {/* Confidence bar */}
          <ConfidenceBar confidence={scanResult.confidence} />

          {/* Impedance plane plot */}
          {scanResult.iq && (
            <ImpedancePlot iq={scanResult.iq} />
          )}

          {/* Alternatives */}
          {scanResult.alternatives && scanResult.alternatives.length > 0 && (
            <View style={styles.alternativesSection}>
              <Text style={styles.sectionTitle}>Closest Alternatives</Text>
              {scanResult.alternatives.map((alt, i) => (
                <View key={i} style={styles.altRow}>
                  <Text style={styles.altName}>{alt}</Text>
                  <Text style={styles.altRank}>#{i + 1}</Text>
                </View>
              ))}
            </View>
          )}

          {/* Lift-off indicator */}
          <View style={styles.liftoffRow}>
            <Text style={styles.liftoffLabel}>Lift-off:</Text>
            <Text style={styles.liftoffValue}>
              {scanResult.liftoff ? `${scanResult.liftoff.toFixed(1)} mm` : 'N/A'}
            </Text>
          </View>

          {/* Scan again button */}
          <TouchableOpacity style={styles.scanAgainButton} onPress={triggerScan}>
            <Text style={styles.scanAgainText}>Scan Again</Text>
          </TouchableOpacity>
        </Animated.View>
      )}

      {error && <Text style={styles.errorText}>{error}</Text>}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: Colors.dark,
  },
  disconnected: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 40,
  },
  disconnectedTitle: {
    fontSize: 24,
    fontWeight: 'bold',
    color: Colors.white,
    marginBottom: 12,
  },
  disconnectedText: {
    fontSize: 16,
    color: Colors.gray,
    textAlign: 'center',
    marginBottom: 30,
  },
  connectButton: {
    backgroundColor: Colors.accent,
    paddingHorizontal: 30,
    paddingVertical: 15,
    borderRadius: 25,
  },
  connectButtonText: {
    color: Colors.white,
    fontSize: 16,
    fontWeight: 'bold',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  statusText: {
    color: Colors.white,
    fontSize: 14,
  },
  idleContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 40,
  },
  idleTitle: {
    fontSize: 28,
    fontWeight: 'bold',
    color: Colors.white,
    marginBottom: 12,
  },
  idleSubtitle: {
    fontSize: 14,
    color: Colors.gray,
    textAlign: 'center',
    marginBottom: 30,
  },
  scanButton: {
    backgroundColor: Colors.accent,
    paddingHorizontal: 40,
    paddingVertical: 18,
    borderRadius: 30,
    elevation: 4,
  },
  scanButtonText: {
    color: Colors.white,
    fontSize: 18,
    fontWeight: 'bold',
  },
  resultContainer: {
    flex: 1,
    padding: 16,
  },
  alloyHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  alloyName: {
    fontSize: 32,
    fontWeight: 'bold',
    color: Colors.white,
  },
  familyBadge: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 12,
  },
  familyText: {
    color: Colors.white,
    fontSize: 12,
    fontWeight: 'bold',
  },
  sectionTitle: {
    color: Colors.lightGray,
    fontSize: 14,
    fontWeight: 'bold',
    marginTop: 16,
    marginBottom: 8,
  },
  alternativesSection: {
    marginTop: 16,
  },
  altRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  altName: {
    color: Colors.white,
    fontSize: 16,
  },
  altRank: {
    color: Colors.gray,
    fontSize: 14,
  },
  liftoffRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 16,
  },
  liftoffLabel: {
    color: Colors.gray,
    fontSize: 14,
  },
  liftoffValue: {
    color: Colors.white,
    fontSize: 14,
    fontWeight: 'bold',
  },
  scanAgainButton: {
    backgroundColor: Colors.primaryLight,
    paddingVertical: 15,
    borderRadius: 20,
    alignItems: 'center',
    marginTop: 20,
  },
  scanAgainText: {
    color: Colors.white,
    fontSize: 16,
    fontWeight: 'bold',
  },
  errorText: {
    color: Colors.red,
    fontSize: 14,
    textAlign: 'center',
    padding: 10,
  },
});

export default LiveScanScreen;