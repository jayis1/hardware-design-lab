/**
 * LiveFlowScreen.js — Real-time perfusion map display
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Switch } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { useDevice, COLORMAPS, CMD } from '../utils/protocol';
import FlowMap from '../components/FlowMap';

export default function LiveFlowScreen() {
  const { connected, status, assembler, colormap, sendCommand, roi } = useDevice();
  const [imaging, setImaging] = useState(false);
  const [flowMap, setFlowMap] = useState(null);
  const [frameCount, setFrameCount] = useState(0);
  const [meanFlow, setMeanFlow] = useState(0);

  // Register frame callback
  useEffect(() => {
    if (assembler) {
      assembler.onFrameComplete = (frame, num) => {
        setFlowMap(new Uint8ClampedArray(frame));
        setFrameCount(num);

        // Compute mean flow in ROI
        let sum = 0, count = 0;
        for (let y = roi.y; y < roi.y + roi.h && y < 480; y++) {
          for (let x = roi.x; x < roi.x + roi.w && x < 640; x++) {
            sum += frame[y * 640 + x];
            count++;
          }
        }
        if (count > 0) setMeanFlow(Math.round(sum / count));
      };
    }
  }, [assembler, roi]);

  const handleStartImaging = useCallback(() => {
    sendCommand(CMD.START_IMAGING);
    setImaging(true);
  }, [sendCommand]);

  const handleStopImaging = useCallback(() => {
    sendCommand(CMD.STOP_IMAGING);
    setImaging(false);
  }, [sendCommand]);

  const handleCalibrate = useCallback(() => {
    sendCommand(CMD.CALIBRATE);
  }, [sendCommand]);

  if (!connected) {
    return (
      <View style={styles.emptyContainer}>
        <Icon name="bluetooth-disabled" size={64} color="#444" />
        <Text style={styles.emptyText}>No device connected</Text>
        <Text style={styles.emptySubtext}>Go to the Device tab to connect</Text>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      {/* Flow map display */}
      <View style={styles.mapContainer}>
        <FlowMap
          flowMap={flowMap}
          colormap={colormap}
          roi={roi}
          width={640}
          height={480}
        />
        {!imaging && (
          <View style={styles.overlay}>
            <Text style={styles.overlayText}>Press Start to begin imaging</Text>
          </View>
        )}
      </View>

      {/* HUD */}
      <View style={styles.hud}>
        <View style={styles.hudItem}>
          <Text style={styles.hudLabel}>Frame</Text>
          <Text style={styles.hudValue}>{frameCount}</Text>
        </View>
        <View style={styles.hudItem}>
          <Text style={styles.hudLabel}>Mean Flow</Text>
          <Text style={styles.hudValue}>{meanFlow}</Text>
        </View>
        <View style={styles.hudItem}>
          <Text style={styles.hudLabel}>FPS</Text>
          <Text style={styles.hudValue}>{status.fps}</Text>
        </View>
        <View style={styles.hudItem}>
          <Text style={styles.hudLabel}>Laser</Text>
          <Text style={[styles.hudValue, { color: status.laserOn ? '#f44336' : '#666' }]}>
            {status.laserOn ? 'ON' : 'OFF'}
          </Text>
        </View>
      </View>

      {/* Controls */}
      <View style={styles.controls}>
        <TouchableOpacity
          style={[styles.btn, imaging ? styles.btnStop : styles.btnStart]}
          onPress={imaging ? handleStopImaging : handleStartImaging}
        >
          <Icon name={imaging ? 'stop' : 'play-arrow'} size={24} color="#fff" />
          <Text style={styles.btnText}>{imaging ? 'Stop' : 'Start'}</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.btnCalibrate} onPress={handleCalibrate}>
          <Icon name="tune" size={24} color="#fff" />
          <Text style={styles.btnText}>Calibrate</Text>
        </TouchableOpacity>
      </View>

      {/* Colormap selector */}
      <View style={styles.colormapRow}>
        <Text style={styles.colormapLabel}>Colormap:</Text>
        {['jet', 'thermal', 'grayscale', 'viridis', 'inferno'].map((cm) => (
          <TouchableOpacity
            key={cm}
            style={[styles.cmBtn, colormap === cm && styles.cmBtnActive]}
            onPress={() => sendCommand(CMD.SET_COLORMAP, cm === 'jet' ? 0 : cm === 'thermal' ? 1 : cm === 'grayscale' ? 2 : cm === 'viridis' ? 3 : 4)}
          >
            <Text style={[styles.cmBtnText, colormap === cm && styles.cmBtnTextActive]}>
              {cm.charAt(0).toUpperCase() + cm.slice(1)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  emptyContainer: { flex: 1, backgroundColor: '#0f0f1e', alignItems: 'center', justifyContent: 'center' },
  emptyText: { color: '#666', fontSize: 18, marginTop: 16 },
  emptySubtext: { color: '#444', fontSize: 14, marginTop: 4 },
  mapContainer: {
    flex: 1, backgroundColor: '#000', borderRadius: 8, overflow: 'hidden',
    justifyContent: 'center', alignItems: 'center', marginBottom: 8,
  },
  overlay: { position: 'absolute', backgroundColor: 'rgba(0,0,0,0.7)', padding: 20, borderRadius: 8 },
  overlayText: { color: '#fff', fontSize: 16 },
  hud: { flexDirection: 'row', justifyContent: 'space-around', paddingVertical: 8, backgroundColor: '#1a1a2e', borderRadius: 8, marginBottom: 8 },
  hudItem: { alignItems: 'center' },
  hudLabel: { color: '#666', fontSize: 10, textTransform: 'uppercase' },
  hudValue: { color: '#00d4ff', fontSize: 18, fontWeight: 'bold' },
  controls: { flexDirection: 'row', justifyContent: 'center', gap: 12, marginBottom: 8 },
  btn: { flexDirection: 'row', alignItems: 'center', padding: 12, borderRadius: 8, minWidth: 120, justifyContent: 'center' },
  btnStart: { backgroundColor: '#2e7d32' },
  btnStop: { backgroundColor: '#c62828' },
  btnCalibrate: { flexDirection: 'row', alignItems: 'center', padding: 12, borderRadius: 8, backgroundColor: '#0066cc', minWidth: 120, justifyContent: 'center' },
  btnText: { color: '#fff', fontSize: 14, marginLeft: 6, fontWeight: '600' },
  colormapRow: { flexDirection: 'row', alignItems: 'center', flexWrap: 'wrap', gap: 4 },
  colormapLabel: { color: '#888', fontSize: 12, marginRight: 4 },
  cmBtn: { paddingHorizontal: 8, paddingVertical: 4, borderRadius: 4, backgroundColor: '#1a1a2e' },
  cmBtnActive: { backgroundColor: '#0066cc' },
  cmBtnText: { color: '#888', fontSize: 11 },
  cmBtnTextActive: { color: '#fff' },
});