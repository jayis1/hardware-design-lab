/*
 * src/screens/DewarDetail.tsx — Per-Dewar live charts and status.
 *
 * Author: jayis1
 * License: MIT
 */
import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Card, IconButton, SegmentedButtons, Text as PaperText } from 'react-native-paper';
import { LineChart } from 'react-native-chart-kit';
import { useRoute, useNavigation } from '@react-navigation/native';
import { DewarReading, STATE_COLOR, RootStackParamList } from '../types';
import { getDewarHistory } from '../cloud/Api';

const RANGES = [
  { label: '24h', value: '24', hours: 24 },
  { label: '7d',  value: '7d', hours: 168 },
  { label: '30d', value: '30d', hours: 720 },
];

export default function DewarDetail() {
  const route = useRoute<any>();
  const nav = useNavigation<any>();
  const dewarId = route.params.dewarId;
  const [range, setRange] = useState('24');
  const [hist, setHist] = useState<DewarReading[]>([]);

  useEffect(() => {
    const r = RANGES.find((x) => x.value === range)!;
    getDewarHistory(dewarId, r.hours).then(setHist).catch(console.warn);
  }, [dewarId, range]);

  const latest = hist[hist.length - 1];
  const color = latest ? STATE_COLOR[latest.state] : '#94A3B8';

  const labels = hist.filter((_, i) => i % Math.max(1, Math.floor(hist.length / 6)) === 0)
    .map((h) => new Date(h.lastHeartbeatEpoch * 1000).toLocaleTimeString([], { hour: '2-digit' }));
  const levelData = hist.map((h) => h.levelPct);
  const rateData  = hist.map((h) => h.levelRatePerHour);
  const gradData  = hist.map((h) => h.gradientSlope);
  const ambData   = hist.map((h) => h.ambientC);

  return (
    <ScrollView style={s.wrap}>
      {/* Status banner */}
      <View style={[s.banner, { backgroundColor: color }]}>
        <Text style={s.bannerState}>{latest?.state ?? '—'}</Text>
        <Text style={s.bannerReason}>{latest?.reason?.replace(/_/g, ' ') ?? ''}</Text>
      </View>

      {/* Range selector */}
      <SegmentedButtons
        value={range}
        onValueChange={setRange}
        buttons={RANGES.map((r) => ({ value: r.value, label: r.label }))}
        style={s.range}
      />

      {/* Live numeric grid */}
      {latest && (
        <View style={s.grid}>
          <Stat label="LN2 Level"   value={`${latest.levelPct.toFixed(1)}%`} />
          <Stat label="Boil-off"    value={`${latest.levelRatePerHour.toFixed(2)}%/h`} />
          <Stat label="Top RTD"     value={`${latest.rtdTopC.toFixed(0)}°C`} />
          <Stat label="Mid RTD"     value={`${latest.rtdMidC.toFixed(0)}°C`} />
          <Stat label="Bottom RTD"  value={`${latest.rtdBotC.toFixed(0)}°C`} />
          <Stat label="Gradient"    value={`${latest.gradientSlope.toFixed(0)}°C`} />
          <Stat label="Tilt"        value={`${latest.tiltDeg.toFixed(1)}°`} />
          <Stat label="Vibration"   value={`${latest.vibRmsG.toFixed(3)}g`} />
          <Stat label="Ambient"     value={`${latest.ambientC.toFixed(1)}°C`} />
          <Stat label="Humidity"    value={`${latest.ambientRh.toFixed(0)}%`} />
          <Stat label="Battery"     value={`${latest.battPct.toFixed(0)}%`} />
          <Stat label="Mains"       value={latest.mainsPresent ? 'OK' : 'LOST'}
                warn={!latest.mainsPresent} />
        </View>
      )}

      {/* Charts */}
      <ChartCard title="LN2 Level (%)">
        {levelData.length > 1 && (
          <LineChart data={{ labels, datasets: [{ data: levelData }] }}
            width={340} height={180} chartConfig={chartConfig}
            bezier style={s.chart} />
        )}
      </ChartCard>
      <ChartCard title="Boil-off Rate (%/h)">
        {rateData.length > 1 && (
          <LineChart data={{ labels, datasets: [{ data: rateData }] }}
            width={340} height={160} chartConfig={chartConfig} bezier style={s.chart} />
        )}
      </ChartCard>
      <ChartCard title="Vapor-Column Gradient (°C)">
        {gradData.length > 1 && (
          <LineChart data={{ labels, datasets: [{ data: gradData }] }}
            width={340} height={160} chartConfig={chartConfig} bezier style={s.chart} />
        )}
      </ChartCard>
      <ChartCard title="Ambient Temperature (°C)">
        {ambData.length > 1 && (
          <LineChart data={{ labels, datasets: [{ data: ambData }] }}
            width={340} height={160} chartConfig={chartConfig} bezier style={s.chart} />
        )}
      </ChartCard>

      {/* Action buttons */}
      <View style={s.actionRow}>
        <TouchableOpacity style={s.actionBtn} onPress={() =>
          nav.navigate('AuditLog', { dewarId })}>
          <IconButton icon="file-document-outline" color="#F1F5F9" />
          <PaperText style={s.actionText}>Audit Log</PaperText>
        </TouchableOpacity>
        <TouchableOpacity style={s.actionBtn} onPress={() =>
          nav.navigate('EscalationChain', { dewarId })}>
          <IconButton icon="account-group-outline" color="#F1F5F9" />
          <PaperText style={s.actionText}>Escalation</PaperText>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

function Stat({ label, value, warn }: { label: string; value: string; warn?: boolean }) {
  return (
    <View style={s.stat}>
      <Text style={s.statLabel}>{label}</Text>
      <Text style={[s.statValue, warn && s.statWarn]}>{value}</Text>
    </View>
  );
}

function ChartCard({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <Card style={s.card}>
      <Card.Title title={title} titleStyle={s.cardTitle} />
      <Card.Content>{children}</Card.Content>
    </Card>
  );
}

const chartConfig = {
  backgroundGradientFrom: '#1E293B',
  backgroundGradientTo:   '#1E293B',
  color: (opacity = 1) => `rgba(0, 170, 204, ${opacity})`,
  labelColor: (opacity = 1) => `rgba(148, 163, 184, ${opacity})`,
  strokeWidth: 2,
  decimalPlaces: 1,
};

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0F172A', padding: 12 },
  banner: { borderRadius: 8, padding: 16, marginBottom: 12, alignItems: 'center' },
  bannerState: { color: '#fff', fontSize: 24, fontWeight: 'bold', letterSpacing: 2 },
  bannerReason: { color: '#fff', fontSize: 14, marginTop: 4 },
  range: { marginBottom: 12 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between', marginBottom: 12 },
  stat: { width: '48%', backgroundColor: '#1E293B', padding: 10, borderRadius: 6, marginBottom: 6 },
  statLabel: { color: '#94A3B8', fontSize: 11, textTransform: 'uppercase' },
  statValue: { color: '#F1F5F9', fontSize: 18, fontWeight: '600' },
  statWarn: { color: '#DC2626' },
  card: { backgroundColor: '#1E293B', marginBottom: 12, borderRadius: 8 },
  cardTitle: { color: '#F1F5F9', fontSize: 14 },
  chart: { borderRadius: 8 },
  actionRow: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 30 },
  actionBtn: { alignItems: 'center' },
  actionText: { color: '#F1F5F9', fontSize: 12 },
});