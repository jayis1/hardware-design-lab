/**
 * NavigationScreen — Turn-by-turn thermal navigation
 *
 * Streams directional thermal arrows to the device based on a simulated
 * navigation route. In production, integrates with Mapbox Navigation SDK.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert } from 'react-native';
import { useDeviceStore } from '../store/deviceStore';
import type { NavigationStep, GlyphDirection } from '../types';

// Simulated navigation route (in production: Mapbox Navigation SDK)
const SAMPLE_ROUTE: NavigationStep[] = [
  { direction: 'east', distanceM: 120, streetName: 'Main St', isArrival: false },
  { direction: 'northeast', distanceM: 85, streetName: 'Oak Ave', isArrival: false },
  { direction: 'north', distanceM: 200, streetName: 'Park Rd', isArrival: false },
  { direction: 'west', distanceM: 50, streetName: 'Elm St', isArrival: false },
  { direction: 'north', distanceM: 30, streetName: 'Destination', isArrival: true },
];

export const NavigationScreen: React.FC = () => {
  const { connected, sendArrow, sendGlyph, settings } = useDeviceStore();
  const [route] = useState<NavigationStep[]>(SAMPLE_ROUTE);
  const [currentStep, setCurrentStep] = useState(0);
  const [isNavigating, setIsNavigating] = useState(false);
  const timerRef = useRef<ReturnType<typeof setInterval> | null>(null);

  useEffect(() => {
    return () => {
      if (timerRef.current) clearInterval(timerRef.current);
    };
  }, []);

  const startNavigation = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to ThermoGlyph first');
      return;
    }
    setIsNavigating(true);
    setCurrentStep(0);
    sendStep(0);

    if (settings.navigationAutoStream) {
      timerRef.current = setInterval(() => {
        setCurrentStep(prev => {
          if (prev >= route.length - 1) {
            if (timerRef.current) clearInterval(timerRef.current);
            setIsNavigating(false);
            sendGlyph({ type: 'shape', shape: 'check', polarity: 'warm', intensity: 200, durationMs: 5000 });
            return prev;
          }
          const next = prev + 1;
          sendStep(next);
          return next;
        });
      }, 4000); // 4 seconds per step (simulated)
    }
  };

  const stopNavigation = () => {
    if (timerRef.current) clearInterval(timerRef.current);
    setIsNavigating(false);
  };

  const sendStep = (index: number) => {
    const step = route[index];
    if (step.isArrival) {
      sendGlyph({ type: 'shape', shape: 'check', polarity: 'warm', intensity: 200, durationMs: 5000 });
    } else {
      sendArrow(step.direction);
    }
  };

  const directionToArrow = (dir: GlyphDirection): string => {
    const arrows: Record<string, string> = {
      north: '↑', northeast: '↗', east: '→', southeast: '↘',
      south: '↓', southwest: '↙', west: '←', northwest: '↖',
    };
    return arrows[dir] ?? '?';
  };

  return (
    <ScrollView style={styles.screen}>
      <View style={styles.header}>
        <Text style={styles.title}>Thermal Navigation</Text>
        <Text style={styles.subtitle}>Turn-by-turn arrows on your skin</Text>
      </View>

      {/* Controls */}
      <View style={styles.controls}>
        {!isNavigating ? (
          <TouchableOpacity style={styles.startBtn} onPress={startNavigation}>
            <Text style={styles.startBtnText}>Start Navigation</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.stopBtn} onPress={stopNavigation}>
            <Text style={styles.stopBtnText}>Stop</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Route overview */}
      <Text style={styles.sectionTitle}>Route ({route.length} steps)</Text>
      {route.map((step, i) => (
        <View
          key={i}
          style={[styles.stepRow, i === currentStep && isNavigating && styles.stepRowActive]}
        >
          <Text style={styles.stepArrow}>{directionToArrow(step.direction)}</Text>
          <View style={styles.stepInfo}>
            <Text style={styles.stepStreet}>{step.streetName}</Text>
            <Text style={styles.stepDistance}>{step.distanceM}m {step.direction}</Text>
          </View>
          {step.isArrival && <Text style={styles.arrivalBadge}>Arrived</Text>}
          {i === currentStep && isNavigating && <Text style={styles.currentBadge}>Now</Text>}
        </View>
      ))}

      {/* Manual direction pad */}
      <Text style={styles.sectionTitle}>Manual Direction</Text>
      <View style={styles.dpad}>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('northwest')}>
          <Text style={styles.dpadText}>↖</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('north')}>
          <Text style={styles.dpadText}>↑</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('northeast')}>
          <Text style={styles.dpadText}>↗</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('west')}>
          <Text style={styles.dpadText}>←</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.dpadBtn, styles.dpadCenter]} onPress={() => sendGlyph({ type: 'shape', shape: 'cross', polarity: 'warm', intensity: 128, durationMs: 2000 })}>
          <Text style={styles.dpadText}>+</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('east')}>
          <Text style={styles.dpadText}>→</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('southwest')}>
          <Text style={styles.dpadText}>↙</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('south')}>
          <Text style={styles.dpadText}>↓</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.dpadBtn} onPress={() => sendArrow('southeast')}>
          <Text style={styles.dpadText}>↘</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 13, marginTop: 4 },
  controls: { alignItems: 'center', marginVertical: 16 },
  startBtn: { backgroundColor: '#2a6', paddingHorizontal: 32, paddingVertical: 14, borderRadius: 12 },
  startBtnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
  stopBtn: { backgroundColor: '#a44', paddingHorizontal: 32, paddingVertical: 14, borderRadius: 12 },
  stopBtnText: { color: '#fff', fontSize: 16, fontWeight: '600' },
  sectionTitle: { color: '#ccc', fontSize: 15, fontWeight: '600', paddingHorizontal: 16, marginTop: 16, marginBottom: 8 },
  stepRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginHorizontal: 8, marginVertical: 3, gap: 12 },
  stepRowActive: { backgroundColor: '#2a2a4e', borderWidth: 1, borderColor: '#4a8' },
  stepArrow: { fontSize: 24, color: '#fa4', width: 32 },
  stepInfo: { flex: 1 },
  stepStreet: { color: '#fff', fontSize: 14, fontWeight: '500' },
  stepDistance: { color: '#888', fontSize: 12, marginTop: 2 },
  arrivalBadge: { color: '#4a8', fontSize: 12, fontWeight: '600' },
  currentBadge: { color: '#fa4', fontSize: 12, fontWeight: 'bold' },
  dpad: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'center', gap: 4, paddingHorizontal: 16, paddingBottom: 20 },
  dpadBtn: { width: 56, height: 56, borderRadius: 12, backgroundColor: '#1a1a2e', justifyContent: 'center', alignItems: 'center' },
  dpadCenter: { backgroundColor: '#333' },
  dpadText: { fontSize: 24, color: '#fa4' },
});