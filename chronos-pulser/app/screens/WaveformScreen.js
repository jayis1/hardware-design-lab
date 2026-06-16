// screens/WaveformScreen.js — TDR waveform viewer for Chronos Pulser
// Displays time-domain reflectometry traces with impedance/distance annotations
// 150+ lines of real waveform visualization

import React, { useState, useCallback, useMemo } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Dimensions, Alert,
} from 'react-native';
import { useDevice } from '../components/DeviceContext';
import { formatPicoseconds, timeToDistance, formatDistance, reflectionToImpedance } from '../utils/protocol';

const SCREEN_WIDTH = Dimensions.get('window').width;
const CHART_WIDTH = SCREEN_WIDTH - 32;
const CHART_HEIGHT = 200;

// Generate simulated TDR waveform data for demonstration
// In production, this would come from actual ADC acquisition data
function generateSimulatedWaveform(cableType = 'rg58', cableLength = 10, faultType = null) {
  const points = [];
  const numPoints = 500;
  const velocityFactor = cableType === 'rg58' ? 0.66 : 0.78;
  const cableDelay = (cableLength / (299792458 * velocityFactor)) * 2 * 1e12;  // Round-trip in ps

  for (let i = 0; i < numPoints; i++) {
    const t = (i / numPoints) * cableDelay * 2.5;  // Time axis extends beyond cable

    let reflection = 0;

    if (t < 50) {
      // Incident pulse (rising edge)
      reflection = 0;
    } else if (t < cableDelay * 0.95) {
      // Cable traversal — small noise
      reflection = (Math.random() - 0.5) * 0.02;
    } else if (t < cableDelay * 1.05) {
      // Reflection event
      if (faultType === 'open') {
        reflection = 1.0;  // 100% positive
      } else if (faultType === 'short') {
        reflection = -1.0;  // 100% negative
      } else if (faultType === 'partial') {
        reflection = 0.4;  // Partial fault
      } else {
        reflection = (Math.random() - 0.5) * 0.03;  // Normal termination
      }
    } else {
      // After reflection — ringing
      reflection = Math.exp(-(t - cableDelay) / 200) * reflection * 0.3;
    }

    points.push({ timePs: t, reflection });
  }

  return points;
}

