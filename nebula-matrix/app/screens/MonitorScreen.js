/**
 * MonitorScreen.js — Nebula Matrix Live Status Dashboard
 *
 * Real-time monitoring dashboard displaying:
 *   - Output frame rate and dropped frame count
 *   - FPGA, DDR3, and ambient temperatures
 *   - Input voltage and current draw
 *   - Ethernet packet statistics
 *   - SPI pixel throughput
 *   - System uptime and error counts
 *   - Fan speed percentage
 *
 * Auto-refreshes every 1 second when connected.
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  ActivityIndicator,
  Platform,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

/* =========================================================================
 * Constants
 * ========================================================================= */

const REFRESH_INTERVAL_MS = 1000;
const THERMAL_WARN_C = 80;
const THERMAL_CRIT_C = 85;

/* =========================================================================
 * MonitorScreen Component
 * ========================================================================= */

const MonitorScreen = ({ protocol, connected, deviceInfo, onRefresh }) => {
  /* State */
  const [status, setStatus] = useState(null);
  const [isRefreshing, setIsRefreshing] = useState(false);
  const [refreshEnabled, setRefreshEnabled] = useState(true);
  const [lastRefresh, setLastRefresh] = useState(null);
  const [errorCount, setErrorCount] = useState(0);

  const intervalRef = useRef(null);
  const mountedRef = useRef(true);

  /* =====================================================================
   * Auto-refresh loop
   * ===================================================================== */

  const fetchStatus = useCallback(async () => {
    if (!connected || !protocol || !refreshEnabled) return;

    setIsRefreshing(true);
    try {
      const newStatus = await protocol.getFullStatus();
      if (mountedRef.current) {
        setStatus(newStatus);
        setLastRefresh(new Date());
        setErrorCount(0);
        if (onRefresh) onRefresh(newStatus);
      }
    } catch (error) {
      if (mountedRef.current) {
        setErrorCount(prev => prev + 1);
        console.warn('Status fetch error:', error.message);
      }
    } finally {
      if (mountedRef.current) {
        setIsRefreshing(false);
      }
    }
  }, [connected, protocol, refreshEnabled, onRefresh]);

  useEffect(() => {
    mountedRef.current = true;

    if (connected && refreshEnabled) {
      /* Initial fetch */
      fetchStatus();
      /* Start interval */
      intervalRef.current = setInterval(fetchStatus, REFRESH_INTERVAL_MS);
    }

    return () => {
      mountedRef.current = false;
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };
  }, [connected, refreshEnabled, fetchStatus]);

  /* =====================================================================
   * Helper: Temperature color
   * ===================================================================== */

  const tempColor = (tempC) => {
    if (!tempC) return '#888';
    if (tempC >= THERMAL_CRIT_C) return '#FF0000';
    if (tempC >= THERMAL_WARN_C) return '#FF9800';
    if (tempC >= 60) return '#FFD700';
    return '#4CAF50';
  };

  const tempIcon = (tempC) => {
    if (!tempC) return 'thermometer';
    if (tempC >= THERMAL_CRIT_C) return 'thermometer-alert';
    if (tempC >= THERMAL_WARN_C) return 'thermometer-high';
    return 'thermometer';
  };

  /* =====================================================================
   * Render
   * ===================================================================== */

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection warning */}
      {!connected && (
        <View style={styles.warningBanner}>
          <Icon name="lan-disconnect" size={24} color="#FF6B6B" />
          <Text style={styles.warningText}>Not connected</Text>
        </View>
      )}

      {/* Refresh controls */}
      <View style={styles.refreshBar}>
        <TouchableOpacity
          style={styles.refreshButton}
          onPress={fetchStatus}
          disabled={!connected}
        >
          <Icon name="refresh" size={18} color={connected ? '#00E5FF' : '#666'} />
          <Text style={[styles.refreshText, !connected && { color: '#666' }]}>
            {isRefreshing ? 'Refreshing...' : 'Refresh Now'}
          </Text>
        </TouchableOpacity>

        <TouchableOpacity
          onPress={() => setRefreshEnabled(!refreshEnabled)}
        >
          <Text style={[styles.autoText, refreshEnabled ? styles.autoOn : styles.autoOff]}>
            {refreshEnabled ? 'Auto: ON' : 'Auto: OFF'}
          </Text>
        </TouchableOpacity>

        {lastRefresh && (
          <Text style={styles.lastRefreshText}>
            {lastRefresh.toLocaleTimeString()}
          </Text>
        )}
      </View>

      {errorCount > 3 && (
        <View style={styles.errorBanner}>
          <Icon name="alert-circle" size={16} color="#FF6B6B" />
          <Text style={styles.errorText}>
            {errorCount} consecutive refresh failures
          </Text>
        </View>
      )}

      {/* ============================================================
       * Section: Frame Rate & Performance
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Performance</Text>
      <View style={styles.metricsGrid}>
        <View style={styles.metricCard}>
          <Icon name="monitor-screenshot" size={28} color="#00E5FF" />
          <Text style={styles.metricValue}>
            {status?.refreshRate || '--'}
          </Text>
          <Text style={styles.metricLabel}>Refresh Hz</Text>
        </View>

        <View style={styles.metricCard}>
          <Icon name="counter" size={28} color="#4CAF50" />
          <Text style={styles.metricValue}>
            {status?.framesOutput?.toLocaleString() || '--'}
          </Text>
          <Text style={styles.metricLabel}>Frames Out</Text>
        </View>

        <View style={styles.metricCard}>
          <Icon name="alert-octagon" size={28} color="#FF9800" />
          <Text style={styles.metricValue}>
            {status?.framesDropped || '0'}
          </Text>
          <Text style={styles.metricLabel}>Dropped</Text>
        </View>

        <View style={styles.metricCard}>
          <Icon name="speedometer" size={28} color="#9C27B0" />
          <Text style={styles.metricValue}>
            {status?.spiPixelRate || '--'}
          </Text>
          <Text style={styles.metricLabel}>MPix/s</Text>
        </View>
      </View>

      {/* ============================================================
       * Section: Temperature
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Temperature</Text>
      <View style={styles.tempGrid}>
        <View style={styles.tempCard}>
          <Icon name={tempIcon(status?.tempFpga)} size={24} color={tempColor(status?.tempFpga)} />
          <Text style={[styles.tempValue, { color: tempColor(status?.tempFpga) }]}>
            {status?.tempFpga != null ? `${status.tempFpga.toFixed(1)}°C` : '--'}
          </Text>
          <Text style={styles.tempLabel}>FPGA</Text>
        </View>

        <View style={styles.tempCard}>
          <Icon name={tempIcon(status?.tempDdrA)} size={24} color={tempColor(status?.tempDdrA)} />
          <Text style={[styles.tempValue, { color: tempColor(status?.tempDdrA) }]}>
            {status?.tempDdrA != null ? `${status.tempDdrA.toFixed(1)}°C` : '--'}
          </Text>
          <Text style={styles.tempLabel}>DDR3 A</Text>
        </View>

        <View style={styles.tempCard}>
          <Icon name={tempIcon(status?.tempDdrB)} size={24} color={tempColor(status?.tempDdrB)} />
          <Text style={[styles.tempValue, { color: tempColor(status?.tempDdrB) }]}>
            {status?.tempDdrB != null ? `${status.tempDdrB.toFixed(1)}°C` : '--'}
          </Text>
          <Text style={styles.tempLabel}>DDR3 B</Text>
        </View>

        <View style={styles.tempCard}>
          <Icon name={tempIcon(status?.tempAmbient)} size={24} color={tempColor(status?.tempAmbient)} />
          <Text style={[styles.tempValue, { color: tempColor(status?.tempAmbient) }]}>
            {status?.tempAmbient != null ? `${status.tempAmbient.toFixed(1)}°C` : '--'}
          </Text>
          <Text style={styles.tempLabel}>Ambient</Text>
        </View>
      </View>

      {/* Thermal warning */}
      {(status?.tempFpga >= THERMAL_WARN_C) && (
        <View style={[styles.thermalBanner,
          status.tempFpga >= THERMAL_CRIT_C ? styles.thermalCrit : styles.thermalWarn]}>
          <Icon
            name={status.tempFpga >= THERMAL_CRIT_C ? 'alert-octagon' : 'alert'}
            size={20}
            color="#fff"
          />
          <Text style={styles.thermalText}>
            {status.tempFpga >= THERMAL_CRIT_C
              ? 'CRITICAL: Thermal shutdown imminent!'
              : `Warning: FPGA temperature at ${status.tempFpga.toFixed(1)}°C`}
          </Text>
        </View>
      )}

      {/* ============================================================
       * Section: Power
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Power</Text>
      <View style={styles.powerRow}>
        <View style={styles.powerCard}>
          <Icon name="flash" size={24} color="#FFD700" />
          <Text style={styles.powerValue}>
            {status?.voltage12v != null ? `${status.voltage12v.toFixed(2)}V` : '--'}
          </Text>
          <Text style={styles.powerLabel}>Input Voltage</Text>
        </View>

        <View style={styles.powerCard}>
          <Icon name="current-ac" size={24} color="#FF9800" />
          <Text style={styles.powerValue}>
            {status?.current12v != null ? `${(status.current12v * 1000).toFixed(0)}mA` : '--'}
          </Text>
          <Text style={styles.powerLabel}>Current Draw</Text>
        </View>

        <View style={styles.powerCard}>
          <Icon name="gauge" size={24} color="#4CAF50" />
          <Text style={styles.powerValue}>
            {status?.powerWatts != null ? `${status.powerWatts.toFixed(1)}W` : '--'}
          </Text>
          <Text style={styles.powerLabel}>Total Power</Text>
        </View>
      </View>

      {/* ============================================================
       * Section: Network Statistics
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Network (Ethernet)</Text>
      <View style={styles.netGrid}>
        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.ethLinkUp ? 'UP' : 'DOWN'}
          </Text>
          <Text style={styles.netLabel}>Link</Text>
        </View>

        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.ethSpeed || '--'}
          </Text>
          <Text style={styles.netLabel}>Speed</Text>
        </View>

        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.ethPacketsRx?.toLocaleString() || '--'}
          </Text>
          <Text style={styles.netLabel}>Packets RX</Text>
        </View>

        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.ethPacketsDropped || '0'}
          </Text>
          <Text style={styles.netLabel}>Dropped</Text>
        </View>
      </View>

      {/* ============================================================
       * Section: SPI Statistics
       * ============================================================ */}
      <Text style={styles.sectionTitle}>SPI Pixel Stream</Text>
      <View style={styles.netGrid}>
        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.spiPixelsRx?.toLocaleString() || '--'}
          </Text>
          <Text style={styles.netLabel}>Pixels RX</Text>
        </View>

        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.spiFramesRx?.toLocaleString() || '--'}
          </Text>
          <Text style={styles.netLabel}>Frames RX</Text>
        </View>

        <View style={styles.netCard}>
          <Text style={[styles.netValue, (status?.spiErrors > 0) && { color: '#FF6B6B' }]}>
            {status?.spiErrors || '0'}
          </Text>
          <Text style={styles.netLabel}>Errors</Text>
        </View>

        <View style={styles.netCard}>
          <Text style={styles.netValue}>
            {status?.spiOverflows || '0'}
          </Text>
          <Text style={styles.netLabel}>Overflows</Text>
        </View>
      </View>

      {/* ============================================================
       * Section: System
       * ============================================================ */}
      <Text style={styles.sectionTitle}>System</Text>
      <View style={styles.sysGrid}>
        <View style={styles.sysCard}>
          <Icon name="fan" size={20} color="#00E5FF" />
          <Text style={styles.sysValue}>
            {status?.fanSpeed != null ? `${status.fanSpeed}%` : '--'}
          </Text>
          <Text style={styles.sysLabel}>Fan Speed</Text>
        </View>

        <View style={styles.sysCard}>
          <Icon name="clock-outline" size={20} color="#888" />
          <Text style={styles.sysValue}>
            {status?.uptime != null ? formatUptime(status.uptime) : '--'}
          </Text>
          <Text style={styles.sysLabel}>Uptime</Text>
        </View>

        <View style={styles.sysCard}>
          <Icon name="alert-circle" size={20} color="#FF6B6B" />
          <Text style={[styles.sysValue, (status?.systemErrors > 0) && { color: '#FF6B6B' }]}>
            {status?.systemErrors != null ? `0x${status.systemErrors.toString(16).toUpperCase()}` : '--'}
          </Text>
          <Text style={styles.sysLabel}>Errors</Text>
        </View>
      </View>
    </ScrollView>
  );
};

