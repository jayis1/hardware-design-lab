/**
 * ConfidenceBar.js — Animated confidence level indicator
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, Animated } from 'react-native';
import Colors from '../constants/Colors';

const ConfidenceBar = ({ confidence }) => {
  const [widthAnim] = useState(new Animated.Value(0));
  const pct = Math.round((confidence || 0) * 100);

  const getColor = (c) => {
    if (c >= 0.7) return Colors.highConfidence;
    if (c >= 0.4) return Colors.mediumConfidence;
    return Colors.lowConfidence;
  };

  const getLabel = (c) => {
    if (c >= 0.7) return 'HIGH CONFIDENCE';
    if (c >= 0.4) return 'MEDIUM CONFIDENCE';
    return 'LOW CONFIDENCE';
  };

  useEffect(() => {
    Animated.timing(widthAnim, {
      toValue: pct,
      duration: 500,
      useNativeDriver: false,
    }).start();
  }, [pct]);

  const color = getColor(confidence || 0);

  return (
    <View style={styles.container}>
      <View style={styles.labelRow}>
        <Text style={[styles.label, { color }]}>{getLabel(confidence || 0)}</Text>
        <Text style={styles.percentage}>{pct}%</Text>
      </View>
      <View style={styles.barBackground}>
        <Animated.View
          style={[
            styles.barFill,
            {
              width: widthAnim.interpolate({
                inputRange: [0, 100],
                outputRange: ['0%', '100%'],
              }),
              backgroundColor: color,
            },
          ]}
        />
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    marginVertical: 12,
  },
  labelRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  label: {
    fontSize: 12,
    fontWeight: 'bold',
  },
  percentage: {
    color: Colors.white,
    fontSize: 14,
    fontWeight: 'bold',
  },
  barBackground: {
    height: 10,
    backgroundColor: Colors.darkGray,
    borderRadius: 5,
    overflow: 'hidden',
  },
  barFill: {
    height: '100%',
    borderRadius: 5,
  },
});

export default ConfidenceBar;