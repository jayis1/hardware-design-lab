/**
 * EAG-7000 — CANFrameCard Component
 *
 * Renders a single CAN frame entry with formatted ID, data, and flags.
 * Used by the CAN Bus screen to display received frames.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text, Chip } from 'react-native-paper';
import { CAN_FLAGS, dlcToBytes } from '../utils/protocol';

function CANFrameCard({ bus, id, dlc, flags, data, timestamp, style }) {
  const isExtended = !!(flags & CAN_FLAGS.EXT);
  const isCANFD = !!(flags & CAN_FLAGS.FDF);
  const hasBRS = !!(flags & CAN_FLAGS.BRS);
  const isESI = !!(flags & CAN_FLAGS.ESI);
  const isRTR = !!(flags & CAN_FLAGS.RTR);

  const formatId = () => {
    if (isExtended) {
      return '0x' + id.toString(16).toUpperCase().padStart(8, '0');
    }
    return '0x' + id.toString(16).toUpperCase().padStart(3, '0');
  };

  const formatData = () => {
    if (!data || data.length === 0) return isRTR ? '[RTR]' : '--';
    const len = Math.min(data.length, dlcToBytes(dlc));
    return Array.from(data.slice(0, len))
      .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
      .join(' ');
  };

  return (
    <View style={[styles.container, style]}>
      <View style={styles.header}>
        <Text style={[styles.bus, { color: bus === 0 ? '#2196F3' : '#9C27B0' }]}>
          CAN{bus}
        </Text>
        <Text style={styles.id}>{formatId()}</Text>
        <Text style={styles.dlc}>[{dlc}]</Text>
        <Text style={styles.timestamp}>{timestamp}</Text>
      </View>
      <View style={styles.body}>
        <Text style={styles.data}>{formatData()}</Text>
        <View style={styles.flags}>
          {isExtended && <Text style={styles.flagExt}>EXT</Text>}
          {isCANFD && <Text style={styles.flagFD}>FD</Text>}
          {hasBRS && <Text style={styles.flagBRS}>BRS</Text>}
          {isESI && <Text style={styles.flagESI}>ESI</Text>}
          {isRTR && <Text style={styles.flagRTR}>RTR</Text>}
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#1A1A2E',
    borderRadius: 6,
    padding: 8,
    marginVertical: 2,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  bus: {
    fontSize: 11,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    width: 40,
  },
  id: {
    color: '#4CAF50',
    fontSize: 12,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    flex: 1,
  },
  dlc: {
    color: '#FFC107',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 28,
  },
  timestamp: {
    color: '#757575',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  body: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 2,
  },
  data: {
    color: '#E0E0E0',
    fontSize: 12,
    fontFamily: 'monospace',
    flex: 1,
  },
  flags: {
    flexDirection: 'row',
  },
  flagExt: {
    color: '#2196F3',
    fontSize: 9,
    backgroundColor: '#1A237E',
    paddingHorizontal: 3,
    paddingVertical: 1,
    marginHorizontal: 1,
    borderRadius: 2,
  },
  flagFD: {
    color: '#FF9800',
    fontSize: 9,
    backgroundColor: '#3E2723',
    paddingHorizontal: 3,
    paddingVertical: 1,
    marginHorizontal: 1,
    borderRadius: 2,
  },
  flagBRS: {
    color: '#4CAF50',
    fontSize: 9,
    backgroundColor: '#1B5E20',
    paddingHorizontal: 3,
    paddingVertical: 1,
    marginHorizontal: 1,
    borderRadius: 2,
  },
  flagESI: {
    color: '#F44336',
    fontSize: 9,
    backgroundColor: '#4A1010',
    paddingHorizontal: 3,
    paddingVertical: 1,
    marginHorizontal: 1,
    borderRadius: 2,
  },
  flagRTR: {
    color: '#9C27B0',
    fontSize: 9,
    backgroundColor: '#2A0845',
    paddingHorizontal: 3,
    paddingVertical: 1,
    marginHorizontal: 1,
    borderRadius: 2,
  },
});

export default CANFrameCard;