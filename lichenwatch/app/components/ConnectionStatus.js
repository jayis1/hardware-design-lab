/*
 * ConnectionStatus.js — Header bar showing BLE connection state.
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useContext } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { BleContext } from '../utils/ble';

export default function ConnectionStatus() {
  const { bleState } = useContext(BleContext);
  const connected = bleState.connectedNodeId !== null;

  return (
    <View style={styles.bar}>
      <Icon
        name={connected ? 'bluetooth-connect' : 'bluetooth'}
        size={20}
        color={connected ? '#2e7d32' : '#9e9e9e'}
      />
      <Text style={styles.text}>
        {connected
          ? `Connected: ${bleState.connectedNodeId}`
          : bleState.status || 'Disconnected'}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  bar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#1b3a1b',
  },
  text: {
    color: '#fff',
    fontSize: 12,
    marginLeft: 6,
  },
});