// screens/LiveMonitorScreen.js — real-time analyte concentrations + heatmap
// Author: jayis1  License: MIT
import React, { useState, useEffect, useRef } from 'react';
import { View, Text, Button, ScrollView, StyleSheet } from 'react-native';
import { theme, ANALYTES } from '../utils/theme';
import ble from '../utils/ble';
import { tempC, depthM, turbNtu, batteryPct, hasOverrange, hasBubble } from '../utils/protocol';
import AnalyteGauge from '../components/AnalyteGauge';
import FluorescenceHeatmap from '../components/FluorescenceHeatmap';

export default function LiveMonitorScreen() {
  const [latest, setLatest] = useState(null);
  const [history, setHistory] = useState([]);   // last 60 samples
  const [surveying, setSurveying] = useState(false);
  const [status, setStatus] = useState(null);
  const subRef = useRef(null);
  const stRef  = useRef(null);

  useEffect(() => {
    subRef.current = ble.subscribeSamples(rec => {
      setLatest(rec);
      setHistory(h => [...h.slice(-59), rec]);
    });
    stRef.current = ble.subscribeStatus(s => setStatus(s));
    return () => {
      if (subRef.current) subRef.current();
      if (stRef.current) stRef.current();
    };
  }, []);

  const start = async () => { try { await ble.startSurvey(); setSurveying(true); } catch {} };
  const stop  = async () => { try { await ble.stopSurvey();  setSurveying(false); } catch {} };

  // Build a 6×4 heatmap proxy from the latest record (using derived analytes
  // as a rough visualization — the real 24-vector arrives over a separate
  // raw characteristic in a full implementation).
  const heat = latest ? buildHeatProxy(latest) : zeroMatrix();

  return (
    <ScrollView style={styles.container} contentContainerStyle={{ paddingBottom: 40 }}>
      <View style={styles.header}>
        <Text style={styles.title}>Live Monitor</Text>
        <View style={styles.controls}>
          <Button title={surveying ? 'Stop' : 'Start'} onPress={surveying ? stop : start} color={surveying ? theme.colors.alarm : theme.colors.good} />
        </View>
      </View>

      {latest ? (
        <>
          <View style={styles.envRow}>
            <EnvChip label="Depth"  value={`${(depthM(latest)).toFixed(2)} m`} />
            <EnvChip label="Temp"   value={`${tempC(latest).toFixed(2)} °C`} />
            <EnvChip label="Battery" value={`${batteryPct(latest).toFixed(0)} %`} />
            <EnvChip label="Seq"    value={`#${latest.seq}`} />
          </View>

          {(hasOverrange(latest) || hasBubble(latest)) && (
            <View style={styles.warnBox}>
              {hasOverrange(latest) && <Text style={styles.warn}>⚠ Overrange on one or more channels</Text>}
              {hasBubble(latest)    && <Text style={styles.warn}>⚠ Bubble detected — check flow cell</Text>}
            </View>
          )}

          {ANALYTES.map(a => (
            <AnalyteGauge
              key={a.key}
              analyteKey={a.key}
              value={valueFor(a.key, latest)}
              unitOverride={a.key === 'turb' ? 'NTU' : null}
            />
          ))}

          <FluorescenceHeatmap data={heat} />

          <Text style={styles.sectionTitle}>Recent samples ({history.length})</Text>
          {history.slice(-8).reverse().map(r => (
            <View key={r.seq} style={styles.histRow}>
              <Text style={styles.histSeq}>#{r.seq}</Text>
              <Text style={styles.histVals}>
                CDOM {r.cdomPpb}  Chl {r.chlaUgl}  Phyc {r.phycUgl}  Oil {r.oilPpb}  NTU {turbNtu(r).toFixed(2)}
              </Text>
            </View>
          ))}
        </>
      ) : (
        <Text style={styles.empty}>Waiting for first sample…</Text>
      )}
    </ScrollView>
  );
}

function valueFor(key, r) {
  switch (key) {
    case 'cdom': return r.cdomPpb;
    case 'chla': return r.chlaUgl;
    case 'phyc': return r.phycUgl;
    case 'oil':  return r.oilPpb;
    case 'turb': return turbNtu(r);
    default: return 0;
  }
}

function buildHeatProxy(r) {
  // Map analyte values into the 6×4 grid at their dominant (ex,det) cell.
  const m = zeroMatrix();
  m[0][0] = Math.min(1, r.oilPpb  / 50);
  m[2][1] = Math.min(1, r.cdomPpb / 500);
  m[3][3] = Math.min(1, r.chlaUgl / 100);
  m[4][2] = Math.min(1, r.phycUgl / 50);
  m[5][0] = Math.min(1, turbNtu(r) / 500);
  return m;
}
function zeroMatrix() { return Array.from({length:6}, () => [0,0,0,0]); }

function EnvChip({ label, value }) {
  return (
    <View style={styles.envChip}>
      <Text style={styles.envLabel}>{label}</Text>
      <Text style={styles.envValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: theme.colors.bg, padding: theme.spacing.md },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: theme.spacing.sm },
  title: { color: theme.colors.text, fontSize: 22, fontWeight: '700' },
  controls: { flexDirection: 'row', gap: 6 },
  envRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 8, marginBottom: theme.spacing.sm },
  envChip: { backgroundColor: theme.colors.surface, borderRadius: theme.radius.md, paddingVertical: 6, paddingHorizontal: 10 },
  envLabel: { color: theme.colors.textDim, fontSize: 10, textTransform: 'uppercase' },
  envValue: { color: theme.colors.text, fontSize: 14, fontFamily: theme.fonts.mono, fontWeight: '600' },
  warnBox: { backgroundColor: 'rgba(229,57,53,0.15)', borderRadius: theme.radius.md, padding: 8, marginVertical: 6 },
  warn: { color: theme.colors.alarm, fontSize: 12 },
  sectionTitle: { color: theme.colors.textDim, fontSize: 13, marginTop: theme.spacing.md, marginBottom: 4, textTransform: 'uppercase' },
  histRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 3 },
  histSeq: { color: theme.colors.accent, fontFamily: theme.fonts.mono, width: 50, fontSize: 12 },
  histVals: { color: theme.colors.text, fontFamily: theme.fonts.mono, fontSize: 11, flex: 1 },
  empty: { color: theme.colors.textDim, textAlign: 'center', marginTop: 60 },
});