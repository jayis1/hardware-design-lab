/**
 * MonitorScreen.js — Real-Time Performance & Thermal Monitor
 *
 * Live monitoring dashboard for the PhotonWeave CGH Engine.
 * Displays real-time performance metrics, temperature readings,
 * power consumption, and system health status with charts.
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  View, Text, StyleSheet, ScrollView, Dimensions, Animated,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { LineChart } from 'react-native-chart-kit';
import { THEME } from '../App';

const SCREEN_WIDTH = Dimensions.get('window').width;
const CHART_WIDTH = SCREEN_WIDTH - 48;
const CHART_HEIGHT = 180;
const HISTORY_LENGTH = 60; // 60 data points = 30 seconds at 500ms poll

//=============================================================================
// MonitorScreen Component
//=============================================================================
export default function MonitorScreen({ protocol, isConnected }) {
  // ── Live Metrics ──
  const [fps, setFps] = useState(0);
  const [frameTimeUs, setFrameTimeUs] = useState(0);
  const [frameCount, setFrameCount] = useState(0);
  const [cghBusy, setCghBusy] = useState(false);
  const [activeEngines, setActiveEngines] = useState(8);

  // ── Temperature ──
  const [temps, setTemps] = useState([0, 0, 0, 0]);
  const [maxTemp, setMaxTemp] = useState(0);

  // ── Power ──
  const [powerRails, setPowerRails] = useState([0, 0, 0, 0, 0, 0]);
  const [totalPower, setTotalPower] = useState(0);

  // ── DDR4 Performance ──
  const [ddr4ReadBw, setDdr4ReadBw] = useState(0);
  const [ddr4WriteBw, setDdr4WriteBw] = useState(0);

  // ── History for Charts ──
  const [fpsHistory, setFpsHistory] = useState(Array(HISTORY_LENGTH).fill(0));
  const [tempHistory, setTempHistory] = useState(Array(HISTORY_LENGTH).fill(0));
  const [powerHistory, setPowerHistory] = useState(Array(HISTORY_LENGTH).fill(0));

  // ── Polling ──
  const pollInterval = useRef(null);
  const historyIndex = useRef(0);

  // Start/stop polling based on connection
  useEffect(() => {
    if (isConnected) {
      pollInterval.current = setInterval(pollDevice, 500);
      // Initial poll
      pollDevice();
    }
    return () => {
      if (pollInterval.current) clearInterval(pollInterval.current);
    };
  }, [isConnected]);

  //===========================================================================
  // Poll Device
  //===========================================================================
  const pollDevice = useCallback(async () => {
    try {
      // Get full status
      const status = await protocol.getStatus();

      // FPS
      const currentFps = status.fpsX1000 / 1000.0;
      setFps(currentFps);
      setFrameTimeUs(status.frameTimeUs || 0);
      setFrameCount(status.frameCount);
      setCghBusy((status.cghStatus & 0x02) !== 0);

      // Temperatures
      const tempsC = status.temperatures.map(t => t / 1000.0);
      setTemps(tempsC);
      setMaxTemp(Math.max(...tempsC));

      // Power rails
      setPowerRails(status.powerRails);
      const totalMw = status.powerRails.reduce((sum, mv) => {
        // Rough power estimate: voltage × assumed current
        return sum + (mv * 0.5); // Simplified
      }, 0);
      setTotalPower(totalMw / 1000.0); // Watts

      // Update history
      updateHistory(currentFps, maxTemp, totalMw / 1000.0);

      // Get performance metrics
      try {
        const perf = await protocol.getPerformance();
        setDdr4ReadBw(perf.ddr4ReadBw);
        setDdr4WriteBw(perf.ddr4WriteBw);
        setActiveEngines(perf.activeFftEngines);
      } catch (perfError) {
        // Performance metrics may not always be available
      }
    } catch (error) {
      console.warn('Monitor poll error:', error.message);
    }
  }, [isConnected]);

  //===========================================================================
  // History Management
  //===========================================================================
  const updateHistory = useCallback((newFps, newTemp, newPower) => {
    setFpsHistory(prev => {
      const next = [...prev];
      next[historyIndex.current % HISTORY_LENGTH] = newFps;
      return next;
    });
    setTempHistory(prev => {
      const next = [...prev];
      next[historyIndex.current % HISTORY_LENGTH] = newTemp;
      return next;
    });
    setPowerHistory(prev => {
      const next = [...prev];
      next[historyIndex.current % HISTORY_LENGTH] = newPower;
      return next;
    });
    historyIndex.current++;
  }, []);

  //===========================================================================
  // Chart Configuration
  //===========================================================================
  const chartConfig = {
    backgroundColor: THEME.surface,
    backgroundGradientFrom: THEME.surface,
    backgroundGradientTo: THEME.surface,
    decimalCount: 1,
    color: (opacity = 1) => `rgba(0, 229, 255, ${opacity})`,
    labelColor: () => THEME.textSecondary,
    style: { borderRadius: 12 },
    propsForDots: { r: '2', strokeWidth: '1', stroke: THEME.primary },
    propsForBackgroundLines: {
      strokeDasharray: '4 4',
      stroke: THEME.border,
      strokeWidth: 0.5,
    },
  };

  const tempChartConfig = {
    ...chartConfig,
    color: (opacity = 1) => `rgba(255, 109, 0, ${opacity})`,
    propsForDots: { r: '2', strokeWidth: '1', stroke: THEME.accent },
  };

  const powerChartConfig = {
    ...chartConfig,
    color: (opacity = 1) => `rgba(124, 77, 255, ${opacity})`,
    propsForDots: { r: '2', strokeWidth: '1', stroke: THEME.secondary },
  };

  //===========================================================================
  // Temperature Color Helper
  //===========================================================================
  const tempColor = (temp) => {
    if (temp > 85) return THEME.error;
    if (temp > 70) return THEME.warning;
    if (temp > 50) return THEME.accent;
    return THEME.success;
  };

  //===========================================================================
  // Render
  //===========================================================================
  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* ── FPS Gauge ── */}
      <View style={styles.gaugeCard}>
        <Text style={styles.gaugeLabel}>FRAMES PER SECOND</Text>
        <Text style={[styles.gaugeValue, {
          color: fps >= 55 ? THEME.success : fps >= 30 ? THEME.warning : THEME.error,
        }]}>
          {fps.toFixed(1)}
        </Text>
        <Text style={styles.gaugeSub}>
          Target: 60.0 fps | Frame {frameCount.toLocaleString()}
        </Text>
        <View style={styles.gaugeBar}>
          <View style={[styles.gaugeFill, {
            width: `${Math.min(fps / 60.0 * 100, 100)}%`,
            backgroundColor: fps >= 55 ? THEME.success : fps >= 30 ? THEME.warning : THEME.error,
          }]} />
        </View>
      </View>

      {/* ── FPS Chart ── */}
      <Text style={styles.sectionTitle}>FPS History</Text>
      <LineChart
        data={{
          labels: Array(6).fill(''),
          datasets: [{ data: fpsHistory.slice(-60).filter((_, i) => i % 10 === 0) }],
        }}
        width={CHART_WIDTH}
        height={CHART_HEIGHT}
        chartConfig={chartConfig}
        bezier
        style={styles.chart}
        withInnerLines={true}
        withOuterLines={false}
        withVerticalLabels={false}
        withHorizontalLabels={true}
        fromZero={false}
        yAxisSuffix=" fps"
      />

      {/* ── Temperature Dashboard ── */}
      <Text style={styles.sectionTitle}>Thermal Monitor</Text>
      <View style={styles.tempGrid}>
        {[
          { label: 'FPGA Zone 1', value: temps[0], icon: 'chip' },
          { label: 'FPGA Zone 2', value: temps[1], icon: 'chip' },
          { label: 'DDR4', value: temps[2], icon: 'memory' },
          { label: 'PMIC', value: temps[3], icon: 'flash' },
        ].map((sensor, idx) => (
          <View key={idx} style={styles.tempCard}>
            <Icon name={sensor.icon} size={18} color={tempColor(sensor.value)} />
            <Text style={styles.tempLabel}>{sensor.label}</Text>
            <Text style={[styles.tempValue, { color: tempColor(sensor.value) }]}>
              {sensor.value.toFixed(1)}°C
            </Text>
          </View>
        ))}
      </View>

      {/* ── Max Temperature Chart ── */}
      <LineChart
        data={{
          labels: Array(6).fill(''),
          datasets: [{ data: tempHistory.slice(-60).filter((_, i) => i % 10 === 0) }],
        }}
        width={CHART_WIDTH}
        height={CHART_HEIGHT}
        chartConfig={tempChartConfig}
        bezier
        style={styles.chart}
        withInnerLines={true}
        withOuterLines={false}
        withVerticalLabels={false}
        withHorizontalLabels={true}
        fromZero={false}
        yAxisSuffix="°C"
      />

      {/* ── Power Monitor ── */}
      <Text style={styles.sectionTitle}>Power Consumption</Text>
      <View style={styles.powerCard}>
        <View style={styles.powerMain}>
          <Icon name="lightning-bolt" size={24} color={THEME.accent} />
          <Text style={styles.powerValue}>{totalPower.toFixed(1)} W</Text>
        </View>
        <View style={styles.powerGrid}>
          {[
            { label: 'VCCINT', value: powerRails[0], nominal: 1000 },
            { label: 'VCCAUX', value: powerRails[1], nominal: 1800 },
            { label: 'VDD_DDR', value: powerRails[2], nominal: 1200 },
            { label: 'MGTAVCC', value: powerRails[3], nominal: 1000 },
            { label: 'MGTAVTT', value: powerRails[4], nominal: 1200 },
            { label: 'VCCO', value: powerRails[5], nominal: 3300 },
          ].map((rail, idx) => {
            const deviation = rail.value > 0
              ? Math.abs(rail.value - rail.nominal) / rail.nominal * 100
              : 0;
            const railColor = deviation < 3 ? THEME.success :
                              deviation < 5 ? THEME.warning : THEME.error;
            return (
              <View key={idx} style={styles.railRow}>
                <Text style={styles.railLabel}>{rail.label}</Text>
                <Text style={[styles.railValue, { color: railColor }]}>
                  {rail.value > 0 ? `${(rail.value / 1000).toFixed(3)} V` : '—'}
                </Text>
              </View>
            );
          })}
        </View>
      </View>

      {/* ── Power Chart ── */}
      <LineChart
        data={{
          labels: Array(6).fill(''),
          datasets: [{ data: powerHistory.slice(-60).filter((_, i) => i % 10 === 0) }],
        }}
        width={CHART_WIDTH}
        height={CHART_HEIGHT}
        chartConfig={powerChartConfig}
        bezier
        style={styles.chart}
        withInnerLines={true}
        withOuterLines={false}
        withVerticalLabels={false}
        withHorizontalLabels={true}
        fromZero={false}
        yAxisSuffix=" W"
      />

      {/* ── DDR4 Bandwidth ── */}
      <Text style={styles.sectionTitle}>Memory Bandwidth</Text>
      <View style={styles.bwCard}>
        <View style={styles.bwItem}>
          <Icon name="arrow-down-bold" size={20} color={THEME.primary} />
          <View style={styles.bwTextCol}>
            <Text style={styles.bwLabel}>Read</Text>
            <Text style={styles.bwValue}>{ddr4ReadBw.toFixed(1)} MB/s</Text>
          </View>
        </View>
        <View style={styles.bwItem}>
          <Icon name="arrow-up-bold" size={20} color={THEME.secondary} />
          <View style={styles.bwTextCol}>
            <Text style={styles.bwLabel}>Write</Text>
            <Text style={styles.bwValue}>{ddr4WriteBw.toFixed(1)} MB/s</Text>
          </View>
        </View>
        <View style={styles.bwItem}>
          <Icon name="engine" size={20} color={THEME.accent} />
          <View style={styles.bwTextCol}>
            <Text style={styles.bwLabel}>FFT Engines</Text>
            <Text style={styles.bwValue}>{activeEngines}/8 active</Text>
          </View>
        </View>
      </View>

      {/* ── System Health ── */}
      <Text style={styles.sectionTitle}>System Health</Text>
      <View style={styles.healthCard}>
        <HealthIndicator
          label="FPGA Ready"
          ok={isConnected}
          icon="check-circle"
        />
        <HealthIndicator
          label="DDR4 Calibrated"
          ok={isConnected}
          icon="memory"
        />
        <HealthIndicator
          label="PCIe Link"
          ok={isConnected}
          icon="lan-connect"
        />
        <HealthIndicator
          label="Clock Lock"
          ok={isConnected}
          icon="clock-outline"
        />
        <HealthIndicator
          label="Thermal OK"
          ok={maxTemp < 80}
          icon="thermometer"
        />
        <HealthIndicator
          label="Power OK"
          ok={totalPower < 75}
          icon="flash"
        />
      </View>

      {/* ── Connection Warning ── */}
      {!isConnected && (
        <View style={styles.warningBanner}>
          <Icon name="alert-circle" size={20} color={THEME.warning} />
          <Text style={styles.warningText}>
            Not connected. Monitoring data unavailable.
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

//=============================================================================
// Health Indicator Sub-Component
//=============================================================================
function HealthIndicator({ label, ok, icon }) {
  return (
    <View style={healthStyles.row}>
      <Icon
        name={ok ? icon : 'alert-circle'}
        size={18}
        color={ok ? THEME.success : THEME.error}
      />
      <Text style={healthStyles.label}>{label}</Text>
      <Text style={[healthStyles.status, { color: ok ? THEME.success : THEME.error }]}>
        {ok ? 'OK' : 'FAIL'}
      </Text>
    </View>
  );
}

const healthStyles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 8,
    paddingHorizontal: 4,
  },
  label: {
    color: THEME.text,
    fontSize: 14,
    fontWeight: '500',
    flex: 1,
    marginLeft: 10,
  },
  status: {
    fontSize: 13,
    fontWeight: '700',
  },
});

