/**
 * ProtocolScreen.js — Protocol decoder display
 * Shows decoded UART, SPI, I²C, CAN, LIN protocol data in real-time
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, StyleSheet, ScrollView, FlatList, Text } from 'react-native';
import { Button, Chip, Card, Paragraph, ToggleButton } from 'react-native-paper';
import { HexaScopeProtocol, COMMANDS } from '../utils/protocol';

const PROTOCOLS = [
  { label: 'UART', value: 'uart' },
  { label: 'SPI', value: 'spi' },
  { label: 'I²C', value: 'i2c' },
  { label: 'CAN', value: 'can' },
  { label: 'LIN', value: 'lin' },
];

const UART_CONFIGS = [
  { baud: 9600 },
  { baud: 19200 },
  { baud: 38400 },
  { baud: 57600 },
  { baud: 115200 },
  { baud: 230400 },
  { baud: 460800 },
  { baud: 921600 },
];

export default function ProtocolScreen({ navigation }) {
  const [activeProtocol, setActiveProtocol] = useState('uart');
  const [baudRate, setBaudRate] = useState(115200);
  const [running, setRunning] = useState(false);
  const [decodedFrames, setDecodedFrames] = useState([]);
  const [stats, setStats] = useState({ totalFrames: 0, errors: 0, bytesPerSec: 0 });
  const protocolRef = useRef(null);

  useEffect(() => {
    const proto = new HexaScopeProtocol();
    proto.onProtocolData = (frame) => {
      setDecodedFrames(prev => {
        const next = [...prev, {
          id: prev.length,
          timestamp: frame.timestamp,
          type: frame.type,
          data: frame.data,
          direction: frame.direction,
          info: frame.info,
        }];
        // Keep only last 500 frames
        if (next.length > 500) next.splice(0, next.length - 500);
        return next;
      });
      setStats(prev => ({
        totalFrames: prev.totalFrames + 1,
        errors: prev.errors + (frame.error ? 1 : 0),
        bytesPerSec: frame.bytesPerSec || prev.bytesPerSec,
      }));
    };
    protocolRef.current = proto;
    return () => proto.stop();
  }, []);

  const handleStart = () => {
    setRunning(true);
    const proto = protocolRef.current;
    if (proto) {
      const protocolCode = activeProtocol === 'uart' ? 0x01 :
                           activeProtocol === 'spi' ? 0x02 :
                           activeProtocol === 'i2c' ? 0x03 :
                           activeProtocol === 'can' ? 0x04 :
                           activeProtocol === 'lin' ? 0x05 : 0x01;
      proto.sendCommand(COMMANDS.SET_PROTOCOL, protocolCode, baudRate);
    }
  };

  const handleStop = () => {
    setRunning(false);
    setDecodedFrames([]);
  };

  const handleClear = () => {
    setDecodedFrames([]);
    setStats({ totalFrames: 0, errors: 0, bytesPerSec: 0 });
  };

  const renderFrame = ({ item }) => (
    <View style={styles.frameRow}>
      <Text style={styles.frameTimestamp}>{item.timestamp.toFixed(6)}s</Text>
      <Text style={[styles.frameDir, item.direction === 'TX' ? styles.dirTx : styles.dirRx]}>
        {item.direction}
      </Text>
      <Text style={styles.frameType}>{item.type}</Text>
      <Text style={styles.frameData}>{item.data}</Text>
      {item.info && <Text style={styles.frameInfo}>{item.info}</Text>}
    </View>
  );

  return (
    <View style={styles.container}>
      {/* Protocol selection */}
      <View style={styles.protocolBar}>
        {PROTOCOLS.map((proto) => (
          <Chip
            key={proto.value}
            selected={activeProtocol === proto.value}
            onPress={() => setActiveProtocol(proto.value)}
            mode="outlined"
            textStyle={styles.chipText}
            style={activeProtocol === proto.value ? styles.chipActive : styles.chip}
          >
            {proto.label}
          </Chip>
        ))}
      </View>

      {/* Configuration */}
      <Card style={styles.configCard}>
        <Card.Content>
          {activeProtocol === 'uart' && (
            <View style={styles.baudRow}>
              <Text style={styles.configLabel}>Baud Rate:</Text>
              <ScrollView horizontal showsHorizontalScrollIndicator={false}>
                {UART_CONFIGS.map((cfg) => (
                  <Chip
                    key={cfg.baud}
                    selected={baudRate === cfg.baud}
                    onPress={() => setBaudRate(cfg.baud)}
                    mode="outlined"
                    style={baudRate === cfg.baud ? styles.baudChipActive : styles.baudChip}
                    textStyle={styles.chipText}
                  >
                    {cfg.baud}
                  </Chip>
                ))}
              </ScrollView>
            </View>
          )}
        </Card.Content>
      </Card>

      {/* Controls */}
      <View style={styles.controls}>
        <Button
          mode="contained"
          onPress={running ? handleStop : handleStart}
          buttonColor={running ? '#FF4444' : '#44FF44'}
        >
          {running ? 'Stop' : 'Start'}
        </Button>
        <Button mode="outlined" onPress={handleClear} textColor="#888">
          Clear
        </Button>
      </View>

      {/* Statistics */}
      <View style={styles.stats}>
        <Text style={styles.statItem}>Frames: {stats.totalFrames}</Text>
        <Text style={styles.statItem}>Errors: {stats.errors}</Text>
        <Text style={styles.statItem}>Rate: {stats.bytesPerSec} B/s</Text>
      </View>

      {/* Decoded frames list */}
      <FlatList
        data={decodedFrames}
        renderItem={renderFrame}
        keyExtractor={(item) => String(item.id)}
        style={styles.frameList}
        inverted
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  protocolBar: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingVertical: 8,
    backgroundColor: '#16213e',
  },
  chip: {
    backgroundColor: '#1a1a2e',
  },
  chipActive: {
    backgroundColor: '#2196F3',
  },
  chipText: {
    color: '#ffffff',
    fontSize: 11,
  },
  configCard: {
    margin: 8,
    backgroundColor: '#16213e',
  },
  configLabel: {
    color: '#ffffff',
    fontSize: 13,
    marginRight: 8,
  },
  baudRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  baudChip: {
    backgroundColor: '#1a1a2e',
    marginHorizontal: 2,
  },
  baudChipActive: {
    backgroundColor: '#2196F3',
    marginHorizontal: 2,
  },
  controls: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 16,
    paddingVertical: 8,
    backgroundColor: '#16213e',
  },
  stats: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    paddingVertical: 4,
    backgroundColor: '#0f1527',
  },
  statItem: {
    color: '#00FF88',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  frameList: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  frameRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 2,
    paddingHorizontal: 8,
    borderBottomWidth: 0.5,
    borderBottomColor: '#333',
  },
  frameTimestamp: {
    color: '#888',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 80,
  },
  frameDir: {
    fontSize: 10,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    width: 24,
  },
  dirTx: {
    color: '#FF6666',
  },
  dirRx: {
    color: '#66FF66',
  },
  frameType: {
    color: '#AAAAFF',
    fontSize: 10,
    fontFamily: 'monospace',
    width: 48,
  },
  frameData: {
    color: '#FFFFFF',
    fontSize: 10,
    fontFamily: 'monospace',
    flex: 1,
  },
  frameInfo: {
    color: '#FFD700',
    fontSize: 10,
    fontFamily: 'monospace',
  },
});