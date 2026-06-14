/**
 * DeviceScreen.js — Vortex-SDR device connection and status screen
 * Shows BLE scan results, connection status, and device info
 */

import React, { useEffect, useState } from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  StyleSheet,
  RefreshControl,
  ActivityIndicator,
} from 'react-native';
import { useBle } from '../components/BleContext';
import { theme } from '../components/Theme';

function DeviceScreen({ navigation }) {
  const {
    isScanning,
    isConnected,
    connectedDevice,
    deviceList,
    deviceStatus,
    startScan,
    stopScan,
    connectToDevice,
    disconnectDevice,
  } = useBle();

  const [refreshing, setRefreshing] = useState(false);

  useEffect(() => {
    if (!isConnected) {
      startScan();
    }
  }, []);

  const handleRefresh = async () => {
    setRefreshing(true);
    await startScan();
    setRefreshing(false);
  };

  const handleDevicePress = async (deviceId) => {
    if (isConnected && connectedDevice === deviceId) {
      await disconnectDevice();
    } else {
      stopScan();
      await connectToDevice(deviceId);
    }
  };

  const getStateLabel = (state) => {
    const labels = {
      0: 'Initializing',
      1: 'Idle',
      2: 'Sweeping (Continuous)',
      3: 'Sweeping (Single)',
      4: 'Zero Span',
      5: 'Data Logging',
      6: 'USB Transfer',
      7: 'Error',
      8: 'Sleep',
    };
    return labels[state] || 'Unknown';
  };

  const formatFreq = (hz) => {
    if (!hz) return '—';
    if (hz >= 1e9) return `${(hz / 1e9).toFixed(3)} GHz`;
    if (hz >= 1e6) return `${(hz / 1e6).toFixed(3)} MHz`;
    if (hz >= 1e3) return `${(hz / 1e3).toFixed(1)} kHz`;
    return `${hz} Hz`;
  };

  const getBatteryIcon = (mv) => {
    if (mv >= 4000) return '🔋'; // Full
    if (mv >= 3700) return '🔋';
    if (mv >= 3500) return '🪫';
    if (mv >= 3200) return '🪫';
    return '⚠️'; // Low
  };

  const getBatteryPercent = (mv) => {
    if (mv >= 4200) return 100;
    if (mv <= 3000) return 0;
    return Math.round(((mv - 3000) / 1200) * 100);
  };

  const renderDevice = ({ item }) => {
    const isThisConnected = isConnected && connectedDevice === item.id;
    return (
      <TouchableOpacity
        style={[
          styles.deviceCard,
          isThisConnected && styles.deviceCardConnected,
        ]}
        onPress={() => handleDevicePress(item.id)}
        activeOpacity={0.7}
      >
        <View style={styles.deviceInfo}>
          <Text style={styles.deviceName}>{item.name}</Text>
          <Text style={styles.deviceId}>
            ID: {item.id.substring(0, 8)}...
          </Text>
          <Text style={styles.deviceRssi}>RSSI: {item.rssi} dBm</Text>
        </View>
        <View style={styles.deviceAction}>
          {isThisConnected ? (
            <View style={styles.connectedBadge}>
              <Text style={styles.connectedText}>Connected</Text>
            </View>
          ) : (
            <Text style={styles.connectText}>Connect</Text>
          )}
        </View>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      {/* Connection status header */}
      <View style={styles.statusHeader}>
        <View style={styles.statusRow}>
          <View
            style={[
              styles.statusDot,
              { backgroundColor: isConnected ? theme.colors.success : theme.colors.error },
            ]}
          />
          <Text style={styles.statusText}>
            {isConnected ? 'Connected' : isScanning ? 'Scanning...' : 'Disconnected'}
          </Text>
        </View>

        {isConnected && (
          <View style={styles.deviceStats}>
            <Text style={styles.statText}>
              State: {getStateLabel(deviceStatus.state)}
            </Text>
            <Text style={styles.statText}>
              Range: {formatFreq(deviceStatus.startFreq)} — {formatFreq(deviceStatus.stopFreq)}
            </Text>
            <Text style={styles.statText}>
              FFT: {deviceStatus.fftSize} pts
            </Text>
            <Text style={styles.statText}>
              Sweeps: {deviceStatus.sweepCount}
            </Text>
            <Text style={styles.statText}>
              {getBatteryIcon(deviceStatus.batteryMv)} {getBatteryPercent(deviceStatus.batteryMv)}%
              ({deviceStatus.batteryMv} mV)
            </Text>
            <Text style={styles.statText}>
              PLL: {deviceStatus.pllLocked ? '✓ Locked' : '✗ Unlocked'}
            </Text>
            <Text style={styles.statText}>
              Temp: {deviceStatus.temperature}°C
            </Text>
          </View>
        )}
      </View>

      {/* Device list */}
      {!isConnected && (
        <View style={styles.scanSection}>
          <View style={styles.scanHeader}>
            <Text style={styles.sectionTitle}>
              {isScanning ? 'Scanning for Vortex-SDR devices...' : 'Available Devices'}
            </Text>
            {isScanning && <ActivityIndicator size="small" color={theme.colors.primary} />}
          </View>

          <FlatList
            data={deviceList}
            renderItem={renderDevice}
            keyExtractor={(item) => item.id}
            refreshControl={
              <RefreshControl
                refreshing={refreshing}
                onRefresh={handleRefresh}
                tintColor={theme.colors.primary}
              />
            }
            ListEmptyComponent={
              <View style={styles.emptyState}>
                <Text style={styles.emptyText}>
                  {isScanning
                    ? 'Looking for Vortex-SDR devices nearby...'
                    : 'No devices found. Pull to refresh.'}
                </Text>
              </View>
            }
            contentContainerStyle={styles.deviceList}
          />
        </View>
      )}

      {/* Disconnect button */}
      {isConnected && (
        <TouchableOpacity
          style={styles.disconnectButton}
          onPress={disconnectDevice}
          activeOpacity={0.7}
        >
          <Text style={styles.disconnectButtonText}>Disconnect</Text>
        </TouchableOpacity>
      )}

      {/* Scan button */}
      {!isConnected && !isScanning && (
        <TouchableOpacity
          style={styles.scanButton}
          onPress={startScan}
          activeOpacity={0.7}
        >
          <Text style={styles.scanButtonText}>Scan for Devices</Text>
        </TouchableOpacity>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background,
    padding: theme.spacing.md,
  },
  statusHeader: {
    backgroundColor: theme.colors.surface,
    borderRadius: theme.borderRadius.lg,
    padding: theme.spacing.lg,
    marginBottom: theme.spacing.md,
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: theme.spacing.sm,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: theme.spacing.sm,
  },
  statusText: {
    fontSize: theme.fontSize.lg,
    fontWeight: theme.fontWeight.semiBold,
    color: theme.colors.text,
  },
  deviceStats: {
    marginTop: theme.spacing.sm,
    paddingLeft: theme.spacing.md,
  },
  statText: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.textSecondary,
    marginBottom: 2,
  },
  scanSection: {
    flex: 1,
  },
  scanHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: theme.spacing.md,
  },
  sectionTitle: {
    fontSize: theme.fontSize.lg,
    fontWeight: theme.fontWeight.semiBold,
    color: theme.colors.text,
  },
  deviceList: {
    flexGrow: 1,
  },
  deviceCard: {
    backgroundColor: theme.colors.surface,
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.lg,
    marginBottom: theme.spacing.sm,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: theme.colors.border,
  },
  deviceCardConnected: {
    borderColor: theme.colors.primary,
    borderWidth: 2,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    fontSize: theme.fontSize.lg,
    fontWeight: theme.fontWeight.semiBold,
    color: theme.colors.text,
  },
  deviceId: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.textDim,
    marginTop: 2,
  },
  deviceRssi: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.textSecondary,
    marginTop: 2,
  },
  deviceAction: {
    marginLeft: theme.spacing.md,
  },
  connectedBadge: {
    backgroundColor: theme.colors.primary,
    borderRadius: theme.borderRadius.sm,
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.sm,
  },
  connectedText: {
    color: theme.colors.background,
    fontSize: theme.fontSize.sm,
    fontWeight: theme.fontWeight.bold,
  },
  connectText: {
    color: theme.colors.primary,
    fontSize: theme.fontSize.md,
    fontWeight: theme.fontWeight.semiBold,
  },
  emptyState: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: theme.spacing.xxl,
  },
  emptyText: {
    color: theme.colors.textDim,
    fontSize: theme.fontSize.md,
    textAlign: 'center',
  },
  disconnectButton: {
    backgroundColor: theme.colors.error,
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.lg,
    alignItems: 'center',
    marginTop: theme.spacing.md,
  },
  disconnectButtonText: {
    color: '#FFFFFF',
    fontSize: theme.fontSize.lg,
    fontWeight: theme.fontWeight.bold,
  },
  scanButton: {
    backgroundColor: theme.colors.primary,
    borderRadius: theme.borderRadius.md,
    padding: theme.spacing.lg,
    alignItems: 'center',
    marginTop: theme.spacing.md,
  },
  scanButtonText: {
    color: theme.colors.background,
    fontSize: theme.fontSize.lg,
    fontWeight: theme.fontWeight.bold,
  },
});

export default DeviceScreen;