/* =========================================================================
 * Helpers
 * ========================================================================= */

const formatUptime = (seconds) => {
  if (!seconds) return '--';
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (d > 0) return `${d}d ${h}h`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m ${s}s`;
};

/* =========================================================================
 * Styles
 * ========================================================================= */

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F3460',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  warningBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3A1A1A',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  warningText: {
    color: '#FF6B6B',
    marginLeft: 10,
    fontSize: 14,
  },
  refreshBar: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 10,
    marginBottom: 12,
  },
  refreshButton: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
  },
  refreshText: {
    color: '#00E5FF',
    fontSize: 13,
    fontWeight: '600',
  },
  autoText: {
    fontSize: 12,
    fontWeight: '700',
  },
  autoOn: {
    color: '#4CAF50',
  },
  autoOff: {
    color: '#666',
  },
  lastRefreshText: {
    color: '#888',
    fontSize: 11,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  errorBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3A1A1A',
    borderRadius: 6,
    padding: 8,
    marginBottom: 12,
  },
  errorText: {
    color: '#FF6B6B',
    marginLeft: 8,
    fontSize: 12,
  },
  sectionTitle: {
    color: '#00E5FF',
    fontSize: 14,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 20,
    marginBottom: 10,
  },
  metricsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  metricCard: {
    width: '47%',
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 14,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  metricValue: {
    color: '#E0E0E0',
    fontSize: 22,
    fontWeight: '700',
    marginTop: 6,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  metricLabel: {
    color: '#888',
    fontSize: 11,
    marginTop: 4,
    textTransform: 'uppercase',
  },
  tempGrid: {
    flexDirection: 'row',
    gap: 8,
  },
  tempCard: {
    flex: 1,
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  tempValue: {
    fontSize: 16,
    fontWeight: '700',
    marginTop: 4,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  tempLabel: {
    color: '#888',
    fontSize: 10,
    marginTop: 2,
    textTransform: 'uppercase',
  },
  thermalBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    borderRadius: 8,
    padding: 10,
    marginTop: 10,
  },
  thermalWarn: {
    backgroundColor: '#FF9800',
  },
  thermalCrit: {
    backgroundColor: '#FF0000',
  },
  thermalText: {
    color: '#fff',
    marginLeft: 8,
    fontSize: 13,
    fontWeight: '600',
    flex: 1,
  },
  powerRow: {
    flexDirection: 'row',
    gap: 8,
  },
  powerCard: {
    flex: 1,
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  powerValue: {
    color: '#E0E0E0',
    fontSize: 16,
    fontWeight: '700',
    marginTop: 4,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  powerLabel: {
    color: '#888',
    fontSize: 10,
    marginTop: 2,
    textTransform: 'uppercase',
  },
  netGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  netCard: {
    width: '47%',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  netValue: {
    color: '#E0E0E0',
    fontSize: 16,
    fontWeight: '700',
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  netLabel: {
    color: '#888',
    fontSize: 10,
    marginTop: 2,
    textTransform: 'uppercase',
  },
  sysGrid: {
    flexDirection: 'row',
    gap: 8,
  },
  sysCard: {
    flex: 1,
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#333',
  },
  sysValue: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: '700',
    marginTop: 4,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  sysLabel: {
    color: '#888',
    fontSize: 10,
    marginTop: 2,
    textTransform: 'uppercase',
  },
});

export default MonitorScreen;
