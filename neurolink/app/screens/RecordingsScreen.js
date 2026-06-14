/**
 * NeuroLink Recordings Screen — View and export recorded biosignal data
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  Alert,
} from 'react-native';
import { useBle } from '../components/BleContext';

export default function RecordingsScreen() {
  const { connected } = useBle();
  const [recordings, setRecordings] = useState([
    { id: '1', date: '2026-06-14 10:30', duration: '5:23', channels: 32, size: '24.1 MB' },
    { id: '2', date: '2026-06-13 14:15', duration: '12:47', channels: 16, size: '18.3 MB' },
    { id: '3', date: '2026-06-12 09:00', duration: '45:12', channels: 32, size: '156.2 MB' },
  ]);
  const [selectedId, setSelectedId] = useState(null);

  const handleExport = useCallback((recording) => {
    Alert.alert(
      'Export Recording',
      `Export ${recording.date} (${recording.size})?\n\nFormat: EDF+ (European Data Format)`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Export', onPress: () => console.log('Export:', recording.id) },
      ]
    );
  }, []);

  const handleDelete = useCallback((recording) => {
    Alert.alert(
      'Delete Recording',
      `Permanently delete recording from ${recording.date}?`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: () => {
            setRecordings((prev) => prev.filter((r) => r.id !== recording.id));
          },
        },
      ]
    );
  }, []);

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={[styles.recordingItem, selectedId === item.id && styles.recordingItemSelected]}
      onPress={() => setSelectedId(item.id === selectedId ? null : item.id)}
    >
      <View style={styles.recordingHeader}>
        <Text style={styles.recordingDate}>{item.date}</Text>
        <Text style={styles.recordingDuration}>{item.duration}</Text>
      </View>
      <View style={styles.recordingDetails}>
        <Text style={styles.recordingDetail}>{item.channels} channels</Text>
        <Text style={styles.recordingDetail}>{item.size}</Text>
      </View>
      {selectedId === item.id && (
        <View style={styles.recordingActions}>
          <TouchableOpacity
            style={styles.exportBtn}
            onPress={() => handleExport(item)}
          >
            <Text style={styles.exportBtnText}>Export EDF+</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.shareBtn}
            onPress={() => {}}
          >
            <Text style={styles.shareBtnText}>Share</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.deleteBtn}
            onPress={() => handleDelete(item)}
          >
            <Text style={styles.deleteBtnText}>Delete</Text>
          </TouchableOpacity>
        </View>
      )}
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Recordings</Text>
        <Text style={styles.headerSub}>
          {recordings.length} recordings on device
        </Text>
      </View>

      <FlatList
        data={recordings}
        renderItem={renderItem}
        keyExtractor={(item) => item.id}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F1A',
    padding: 16,
  },
  header: {
    marginBottom: 16,
  },
  headerTitle: {
    color: '#4FC3F7',
    fontSize: 20,
    fontWeight: '700',
  },
  headerSub: {
    color: '#607D8B',
    fontSize: 13,
    marginTop: 4,
  },
  list: {
    paddingBottom: 16,
  },
  recordingItem: {
    backgroundColor: '#16213E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  recordingItemSelected: {
    borderColor: '#4FC3F7',
    backgroundColor: '#1A2A4E',
  },
  recordingHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  recordingDate: {
    color: '#E8E8E8',
    fontSize: 15,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  recordingDuration: {
    color: '#4FC3F7',
    fontSize: 14,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  recordingDetails: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  recordingDetail: {
    color: '#607D8B',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  recordingActions: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 12,
    paddingTop: 12,
    borderTopWidth: 1,
    borderTopColor: '#2A3A5C',
  },
  exportBtn: {
    backgroundColor: '#1A5C3A',
    borderRadius: 6,
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderWidth: 1,
    borderColor: '#4CAF50',
  },
  exportBtnText: {
    color: '#FFFFFF',
    fontSize: 12,
    fontWeight: '600',
  },
  shareBtn: {
    backgroundColor: '#1A3A5C',
    borderRadius: 6,
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderWidth: 1,
    borderColor: '#4FC3F7',
  },
  shareBtnText: {
    color: '#4FC3F7',
    fontSize: 12,
    fontWeight: '600',
  },
  deleteBtn: {
    backgroundColor: '#3E1A1A',
    borderRadius: 6,
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderWidth: 1,
    borderColor: '#F44336',
  },
  deleteBtnText: {
    color: '#FF8A80',
    fontSize: 12,
    fontWeight: '600',
  },
});