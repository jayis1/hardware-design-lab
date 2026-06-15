/**
 * DataViewScreen — CV input monitor and real-time data display
 * Shows analog CV input levels, MIDI activity, voice allocator status
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import Slider from 'react-native-slider';
import { WaveForgeProtocol } from '../utils/protocol';

const PROTO = new WaveForgeProtocol();

export default function DataViewScreen({ navigation }) {
  const [cvValues, setCvValues] = useState([0, 0, 0, 0]);
  const [cvVoltages, setCvVoltages] = useState([0.0, 0.0, 0.0, 0.0]);
  const [voiceCount, setVoiceCount] = useState(0);
  const [midiActivity, setMidiActivity] = useState(false);
  const [sampleRate, setSampleRate] = useState(48000);
  const [cpuLoad, setCpuLoad] = useState(0);
  const [bufferLevel, setBufferLevel] = useState(0.5);
  const intervalRef = useRef(null);

  // Simulated data polling
  useEffect(() => {
    intervalRef.current = setInterval(() => {
      // In production: request data from device via BLE/USB
      // Simulate CV values with slight random variation
      setCvValues(prev => prev.map(v => {
        const newV = v + (Math.random() - 0.5) * 50;
        return Math.max(0, Math.min(4095, newV));
      }));
      setCvVoltages(prev => prev.map((v, i) => (cvValues[i] / 4095 * 5.0).toFixed(2)));
      setVoiceCount(Math.floor(Math.random() * 8));
      setCpuLoad(prev => Math.max(5, Math.min(95, prev + (Math.random() - 0.5) * 10)));
      setBufferLevel(prev => Math.max(0.1, Math.min(0.9, prev + (Math.random() - 0.5) * 0.1)));
    }, 100);

    return () => {
      if (intervalRef.current) clearInterval(intervalRef.current);
    };
  }, []);

  const cvLabels = ['CV1 (Pitch)', 'CV2 (Filter)', 'CV3 (Volume)', 'CV4 (LFO Rate)'];
  const cvColors = ['#e94560', '#0f3460', '#4caf50', '#ff9800'];

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* System Status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>System Status</Text>
        <View style={styles.statusGrid}>
          <View style={styles.statusItem}>
            <Text style={styles.statusLabel}>Sample Rate</Text>
            <Text style={styles.statusValue}>{sampleRate} Hz</Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusLabel}>CPU Load</Text>
            <Text style={[styles.statusValue, cpuLoad > 80 && styles.statusWarning]}>
              {cpuLoad.toFixed(0)}%
            </Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusLabel}>Active Voices</Text>
            <Text style={styles.statusValue}>{voiceCount}/64</Text>
          </View>
          <View style={styles.statusItem}>
            <Text style={styles.statusLabel}>MIDI</Text>
            <Text style={[styles.statusValue, midiActivity && styles.midiActive]}>
              {midiActivity ? '●' : '○'}
            </Text>
          </View>
        </View>
      </View>

      {/* Buffer Level */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Audio Buffer</Text>
        <View style={styles.bufferBar}>
          <View style={[styles.bufferFill, { width: `${bufferLevel * 100}%` }]} />
        </View>
        <Text style={styles.bufferLabel}>
          {`${(bufferLevel * 100).toFixed(0)}% — ${(bufferLevel * 256).toFixed(0)} / 256 samples`}
        </Text>
      </View>

      {/* CV Input Levels */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>CV Input Monitor</Text>
        {cvLabels.map((label, idx) => (
          <View key={idx} style={styles.cvItem}>
            <Text style={styles.cvName}>{label}</Text>
            <View style={styles.cvBarContainer}>
              <View style={[styles.cvBarFill, {
                width: `${(cvValues[idx] / 4095) * 100}%`,
                backgroundColor: cvColors[idx],
              }]} />
            </View>
            <View style={styles.cvValues}>
              <Text style={styles.cvValue}>
                {(cvValues[idx] / 4095 * 5.0).toFixed(2)}V
              </Text>
              <Text style={styles.cvRaw}>
                {Math.round(cvValues[idx])}
              </Text>
            </View>
          </View>
        ))}
      </View>

      {/* Voice Allocator */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Voice Allocator</Text>
        <View style={styles.voiceGrid}>
          {Array.from({ length: 16 }, (_, i) => (
            <View
              key={i}
              style={[
                styles.voiceCell,
                i < voiceCount && styles.voiceCellActive,
              ]}
            >
              <Text style={styles.voiceCellText}>{i + 1}</Text>
            </View>
          ))}
        </View>
        <Text style={styles.voiceNote}>
          Showing first 16 of 64 voices
        </Text>
      </View>

      {/* Raw Protocol Monitor */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Protocol Debug</Text>
        <Text style={styles.protocolLabel}>
          Last TX: {PROTO.buildFrame(PROTO.CMD_FILTER_CUTOFF, 0.8).toString('hex')}
        </Text>
        <Text style={styles.protocolLabel}>
          Frame format: [0xAA] [LEN] [CMD] [DATA...] [CRC8]
        </Text>
        <TouchableOpacity
          style={styles.protocolButton}
          onPress={() => {
            // Send test frame
            const frame = PROTO.buildFrame(PROTO.CMD_PING, 0);
            console.log('Ping:', frame.toString('hex'));
          }}
        >
          <Text style={styles.protocolButtonText}>Send Ping</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 16,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  statusGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
  },
  statusItem: {
    flex: 1,
    minWidth: '45%',
    backgroundColor: '#0f3460',
    borderRadius: 8,
    padding: 12,
  },
  statusLabel: {
    color: '#a0a0a0',
    fontSize: 12,
  },
  statusValue: {
    color: '#e0e0e0',
    fontSize: 20,
    fontWeight: 'bold',
  },
  statusWarning: {
    color: '#ff9800',
  },
  midiActive: {
    color: '#4caf50',
  },
  bufferBar: {
    height: 24,
    backgroundColor: '#0f3460',
    borderRadius: 12,
    overflow: 'hidden',
    marginBottom: 8,
  },
  bufferFill: {
    height: '100%',
    backgroundColor: '#4caf50',
    borderRadius: 12,
  },
  bufferLabel: {
    color: '#a0a0a0',
    fontSize: 13,
  },
  cvItem: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
  },
  cvName: {
    color: '#e0e0e0',
    fontSize: 13,
    width: 110,
  },
  cvBarContainer: {
    flex: 1,
    height: 16,
    backgroundColor: '#0f3460',
    borderRadius: 8,
    overflow: 'hidden',
    marginRight: 8,
  },
  cvBarFill: {
    height: '100%',
    borderRadius: 8,
  },
  cvValues: {
    width: 80,
    alignItems: 'flex-end',
  },
  cvValue: {
    color: '#e0e0e0',
    fontSize: 13,
    fontWeight: 'bold',
  },
  cvRaw: {
    color: '#666',
    fontSize: 11,
  },
  voiceGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 4,
  },
  voiceCell: {
    width: 28,
    height: 28,
    backgroundColor: '#0f3460',
    borderRadius: 4,
    alignItems: 'center',
    justifyContent: 'center',
  },
  voiceCellActive: {
    backgroundColor: '#e94560',
  },
  voiceCellText: {
    color: '#e0e0e0',
    fontSize: 10,
    fontWeight: 'bold',
  },
  voiceNote: {
    color: '#666',
    fontSize: 11,
    marginTop: 8,
  },
  protocolLabel: {
    color: '#a0a0a0',
    fontSize: 12,
    marginBottom: 4,
    fontFamily: 'monospace',
  },
  protocolButton: {
    backgroundColor: '#0f3460',
    borderRadius: 8,
    paddingVertical: 10,
    alignItems: 'center',
    marginTop: 8,
  },
  protocolButtonText: {
    color: '#e0e0e0',
    fontSize: 14,
  },
});