// ConnectionsScreen.js — Aether-Link NVMe-oF Connection Management
// Displays all active NVMe-oF/TCP connections, allows adding new
// connections to storage targets, and deleting existing ones.

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  RefreshControl,
  TouchableOpacity,
  TextInput,
  Modal,
  Alert,
  ActivityIndicator,
  FlatList,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { useDevice } from '../components/DeviceContext';
import { getTCPStateName, TCP_STATES } from '../utils/protocol';

// ============================================================================
// Connections Screen Component
// ============================================================================

export default function ConnectionsScreen() {
  const {
    connected,
    connections,
    addConnection,
    deleteConnection,
    sendCommand,
    CMD,
  } = useDevice();

  const [refreshing, setRefreshing] = useState(false);
  const [addModalVisible, setAddModalVisible] = useState(false);
  const [deleteConfirmVisible, setDeleteConfirmVisible] = useState(false);
  const [selectedConn, setSelectedConn] = useState(null);
  const [targetIP, setTargetIP] = useState('');
  const [targetPort, setTargetPort] = useState('4420');
  const [targetPortId, setTargetPortId] = useState(0);
  const [adding, setAdding] = useState(false);
  const [deleting, setDeleting] = useState(false);

  const onRefresh = useCallback(async () => {
    setRefreshing(true);
    await new Promise((resolve) => setTimeout(resolve, 1000));
    setRefreshing(false);
  }, []);

  // ==========================================================================
  // Add Connection Handler
  // ==========================================================================

  const handleAddConnection = useCallback(async () => {
    if (!targetIP || !targetPort) {
      Alert.alert('Invalid Input', 'Please enter a valid IP address and port.');
      return;
    }

    // Validate IP format
    const ipParts = targetIP.split('.');
    if (ipParts.length !== 4 || ipParts.some((p) => isNaN(Number(p)) || Number(p) < 0 || Number(p) > 255)) {
      Alert.alert('Invalid IP', 'Please enter a valid IPv4 address (e.g., 192.168.1.100).');
      return;
    }

    const port = parseInt(targetPort, 10);
    if (isNaN(port) || port < 1 || port > 65535) {
      Alert.alert('Invalid Port', 'Port must be between 1 and 65535.');
      return;
    }

    setAdding(true);
    try {
      const success = await addConnection(targetIP, port, targetPortId);
      if (success) {
        setAddModalVisible(false);
        setTargetIP('');
        setTargetPort('4420');
        Alert.alert('Success', `Connection to ${targetIP}:${port} initiated.`);
      } else {
        Alert.alert('Failed', 'Could not create connection. Check device status.');
      }
    } catch (err) {
      Alert.alert('Error', err.message || 'Connection failed.');
    } finally {
      setAdding(false);
    }
  }, [targetIP, targetPort, targetPortId, addConnection]);

  // ==========================================================================
  // Delete Connection Handler
  // ==========================================================================

  const handleDeleteConnection = useCallback(async () => {
    if (!selectedConn) return;

    setDeleting(true);
    try {
      const success = await deleteConnection(selectedConn.connId);
      if (success) {
        setDeleteConfirmVisible(false);
        setSelectedConn(null);
      } else {
        Alert.alert('Failed', 'Could not delete connection.');
      }
    } catch (err) {
      Alert.alert('Error', err.message || 'Delete failed.');
    } finally {
      setDeleting(false);
    }
  }, [selectedConn, deleteConnection]);

  const confirmDelete = useCallback((conn) => {
    setSelectedConn(conn);
    setDeleteConfirmVisible(true);
  }, []);

  // ==========================================================================
  // Not Connected State
  // ==========================================================================

  if (!connected) {
    return (
      <View style={styles.centeredContainer}>
        <Icon name="lan-disconnect" size={64} color="#6B7280" />
        <Text style={styles.disconnectedTitle}>Not Connected</Text>
        <Text style={styles.disconnectedSubtitle}>
          Connect to your Aether-Link device to manage NVMe-oF connections
        </Text>
      </View>
    );
  }

  // ==========================================================================
  // Connection State Colors
  // ==========================================================================

  const getStateColor = (state) => {
    switch (state) {
      case 4: return '#10B981';  // ESTABLISHED — green
      case 2: return '#3B82F6';  // SYN_SENT — blue
      case 3: return '#3B82F6';  // SYN_RCVD — blue
      case 0: return '#6B7280';  // CLOSED — gray
      case 10: return '#F59E0B'; // TIME_WAIT — amber
      default: return '#EF4444'; // Error/transition — red
    }
  };

  // ==========================================================================
  // Render Connection Item
  // ==========================================================================

  const renderConnectionItem = ({ item }) => (
    <TouchableOpacity
      style={styles.connCard}
      onPress={() => confirmDelete(item)}
      onLongPress={() => {
        Alert.alert(
          'Connection Details',
          `ID: ${item.connId}\n` +
          `State: ${getTCPStateName(item.state)}\n` +
          `Port: ${item.portId}\n` +
          `Source: ${item.srcIP}:${item.srcPort}\n` +
          `Target: ${item.dstIP}:${item.dstPort}\n` +
          `TX: ${formatBytes(item.txBytes)}\n` +
          `RX: ${formatBytes(item.rxBytes)}`,
          [{ text: 'OK' }]
        );
      }}
    >
      <View style={styles.connHeader}>
        <Icon
          name={item.state === 4 ? 'lan-connect' : 'lan-pending'}
          size={20}
          color={getStateColor(item.state)}
        />
        <Text style={styles.connId}>[{item.connId}]</Text>
        <Text style={[styles.connState, { color: getStateColor(item.state) }]}>
          {getTCPStateName(item.state)}
        </Text>
        <Icon name="delete-outline" size={18} color="#6B7280" style={styles.deleteIcon} />
      </View>

      <View style={styles.connDetails}>
        <View style={styles.connEndpoint}>
          <Text style={styles.connLabel}>Target</Text>
          <Text style={styles.connValue}>{item.dstIP}:{item.dstPort}</Text>
        </View>
        <View style={styles.connEndpoint}>
          <Text style={styles.connLabel}>Source</Text>
          <Text style={styles.connValue}>{item.srcIP}:{item.srcPort}</Text>
        </View>
        <View style={styles.connEndpoint}>
          <Text style={styles.connLabel}>Port</Text>
          <Text style={styles.connValue}>QSFP28 #{item.portId}</Text>
        </View>
      </View>

      <View style={styles.connStats}>
        <View style={styles.connStatItem}>
          <Icon name="upload" size={14} color="#3B82F6" />
          <Text style={styles.connStatValue}>{formatBytes(item.txBytes)}</Text>
          <Text style={styles.connStatLabel}>TX</Text>
        </View>
        <View style={styles.connStatItem}>
          <Icon name="download" size={14} color="#10B981" />
          <Text style={styles.connStatValue}>{formatBytes(item.rxBytes)}</Text>
          <Text style={styles.connStatLabel}>RX</Text>
        </View>
      </View>
    </TouchableOpacity>
  );

  // ==========================================================================
  // Render
  // ==========================================================================

  return (
    <View style={styles.container}>
      {/* Header with Add Button */}
      <View style={styles.header}>
        <Text style={styles.headerTitle}>
          {connections.length} Active Connection{connections.length !== 1 ? 's' : ''}
        </Text>
        <TouchableOpacity
          style={styles.addButton}
          onPress={() => setAddModalVisible(true)}
        >
          <Icon name="plus" size={20} color="#FFFFFF" />
          <Text style={styles.addButtonText}>Add</Text>
        </TouchableOpacity>
      </View>

      {/* Connection List */}
      {connections.length === 0 ? (
        <View style={styles.emptyContainer}>
          <Icon name="lan-disconnect" size={48} color="#374151" />
          <Text style={styles.emptyText}>No active connections</Text>
          <Text style={styles.emptySubtext}>
            Add an NVMe-oF storage target to begin
          </Text>
        </View>
      ) : (
        <FlatList
          data={connections}
          renderItem={renderConnectionItem}
          keyExtractor={(item) => item.connId.toString()}
          contentContainerStyle={styles.listContent}
          refreshControl={
            <RefreshControl refreshing={refreshing} onRefresh={onRefresh} tintColor="#3B82F6" />
          }
        />
      )}

      {/* Add Connection Modal */}
      <Modal
        visible={addModalVisible}
        animationType="slide"
        transparent={true}
        onRequestClose={() => setAddModalVisible(false)}
      >
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Add NVMe-oF Connection</Text>

            <Text style={styles.inputLabel}>Target IP Address</Text>
            <TextInput
              style={styles.input}
              value={targetIP}
              onChangeText={setTargetIP}
              placeholder="192.168.1.100"
              placeholderTextColor="#4B5563"
              keyboardType="numeric"
              autoCapitalize="none"
            />

            <Text style={styles.inputLabel}>Target Port</Text>
            <TextInput
              style={styles.input}
              value={targetPort}
              onChangeText={setTargetPort}
              placeholder="4420"
              placeholderTextColor="#4B5563"
              keyboardType="numeric"
            />

            <Text style={styles.inputLabel}>Local QSFP28 Port</Text>
            <View style={styles.portSelector}>
              <TouchableOpacity
                style={[styles.portOption, targetPortId === 0 && styles.portOptionSelected]}
                onPress={() => setTargetPortId(0)}
              >
                <Text style={[styles.portOptionText, targetPortId === 0 && styles.portOptionTextSelected]}>
                  Port 0
                </Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.portOption, targetPortId === 1 && styles.portOptionSelected]}
                onPress={() => setTargetPortId(1)}
              >
                <Text style={[styles.portOptionText, targetPortId === 1 && styles.portOptionTextSelected]}>
                  Port 1
                </Text>
              </TouchableOpacity>
            </View>

            <View style={styles.modalButtons}>
              <TouchableOpacity
                style={styles.cancelButton}
                onPress={() => setAddModalVisible(false)}
              >
                <Text style={styles.cancelButtonText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.confirmButton, adding && styles.buttonDisabled]}
                onPress={handleAddConnection}
                disabled={adding}
              >
                {adding ? (
                  <ActivityIndicator size="small" color="#FFFFFF" />
                ) : (
                  <Text style={styles.confirmButtonText}>Connect</Text>
                )}
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>

      {/* Delete Confirmation Modal */}
      <Modal
        visible={deleteConfirmVisible}
        animationType="fade"
        transparent={true}
        onRequestClose={() => setDeleteConfirmVisible(false)}
      >
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Icon name="alert-circle" size={48} color="#EF4444" style={styles.warnIcon} />
            <Text style={styles.modalTitle}>Delete Connection?</Text>
            {selectedConn && (
              <Text style={styles.deleteDetailText}>
                Connection [{selectedConn.connId}] to {selectedConn.dstIP}:{selectedConn.dstPort}
                {'\n'}State: {getTCPStateName(selectedConn.state)}
                {'\n'}TX: {formatBytes(selectedConn.txBytes)} / RX: {formatBytes(selectedConn.rxBytes)}
              </Text>
            )}
            <Text style={styles.deleteWarning}>
              This will terminate the NVMe-oF connection. Any pending I/O will be aborted.
            </Text>

            <View style={styles.modalButtons}>
              <TouchableOpacity
                style={styles.cancelButton}
                onPress={() => setDeleteConfirmVisible(false)}
              >
                <Text style={styles.cancelButtonText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.deleteButton, deleting && styles.buttonDisabled]}
                onPress={handleDeleteConnection}
                disabled={deleting}
              >
                {deleting ? (
                  <ActivityIndicator size="small" color="#FFFFFF" />
                ) : (
                  <Text style={styles.deleteButtonText}>Delete</Text>
                )}
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
}

