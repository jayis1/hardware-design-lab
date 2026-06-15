/**
 * screens/RadarScreen.js — Real-time radar point cloud visualization
 *
 * Displays detected points as a 2D top-down range/angle plot.
 * Color-coded by velocity (blue=approaching, red=receding).
 * Updates at ~20 fps via BLE/USB protocol.
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Dimensions,
  Alert,
} from 'react-native';
import { Svg, Circle, Line, Text as SvgText, G } from 'react-native-svg';
import RadarPlot from '../components/RadarPlot';
import { protocol } from '../../App';

const WINDOW_WIDTH = Dimensions.get('window').width;
const PLOT_SIZE = WINDOW_WIDTH - 40;

export default function RadarScreen({ navigation }) {
  const [points, setPoints] = useState([]);
  const [connected, setConnected] = useState(false);
  const [radarStatus, setRadarStatus] = useState('Idle');
  const [pointCount, setPointCount] = useState(0);
  const [frameRate, setFrameRate] = useState(0);
  const frameCountRef = useRef(0);
  const lastTimeRef = useRef(Date.now());

  // Handle incoming point cloud data
  const handlePointCloud = useCallback((frame) => {
    if (frame && frame.points) {
      setPoints(frame.points);
      setPointCount(frame.points.length);

      // Calculate frame rate
      frameCountRef.current++;
      const now = Date.now();
      if (now - lastTimeRef.current >= 1000) {
        setFrameRate(frameCountRef.current);
        frameCountRef.current = 0;
        lastTimeRef.current = now;
      }
    }
  }, []);

  useEffect(() => {
    protocol.onPointCloud(handlePointCloud);
    protocol.onConnectionChange((isConnected) => {
      setConnected(isConnected);
      setRadarStatus(isConnected ? 'Connected' : 'Disconnected');
    });

    return () => {
      protocol.offPointCloud(handlePointCloud);
    };
  }, [handlePointCloud]);

  const handleConnect = () => {
    if (connected) {
      protocol.disconnect();
    } else {
      navigation.navigate('Connection');
    }
  };

  const handleStartStop = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to PicoRadar first.');
      return;
    }
    if (radarStatus === 'Running') {
      protocol.sendCommand(ProtocolCommands.CMD_RADAR_STOP);
      setRadarStatus('Idle');
    } else {
      protocol.sendCommand(ProtocolCommands.CMD_RADAR_START);
      setRadarStatus('Running');
    }
  };

  return (
    <View style={styles.container}>
      {/* Status Bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <View
            style={[
              styles.statusDot,
              { backgroundColor: connected ? '#4CD964' : '#FF3B30' },
            ]}
          />
          <Text style={styles.statusText}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <Text style={styles.statusText}>Status: {radarStatus}</Text>
        <Text style={styles.statusText}>Points: {pointCount}</Text>
        <Text style={styles.statusText}>{frameRate} fps</Text>
      </View>

      {/* Radar Plot */}
      <View style={styles.plotContainer}>
        <RadarPlot
          points={points}
          size={PLOT_SIZE}
          maxRange={12}
          showGrid={true}
        />
      </View>

      {/* Info Panel */}
      <View style={styles.infoPanel}>
        <Text style={styles.infoText}>
          Range: 0–12 m | Angle: ±45° | BW: 4 GHz
        </Text>
        <Text style={styles.infoText}>
          Resolution: R=3.75 cm | V=0.3 m/s | θ=15°
        </Text>
      </View>

      {/* Control Buttons */}
      <View style={styles.controls}>
        <TouchableOpacity
          style={[styles.button, !connected && styles.buttonDisabled]}
          onPress={handleStartStop}
        >
          <Text style={styles.buttonText}>
            {radarStatus === 'Running' ? 'Stop' : 'Start'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.button} onPress={handleConnect}>
          <Text style={styles.buttonText}>
            {connected ? 'Disconnect' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#000000',
    padding: 10,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    paddingHorizontal: 4,
    borderBottomWidth: 1,
    borderBottomColor: '#333333',
  },
  statusItem: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 6,
  },
  statusText: {
    color: '#AAAAAA',
    fontSize: 12,
  },
  plotContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  infoPanel: {
    paddingVertical: 8,
    borderTopWidth: 1,
    borderTopColor: '#333333',
  },
  infoText: {
    color: '#888888',
    fontSize: 11,
    textAlign: 'center',
  },
  controls: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingVertical: 12,
  },
  button: {
    backgroundColor: '#007AFF',
    paddingVertical: 12,
    paddingHorizontal: 32,
    borderRadius: 8,
    minWidth: 120,
    alignItems: 'center',
  },
  buttonDisabled: {
    backgroundColor: '#555555',
  },
  buttonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '600',
  },
});