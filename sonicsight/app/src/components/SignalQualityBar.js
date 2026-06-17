/**
 * SignalQualityBar.js — SonicSight Companion
 * Horizontal bar chart showing per-channel SNR and coupling quality
 * for all 32 transducer channels.
 * @author jayis1
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const BAR_WIDTH = 5;
const BAR_GAP = 2;
const CHANNELS = 32;

const SignalQualityBar = ({ data }) => {
  if (!data || data.length === 0) {
    /* Render placeholder with all inactive */
    data = Array(CHANNELS).fill(null).map((_, i) => ({ snr: 0, active: false }));
  }

  const getBarColor = (snr) => {
    if (snr < 3) return '#f44336';   /* Red — bad */
    if (snr < 10) return '#ff9800';  /* Orange — marginal */
    if (snr < 20) return '#4caf50';  /* Green — good */
    return '#00d2ff';                 /* Cyan — excellent */
  };

  const getBarHeight = (snr) => {
    return Math.min(40, Math.max(4, snr * 2));
  };

  return (
    <View style={styles.container}>
      <View style={styles.barRow}>
        {data.slice(0, CHANNELS).map((ch, i) => (
          <View key={i} style={styles.barGroup}>
            <View style={[
              styles.bar,
              {
                height: getBarHeight(ch.snr),
                backgroundColor: ch.active ? getBarColor(ch.snr) : '#333',
                width: BAR_WIDTH,
              },
            ]} />
            <Text style={[
              styles.label,
              { color: ch.active ? '#aaa' : '#444' },
            ]}>{i + 1}</Text>
          </View>
        ))}
      </View>
      <View style={styles.legend}>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#f44336' }]} />
          <Text style={styles.legendText}>{'<3 dB'}</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#ff9800' }]} />
          <Text style={styles.legendText}>3–10 dB</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#4caf50' }]} />
          <Text style={styles.legendText}>10–20 dB</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#00d2ff' }]} />
          <Text style={styles.legendText}>{'>20 dB'}</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#333' }]} />
          <Text style={styles.legendText}>Inactive</Text>
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { paddingVertical: 4 },
  barRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'flex-end',
    height: 48,
  },
  barGroup: { alignItems: 'center', width: BAR_WIDTH + BAR_GAP },
  bar: {
    borderRadius: 1,
    marginBottom: 2,
  },
  label: { fontSize: 6, textAlign: 'center' },
  legend: {
    flexDirection: 'row', justifyContent: 'center',
    marginTop: 6,
  },
  legendItem: {
    flexDirection: 'row', alignItems: 'center',
    marginHorizontal: 6,
  },
  legendDot: { width: 6, height: 6, borderRadius: 3, marginRight: 3 },
  legendText: { color: '#888', fontSize: 9 },
});

export default SignalQualityBar;