// ============================================================================
// Formatting Helpers
// ============================================================================

function formatBytes(num) {
  if (num === undefined || num === null) return '0 B';
  if (num >= 1e9) return `${(num / 1e9).toFixed(2)} GB`;
  if (num >= 1e6) return `${(num / 1e6).toFixed(2)} MB`;
  if (num >= 1e3) return `${(num / 1e3).toFixed(2)} KB`;
  return `${num} B`;
}

// ============================================================================
// Styles
// ============================================================================

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E1A',
  },
  centeredContainer: {
    flex: 1,
    backgroundColor: '#0A0E1A',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 32,
  },
  disconnectedTitle: {
    fontSize: 22,
    fontWeight: '700',
    color: '#E5E7EB',
    marginTop: 16,
  },
  disconnectedSubtitle: {
    fontSize: 14,
    color: '#9CA3AF',
    textAlign: 'center',
    marginTop: 8,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#1F2937',
  },
  headerTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#E5E7EB',
  },
  addButton: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3B82F6',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
  },
  addButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
    marginLeft: 6,
  },
  listContent: {
    padding: 12,
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 32,
  },
  emptyText: {
    fontSize: 18,
    fontWeight: '600',
    color: '#9CA3AF',
    marginTop: 16,
  },
  emptySubtext: {
    fontSize: 14,
    color: '#6B7280',
    textAlign: 'center',
    marginTop: 8,
  },
  connCard: {
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 14,
    marginBottom: 10,
    borderWidth: 1,
    borderColor: '#1F2937',
  },
  connHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 10,
  },
  connId: {
    fontSize: 14,
    fontWeight: '700',
    color: '#E5E7EB',
    marginLeft: 8,
  },
  connState: {
    fontSize: 13,
    fontWeight: '600',
    marginLeft: 8,
    flex: 1,
  },
  deleteIcon: {
    marginLeft: 8,
  },
  connDetails: {
    flexDirection: 'row',
    marginBottom: 10,
  },
  connEndpoint: {
    flex: 1,
  },
  connLabel: {
    fontSize: 10,
    color: '#6B7280',
    textTransform: 'uppercase',
    letterSpacing: 0.5,
  },
  connValue: {
    fontSize: 13,
    color: '#E5E7EB',
    marginTop: 2,
  },
  connStats: {
    flexDirection: 'row',
    borderTopWidth: 1,
    borderTopColor: '#1F2937',
    paddingTop: 10,
  },
  connStatItem: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
  },
  connStatValue: {
    fontSize: 13,
    fontWeight: '600',
    color: '#E5E7EB',
    marginLeft: 6,
    marginRight: 4,
  },
  connStatLabel: {
    fontSize: 11,
    color: '#6B7280',
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 24,
  },
  modalContent: {
    backgroundColor: '#111827',
    borderRadius: 16,
    padding: 24,
    width: '100%',
    maxWidth: 400,
    borderWidth: 1,
    borderColor: '#1F2937',
  },
  modalTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: '#E5E7EB',
    marginBottom: 16,
  },
  inputLabel: {
    fontSize: 13,
    color: '#9CA3AF',
    marginBottom: 6,
    marginTop: 12,
  },
  input: {
    backgroundColor: '#1F2937',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    color: '#E5E7EB',
    borderWidth: 1,
    borderColor: '#374151',
  },
  portSelector: {
    flexDirection: 'row',
    gap: 8,
  },
  portOption: {
    flex: 1,
    backgroundColor: '#1F2937',
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#374151',
  },
  portOptionSelected: {
    borderColor: '#3B82F6',
    backgroundColor: '#1E3A5F',
  },
  portOptionText: {
    fontSize: 14,
    color: '#9CA3AF',
  },
  portOptionTextSelected: {
    color: '#3B82F6',
    fontWeight: '600',
  },
  modalButtons: {
    flexDirection: 'row',
    justifyContent: 'flex-end',
    marginTop: 20,
    gap: 12,
  },
  cancelButton: {
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#374151',
  },
  cancelButtonText: {
    color: '#9CA3AF',
    fontSize: 14,
    fontWeight: '600',
  },
  confirmButton: {
    backgroundColor: '#3B82F6',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
  },
  confirmButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
  deleteButton: {
    backgroundColor: '#EF4444',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
  },
  deleteButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
  buttonDisabled: {
    opacity: 0.6,
  },
  warnIcon: {
    alignSelf: 'center',
    marginBottom: 12,
  },
  deleteDetailText: {
    fontSize: 13,
    color: '#D1D5DB',
    backgroundColor: '#1F2937',
    padding: 12,
    borderRadius: 8,
    marginBottom: 12,
    lineHeight: 20,
  },
  deleteWarning: {
    fontSize: 12,
    color: '#EF4444',
    marginBottom: 8,
  },
});
