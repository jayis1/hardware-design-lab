/**
 * EAG-7000 — CAN-FD Bus Monitor Screen
 *
 * Real-time display of CAN-FD messages from both buses.
 * Allows sending CAN frames and filtering by ID/type.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useContext, useRef } from 'react';
import {
  View, ScrollView, FlatList, StyleSheet, TextInput, TouchableOpacity,
} from 'react-native';
import { Text, Card, Button, Switch, Chip, Divider, IconButton } from 'react-native-paper';
import { BLEContext } from '../components/BLEProvider';
import {
  MSG_TYPE, CAN_FLAGS, decodeCANFrame, encodeCANFrame, bytesToDLC, dlcToBytes,
} from '../utils/protocol';

// Maximum CAN frames to keep in history
const MAX_HISTORY = 500;

function CANBusScreen() {
  const { connected, subscribeToCharacteristic, writeCharacteristic } = useContext(BLEContext);

  // CAN frame history
  const [frames, setFrames] = useState([]);
  const [can0Count, setCan0Count] = useState(0);
  const [can1Count, setCan1Count] = useState(0);

  // Filter settings
  const [filterEnabled, setFilterEnabled] = useState(false);
  const [filterId, setFilterId] = useState('');
  const [filterExt, setFilterExt] = useState(false);

  // Send form
  const [sendBus, setSendBus] = useState(0);
  const [sendId, setSendId] = useState('');
  const [sendData, setSendData] = useState('');
  const [sendExt, setSendExt] = useState(false);
  const [sendFdf, setSendFdf] = useState(false);
  const [sendBrs, setSendBrs] = useState(false);

  // Auto-scroll
  const flatListRef = useRef(null);
  const [autoScroll, setAutoScroll] = useState(true);

  useEffect(() => {
    if (!connected) return;

    const unsubscribe = subscribeToCharacteristic((data) => {
      const bytes = new Uint8Array(data);
      if (bytes.length < 6) return;
      if (bytes[0] !== MSG_TYPE.CAN_RX) return;

      const frame = decodeCANFrame(bytes);
      const now = new Date();
      const timestamp = now.toLocaleTimeString('en-US', { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' })
        + '.' + String(now.getMilliseconds()).padStart(3, '0');

      const entry = {
        key: `${frame.bus}-${frame.id}-${Date.now()}-${Math.random()}`,
        bus: frame.bus,
        id: frame.id,
        dlc: frame.dlc,
        flags: frame.flags,
        data: frame.data,
        timestamp,
      };

      setFrames(prev => {
        const updated = [entry, ...prev].slice(0, MAX_HISTORY);
        return updated;
      });

      if (frame.bus === 0) setCan0Count(c => c + 1);
      else setCan1Count(c => c + 1);
    });

    return () => {
      if (unsubscribe) unsubscribe();
    };
  }, [connected, subscribeToCharacteristic]);

  // Filter frames
  const filteredFrames = filterEnabled
    ? frames.filter(f => {
        if (filterId) {
          const idNum = parseInt(filterId, 16);
          if (isNaN(idNum)) return true;
          if (filterExt !== !!(f.flags & CAN_FLAGS.EXT)) return false;
          return f.id === idNum;
        }
        return true;
      })
    : frames;

  const handleSend = async () => {
    if (!connected) return;

    const id = parseInt(sendId, 16) || 0;
    const dataBytes = sendData.trim()
      ? sendData.trim().split(/\s+/).map(h => parseInt(h, 16) & 0xFF)
      : [];
    const dlc = bytesToDLC(dataBytes.length);
    let flags = 0;
    if (sendExt) flags |= CAN_FLAGS.EXT;
    if (sendFdf) flags |= CAN_FLAGS.FDF;
    if (sendBrs) flags |= CAN_FLAGS.BRS;

    const payload = new Uint8Array(dataBytes);
    const packet = encodeCANFrame(sendBus, id, dlc, flags, payload);
    await writeCharacteristic(packet);
  };

  const handleClear = () => {
    setFrames([]);
    setCan0Count(0);
    setCan1Count(0);
  };

  const formatData = (data, dlc) => {
    if (!data || data.length === 0) return '--';
    return Array.from(data.slice(0, dlcToBytes(dlc)))
      .map(b => b.toString(16).toUpperCase().padStart(2, '0'))
      .join(' ');
  };

  const formatId = (id, flags) => {
    if (flags & CAN_FLAGS.EXT) {
      return id.toString(16).toUpperCase().padStart(8, '0') + 'x';
    }
    return id.toString(16).toUpperCase().padStart(3, '0');
  };

  const renderFrame = ({ item }) => (
    <View style={[styles.frameRow, item.bus === 0 ? styles.frameCan0 : styles.frameCan1]}>
      <Text style={styles.frameTimestamp}>{item.timestamp}</Text>
      <Text style={styles.frameBus}>CAN{item.bus}</Text>
      <Text style={styles.frameId}>{formatId(item.id, item.flags)}</Text>
      <Text style={styles.frameDlc}>[{item.dlc}]</Text>
      <Text style={styles.frameData}>{formatData(item.data, item.dlc)}</Text>
      <View style={styles.frameFlags}>
        {item.flags & CAN_FLAGS.EXT ? <Text style={styles.flagChip}>EXT</Text> : null}
        {item.flags & CAN_FLAGS.FDF ? <Text style={styles.flagChip}>FD</Text> : null}
        {item.flags & CAN_FLAGS.BRS ? <Text style={styles.flagChip}>BRS</Text> : null}
        {item.flags & CAN_FLAGS.ESI ? <Text style={styles.flagChip}>ESI</Text> : null}
      </View>
    </View>
  );

  return (
    <View style={styles.container}>
      {/* Bus Statistics */}
      <View style={styles.statsRow}>
        <Chip icon="bus" style={styles.chip} textStyle={styles.chipText}>
          CAN0: {can0Count}
        </Chip>
        <Chip icon="bus" style={styles.chip} textStyle={styles.chipText}>
          CAN1: {can1Count}
        </Chip>
        <IconButton icon="delete-outline" size={20} onPress={handleClear} />
      </View>

      {/* Filter */}
      <View style={styles.filterRow}>
        <Switch value={filterEnabled} onValueChange={setFilterEnabled} />
        <Text style={styles.filterLabel}>Filter</Text>
        <TextInput
          style={styles.filterInput}
          placeholder="ID (hex)"
          placeholderTextColor="#666"
          value={filterId}
          onChangeText={setSendId => setFilterId(setSendId)}
          editable={filterEnabled}
        />
        <Switch value={filterExt} onValueChange={setFilterExt} disabled={!filterEnabled} />
        <Text style={styles.filterLabel}>EXT</Text>
      </View>

      {/* Frame List */}
      <FlatList
        ref={flatListRef}
        data={filteredFrames}
        renderItem={renderFrame}
        keyExtractor={item => item.key}
        style={styles.frameList}
        inverted={autoScroll}
      />

      {/* Send Form */}
      <Card style={styles.sendCard}>
        <Card.Title title="Transmit CAN Frame" titleStyle={{ color: '#E0E0E0', fontSize: 14 }} />
        <Card.Content>
          <View style={styles.sendRow}>
            <Text style={styles.sendLabel}>Bus:</Text>
            <Button mode={sendBus === 0 ? 'contained' : 'outlined'}
              onPress={() => setSendBus(0)} compact>CAN0</Button>
            <Button mode={sendBus === 1 ? 'contained' : 'outlined'}
              onPress={() => setSendBus(1)} compact>CAN1</Button>
          </View>
          <View style={styles.sendRow}>
            <Text style={styles.sendLabel}>ID:</Text>
            <TextInput
              style={styles.sendInput}
              placeholder="0x..."
              placeholderTextColor="#666"
              value={sendId}
              onChangeText={setSendId}
            />
          </View>
          <View style={styles.sendRow}>
            <Text style={styles.sendLabel}>Data:</Text>
            <TextInput
              style={[styles.sendInput, { flex: 2 }]}
              placeholder="AA BB CC DD"
              placeholderTextColor="#666"
              value={sendData}
              onChangeText={setSendData}
            />
          </View>
          <View style={styles.sendRow}>
            <Switch value={sendExt} onValueChange={setSendExt} />
            <Text style={styles.switchLabel}>EXT</Text>
            <Switch value={sendFdf} onValueChange={setSendFdf} />
            <Text style={styles.switchLabel}>FDF</Text>
            <Switch value={sendBrs} onValueChange={setSendBrs} />
            <Text style={styles.switchLabel}>BRS</Text>
          </View>
          <Button mode="contained" onPress={handleSend} disabled={!connected}
            style={styles.sendButton}>
            Send Frame
          </Button>
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F23',
  },
  statsRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 8,
    paddingVertical: 4,
  },
  chip: {
    marginHorizontal: 4,
    backgroundColor: '#1A1A2E',
  },
  chipText: {
    color: '#E0E0E0',
  },
  filterRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 8,
    paddingVertical: 4,
    backgroundColor: '#1A1A2E',
  },
  filterLabel: {
    color: '#B0B0B0',
    fontSize: 12,
    marginHorizontal: 4,
  },
  filterInput: {
    flex: 1,
    color: '#E0E0E0',
    backgroundColor: '#252540',
    marginHorizontal: 4,
    paddingHorizontal: 8,
    paddingVertical: 4,
    fontSize: 14,
  },
  frameList: {
    flex: 1,
  },
  frameRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 4,
    paddingVertical: 2,
    borderBottomWidth: 1,
    borderBottomColor: '#252540',
  },
  frameCan0: {
    backgroundColor: '#0F0F23',
  },
  frameCan1: {
    backgroundColor: '#111130',
  },
  frameTimestamp: {
    color: '#757575',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 80,
  },
  frameBus: {
    color: '#2196F3',
    fontSize: 11,
    fontFamily: 'monospace',
    width: 44,
  },
  frameId: {
    color: '#4CAF50',
    fontSize: 12,
    fontFamily: 'monospace',
    width: 90,
  },
  frameDlc: {
    color: '#FFC107',
    fontSize: 11,
    fontFamily: 'monospace',
    width: 30,
  },
  frameData: {
    color: '#E0E0E0',
    fontSize: 11,
    fontFamily: 'monospace',
    flex: 1,
  },
  frameFlags: {
    flexDirection: 'row',
  },
  flagChip: {
    color: '#FF9800',
    fontSize: 9,
    backgroundColor: '#333',
    paddingHorizontal: 3,
    paddingVertical: 1,
    marginHorizontal: 1,
    borderRadius: 2,
  },
  sendCard: {
    margin: 8,
    backgroundColor: '#1A1A2E',
  },
  sendRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginVertical: 4,
  },
  sendLabel: {
    color: '#B0B0B0',
    fontSize: 13,
    width: 44,
  },
  sendInput: {
    flex: 1,
    color: '#E0E0E0',
    backgroundColor: '#252540',
    paddingHorizontal: 8,
    paddingVertical: 4,
    fontSize: 14,
    fontFamily: 'monospace',
  },
  switchLabel: {
    color: '#B0B0B0',
    fontSize: 12,
    marginHorizontal: 4,
  },
  sendButton: {
    marginTop: 8,
    backgroundColor: '#2196F3',
  },
});

export default CANBusScreen;