//=============================================================================
// Styles
//=============================================================================
const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.background,
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  gaugeCard: {
    backgroundColor: THEME.surface,
    borderRadius: 16,
    padding: 20,
    alignItems: 'center',
    marginBottom: 20,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  gaugeLabel: {
    color: THEME.textSecondary,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 2,
    marginBottom: 8,
  },
  gaugeValue: {
    fontSize: 56,
    fontWeight: '800',
    fontVariant: ['tabular-nums'],
  },
  gaugeSub: {
    color: THEME.textSecondary,
    fontSize: 13,
    marginTop: 4,
    marginBottom: 12,
  },
  gaugeBar: {
    width: '100%',
    height: 6,
    backgroundColor: THEME.border,
    borderRadius: 3,
    overflow: 'hidden',
  },
  gaugeFill: {
    height: '100%',
    borderRadius: 3,
  },
  sectionTitle: {
    color: THEME.primary,
    fontSize: 14,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 20,
    marginBottom: 12,
  },
  chart: {
    borderRadius: 12,
    marginBottom: 8,
  },
  tempGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  tempCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 14,
    width: '48%',
    alignItems: 'center',
    marginBottom: 8,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  tempLabel: {
    color: THEME.textSecondary,
    fontSize: 11,
    fontWeight: '600',
    marginTop: 6,
    marginBottom: 4,
  },
  tempValue: {
    fontSize: 22,
    fontWeight: '700',
    fontVariant: ['tabular-nums'],
  },
  powerCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  powerMain: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 16,
  },
  powerValue: {
    color: THEME.accent,
    fontSize: 32,
    fontWeight: '800',
    marginLeft: 10,
    fontVariant: ['tabular-nums'],
  },
  powerGrid: {
    borderTopWidth: 1,
    borderTopColor: THEME.border,
    paddingTop: 12,
  },
  railRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  railLabel: {
    color: THEME.textSecondary,
    fontSize: 13,
    fontWeight: '500',
  },
  railValue: {
    fontSize: 13,
    fontWeight: '600',
    fontVariant: ['tabular-nums'],
  },
  bwCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  bwItem: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  bwTextCol: {
    marginLeft: 8,
  },
  bwLabel: {
    color: THEME.textSecondary,
    fontSize: 11,
    fontWeight: '600',
  },
  bwValue: {
    color: THEME.text,
    fontSize: 14,
    fontWeight: '700',
    fontVariant: ['tabular-nums'],
  },
  healthCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  warningBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: THEME.warning + '15',
    borderRadius: 8,
    padding: 12,
    marginTop: 8,
    borderWidth: 1,
    borderColor: THEME.warning + '40',
  },
  warningText: {
    color: THEME.warning,
    fontSize: 13,
    fontWeight: '500',
    marginLeft: 8,
    flex: 1,
  },
});
