/**
 * NeuroLink Stream Screen — Live biosignal waveform display
 * Shows real-time channel data, connection status, and streaming controls
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Dimensions,
} from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import { useBle } from '../components/BleContext';
import { COMMAND_IDS, buildSetSampleRateCommand } from '../utils/protocol';

const SCREEN_WIDTH = Dimensions.get('window').width;
const NUM_DISPLAY_CHANNELS = 8;
const WAVEFORM_HISTORY = 100;

export default function StreamScreen() {
  const {
    connected,
    streaming,
    channelData,
    batteryLevel,
    startStreaming,
    stopStreaming,
    sendCommand,
    error,
  } = useBle();

  const [selectedChannel, setSelectedChannel] = useState(0);
  const [sampleRate, setSampleRate] = useState(1000);
  const [waveformHistory, setWaveformHistory] = useState(
    Array.from({ length: NUM_DISPLAY_CHANNELS }, () => [])
  );
  const prevDataRef = useRef(null);

  // Update waveform history when new data arrives
  useEffect(() => {
    if (channelData && channelData !== prevDataRef.current) {
      prevDataRef.current = channelData;
      setWaveformHistory((prev) => {
        const next = prev.map((arr, i) => {
          const newVal = channelData[i] || 0;
          const updated = [...arr, newVal];
          return updated.length > WAVEFORM_HISTORY
            ? updated.slice(-WAVEFORM_HISTORY)
            : updated;
        });
        return next;
      });
    }
  }, [channelData]);

  const handleStartStop = useCallback(() => {
    if (streaming) {
      stopStreaming();
    } else {
      startStreaming();
    }
  }, [streaming, startStreaming, stopStreaming]);

  const handleSampleRateChange = useCallback((rate) => {
    setSampleRate(rate);
    sendCommand(COMMAND_IDS.SET_SAMPLE_RATE, [rate]);
  }, [sendCommand]);

  const channelLabels = [
    'Fp1', 'Fp2', 'F3', 'F4', 'C3', 'C4', 'P3', 'P4',
    'O1', 'O2', 'F7', 'F8', 'T3', 'T4', 'T5', 'T6',
    'Fz', 'Cz', 'Pz', 'Oz', 'A1', 'A2', 'EMG1', 'EMG2',
    'EMG3', 'EMG4', 'EMG5', 'EMG6', 'EMG7', 'EMG8', 'GND', 'REF',
  ];

  return (
    <View style={styles.container}>
      {/* Status Bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Connection</Text>
          <View style={[styles.statusDot, connected && styles.statusDotActive]} />
          <Text style={styles.statusValue}>{connected ? 'Connected' : 'Disconnected'}</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Battery</Text>
          <Text style={styles.statusValue}>{batteryLevel}%</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Rate</Text>
          <Text style={styles.statusValue}>{sampleRate} SPS</Text>
        </View>
      </View>

      {/* Error Display */}
      {error && (
        <View style={styles.errorBar}>
          <Text style={styles.errorText}>{error}</Text>
        </View>
      )}

      {/* Waveform Display */}
      <View style={styles.chartContainer}>
        <Text style={styles.channelTitle}>
          {channelLabels[selectedChannel]} (CH {selectedChannel})
        </Text>
        <LineChart
          data={{
            labels: Array.from({ length: WAVEFORM_HISTORY }, (_, i) => ''),
            datasets: [{
              data: waveformHistory[selectedChannel]?.length > 0
                ? waveformHistory[selectedChannel]
                : [0],
              color: (opacity = 1) => `rgba(79, 195, 247, ${opacity})`,
              strokeWidth: 1.5,
            }],
          }}
          width={SCREEN_WIDTH - 32}
          height={200}
          chartConfig={{
            backgroundColor: '#0F0F1A',
            backgroundGradientFrom: '#0F0F1A',
            backgroundGradientTo: '#16213E',
            decimalPlaces: 1,
            color: (opacity = 1) => `rgba(79, 195, 247, ${opacity})`,
            labelColor: () => '#607D8B',
            style: { borderRadius: 8 },
            propsForDots: { r: '0' },
          }}
          style={styles.chart}
          withDots={false}
          withVerticalLabels={false}
          withHorizontalLabels={true}
          yAxisSuffix="µV"
        />
      </View>

      {/* Channel Selector */}
      <ScrollView horizontal style={styles.channelSelector} showsHorizontalScrollIndicator={false}>
        {channelLabels.slice(0, 16).map((label, i) => (
          <TouchableOpacity
            key={i}
            style={[
              styles.channelBtn,
              selectedChannel === i && styles.channelBtnActive,
            ]}
            onPress={() => setSelectedChannel(i)}
          >
            <Text style={[
              styles.channelBtnText,
              selectedChannel === i && styles.channelBtnTextActive,
            ]}>
              {label}
            </Text>
          </TouchableOpacity>
        ))}
      </ScrollView>

      {/* Sample Rate Selector */}
      <View style={styles.rateSelector}>
        <Text style={styles.rateLabel}>Sample Rate:</Text>
        {[250, 500, 1000, 2000, 4000].map((rate) => (
          <TouchableOpacity
            key={rate}
            style={[styles.rateBtn, sampleRate === rate && styles.rateBtnActive]}
            onPress={() => handleSampleRateChange(rate)}
          >
            <Text style={[styles.rateBtnText, sampleRate === rate && styles.rateBtnTextActive]}>
              {rate >= 1000 ? `${rate / 1000}k` : rate}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Start/Stop Button */}
      <TouchableOpacity
        style={[styles.streamBtn, streaming ? styles.streamBtnStop : styles.streamBtnStart]}
        onPress={handleStartStop}
        disabled={!connected}
      >
        <Text style={styles.streamBtnText}>
          {!connected ? 'No Device' : streaming ? '⏹ Stop Stream' : '▶ Start Stream'}
        </Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F1A',
    padding: 16,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 12,
    paddingHorizontal: 8,
    backgroundColor: '#16213E',
    borderRadius: 12,
    marginBottom: 12,
  },
  statusItem: {
    alignItems: 'center',
    flexDirection: 'row',
  },
  statusLabel: {
    color: '#607D8B',
    fontSize: 11,
    marginRight: 4,
    textTransform: 'uppercase',
  },
  statusValue: {
    color: '#E8E8E8',
    fontSize: 13,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: '#F44336',
    marginRight: 4,
  },
  statusDotActive: {
    backgroundColor: '#4CAF50',
  },
  errorBar: {
    backgroundColor: '#3E1A1A',
    borderRadius: 8,
    padding: 8,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#F44336',
  },
  errorText: {
    color: '#FF8A80',
    fontSize: 12,
  },
  chartContainer: {
    backgroundColor: '#16213E',
    borderRadius: 12,
    padding: 12,
    marginBottom: 12,
  },
  channelTitle: {
    color: '#4FC3F7',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 8,
    fontFamily: 'monospace',
  },
  chart: {
    borderRadius: 8,
  },
  channelSelector: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  channelBtn: {
    backgroundColor: '#1E2A45',
    borderRadius: 8,
    paddingHorizontal: 10,
    paddingVertical: 8,
    marginHorizontal: 3,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  channelBtnActive: {
    backgroundColor: '#1A3A5C',
    borderColor: '#4FC3F7',
  },
  channelBtnText: {
    color: '#78909C',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  channelBtnTextActive: {
    color: '#4FC3F7',
    fontWeight: '700',
  },
  rateSelector: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
  },
  rateLabel: {
    color: '#607D8B',
    fontSize: 12,
    marginRight: 8,
  },
  rateBtn: {
    backgroundColor: '#1E2A45',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 6,
    marginHorizontal: 4,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  rateBtnActive: {
    backgroundColor: '#1A3A5C',
    borderColor: '#4FC3F7',
  },
  rateBtnText: {
    color: '#78909C',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  rateBtnTextActive: {
    color: '#4FC3F7',
    fontWeight: '600',
  },
  streamBtn: {
    borderRadius: 12,
    paddingVertical: 16,
    alignItems: 'center',
    justifyContent: 'center',
  },
  streamBtnStart: {
    backgroundColor: '#1B5E20',
  },
  streamBtnStop: {
    backgroundColor: '#B71C1C',
  },
  streamBtnText: {
    color: '#FFFFFF',
    fontSize: 18,
    fontWeight: '700',
  },
});