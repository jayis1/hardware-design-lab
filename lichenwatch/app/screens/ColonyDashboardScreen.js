/*
 * ColonyDashboardScreen.js — Live vital signs of the connected lichen colony.
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect, useContext } from 'react';
import {
  View, Text, StyleSheet, ScrollView, RefreshControl,
} from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import ConnectionStatus from '../components/ConnectionStatus';
import VitalCard from '../components/VitalCard';
import { BleContext, readNodeStatus, loadRecords } from '../utils/ble';
import {
  STATE_NAMES, STATE_DESCRIPTIONS, STATE_COLORS,
  interpretYield, interpretLndvi, interpretWetness,
  interpretClicks, interpretBattery, formatUptime,
} from '../utils/lichen';

export default function ColonyDashboardScreen() {
  const { bleState, setBleState } = useContext(BleContext);
  const [status, setStatus] = useState(null);
  const [history, setHistory] = useState([]);
  const [refreshing, setRefreshing] = useState(false);

  const refresh = async () => {
    setRefreshing(true);
    if (bleState.connectedNodeId) {
      const s = await readNodeStatus(bleState.connectedNodeId);
      setStatus(s);
      const recs = await loadRecords(bleState.connectedNodeId);
      setHistory(recs.slice(-48));  /* last 48 records (12 h at 15 min) */
    }
    setRefreshing(false);
  };

  useEffect(() => {
    refresh();
  }, [bleState.connectedNodeId]);

  const yieldInterp = status ? interpretYield(status.fv_fm) : null;
  const lndviInterp = status ? interpretLndvi(status.lndvi) : null;
  const wetInterp = status ? interpretWetness(status.wetness) : null;
  const battInterp = status ? interpretBattery(status.battery_mv) : null;

  const chartData = {
    labels: history.map((_, i) => (i % 8 === 0 ? `${i}` : '')),
    datasets: [
      {
        data: history.map((r) => r.fv_fm),
        color: (opacity = 1) => `rgba(46,125,50,${opacity})`,
        strokeWidth: 2,
      },
      {
        data: history.map((r) => r.lndvi),
        color: (opacity = 1) => `rgba(21,101,192,${opacity})`,
        strokeWidth: 2,
      },
    ],
    legend: ['Fv/Fm', 'LNDVI'],
  };

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={refresh} />
      }
    >
      <ConnectionStatus />

      {!status ? (
        <View style={styles.empty}>
          <Text style={styles.emptyText}>
            No node connected. Go to the Sync tab to scan for a nearby
            Lichenwatch node and connect.
          </Text>
        </View>
      ) : (
        <>
          {/* Colony state banner */}
          <View
            style={[
              styles.stateBanner,
              { backgroundColor: STATE_COLORS[status.class_state] || '#666' },
            ]}
          >
            <Text style={styles.stateLabel}>COLONY STATE</Text>
            <Text style={styles.stateValue}>
              {STATE_NAMES[status.class_state] || 'Unknown'}
            </Text>
            <Text style={styles.stateConf}>
              Confidence: {status.confidence}%
            </Text>
          </View>

          <Text style={styles.sectionTitle}>Vital Signs</Text>
          <View style={styles.vitalRow}>
            <VitalCard
              label="PSII Yield"
              value={status.fv_fm.toFixed(3)}
              color={yieldInterp?.color}
              sublabel={yieldInterp?.label}
            />
            <VitalCard
              label="LNDVI"
              value={status.lndvi.toFixed(3)}
              color={lndviInterp?.color}
              sublabel={lndviInterp?.label}
            />
          </View>
          <View style={styles.vitalRow}>
            <VitalCard
              label="Chl Index"
              value={status.chl_index.toFixed(2)}
              sublabel="R560 / R670"
            />
            <VitalCard
              label="Wetness"
              value={(status.wetness * 100).toFixed(0)}
              unit="%"
              color={wetInterp?.color}
              sublabel={wetInterp?.label}
            />
          </View>
          <View style={styles.vitalRow}>
            <VitalCard
              label="Battery"
              value={(status.battery_mv / 1000).toFixed(2)}
              unit="V"
              color={battInterp?.color}
              sublabel={`${battInterp?.label} (~${battInterp?.pct}%)`}
            />
            <VitalCard
              label="Uptime"
              value={formatUptime(status.uptime_s)}
              sublabel={`Seq #${status.seq}`}
            />
          </View>

          <Text style={styles.sectionTitle}>12-Hour Trend</Text>
          {history.length > 1 ? (
            <LineChart
              data={chartData}
              width={340}
              height={180}
              chartConfig={{
                backgroundGradientFrom: '#f5f5f5',
                backgroundGradientTo: '#f5f5f5',
                color: (opacity = 1) => `rgba(27,58,27,${opacity})`,
                labelColor: (opacity = 1) => `rgba(0,0,0,${opacity})`,
              }}
              bezier
              style={styles.chart}
            />
          ) : (
            <Text style={styles.emptyText}>
              No trend data yet. Perform a walk-by sync to download history.
            </Text>
          )}

          <Text style={styles.sectionTitle}>Interpretation</Text>
          <Text style={styles.description}>
            {STATE_DESCRIPTIONS[status.class_state] || ''}
          </Text>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#fafafa' },
  empty: { padding: 30, alignItems: 'center' },
  emptyText: { color: '#666', textAlign: 'center', fontSize: 14 },
  stateBanner: {
    padding: 16,
    margin: 8,
    borderRadius: 10,
    alignItems: 'center',
  },
  stateLabel: { color: '#fff', fontSize: 12, letterSpacing: 1 },
  stateValue: { color: '#fff', fontSize: 28, fontWeight: 'bold' },
  stateConf: { color: '#fff', fontSize: 12, marginTop: 4 },
  sectionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#1b3a1b',
    margin: 12,
    marginTop: 16,
  },
  vitalRow: { flexDirection: 'row', justifyContent: 'space-between' },
  chart: { margin: 8, borderRadius: 8 },
  description: { padding: 12, color: '#333', fontSize: 13, lineHeight: 18 },
});