/**
 * SensorGauge.js — Animated Radial Gauge Component
 *
 * Displays a sensor value as a circular gauge with a label, value,
 * unit, and color-coded arc. Used on the Dashboard screen.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

export default function SensorGauge({ label, value, unit, color, icon, max = 100 }) {
  const [displayValue, setDisplayValue] = useState(0);

  // Smooth animation toward target value
  useEffect(() => {
    const target = parseFloat(value) || 0;
    const start = displayValue;
    const diff = target - start;
    if (Math.abs(diff) < 0.01) {
      setDisplayValue(target);
      return;
    }

    const steps = 20;
    let step = 0;
    const interval = setInterval(() => {
      step++;
      const progress = step / steps;
      const eased = 1 - Math.pow(1 - progress, 3); // ease-out cubic
      setDisplayValue(start + diff * eased);
      if (step >= steps) clearInterval(interval);
    }, 25);

    return () => clearInterval(interval);
  }, [value]);

  const pct = Math.min(displayValue / max, 1.0);
  const arcAngle = pct * 270; // 270-degree arc
  const radius = 32;
  const circumference = 2 * Math.PI * radius;
  const arcLength = (arcAngle / 360) * circumference;

  // Format value for display
  const formatValue = (v) => {
    if (Math.abs(v) >= 1000) return v.toFixed(0);
    if (Math.abs(v) >= 100) return v.toFixed(1);
    if (Math.abs(v) >= 10) return v.toFixed(1);
    if (Math.abs(v) >= 1) return v.toFixed(2);
    return v.toFixed(3);
  };

  return (
    <View style={styles.container}>
      <View style={styles.gaugeContainer}>
        {/* SVG-like circle using bordered View */}
        <View style={[styles.gaugeRing, { borderColor: color }]}>
          <View style={styles.gaugeInner}>
            <Icon name={icon} size={20} color={color} />
            <Text style={[styles.valueText, { color }]}>
              {formatValue(displayValue)}
            </Text>
            {unit ? <Text style={styles.unitText}>{unit}</Text> : null}
          </View>
        </View>
      </View>
      <Text style={styles.labelText}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { alignItems: 'center', width: 100 },
  gaugeContainer: { marginVertical: 4 },
  gaugeRing: {
    width: 70,
    height: 70,
    borderRadius: 35,
    borderWidth: 3,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: 'rgba(255,255,255,0.05)',
  },
  gaugeInner: { alignItems: 'center', justifyContent: 'center' },
  valueText: { fontSize: 14, fontWeight: 'bold', marginTop: 2 },
  unitText: { fontSize: 9, color: '#888' },
  labelText: { color: '#999', fontSize: 11, marginTop: 4, textAlign: 'center' },
});