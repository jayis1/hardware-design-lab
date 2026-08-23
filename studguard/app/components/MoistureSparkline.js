/*
 * MoistureSparkline.js — StudGuard mini trend visual
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const h = React.createElement;

export default function MoistureSparkline({ values }) {
  const max = Math.max(...values, 1);
  return h(
    View,
    { style: styles.wrap },
    h(View, { style: styles.row },
      ...values.map((v, index) => h(View, { key: `bar-${index}`, style: styles.barWrap },
        h(View, { style: [styles.bar, { height: 18 + (v / max) * 54 }] })
      ))
    ),
    h(Text, { style: styles.caption }, 'Trend bars represent normalized leak activity history')
  );
}

const styles = StyleSheet.create({
  wrap: {
    backgroundColor: '#0f172a',
    borderRadius: 12,
    padding: 12
  },
  row: {
    flexDirection: 'row',
    alignItems: 'flex-end'
  },
  barWrap: {
    width: 20,
    justifyContent: 'flex-end',
    alignItems: 'center',
    marginRight: 6
  },
  bar: {
    width: 16,
    borderRadius: 8,
    backgroundColor: '#38bdf8'
  },
  caption: {
    marginTop: 10,
    color: '#94a3b8',
    fontSize: 12
  }
});
