/**
 * LiveScanScreen.js — SonicSight Companion
 * Real-time tomogram display with velocity colour mapping, signal quality
 * indicators, and gain/filter controls.
 * @author jayis1
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Dimensions,
  PanResponder, Slider, Switch,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import TomogramView from '../components/TomogramView';
import SignalQualityBar from '../components/SignalQualityBar';

const SCREEN_WIDTH = Dimensions.get('window').width;
const TOMO_SIZE = Math.min(SCREEN_WIDTH - 32, 360);

const LiveScanScreen = ({ route }) => {
  const bleService = route?.params?.bleService;
  const [isScanning, setIsScanning] = useState(false);
  const [isContinuous, setIsContinuous] = useState(false);
  const [gain, setGain] = useState(20);
  const [filterFc, setFilterFc] = useState(50);
  const [lambda, setLambda] = useState(0.5);
  const [tomogramData, setTomogramData] = useState(null);
  const [velMin, setVelMin] = useState(0);
  const [velMax, setVelMax] = useState(0);
  const [signalQuality, setSignalQuality] = useState(
    Array(32).fill(0).map(() => ({ snr: 0, active: true }))
  );
  const [progress, setProgress] = useState({ current: 0, total: 32 });
  const [crosshairValue, setCrosshairValue] = useState(null);

  const crosshairPos = useRef({ x: TOMO_SIZE / 2, y: TOMO_SIZE / 2 });

  useEffect(() => {
    if (!bleService) return;

    const sub = bleService.onTomogram((data) => {
      setTomogramData(data.image);
      setVelMin(data.velMin);
      setVelMax(data.velMax);
      setSignalQuality(data.signalQuality || signalQuality);
    });

    const progressSub = bleService.onProgress((p) => {
      setProgress(p);
    });

    return () => {
      sub?.remove();
      progressSub?.remove();
    };
  }, [bleService]);

  const handleStartScan = () => {
    setIsScanning(true);
    bleService?.sendCommand('CMD=START');
    if (!isContinuous) {
      setTimeout(() => setIsScanning(false), 5000);
    }
  };

  const handleStopScan = () => {
    setIsScanning(false);
    bleService?.sendCommand('CMD=STOP');
  };

  const panResponder = PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onMoveShouldSetPanResponder: () => true,
    onPanResponderGrant: (evt) => {
      const { locationX, locationY } = evt.nativeEvent;
      crosshairPos.current = { x: locationX, y: locationY };
      updateCrosshairValue(locationX, locationY);
    },
    onPanResponderMove: (evt) => {
      const { locationX, locationY } = evt.nativeEvent;
      crosshairPos.current = { x: locationX, y: locationY };
      updateCrosshairValue(locationX, locationY);
    },
  });

  const updateCrosshairValue = (x, y) => {
    if (!tomogramData) return;
    const gridSize = 64;
    const scale = TOMO_SIZE / gridSize;
    const gx = Math.floor(x / scale);
    const gy = Math.floor(y / scale);
    if (gx >= 0 && gx < gridSize && gy >= 0 && gy < gridSize) {
      const vel = tomogramData[gy * gridSize + gx];
      setCrosshairValue(vel > 0 ? vel.toFixed(0) : '—');
    }
  };

  return (
    <View style={styles.container}>
      {/* Controls Bar */}
      <View style={styles.controlsBar}>
        <TouchableOpacity
          style={[styles.scanButton, isScanning && styles.scanButtonActive]}
          onPress={isScanning ? handleStopScan : handleStartScan}>
          <Icon name={isScanning ? 'stop-circle' : 'play-circle'}
                size={28} color="#fff" />
          <Text style={styles.scanButtonText}>
            {isScanning ? 'Stop' : 'Scan'}
          </Text>
        </TouchableOpacity>

        <View style={styles.controlGroup}>
          <Text style={styles.controlLabel}>Continuous</Text>
          <Switch value={isContinuous} onValueChange={setIsContinuous}
                  trackColor={{ false: '#333', true: '#00d2ff' }}
                  thumbColor={isContinuous ? '#fff' : '#aaa'} />
        </View>

        <View style={styles.controlGroup}>
          <Text style={styles.controlLabel}>Gain</Text>
          <Text style={styles.controlValue}>{gain} dB</Text>
          <Slider style={styles.slider} minimumValue={0} maximumValue={40}
                  value={gain} onValueChange={(v) => {
                    setGain(Math.round(v));
                    bleService?.sendCommand(`CMD=GAIN:${Math.round(v)}`);
                  }}
                  minimumTrackTintColor="#00d2ff"
                  maximumTrackTintColor="#333"
                  thumbTintColor="#fff" />
        </View>
      </View>

      {/* Tomogram View */}
      <View style={styles.tomogramContainer} {...panResponder.panHandlers}>
        <TomogramView
          data={tomogramData}
          width={TOMO_SIZE}
          height={TOMO_SIZE}
          crosshair={crosshairPos.current}
          velMin={velMin}
          velMax={velMax}
        />
        {crosshairValue && (
          <View style={styles.crosshairLabel}>
            <Text style={styles.crosshairText}>{crosshairValue} m/s</Text>
          </View>
        )}
      </View>

      {/* Progress & Stats Bar */}
      <View style={styles.statsBar}>
        <Text style={styles.statsText}>
          Ray: {progress.current}/{progress.total}
        </Text>
        <Text style={styles.statsText}>
          Vmin: {velMin.toFixed(0)} m/s
        </Text>
        <Text style={styles.statsText}>
          Vmax: {velMax.toFixed(0)} m/s
        </Text>
      </View>

      {/* Signal Quality Bars */}
      <View style={styles.signalSection}>
        <Text style={styles.sectionLabel}>Signal Quality</Text>
        <SignalQualityBar data={signalQuality} />
      </View>

      {/* Additional Controls */}
      <View style={styles.extraControls}>
        <View style={styles.controlGroup}>
          <Text style={styles.controlLabel}>Filter (kHz)</Text>
          <Slider style={{ width: 100 }} minimumValue={20} maximumValue={200}
                  value={filterFc} onValueChange={(v) => {
                    setFilterFc(Math.round(v));
                    bleService?.sendCommand(`CMD=FILTER:${Math.round(v)}`);
                  }}
                  minimumTrackTintColor="#ff9800"
                  maximumTrackTintColor="#333"
                  thumbTintColor="#fff" />
          <Text style={styles.controlValue}>{filterFc} kHz</Text>
        </View>
        <View style={styles.controlGroup}>
          <Text style={styles.controlLabel}>λ (reg)</Text>
          <Slider style={{ width: 100 }} minimumValue={0.01} maximumValue={100}
                  value={lambda} onValueChange={(v) => {
                    setLambda(v);
                    bleService?.sendCommand(`CMD=LAMBDA:${v.toFixed(3)}`);
                  }}
                  minimumTrackTintColor="#e040fb"
                  maximumTrackTintColor="#333"
                  thumbTintColor="#fff" />
          <Text style={styles.controlValue}>{lambda.toFixed(2)}</Text>
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23' },
  controlsBar: {
    flexDirection: 'row', alignItems: 'center',
    padding: 8, backgroundColor: '#1a1a2e',
    borderBottomWidth: 1, borderBottomColor: '#16213e',
  },
  scanButton: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#4caf50', borderRadius: 8,
    padding: 8, paddingHorizontal: 16, marginRight: 12,
  },
  scanButtonActive: { backgroundColor: '#f44336' },
  scanButtonText: { color: '#fff', fontSize: 14, fontWeight: 'bold', marginLeft: 4 },
  controlGroup: { flexDirection: 'row', alignItems: 'center', marginHorizontal: 4 },
  controlLabel: { color: '#aaa', fontSize: 11, marginRight: 4 },
  controlValue: { color: '#fff', fontSize: 12, minWidth: 40, textAlign: 'right' },
  slider: { width: 60, height: 20 },
  tomogramContainer: {
    alignSelf: 'center', margin: 12,
    borderRadius: 8, overflow: 'hidden',
    backgroundColor: '#000',
    borderWidth: 2, borderColor: '#16213e',
  },
  crosshairLabel: {
    position: 'absolute', bottom: 4, right: 4,
    backgroundColor: 'rgba(0,0,0,0.7)', borderRadius: 4,
    padding: 4, paddingHorizontal: 8,
  },
  crosshairText: { color: '#fff', fontSize: 12 },
  statsBar: {
    flexDirection: 'row', justifyContent: 'space-around',
    backgroundColor: '#1a1a2e', padding: 8, marginHorizontal: 12,
    borderRadius: 8, marginBottom: 8,
  },
  statsText: { color: '#00d2ff', fontSize: 12 },
  signalSection: {
    marginHorizontal: 12, marginBottom: 8,
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 8,
  },
  sectionLabel: { color: '#aaa', fontSize: 12, marginBottom: 4 },
  extraControls: {
    flexDirection: 'row', justifyContent: 'space-around',
    backgroundColor: '#1a1a2e', marginHorizontal: 12,
    borderRadius: 8, padding: 8, marginBottom: 16,
  },
});

export default LiveScanScreen;