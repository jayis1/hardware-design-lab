/**
 * WaveformScreen.js — Main oscilloscope waveform display
 * Shows real-time waveforms from all 4 analog + 2 digital channels
 * Supports touch zoom, pan, trigger point marker, and measurement cursors
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import { View, StyleSheet, Dimensions, PanResponder, Text } from 'react-native';
import { Button, IconButton, Surface, Divider } from 'react-native-paper';
import WaveformView from '../components/WaveformView';
import TriggerControls from '../components/TriggerControls';
import { HexaScopeProtocol, COMMANDS } from '../utils/protocol';

const { width: SCREEN_WIDTH, height: SCREEN_HEIGHT } = Dimensions.get('window');

// Channel colors
const CHANNEL_COLORS = {
  ch1: '#FFD700',  // Gold
  ch2: '#00FF00',  // Green
  ch3: '#FF4444',  // Red
  ch4: '#4488FF',  // Blue
  d1:  '#FF00FF',  // Magenta
  d2:  '#00FFFF',  // Cyan
};

// Vertical sensitivity options (V/div)
const VDIV_OPTIONS = [0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5];

// Timebase options (s/div)
const TDIV_OPTIONS = [
  0.000000002, 0.000000005, 0.00000001, 0.00000002, 0.00000005,
  0.0000001, 0.0000002, 0.0000005, 0.001, 0.002, 0.005,
  0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5
];

export default function WaveformScreen({ navigation }) {
  const [running, setRunning] = useState(false);
  const [channelData, setChannelData] = useState({
    ch1: new Uint8Array(1000),
    ch2: new Uint8Array(1000),
    ch3: new Uint8Array(1000),
    ch4: new Uint8Array(1000),
    d1: new Uint8Array(1000),
    d2: new Uint8Array(1000),
  });
  const [vdiv, setVdiv] = useState(0);       // Index into VDIV_OPTIONS
  const [tdiv, setTdiv] = useState(8);        // Index into TDIV_OPTIONS (100 µs/div)
  const [triggerLevel, setTriggerLevel] = useState(128); // 8-bit threshold
  const [triggerChannel, setTriggerChannel] = useState(0);
  const [triggerType, setTriggerType] = useState('rising');
  const [connected, setConnected] = useState(false);
  const [measurements, setMeasurements] = useState({
    frequency: 0, vpp: 0, vrms: 0, vavg: 0
  });

  const protocolRef = useRef(null);
  const frameCountRef = useRef(0);

  // Initialize protocol handler
  useEffect(() => {
    const proto = new HexaScopeProtocol();
    proto.onData = (frame) => {
      setChannelData(prev => ({
        ...prev,
        ch1: frame.channels[0] || prev.ch1,
        ch2: frame.channels[1] || prev.ch2,
        ch3: frame.channels[2] || prev.ch3,
        ch4: frame.channels[3] || prev.ch4,
        d1: frame.channels[4] || prev.d1,
        d2: frame.channels[5] || prev.d2,
      }));
      frameCountRef.current++;
      // Compute measurements from channel 1 data
      if (frame.channels[0]) {
        computeMeasurements(frame.channels[0]);
      }
    };
    proto.onConnect = () => setConnected(true);
    proto.onDisconnect = () => setConnected(false);
    protocolRef.current = proto;
    proto.start();

    return () => proto.stop();
  }, []);

  // Compute basic measurements from waveform data
  const computeMeasurements = useCallback((data) => {
    let vpp = 0, vsum = 0, vsumsq = 0;
    let vmin = 255, vmax = 0;
    for (let i = 0; i < data.length; i++) {
      const v = data[i];
      if (v < vmin) vmin = v;
      if (v > vmax) vmax = v;
      vsum += v;
      vsumsq += v * v;
    }
    vpp = vmax - vmin;
    const vavg = vsum / data.length;
    const vrms = Math.sqrt(vsumsq / data.length);

    // Simple frequency estimation: count zero crossings
    const mid = (vmax + vmin) / 2;
    let crossings = 0;
    for (let i = 1; i < data.length; i++) {
      if ((data[i - 1] < mid && data[i] >= mid) ||
          (data[i - 1] >= mid && data[i] < mid)) {
        crossings++;
      }
    }
    const freq = (crossings / 2) / (data.length * TDIV_OPTIONS[tdiv] * 10 / data.length);

    setMeasurements({
      vpp: (vpp * 20 / 256).toFixed(3),     // Convert to volts
      vrms: (vrms * 20 / 256).toFixed(3),
      vavg: (vavg * 20 / 256).toFixed(3),
      frequency: freq > 0 ? (freq >= 1e6 ? (freq / 1e6).toFixed(2) + ' MHz'
                            : freq >= 1e3 ? (freq / 1e3).toFixed(2) + ' kHz'
                            : freq.toFixed(2) + ' Hz') : '--'
    });
  }, [tdiv]);

  // Arm/stop trigger
  const handleArmStop = useCallback(() => {
    const proto = protocolRef.current;
    if (!proto) return;

    if (running) {
      // Stop
      proto.sendCommand(COMMANDS.STOP_ACQUISITION, 0, 0);
      setRunning(false);
    } else {
      // Arm trigger
      const trigType = triggerType === 'rising' ? 0x01 :
                       triggerType === 'falling' ? 0x02 :
                       triggerType === 'both' ? 0x03 : 0x01;
      proto.sendCommand(COMMANDS.ARM_TRIGGER, trigType, triggerChannel);
      setRunning(true);
    }
  }, [running, triggerType, triggerChannel]);

  // Single capture
  const handleSingle = useCallback(() => {
    const proto = protocolRef.current;
    if (!proto) return;
    const trigType = triggerType === 'rising' ? 0x01 :
                     triggerType === 'falling' ? 0x02 : 0x01;
    proto.sendCommand(COMMANDS.ARM_TRIGGER, trigType | 0x80, triggerChannel);
    setRunning(true);
  }, [triggerType, triggerChannel]);

  // Auto setup — find best V/div and T/div for current signal
  const handleAuto = useCallback(() => {
    const data = channelData.ch1;
    if (!data) return;
    let vmin = 255, vmax = 0;
    for (let i = 0; i < data.length; i++) {
      if (data[i] < vmin) vmin = data[i];
      if (data[i] > vmax) vmax = data[i];
    }
    const vrange = vmax - vmin;
    // Find V/div that uses ~60% of screen
    const targetRange = vrange * 256 / (vrange || 1) * 0.6;
    let bestVdiv = 0;
    for (let i = 0; i < VDIV_OPTIONS.length; i++) {
      if (VDIV_OPTIONS[i] * 8 >= targetRange * 20 / 256) {
        bestVdiv = i;
        break;
      }
    }
    setVdiv(bestVdiv);

    // Set trigger level to midpoint
    setTriggerLevel(Math.round((vmax + vmin) / 2));
  }, [channelData.ch1]);

  // Change timebase
  const handleTdivChange = useCallback((direction) => {
    setTdiv(prev => {
      const next = prev + direction;
      return Math.max(0, Math.min(TDIV_OPTIONS.length - 1, next));
    });
  }, []);

  // Change vertical sensitivity
  const handleVdivChange = useCallback((direction) => {
    setVdiv(prev => {
      const next = prev + direction;
      return Math.max(0, Math.min(VDIV_OPTIONS.length - 1, next));
    });
  }, []);

  // Format timebase for display
  const formatTdiv = (t) => {
    if (t >= 1) return t.toFixed(0) + ' s/div';
    if (t >= 0.001) return (t * 1000).toFixed(t >= 0.1 ? 0 : 1) + ' ms/div';
    if (t >= 0.000001) return (t * 1e6).toFixed(t >= 1e-5 ? 0 : 1) + ' µs/div';
    return (t * 1e9).toFixed(0) + ' ns/div';
  };

  const formatVdiv = (v) => {
    if (v >= 1) return v.toFixed(v >= 2 ? 0 : 1) + ' V/div';
    if (v >= 0.001) return (v * 1000).toFixed(v >= 0.1 ? 0 : 1) + ' mV/div';
    return (v * 1e6).toFixed(0) + ' µV/div';
  };

  return (
    <View style={styles.container}>
      {/* Top toolbar */}
      <View style={styles.toolbar}>
        <IconButton
          icon={running ? 'stop' : 'play'}
          mode="contained"
          onPress={handleArmStop}
          iconColor={running ? '#FF4444' : '#44FF44'}
          containerColor="#16213e"
        />
        <IconButton
          icon="ray-start"
          mode="contained"
          onPress={handleSingle}
          iconColor="#4488FF"
          containerColor="#16213e"
        />
        <IconButton
          icon="auto-fix"
          mode="contained"
          onPress={handleAuto}
          iconColor="#FFD700"
          containerColor="#16213e"
        />
        <View style={styles.toolbarSpacer} />
        <Text style={styles.statusText}>
          {connected ? '● CONNECTED' : '○ DISCONNECTED'}
        </Text>
      </View>

      {/* Waveform display area */}
      <Surface style={styles.waveformContainer}>
        <WaveformView
          channelData={channelData}
          colors={CHANNEL_COLORS}
          vdiv={VDIV_OPTIONS[vdiv]}
          tdiv={TDIV_OPTIONS[tdiv]}
          triggerLevel={triggerLevel}
          triggerChannel={triggerChannel}
          running={running}
        />
      </Surface>

      {/* Measurements bar */}
      <View style={styles.measurements}>
        <View style={styles.measurementItem}>
          <Text style={styles.measLabel}>Vpp</Text>
          <Text style={styles.measValue}>{measurements.vpp} V</Text>
        </View>
        <View style={styles.measurementItem}>
          <Text style={styles.measLabel}>Vrms</Text>
          <Text style={styles.measValue}>{measurements.vrms} V</Text>
        </View>
        <View style={styles.measurementItem}>
          <Text style={styles.measLabel}>Vavg</Text>
          <Text style={styles.measValue}>{measurements.vavg} V</Text>
        </View>
        <View style={styles.measurementItem}>
          <Text style={styles.measLabel}>Freq</Text>
          <Text style={styles.measValue}>{measurements.frequency}</Text>
        </View>
      </View>

      {/* Trigger controls */}
      <TriggerControls
        triggerLevel={triggerLevel}
        triggerChannel={triggerChannel}
        triggerType={triggerType}
        onTriggerLevelChange={setTriggerLevel}
        onTriggerChannelChange={setTriggerChannel}
        onTriggerTypeChange={setTriggerType}
      />

      {/* Bottom controls: timebase and vertical */}
      <View style={styles.bottomControls}>
        <View style={styles.controlGroup}>
          <IconButton icon="chevron-left" onPress={() => handleTdivChange(-1)} size={20} />
          <Text style={styles.controlLabel}>{formatTdiv(TDIV_OPTIONS[tdiv])}</Text>
          <IconButton icon="chevron-right" onPress={() => handleTdivChange(1)} size={20} />
        </View>
        <View style={styles.controlGroup}>
          <IconButton icon="chevron-left" onPress={() => handleVdivChange(-1)} size={20} />
          <Text style={styles.controlLabel}>{formatVdiv(VDIV_OPTIONS[vdiv])}</Text>
          <IconButton icon="chevron-right" onPress={() => handleVdivChange(1)} size={20} />
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  toolbar: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
    paddingHorizontal: 8,
    backgroundColor: '#16213e',
  },
  toolbarSpacer: {
    flex: 1,
  },
  statusText: {
    color: '#888',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  waveformContainer: {
    flex: 1,
    margin: 4,
    backgroundColor: '#0a0a1a',
    elevation: 4,
  },
  measurements: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingVertical: 6,
    backgroundColor: '#16213e',
  },
  measurementItem: {
    alignItems: 'center',
  },
  measLabel: {
    color: '#888',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  measValue: {
    color: '#00FF88',
    fontSize: 13,
    fontFamily: 'monospace',
  },
  bottomControls: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
    paddingHorizontal: 12,
    backgroundColor: '#16213e',
  },
  controlGroup: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  controlLabel: {
    color: '#FFFFFF',
    fontSize: 14,
    fontFamily: 'monospace',
    minWidth: 80,
    textAlign: 'center',
  },
});