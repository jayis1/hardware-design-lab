/**
 * StatusCard.js - Reusable status card component
 * Displays a labeled value with optional action button
 */

import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ActivityIndicator,
} from 'react-native';

export default function StatusCard({
  title,
  value,
  status = 'neutral',
  onPress,
  buttonText,
  loading = false,
}) {
  const statusColors = {
    connected: '#4caf50',
    disconnected: '#e94560',
    good: '#4caf50',
    low: '#ff9800',
    neutral: '#a0a0b0',
  };

  const statusColor = statusColors[status] || '#a0a0b0';

  return (
    <View style={styles.card}>
      <View style={styles.cardHeader}>
        <Text style={styles.cardTitle}>{title}</Text>
        <View style={[styles.statusDot, { backgroundColor: statusColor }]} />
      </View>
      <Text style={styles.cardValue}>{value}</Text>
      {buttonText && (
        <TouchableOpacity
          style={[styles.cardButton, { borderColor: statusColor }]}
          onPress={onPress}
          disabled={loading}
        >
          {loading ? (
            <ActivityIndicator size="small" color={statusColor} />
          ) : (
            <Text style={[styles.cardButtonText, { color: statusColor }]}>
              {buttonText}
            </Text>
          )}
        </TouchableOpacity>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    width: '100%',
    marginBottom: 12,
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  cardTitle: {
    fontSize: 14,
    color: '#a0a0b0',
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
  },
  cardValue: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#ffffff',
    marginBottom: 12,
  },
  cardButton: {
    borderWidth: 1,
    borderRadius: 8,
    paddingVertical: 10,
    paddingHorizontal: 20,
    alignItems: 'center',
  },
  cardButtonText: {
    fontSize: 14,
    fontWeight: '600',
  },
});