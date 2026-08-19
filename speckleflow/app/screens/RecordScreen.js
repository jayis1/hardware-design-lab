/**
 * RecordScreen.js — Session recording and replay
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useRef, useCallback } from 'react';
import {
  View, Text, TouchableOpacity, FlatList, StyleSheet,
  Alert, Share,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { useDevice, CMD } from '../utils/protocol';
import FlowMap from '../components/FlowMap';

export default function RecordScreen() {
  const { connected, assembler, colormap, sendCommand, status } = useDevice();
  const [recording, setRecording] = useState(false);
  const [recordedFrames, setRecordedFrames] = useState([]);
  const [sessionStart, setSessionStart] = useState(0);
  const [replayIndex, setReplayIndex] = useState(-1);
  const recordTimerRef = useRef(null);

  const handleStartRecord = useCallback(() => {
    setRecordedFrames([]);
    setSessionStart(Date.now());
    setRecording(true);
    sendCommand(CMD.SET_SD_LOG, 1);  // enable SD logging on device

    // Capture frames at ~10 fps for replay
    let frameBuf = null;
    recordTimerRef.current = setInterval(() => {
      if (assembler && assembler.frame) {
        frameBuf = new Uint8ClampedArray(assembler.frame);
        setRecordedFrames(prev => {
          const next = [...prev, { timestamp: Date.now() - sessionStart, frame: frameBuf }];
          // Keep max 600 frames (60 seconds at 10 fps)
          if (next.length > 600) return next.slice(-600);
          return next;
        });
      }
    }, 100);
  }, [assembler, sendCommand, sessionStart]);

  const handleStopRecord = useCallback(() => {
    setRecording(false);
    sendCommand(CMD.SET_SD_LOG, 0);
    if (recordTimerRef.current) {
      clearInterval(recordTimerRef.current);
      recordTimerRef.current = null;
    }
  }, [sendCommand]);

  const handleReplay = useCallback((index) => {
    setReplayIndex(index);
  }, []);

  const handleExport = useCallback(async () => {
    if (recordedFrames.length === 0) {
      Alert.alert('No Data', 'Record a session first.');
      return;
    }
    // Export as CSV (mean flow per frame)
    let csv = 'frame,timestamp_ms,mean_flow\n';
    recordedFrames.forEach((f, i) => {
      let sum = 0;
      for (let j = 0; j < f.frame.length; j += 64) {
        sum += f.frame[j];
      }
      const mean = Math.round(sum / (f.frame.length / 64));
      csv += `${i},${f.timestamp},${mean}\n`;
    });
    try {
      await Share.share({ message: csv, title: 'SpeckleFlow Session Export' });
    } catch (e) {
      Alert.alert('Export Error', e.message);
    }
  }, [recordedFrames]);

  const renderFrameItem = useCallback(({ item, index }) => (
    <TouchableOpacity style={styles.frameItem} onPress={() => handleReplay(index)}>
      <Text style={styles.frameText}>Frame {index} · {item.timestamp}ms</Text>
    </TouchableOpacity>
  ), [handleReplay]);

  if (!connected) {
    return (
      <View style={styles.emptyContainer}>
        <Icon name="videocam-off" size={64} color="#444" />
        <Text style={styles.emptyText}>No device connected</Text>
      </View>
    );
  }

  const replayFrame = replayIndex >= 0 ? recordedFrames[replayIndex]?.frame : null;

  return (
    <View style={styles.container}>
      {/* Replay view */}
      {replayFrame && (
        <View style={styles.replayContainer}>
          <FlowMap
            flowMap={replayFrame}
            colormap={colormap}
            width={640}
            height={480}
          />
          <TouchableOpacity style={styles.closeReplay} onPress={() => setReplayIndex(-1)}>
            <Icon name="close" size={20} color="#fff" />
          </TouchableOpacity>
          <Text style={styles.replayLabel}>Replaying Frame {replayIndex}</Text>
        </View>
      )}

      {/* Record controls */}
      <View style={styles.controls}>
        <TouchableOpacity
          style={[styles.recordBtn, recording ? styles.recordingBtn : styles.idleBtn]}
          onPress={recording ? handleStopRecord : handleStartRecord}
        >
          <Icon name={recording ? 'stop' : 'fiber-manual-record'} size={28} color="#fff" />
          <Text style={styles.recordBtnText}>
            {recording ? 'Stop Recording' : 'Start Recording'}
          </Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.exportBtn} onPress={handleExport}>
          <Icon name="file-download" size={24} color="#fff" />
          <Text style={styles.exportBtnText}>Export CSV</Text>
        </TouchableOpacity>
      </View>

      {/* Session info */}
      <View style={styles.sessionInfo}>
        <Text style={styles.sessionText}>
          Frames: {recordedFrames.length} · Duration: {recordedFrames.length > 0 ? (recordedFrames[recordedFrames.length - 1].timestamp / 1000).toFixed(1) : 0}s
        </Text>
        <Text style={styles.sessionText}>
          SD Card: {status.frameCount > 0 ? `${status.frameCount} frames on card` : 'No SD data'}
        </Text>
      </View>

      {/* Frame list */}
      <Text style={styles.sectionTitle}>Recorded Frames</Text>
      <FlatList
        data={recordedFrames}
        keyExtractor={(item, index) => index.toString()}
        renderItem={renderFrameItem}
        style={styles.frameList}
        numColumns={3}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  emptyContainer: { flex: 1, backgroundColor: '#0f0f1e', alignItems: 'center', justifyContent: 'center' },
  emptyText: { color: '#666', fontSize: 18, marginTop: 16 },
  replayContainer: { height: 200, backgroundColor: '#000', borderRadius: 8, marginBottom: 12, overflow: 'hidden' },
  closeReplay: { position: 'absolute', top: 8, right: 8, backgroundColor: 'rgba(0,0,0,0.6)', borderRadius: 12, padding: 4 },
  replayLabel: { position: 'absolute', bottom: 8, left: 8, color: '#fff', fontSize: 12, backgroundColor: 'rgba(0,0,0,0.6)', paddingHorizontal: 8, paddingVertical: 2, borderRadius: 4 },
  controls: { flexDirection: 'row', justifyContent: 'center', gap: 12, marginBottom: 12 },
  recordBtn: { flexDirection: 'row', alignItems: 'center', padding: 14, borderRadius: 8, flex: 1, justifyContent: 'center' },
  idleBtn: { backgroundColor: '#2e7d32' },
  recordingBtn: { backgroundColor: '#c62828' },
  recordBtnText: { color: '#fff', fontSize: 14, marginLeft: 8, fontWeight: '600' },
  exportBtn: { flexDirection: 'row', alignItems: 'center', padding: 14, borderRadius: 8, backgroundColor: '#0066cc', justifyContent: 'center' },
  exportBtnText: { color: '#fff', fontSize: 14, marginLeft: 6, fontWeight: '600' },
  sessionInfo: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 12 },
  sessionText: { color: '#aaa', fontSize: 13, paddingVertical: 2 },
  sectionTitle: { color: '#888', fontSize: 12, marginBottom: 8, textTransform: 'uppercase' },
  frameList: { flex: 1 },
  frameItem: { backgroundColor: '#1a1a2e', borderRadius: 4, padding: 8, margin: 2, flex: 1 },
  frameText: { color: '#aaa', fontSize: 11 },
});