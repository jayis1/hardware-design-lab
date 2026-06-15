/**
 * CredentialScreen.js - CyberGuard Credential Management
 * View, manage, and delete FIDO2 resident keys
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  Alert,
  Swipeable,
} from 'react-native';

// Sample credential data (in production, fetched from device)
const SAMPLE_CREDENTIALS = [
  { id: '1', rpId: 'github.com', user: 'alice@example.com', created: '2026-01-15' },
  { id: '2', rpId: 'google.com', user: 'alice@gmail.com', created: '2026-02-20' },
  { id: '3', rpId: 'bank.example.com', user: 'alice_banking', created: '2026-03-10' },
];

export default function CredentialScreen({ navigation }) {
  const [credentials, setCredentials] = useState(SAMPLE_CREDENTIALS);
  const [loading, setLoading] = useState(false);

  const deleteCredential = useCallback((id) => {
    Alert.alert(
      'Delete Credential',
      'This will remove this resident key from your CyberGuard token. You will need to re-register with this site.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: () => {
            // Send DELETE_CREDENTIAL command to device
            setCredentials(prev => prev.filter(c => c.id !== id));
          },
        },
      ]
    );
  }, []);

  const renderRightActions = useCallback((id) => (
    <TouchableOpacity
      style={styles.deleteAction}
      onPress={() => deleteCredential(id)}
    >
      <Text style={styles.deleteActionText}>Delete</Text>
    </TouchableOpacity>
  ), [deleteCredential]);

  const renderCredential = useCallback(({ item }) => (
    <Swipeable renderRightActions={() => renderRightActions(item.id)}>
      <View style={styles.credentialCard}>
        <View style={styles.credentialIcon}>
          <Text style={styles.credentialIconText}>
            {item.rpId.charAt(0).toUpperCase()}
          </Text>
        </View>
        <View style={styles.credentialInfo}>
          <Text style={styles.credentialRp}>{item.rpId}</Text>
          <Text style={styles.credentialUser}>{item.user}</Text>
          <Text style={styles.credentialDate}>Added: {item.created}</Text>
        </View>
        <View style={styles.credentialBadge}>
          <Text style={styles.credentialBadgeText}>FIDO2</Text>
        </View>
      </View>
    </Swipeable>
  ), [renderRightActions]);

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>
          {credentials.length} / 50 Resident Keys
        </Text>
        <View style={styles.progressBar}>
          <View
            style={[
              styles.progressFill,
              { width: `${(credentials.length / 50) * 100}%` },
            ]}
          />
        </View>
      </View>

      <FlatList
        data={credentials}
        renderItem={renderCredential}
        keyExtractor={(item) => item.id}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.emptyState}>
            <Text style={styles.emptyIcon}>🔑</Text>
            <Text style={styles.emptyText}>
              No credentials stored yet.
            </Text>
            <Text style={styles.emptySubtext}>
              Register with a website to add FIDO2 credentials.
            </Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
  header: {
    padding: 16,
    backgroundColor: '#1a1a2e',
  },
  headerTitle: {
    fontSize: 16,
    color: '#ffffff',
    marginBottom: 8,
  },
  progressBar: {
    height: 4,
    backgroundColor: '#333',
    borderRadius: 2,
  },
  progressFill: {
    height: 4,
    backgroundColor: '#e94560',
    borderRadius: 2,
  },
  list: {
    padding: 8,
  },
  credentialCard: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
  },
  credentialIcon: {
    width: 48,
    height: 48,
    borderRadius: 24,
    backgroundColor: '#0f3460',
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 12,
  },
  credentialIconText: {
    fontSize: 24,
    color: '#ffffff',
    fontWeight: 'bold',
  },
  credentialInfo: {
    flex: 1,
  },
  credentialRp: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  credentialUser: {
    fontSize: 14,
    color: '#a0a0b0',
    marginTop: 2,
  },
  credentialDate: {
    fontSize: 12,
    color: '#666',
    marginTop: 2,
  },
  credentialBadge: {
    backgroundColor: '#533483',
    borderRadius: 8,
    paddingHorizontal: 8,
    paddingVertical: 4,
  },
  credentialBadgeText: {
    fontSize: 10,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  deleteAction: {
    backgroundColor: '#e94560',
    justifyContent: 'center',
    alignItems: 'center',
    width: 80,
    borderRadius: 12,
    marginLeft: 8,
  },
  deleteActionText: {
    color: '#ffffff',
    fontWeight: 'bold',
    fontSize: 14,
  },
  emptyState: {
    alignItems: 'center',
    marginTop: 60,
  },
  emptyIcon: {
    fontSize: 48,
    marginBottom: 16,
  },
  emptyText: {
    fontSize: 18,
    color: '#ffffff',
    fontWeight: 'bold',
  },
  emptySubtext: {
    fontSize: 14,
    color: '#a0a0b0',
    marginTop: 8,
    textAlign: 'center',
  },
});