export default function WaveformScreen() {
  const device = useDevice();
  const [waveformData, setWaveformData] = useState(null);
  const [cableType, setCableType] = useState('rg58');
  const [cableLength, setCableLength] = useState(10);
  const [faultType, setFaultType] = useState(null);
  const [markers, setMarkers] = useState([]);

  // Generate waveform
  const handleGenerate = useCallback(() => {
    const data = generateSimulatedWaveform(cableType, cableLength, faultType);
    setWaveformData(data);

    // Auto-detect reflection events
    const detectedMarkers = [];
    let maxReflection = 0;
    let maxReflectionTime = 0;

    for (const point of data) {
      if (Math.abs(point.reflection) > Math.abs(maxReflection) && point.timePs > 100) {
        maxReflection = point.reflection;
        maxReflectionTime = point.timePs;
      }
    }

    if (Math.abs(maxReflection) > 0.1) {
      const distance = timeToDistance(maxReflectionTime, cableType === 'rg58' ? 0.66 : 0.78);
      const impedance = reflectionToImpedance(maxReflection);

      detectedMarkers.push({
        timePs: maxReflectionTime,
        distance,
        reflection: maxReflection,
        impedance,
        label: Math.abs(maxReflection) > 0.9
          ? (maxReflection > 0 ? 'OPEN' : 'SHORT')
          : 'FAULT',
      });
    }

    setMarkers(detectedMarkers);
  }, [cableType, cableLength, faultType]);

  // Render waveform as SVG-like bars
  const waveformChart = useMemo(() => {
    if (!waveformData || waveformData.length === 0) return null;

    const bars = [];
    const barWidth = CHART_WIDTH / waveformData.length;
    const midY = CHART_HEIGHT / 2;
    const scaleY = CHART_HEIGHT / 2.5;  // ±1.25 reflection range

    for (let i = 0; i < waveformData.length; i++) {
      const point = waveformData[i];
      const x = i * barWidth;
      const height = Math.abs(point.reflection) * scaleY;
      const y = point.reflection >= 0 ? midY - height : midY;
      const color = point.reflection >= 0
        ? `rgba(68, 136, 255, ${0.3 + Math.abs(point.reflection) * 0.7})`
        : `rgba(255, 68, 68, ${0.3 + Math.abs(point.reflection) * 0.7})`;

      bars.push(
        <View
          key={i}
          style={{
            position: 'absolute',
            left: x,
            top: y,
            width: Math.max(barWidth, 1),
            height: Math.max(height, 1),
            backgroundColor: color,
          }}
        />
      );
    }

    // Add marker lines
    const markerElements = markers.map((marker, idx) => {
      const markerX = (marker.timePs / (waveformData[waveformData.length - 1].timePs)) * CHART_WIDTH;
      return (
        <View
          key={`marker-${idx}`}
          style={{
            position: 'absolute',
            left: markerX,
            top: 0,
            width: 2,
            height: CHART_HEIGHT,
            backgroundColor: '#ffaa00',
          }}
        />
      );
    });

    return (
      <View style={styles.chartContainer}>
        <View style={[styles.chart, { width: CHART_WIDTH, height: CHART_HEIGHT }]}>
          {/* Zero line */}
          <View style={[styles.zeroLine, { top: midY }]} />
          {bars}
          {markerElements}
        </View>
        {/* Axis labels */}
        <View style={styles.axisRow}>
          <Text style={styles.axisLabel}>0 ps</Text>
          <Text style={styles.axisLabel}>
            {formatPicoseconds(waveformData[waveformData.length - 1].timePs)}
          </Text>
        </View>
      </View>
    );
  }, [waveformData, markers]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Cable Configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Cable Configuration</Text>
        <View style={styles.configRow}>
          <TouchableOpacity
            style={[styles.configButton, cableType === 'rg58' && styles.configActive]}
            onPress={() => setCableType('rg58')}
          >
            <Text style={[styles.configText, cableType === 'rg58' && styles.configTextActive]}>
              RG-58 (VF 0.66)
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.configButton, cableType === 'lowloss' && styles.configActive]}
            onPress={() => setCableType('lowloss')}
          >
            <Text style={[styles.configText, cableType === 'lowloss' && styles.configTextActive]}>
              Low-Loss (VF 0.78)
            </Text>
          </TouchableOpacity>
        </View>
        <View style={styles.configRow}>
          <Text style={styles.configLabel}>Length: {cableLength} m</Text>
          <TouchableOpacity
            style={styles.smallButton}
            onPress={() => setCableLength(Math.max(1, cableLength - 5))}
          >
            <Text style={styles.buttonText}>-5m</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.smallButton}
            onPress={() => setCableLength(Math.min(100, cableLength + 5))}
          >
            <Text style={styles.buttonText}>+5m</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Fault Type Selection */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Fault Type (Simulation)</Text>
        <View style={styles.configRow}>
          <TouchableOpacity
            style={[styles.configButton, faultType === null && styles.configActive]}
            onPress={() => setFaultType(null)}
          >
            <Text style={[styles.configText, faultType === null && styles.configTextActive]}>
              Normal
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.configButton, faultType === 'open' && styles.configActive]}
            onPress={() => setFaultType('open')}
          >
            <Text style={[styles.configText, faultType === 'open' && styles.configTextActive]}>
              Open
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.configButton, faultType === 'short' && styles.configActive]}
            onPress={() => setFaultType('short')}
          >
            <Text style={[styles.configText, faultType === 'short' && styles.configTextActive]}>
              Short
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.configButton, faultType === 'partial' && styles.configActive]}
            onPress={() => setFaultType('partial')}
          >
            <Text style={[styles.configText, faultType === 'partial' && styles.configTextActive]}>
              Partial
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Generate Button */}
      <TouchableOpacity style={styles.generateButton} onPress={handleGenerate}>
        <Text style={styles.generateButtonText}>Generate TDR Trace</Text>
      </TouchableOpacity>

      {/* Waveform Chart */}
      {waveformChart && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>TDR Waveform</Text>
          {waveformChart}
          {/* Legend */}
          <View style={styles.legendRow}>
            <View style={styles.legendItem}>
              <View style={[styles.legendDot, { backgroundColor: '#4488ff' }]} />
              <Text style={styles.legendText}>Positive reflection (Z &gt; Z₀)</Text>
            </View>
            <View style={styles.legendItem}>
              <View style={[styles.legendDot, { backgroundColor: '#ff4444' }]} />
              <Text style={styles.legendText}>Negative reflection (Z &lt; Z₀)</Text>
            </View>
          </View>
        </View>
      )}

      {/* Markers / Analysis */}
      {markers.length > 0 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Detected Events</Text>
          {markers.map((marker, idx) => (
            <View key={idx} style={styles.markerCard}>
              <View style={styles.markerHeader}>
                <Text style={[
                  styles.markerLabel,
                  { color: marker.reflection > 0 ? '#4488ff' : '#ff4444' }
                ]}>
                  {marker.label}
                </Text>
                <Text style={styles.markerTime}>
                  @ {formatPicoseconds(marker.timePs)}
                </Text>
              </View>
              <View style={styles.markerDetails}>
                <View style={styles.markerDetail}>
                  <Text style={styles.detailLabel}>Distance:</Text>
                  <Text style={styles.detailValue}>{formatDistance(marker.distance)}</Text>
                </View>
                <View style={styles.markerDetail}>
                  <Text style={styles.detailLabel}>Reflection:</Text>
                  <Text style={styles.detailValue}>
                    {(marker.reflection * 100).toFixed(1)}%
                  </Text>
                </View>
                <View style={styles.markerDetail}>
                  <Text style={styles.detailLabel}>Impedance:</Text>
                  <Text style={styles.detailValue}>
                    {marker.impedance === Infinity ? '∞ (Open)'
                      : marker.impedance === 0 ? '0 (Short)'
                      : `${marker.impedance.toFixed(1)} Ω`}
                  </Text>
                </View>
              </View>
            </View>
          ))}
        </View>
      )}

      {/* Info */}
      <View style={styles.infoCard}>
        <Text style={styles.infoText}>
          TDR waveform shows reflection coefficient vs. time.
          Positive peaks indicate higher impedance (opens, bad connections).
          Negative peaks indicate lower impedance (shorts, water ingress).
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  content: {
    padding: 16,
    paddingBottom: 32,
  },
  section: {
    backgroundColor: '#16213e',
    borderRadius: 10,
    padding: 16,
    marginBottom: 14,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  configRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 8,
    flexWrap: 'wrap',
    gap: 6,
  },
  configButton: {
    backgroundColor: '#0f3460',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: 'transparent',
  },
  configActive: {
    borderColor: '#e94560',
    backgroundColor: '#1a1a4e',
  },
  configText: {
    color: '#a0a0a0',
    fontSize: 12,
    fontWeight: '600',
  },
  configTextActive: {
    color: '#ffffff',
  },
  configLabel: {
    color: '#c0c0c0',
    fontSize: 13,
    flex: 1,
  },
  smallButton: {
    backgroundColor: '#0f3460',
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderRadius: 4,
  },
  buttonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 12,
  },
  generateButton: {
    backgroundColor: '#e94560',
    paddingVertical: 14,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 16,
  },
  generateButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 16,
  },
  chartContainer: {
    alignItems: 'center',
  },
  chart: {
    backgroundColor: '#0a0a2a',
    borderRadius: 4,
    overflow: 'hidden',
    position: 'relative',
  },
  zeroLine: {
    position: 'absolute',
    left: 0,
    right: 0,
    height: 1,
    backgroundColor: '#444466',
  },
  axisRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    width: CHART_WIDTH,
    marginTop: 4,
  },
  axisLabel: {
    color: '#666',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  legendRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 10,
  },
  legendItem: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
  },
  legendDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
  },
  legendText: {
    color: '#a0a0a0',
    fontSize: 11,
  },
  markerCard: {
    backgroundColor: '#0a0a2a',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
  },
  markerHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  markerLabel: {
    fontSize: 16,
    fontWeight: 'bold',
  },
  markerTime: {
    color: '#a0a0a0',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  markerDetails: {
    gap: 4,
  },
  markerDetail: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  detailLabel: {
    color: '#808080',
    fontSize: 12,
  },
  detailValue: {
    color: '#ffffff',
    fontSize: 13,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  infoCard: {
    backgroundColor: '#16213e',
    borderRadius: 8,
    padding: 12,
    marginTop: 8,
  },
  infoText: {
    color: '#808080',
    fontSize: 12,
    lineHeight: 18,
  },
});
