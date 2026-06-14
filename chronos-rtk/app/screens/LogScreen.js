/**
 * LogScreen.js — Observation log viewer for Chronos-RTK
 * Shows raw observation data stored on SPI flash
 */

import React, { useContext, useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  FlatList,
} from 'react-native';
import { ChronosContext } from '../components/ChronosContext';
import StatusCard from '../components/StatusCard';

export default function LogScreen() {
  const { isConnected, sendCommand, logs } = useContext(ChronosContext);
  const [autoScroll, setAutoScroll] = useState(true);
  const [logLevel, setLogLevel] = useState('all');

  const filteredLogs = logs?.filter((entry) => {
    if (logLevel === 'all') return true;
    return entry.type === logLevel;
  }) ?? [];

  const renderItem = ({ item }) => (
    <View style={[styles.logEntry, { borderLeftColor: getLogColor(item.type) }]}>
      <View style={styles.logHeader}>
        <Text style={[styles.logType, { color: getLogColor(item.type) }]}>
          {item.type}
        </Text>
        <Text style={styles.logTime}>{item.timestamp}</Text>
      </View>
      <Text style={styles.logMessage} numberOfLines={3}>
        {item.message}
      </Text>
    </View>
  );

  const getLogColor = (type) => {
    switch (type) {
      case 'NMEA': return '#4CAF50';
      case 'RTCM': return '#2196F3';
      case 'UBX': return '#FF9800';
      case 'LORA': return '#9C27B0';
      case 'ERR': return '#F44336';
      default: return '#8b949e';
    }
  };

  return (
    <View style={styles.container}>
      {/* Filter bar */}
      <View style={styles.filterBar}>
        {['all', 'NMEA', 'RTCM', 'UBX', 'LORA', 'ERR'].map((level) => (
          <TouchableOpacity
            key={level}
            style={[
              styles.filterButton,
              logLevel === level && styles.filterButtonActive,
            ]}
            onPress={() => setLogLevel(level)}
          >
            <Text
              style={[
                styles.filterButtonText,
                logLevel === level && styles.filterButtonTextActive,
              ]}
            >
              {level}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Log list */}
      <FlatList
        data={filteredLogs}
        renderItem={renderItem}
        keyExtractor={(item, index) => `${index}`}
        style={styles.logList}
        inverted={false}
      />

      {/* Action buttons */}
      <View style={styles.actionBar}>
        <TouchableOpacity
          style={[styles.actionButton, { opacity: isConnected ? 1 : 0.5 }]}
          onPress={() => sendCommand('DOWNLOAD_LOG', '1')}
          disabled={!isConnected}
        >
          <Text style={styles.actionButtonText}>Download</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, { opacity: isConnected ? 1 : 0.5 }]}
          onPress={() => sendCommand('CLEAR_LOG', '1')}
          disabled={!isConnected}
        >
          <Text style={styles.actionButtonText}>Clear</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, { opacity: isConnected ? 1 : 0.5 }]}
          onPress={() => setAutoScroll(!autoScroll)}
        >
          <Text style={styles.actionButtonText}>
            {autoScroll ? 'Auto' : 'Manual'}
          </Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1117',
  },
  filterBar: {
    flexDirection: 'row',
    padding: 8,
    backgroundColor: '#161b22',
    borderBottomWidth: 1,
    borderBottomColor: '#21262d',
  },
  filterButton: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    marginHorizontal: 2,
    borderRadius: 12,
    backgroundColor: '#21262d',
  },
  filterButtonActive: {
    backgroundColor: '#2196F3',
  },
  filterButtonText: {
    color: '#8b949e',
    fontSize: 12,
    fontWeight: '600',
  },
  filterButtonTextActive: {
    color: '#fff',
  },
  logList: {
    flex: 1,
    padding: 8,
  },
  logEntry: {
    backgroundColor: '#161b22',
    padding: 10,
    marginVertical: 2,
    borderRadius: 6,
    borderLeftWidth: 3,
  },
  logHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  logType: {
    fontSize: 11,
    fontWeight: 'bold',
  },
  logTime: {
    color: '#484f58',
    fontSize: 10,
  },
  logMessage: {
    color: '#c9d1d9',
    fontSize: 12,
    fontFamily: 'Courier',
  },
  actionBar: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    padding: 8,
    backgroundColor: '#161b22',
    borderTopWidth: 1,
    borderTopColor: '#21262d',
  },
  actionButton: {
    backgroundColor: '#21262d',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
  },
  actionButtonText: {
    color: '#e6edf3',
    fontSize: 14,
    fontWeight: '600',
  },
});