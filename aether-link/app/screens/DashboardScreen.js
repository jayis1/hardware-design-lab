// DashboardScreen.js — Aether-Link Real-Time Telemetry Dashboard
// Displays live device status, temperatures, power, network links,
// and performance metrics with auto-refreshing gauges and charts.

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  RefreshControl,
  Dimensions,
  ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { useDevice } from '../components/DeviceContext';
import TelemetryGauge from '../components/TelemetryGauge';
import { getTCPStateName } from '../utils/protocol';

const { width: SCREEN_WIDTH } = Dimensions.get('window');

// ============================================================================
// Dashboard Screen Component
// ============================================================================

export default function DashboardScreen() {
  const {
    connected,
    connecting,
    telemetry,
    stats,
    connections,
    firmwareVersion,
    deviceStatus,
    deviceFeatures,
    connect,
  } = useDevice();

  const [refreshing, setRefreshing] = useState(false);

  const onRefresh = useCallback(async () => {
    setRefreshing(true);
    // Data is auto-polled; just wait briefly for next update
    await new Promise((resolve) => setTimeout(resolve, 1000));
    setRefreshing(false);
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
          Connect to your Aether-Link device to view telemetry
        </Text>
        {connecting ? (
          <ActivityIndicator size="large" color="#3B82F6" style={{ marginTop: 20 }} />
        ) : (
          <TouchableOpacity style={styles.connectButton} onPress={connect}>
            <Text style={styles.connectButtonText}>Connect</Text>
          </TouchableOpacity>
        )}
      </View>
    );
  }

  // ==========================================================================
  // Status Bit Helpers
  // ==========================================================================

  const isStatusBitSet = (bit) => {
    return deviceStatus ? (deviceStatus & (1 << bit)) !== 0 : false;
  };

  const statusItems = [
    { bit: 0, label: 'FPGA Ready', icon: 'chip' },
    { bit: 1, label: 'DDR4 Calibrated', icon: 'memory' },
    { bit: 2, label: 'PCIe Gen4 Up', icon: 'expansion-card' },
    { bit: 3, label: 'Port 0 Link', icon: 'ethernet-cable' },
    { bit: 4, label: 'Port 1 Link', icon: 'ethernet-cable' },
    { bit: 5, label: 'NVMe Enabled', icon: 'harddisk' },
    { bit: 6, label: 'Throttling', icon: 'thermometer-alert' },
    { bit: 9, label: 'Secure Boot OK', icon: 'shield-check' },
    { bit: 10, label: 'QSFP0 Present', icon: 'transceiver' },
    { bit: 11, label: 'QSFP1 Present', icon: 'transceiver' },
  ];

  // ==========================================================================
  // Render
  // ==========================================================================

  return (
    <ScrollView
      style={styles.container}
      contentContainerStyle={styles.contentContainer}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} tintColor="#3B82F6" />
      }
    >
      {/* Firmware Version Banner */}
      {firmwareVersion && (
        <View style={styles.versionBanner}>
          <Icon name="information-outline" size={16} color="#9CA3AF" />
          <Text style={styles.versionText}>
            Firmware v{(firmwareVersion >> 24) & 0xFF}.{(firmwareVersion >> 16) & 0xFF}.
            {(firmwareVersion >> 8) & 0xFF}.{firmwareVersion & 0xFF}
          </Text>
        </View>
      )}

      {/* Status Overview */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Status</Text>
        <View style={styles.statusGrid}>
          {statusItems.map((item) => (
            <View key={item.bit} style={styles.statusItem}>
              <Icon
                name={item.icon}
                size={20}
                color={isStatusBitSet(item.bit) ? '#10B981' : '#6B7280'}
              />
              <Text style={[
                styles.statusLabel,
                { color: isStatusBitSet(item.bit) ? '#10B981' : '#6B7280' }
              ]}>
                {item.label}
              </Text>
            </View>
          ))}
        </View>
      </View>

      {/* Temperature Gauges */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Temperatures</Text>
        <View style={styles.gaugeRow}>
          <TelemetryGauge
            label="FPGA"
            value={telemetry?.fpgaTemp ?? 0}
            unit="°C"
            min={0}
            max={100}
            warnThreshold={85}
            critThreshold={95}
            icon="chip"
          />
          <TelemetryGauge
            label="QSFP0"
            value={telemetry?.qsfp0Temp ?? 0}
            unit="°C"
            min={0}
            max={80}
            warnThreshold={65}
            critThreshold={75}
            icon="transceiver"
          />
          <TelemetryGauge
            label="QSFP1"
            value={telemetry?.qsfp1Temp ?? 0}
            unit="°C"
            min={0}
            max={80}
            warnThreshold={65}
            critThreshold={75}
            icon="transceiver"
          />
        </View>
      </View>

      {/* Power & Fans */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Power & Cooling</Text>
        <View style={styles.powerRow}>
          <View style={styles.powerCard}>
            <Icon name="lightning-bolt" size={24} color="#F59E0B" />
            <Text style={styles.powerValue}>
              {telemetry ? (telemetry.powerMW / 1000).toFixed(1) : '--'} W
            </Text>
            <Text style={styles.powerLabel}>Board Power</Text>
            <Text style={styles.powerSubtext}>
              {telemetry ? `${(telemetry.voltageMV / 1000).toFixed(2)}V · ${(telemetry.currentMA / 1000).toFixed(2)}A` : '--'}
            </Text>
          </View>
          <View style={styles.powerCard}>
            <Icon name="fan" size={24} color="#3B82F6" />
            <Text style={styles.powerValue}>
              {telemetry?.fan0RPM ?? '--'} RPM
            </Text>
            <Text style={styles.powerLabel}>Fan 0</Text>
            <Text style={styles.powerSubtext}>
              PWM: {telemetry?.fanPWM ?? '--'}/255
            </Text>
          </View>
          <View style={styles.powerCard}>
            <Icon name="fan" size={24} color="#3B82F6" />
            <Text style={styles.powerValue}>
              {telemetry?.fan1RPM ?? '--'} RPM
            </Text>
            <Text style={styles.powerLabel}>Fan 1</Text>
          </View>
        </View>
      </View>

      {/* Network Ports */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Network Ports</Text>
        <View style={styles.portRow}>
          <View style={styles.portCard}>
            <Icon
              name="ethernet-cable"
              size={24}
              color={telemetry?.port0Status ? '#10B981' : '#EF4444'}
            />
            <Text style={styles.portLabel}>Port 0</Text>
            <Text style={[
              styles.portStatus,
              { color: telemetry?.port0Status ? '#10B981' : '#EF4444' }
            ]}>
              {telemetry?.port0Status ? 'UP' : 'DOWN'}
            </Text>
            {telemetry?.port0Speed > 0 && (
              <Text style={styles.portSpeed}>
                {telemetry.port0Speed >= 100000 ? '100G' :
                 telemetry.port0Speed >= 40000 ? '40G' :
                 telemetry.port0Speed >= 25000 ? '25G' :
                 telemetry.port0Speed >= 10000 ? '10G' :
                 `${(telemetry.port0Speed / 1000).toFixed(0)}G`}
              </Text>
            )}
          </View>
          <View style={styles.portCard}>
            <Icon
              name="ethernet-cable"
              size={24}
              color={telemetry?.port1Status ? '#10B981' : '#EF4444'}
            />
            <Text style={styles.portLabel}>Port 1</Text>
            <Text style={[
              styles.portStatus,
              { color: telemetry?.port1Status ? '#10B981' : '#EF4444' }
            ]}>
              {telemetry?.port1Status ? 'UP' : 'DOWN'}
            </Text>
            {telemetry?.port1Speed > 0 && (
              <Text style={styles.portSpeed}>
                {telemetry.port1Speed >= 100000 ? '100G' :
                 telemetry.port1Speed >= 40000 ? '40G' :
                 telemetry.port1Speed >= 25000 ? '25G' :
                 telemetry.port1Speed >= 10000 ? '10G' :
                 `${(telemetry.port1Speed / 1000).toFixed(0)}G`}
              </Text>
            )}
          </View>
        </View>
      </View>

      {/* Performance Stats */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Performance</Text>
        <View style={styles.statsGrid}>
          <StatItem label="TX Packets" value={formatBigNumber(stats?.txPackets)} icon="upload" />
          <StatItem label="RX Packets" value={formatBigNumber(stats?.rxPackets)} icon="download" />
          <StatItem label="TX Bytes" value={formatBytes(stats?.txBytes)} icon="upload" />
          <StatItem label="RX Bytes" value={formatBytes(stats?.rxBytes)} icon="download" />
          <StatItem label="TX Drops" value={formatBigNumber(stats?.txDrops)} icon="alert-circle" color="#EF4444" />
          <StatItem label="RX Drops" value={formatBigNumber(stats?.rxDrops)} icon="alert-circle" color="#EF4444" />
          <StatItem label="TCP Retrans" value={formatBigNumber(stats?.tcpRetransmissions)} icon="refresh" color="#F59E0B" />
          <StatItem label="NVMe IOPS" value={formatBigNumber(stats?.nvmeIOPS)} icon="harddisk" color="#10B981" />
        </View>
      </View>

      {/* Active Connections Summary */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>
          Active Connections ({connections.length})
        </Text>
        {connections.slice(0, 5).map((conn) => (
          <View key={conn.connId} style={styles.connSummary}>
            <Icon
              name="lan-connect"
              size={16}
              color={conn.state === 4 ? '#10B981' : '#F59E0B'}
            />
            <Text style={styles.connText}>
              [{conn.connId}] {conn.dstIP}:{conn.dstPort} — {getTCPStateName(conn.state)}
            </Text>
            <Text style={styles.connBytes}>
              {formatBytes(conn.txBytes)} TX / {formatBytes(conn.rxBytes)} RX
            </Text>
          </View>
        ))}
        {connections.length > 5 && (
          <Text style={styles.moreText}>
            +{connections.length - 5} more — see Connections tab
          </Text>
        )}
      </View>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

// ============================================================================
// Stat Item Sub-Component
// ============================================================================

function StatItem({ label, value, icon, color = '#E5E7EB' }) {
  return (
    <View style={styles.statItem}>
      <Icon name={icon} size={18} color={color} />
      <Text style={[styles.statValue, { color }]}>{value}</Text>
      <Text style={styles.statLabel}>{label}</Text>
    </View>
  );
}

// ============================================================================
// Formatting Helpers
// ============================================================================

function formatBigNumber(num) {
  if (num === undefined || num === null) return '--';
  if (typeof num === 'bigint') num = Number(num);
  if (num >= 1e12) return `${(num / 1e12).toFixed(1)}T`;
  if (num >= 1e9) return `${(num / 1e9).toFixed(1)}B`;
  if (num >= 1e6) return `${(num / 1e6).toFixed(1)}M`;
  if (num >= 1e3) return `${(num / 1e3).toFixed(1)}K`;
  return num.toString();
}

function formatBytes(num) {
  if (num === undefined || num === null) return '--';
  if (typeof num === 'bigint') num = Number(num);
  if (num >= 1e12) return `${(num / 1e12).toFixed(2)} TB`;
  if (num >= 1e9) return `${(num / 1e9).toFixed(2)} GB`;
  if (num >= 1e6) return `${(num / 1e6).toFixed(2)} MB`;
  if (num >= 1e3) return `${(num / 1e3).toFixed(2)} KB`;
  return `${num} B`;
}

// ============================================================================
// Styles
// ============================================================================

import { TouchableOpacity } from 'react-native';

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E1A',
  },
  contentContainer: {
    padding: 16,
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
  connectButton: {
    marginTop: 24,
    backgroundColor: '#3B82F6',
    paddingHorizontal: 32,
    paddingVertical: 12,
    borderRadius: 8,
  },
  connectButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '600',
  },
  versionBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#111827',
    padding: 10,
    borderRadius: 8,
    marginBottom: 16,
  },
  versionText: {
    color: '#9CA3AF',
    fontSize: 13,
    marginLeft: 8,
  },
  section: {
    marginBottom: 20,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '700',
    color: '#E5E7EB',
    marginBottom: 12,
  },
  statusGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 12,
  },
  statusItem: {
    flexDirection: 'row',
    alignItems: 'center',
    width: '50%',
    paddingVertical: 6,
  },
  statusLabel: {
    fontSize: 13,
    marginLeft: 8,
  },
  gaugeRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  powerRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  powerCard: {
    flex: 1,
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 12,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  powerValue: {
    fontSize: 20,
    fontWeight: '700',
    color: '#E5E7EB',
    marginTop: 4,
  },
  powerLabel: {
    fontSize: 12,
    color: '#9CA3AF',
    marginTop: 2,
  },
  powerSubtext: {
    fontSize: 11,
    color: '#6B7280',
    marginTop: 2,
  },
  portRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  portCard: {
    flex: 1,
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  portLabel: {
    fontSize: 14,
    fontWeight: '600',
    color: '#E5E7EB',
    marginTop: 8,
  },
  portStatus: {
    fontSize: 18,
    fontWeight: '700',
    marginTop: 4,
  },
  portSpeed: {
    fontSize: 13,
    color: '#9CA3AF',
    marginTop: 2,
  },
  statsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 8,
  },
  statItem: {
    width: '25%',
    alignItems: 'center',
    paddingVertical: 10,
  },
  statValue: {
    fontSize: 14,
    fontWeight: '700',
    marginTop: 4,
  },
  statLabel: {
    fontSize: 10,
    color: '#9CA3AF',
    marginTop: 2,
    textAlign: 'center',
  },
  connSummary: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#111827',
    borderRadius: 8,
    padding: 10,
    marginBottom: 6,
  },
  connText: {
    flex: 1,
    fontSize: 13,
    color: '#E5E7EB',
    marginLeft: 8,
  },
  connBytes: {
    fontSize: 11,
    color: '#6B7280',
  },
  moreText: {
    fontSize: 12,
    color: '#6B7280',
    textAlign: 'center',
    marginTop: 4,
  },
});
