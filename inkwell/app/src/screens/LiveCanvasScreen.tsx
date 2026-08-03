// LiveCanvasScreen.tsx — Primary screen: live handwriting capture canvas
//
// Subscribes to the BLE stroke stream, accumulates segments, renders them on
// the StrokeCanvas in real time, persists each segment to the local SQLite
// database, and shows the live pressure meter and pen status. A "Start
// session" / "Stop session" button sends the control command to the pen.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState, useCallback } from 'react';
import { View, Text, Button, StyleSheet, Dimensions } from 'react-native';
import bleManager from '../ble/BleManager';
import { ControlCommand, StrokeSegment, powerStateName } from '../ble/protocol';
import StrokeCanvas from '../components/StrokeCanvas';
import PressureMeter from '../components/PressureMeter';
import { startSession, endSession, insertStroke, getLastSeq } from '../db/database';

const SCREEN_W = Dimensions.get('window').width;
const SCREEN_H = Dimensions.get('window').height - 120;

export default function LiveCanvasScreen() {
  const [segments, setSegments] = useState<StrokeSegment[]>([]);
  const [battery, setBattery] = useState(0);
  const [powerState, setPowerState] = useState(0);
  const [flashFill, setFlashFill] = useState(0);
  const [connected, setConnected] = useState(false);
  const [sessionActive, setSessionActive] = useState(false);
  const [sessionId, setSessionId] = useState(-1);
  const [cursorX, setCursorX] = useState(0);
  const [cursorY, setCursorY] = useState(0);
  const [pressure, setPressure] = useState(0);

  useEffect(() => {
    const unsubStroke = bleManager.onStroke((seg) => {
      setSegments(prev => {
        const next = [...prev, seg];
        if (next.length > 5000) next.shift();
        return next;
      });
      setPressure(seg.pressureMN);

      // Accumulate absolute cursor position for DB storage.
      setCursorX(x => x + seg.dxUm);
      setCursorY(y => y + seg.dyUm);

      if (sessionActive && sessionId >= 0) {
        insertStroke(sessionId, seg.seq, seg.tsMs,
          cursorX + seg.dxUm, cursorY + seg.dyUm,
          seg.pressureMN, seg.flags.penDown ? 1 : 0);
      }
    });

    const unsubStatus = bleManager.onStatus((s) => {
      setBattery(s.batteryPct);
      setPowerState(s.powerState);
      setFlashFill(s.flashFillPct);
    });

    const unsubConn = bleManager.onConnection((c) => setConnected(c));

    return () => { unsubStroke(); unsubStatus(); unsubConn(); };
  }, [sessionActive, sessionId, cursorX, cursorY]);

  const handleStart = useCallback(async () => {
    const id = await startSession();
    setSessionId(id);
    setSessionActive(true);
    setSegments([]);
    setCursorX(0); setCursorY(0);
    await bleManager.sendControl(ControlCommand.START_SESSION);
  }, []);

  const handleStop = useCallback(async () => {
    await bleManager.sendControl(ControlCommand.STOP_SESSION);
    setSessionActive(false);
    if (sessionId >= 0) await endSession(sessionId, 0);
  }, [sessionId]);

  const handleReplay = useCallback(async () => {
    const last = await getLastSeq();
    // Ask the pen to send anything after our last-known sequence.
    await bleManager.requestReplay(last + 1, 0xFFFFFFFF);
  }, []);

  return (
    <View style={styles.container}>
      <View style={styles.statusBar}>
        <Text style={[styles.statusDot, connected ? styles.dotGreen : styles.dotRed]}>●</Text>
        <Text style={styles.statusText}>{connected ? 'Connected' : 'Disconnected'}</Text>
        <Text style={styles.statusItem}>🔋 {battery}%</Text>
        <Text style={styles.statusItem}>{powerStateName(powerState)}</Text>
        <Text style={styles.statusItem}>Flash {flashFill}%</Text>
      </View>

      <StrokeCanvas segments={segments} width={SCREEN_W} height={SCREEN_H} />

      <View style={styles.controls}>
        <PressureMeter pressureMN={pressure} thresholdMN={150} />
        <View style={styles.buttons}>
          <Button title="Start session"  onPress={handleStart}  disabled={!connected || sessionActive} />
          <Button title="Stop session"   onPress={handleStop}   disabled={!sessionActive} />
          <Button title="Sync missing"   onPress={handleReplay} disabled={!connected} />
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container:  { flex: 1, backgroundColor: '#1a1a2e' },
  statusBar:  { flexDirection: 'row', alignItems: 'center', padding: 8,
                backgroundColor: '#16213e' },
  statusDot:  { fontSize: 14 },
  dotGreen:   { color: '#4caf50' },
  dotRed:     { color: '#f44336' },
  statusText: { color: '#eee', fontSize: 13, marginHorizontal: 6 },
  statusItem: { color: '#bbb', fontSize: 12, marginHorizontal: 8 },
  controls:   { padding: 10, backgroundColor: '#0f3460' },
  buttons:    { flexDirection: 'row', justifyContent: 'space-around', marginTop: 8 },
});