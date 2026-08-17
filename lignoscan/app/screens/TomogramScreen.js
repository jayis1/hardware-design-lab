// ============================================================
// LignoScan App — Tomogram Screen
// Displays the 2D acoustic tomogram of the tree trunk
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React, { useState, useMemo } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Dimensions, ScrollView } from 'react-native';
import Svg, { Circle, Line, Path, G, Text as SvgText } from 'react-native-svg';
import { useBle } from '../utils/BleContext';
import { DECAY_COLORS, DECAY_LABELS, severityFromTDI, formatCoordinate } from '../utils/protocol';

const SCREEN_WIDTH = Dimensions.get('window').width;
const TOMOGRAM_SIZE = Math.min(SCREEN_WIDTH - 40, 350);

export default function TomogramScreen({ navigation }) {
  const { tomogram, gpsData, scanStatus } = useBle();
  const [selectedCell, setSelectedCell] = useState(null);
  const [showLegend, setShowLegend] = useState(true);

  const severity = useMemo(() => {
    if (!tomogram) return null;
    return severityFromTDI(tomogram.tdi);
  }, [tomogram]);

  const renderTomogram = () => {
    if (!tomogram) {
      return (
        <View style={styles.emptyContainer}>
          <Text style={styles.emptyText}>No tomogram data</Text>
          <Text style={styles.emptySubtext}>
            Run a scan from the device to see the acoustic tomogram here.
          </Text>
        </View>
      );
    }

    const center = TOMOGRAM_SIZE / 2;
    const radius = TOMOGRAM_SIZE / 2 - 30;

    // Determine grid dimensions from cell count
    // Assuming 8 radial × 16 angular = 128 cells
    const nAngular = 16;
    const nRadial = Math.ceil(tomogram.nCells / nAngular);

    // Draw cells as colored arc segments in polar grid
    const cells = [];
    for (let i = 0; i < tomogram.cells.length; i++) {
      const cell = tomogram.cells[i];
      const aIdx = Math.floor(i / nRadial);
      const rIdx = i % nRadial;

      const rInner = (rIdx / nRadial) * radius;
      const rOuter = ((rIdx + 1) / nRadial) * radius;
      const angleStart = (aIdx / nAngular) * 2 * Math.PI - Math.PI / 2;
      const angleEnd = ((aIdx + 1) / nAngular) * 2 * Math.PI - Math.PI / 2;

      const x1 = center + rInner * Math.cos(angleStart);
      const y1 = center + rInner * Math.sin(angleStart);
      const x2 = center + rOuter * Math.cos(angleStart);
      const y2 = center + rOuter * Math.sin(angleStart);
      const x3 = center + rOuter * Math.cos(angleEnd);
      const y3 = center + rOuter * Math.sin(angleEnd);
      const x4 = center + rInner * Math.cos(angleEnd);
      const y4 = center + rInner * Math.sin(angleEnd);

      const largeArc = (angleEnd - angleStart) > Math.PI ? 1 : 0;
      const color = DECAY_COLORS[cell.classification] || '#888';

      const pathData = `M ${x1} ${y1} L ${x2} ${y2} A ${rOuter} ${rOuter} 0 ${largeArc} 1 ${x3} ${y3} L ${x4} ${y4} A ${rInner} ${rInner} 0 ${largeArc} 0 ${x1} ${y1} Z`;

      cells.push(
        <Path
          key={`cell-${i}`}
          d={pathData}
          fill={color}
          stroke="#fff"
          strokeWidth="0.5"
          onPress={() => setSelectedCell({ index: i, ...cell })}
        />
      );
    }

    // Draw sensor position markers
    const nSensors = 12;
    const sensorMarkers = [];
    for (let i = 0; i < nSensors; i++) {
      const angle = (i / nSensors) * 2 * Math.PI - Math.PI / 2;
      const x = center + (radius + 15) * Math.cos(angle);
      const y = center + (radius + 15) * Math.sin(angle);
      sensorMarkers.push(
        <G key={`sensor-${i}`}>
          <Circle cx={x} cy={y} r="6" fill="#1a3a1a" />
          <SvgText x={x} y={y + 2} fontSize="7" fill="#fff" textAnchor="middle">
            {i}
          </SvgText>
        </G>
      );
    }

    return (
      <Svg width={TOMOGRAM_SIZE} height={TOMOGRAM_SIZE}>
        {/* Trunk outline */}
        <Circle cx={center} cy={center} r={radius} fill="none" stroke="#1a3a1a" strokeWidth="2" />
        {/* Tomogram cells */}
        {cells}
        {/* Sensor markers */}
        {sensorMarkers}
      </Svg>
    );
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Acoustic Tomogram</Text>

        {scanStatus && scanStatus.state === 2 && (
          <View style={styles.progressContainer}>
            <View style={styles.progressBar}>
              <View style={[styles.progressFill, { width: `${scanStatus.progress}%` }]} />
            </View>
            <Text style={styles.progressText}>Scanning... {scanStatus.progress}%</Text>
          </View>
        )}

        {tomogram && (
          <View style={styles.tdiContainer}>
            <Text style={styles.tdiLabel}>Decay Index (TDI):</Text>
            <Text style={[styles.tdiValue, { color: severity?.color }]}>
              {(tomogram.tdi * 100).toFixed(1)}%
            </Text>
            <Text style={[styles.severityLabel, { color: severity?.color }]}>
              {severity?.label}
            </Text>
          </View>
        )}

        {renderTomogram()}

        {/* Selected cell info */}
        {selectedCell && (
          <View style={styles.cellInfoContainer}>
            <Text style={styles.cellInfoTitle}>Cell #{selectedCell.index}</Text>
            <Text style={styles.cellInfoText}>
              Velocity: {selectedCell.velocity.toFixed(0)} m/s
            </Text>
            <Text style={styles.cellInfoText}>
              Classification: {DECAY_LABELS[selectedCell.classification]}
            </Text>
            <View style={[styles.classIndicator, { backgroundColor: DECAY_COLORS[selectedCell.classification] }]} />
          </View>
        )}

        {/* Legend */}
        {tomogram && showLegend && (
          <View style={styles.legend}>
            <Text style={styles.legendTitle}>Legend</Text>
            {Object.entries(DECAY_LABELS).map(([key, label]) => (
              <View key={key} style={styles.legendItem}>
                <View style={[styles.legendColor, { backgroundColor: DECAY_COLORS[key] }]} />
                <Text style={styles.legendText}>{label}</Text>
              </View>
            ))}
          </View>
        )}

        {/* GPS Info */}
        {gpsData && (
          <View style={styles.gpsContainer}>
            <Text style={styles.gpsTitle}>GPS Location</Text>
            <Text style={styles.gpsText}>
              {formatCoordinate(gpsData.latitude, gpsData.longitude)}
            </Text>
            <Text style={styles.gpsText}>
              Altitude: {gpsData.altitude.toFixed(0)} m | Sats: {gpsData.satellites}
            </Text>
            <Text style={styles.gpsText}>Time: {gpsData.timestamp}</Text>
          </View>
        )}

        {/* Actions */}
        {tomogram && (
          <View style={styles.actionRow}>
            <TouchableOpacity
              style={styles.actionButton}
              onPress={() => navigation.navigate('Report')}
            >
              <Text style={styles.actionButtonText}>Generate Report</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.saveButton}
              onPress={() => navigation.navigate('ScanList')}
            >
              <Text style={styles.actionButtonText}>Save & View History</Text>
            </TouchableOpacity>
          </View>
        )}
      </View>

      <Text style={styles.footer}>
        Author: jayis1 — Copyright © 2026 — MIT License
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f0' },
  card: {
    backgroundColor: '#fff',
    margin: 12,
    padding: 16,
    borderRadius: 10,
    alignItems: 'center',
    elevation: 2,
  },
  title: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#1a3a1a',
    marginBottom: 12,
  },
  progressContainer: { width: '100%', marginBottom: 12 },
  progressBar: {
    height: 8,
    backgroundColor: '#e0e0e0',
    borderRadius: 4,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: '#2d8a2d',
  },
  progressText: {
    textAlign: 'center',
    marginTop: 4,
    color: '#666',
    fontSize: 13,
  },
  tdiContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    flexWrap: 'wrap',
    marginBottom: 12,
    gap: 8,
  },
  tdiLabel: { fontSize: 16, color: '#333' },
  tdiValue: { fontSize: 24, fontWeight: 'bold' },
  severityLabel: { fontSize: 14, fontWeight: 'bold' },
  emptyContainer: { padding: 40, alignItems: 'center' },
  emptyText: { fontSize: 18, color: '#999', marginBottom: 8 },
  emptySubtext: { fontSize: 14, color: '#aaa', textAlign: 'center' },
  cellInfoContainer: {
    marginTop: 12,
    padding: 12,
    backgroundColor: '#f9f9f0',
    borderRadius: 8,
    width: '100%',
  },
  cellInfoTitle: { fontSize: 15, fontWeight: 'bold', color: '#1a3a1a' },
  cellInfoText: { fontSize: 14, color: '#555', marginTop: 4 },
  classIndicator: {
    width: 20,
    height: 20,
    borderRadius: 4,
    marginTop: 8,
  },
  legend: {
    marginTop: 12,
    padding: 10,
    backgroundColor: '#f9f9f0',
    borderRadius: 8,
    width: '100%',
  },
  legendTitle: { fontSize: 14, fontWeight: 'bold', marginBottom: 6 },
  legendItem: { flexDirection: 'row', alignItems: 'center', marginVertical: 3 },
  legendColor: { width: 16, height: 16, borderRadius: 3, marginRight: 8 },
  legendText: { fontSize: 13, color: '#333' },
  gpsContainer: {
    marginTop: 12,
    padding: 10,
    backgroundColor: '#eef5ee',
    borderRadius: 8,
    width: '100%',
  },
  gpsTitle: { fontSize: 14, fontWeight: 'bold', color: '#1a3a1a', marginBottom: 4 },
  gpsText: { fontSize: 12, color: '#555', marginTop: 2 },
  actionRow: { flexDirection: 'row', marginTop: 16, gap: 8 },
  actionButton: {
    backgroundColor: '#1a3a1a',
    padding: 12,
    borderRadius: 8,
    flex: 1,
    alignItems: 'center',
  },
  saveButton: {
    backgroundColor: '#2d8a2d',
    padding: 12,
    borderRadius: 8,
    flex: 1,
    alignItems: 'center',
  },
  actionButtonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  footer: { textAlign: 'center', fontSize: 11, color: '#aaa', marginVertical: 12 },
});

// EOF — TomogramScreen.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT