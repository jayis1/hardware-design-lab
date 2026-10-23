// QuadraAlign companion application
// Author: jayis1
import React, {useEffect, useMemo, useState} from 'react';
import {SafeAreaView, ScrollView, StatusBar, StyleSheet, Text, TouchableOpacity, View} from 'react-native';
import {Capture, commands, demoMeasurement, FootIndex, Measurement, Project} from './src/model';

const palette = {bg: '#09131c', panel: '#122430', ink: '#edf7f6', muted: '#8ca7ad', cyan: '#48d7ce',
  green: '#72e09d', amber: '#ffc857', red: '#ff6b6b', line: '#24424e'};
const feetNames = ['front-left', 'front-right', 'rear-left', 'rear-right'];
const project: Project = {name: 'Laundry press #4', widthMm: 600, depthMm: 500, screwPitchMm: 1.5, imbalanceLimit: 12, tiltLimit: 0.35};

type Tab = 'Live' | 'Guide' | 'Captures';
const number = (value: number, places = 1) => Number.isFinite(value) ? value.toFixed(places) : '—';

const Header = ({connected, battery}: {connected: boolean; battery: number}) => (
  <View style={styles.header}>
    <View><Text style={styles.brand}>QUADRAALIGN</Text><Text style={styles.subtitle}>machine-foot balancing console</Text></View>
    <View style={[styles.connection, connected && styles.connectionOn]}>
      <Text style={styles.connectionText}>{connected ? `LIVE · ${Math.round((battery - 3300) / 9)}%` : 'DEMO'}</Text>
    </View>
  </View>
);

const FootCard = ({index, value, recommended}: {index: FootIndex; value: number; recommended: boolean}) => (
  <View style={[styles.foot, recommended && styles.footRecommended]}>
    <Text style={styles.footLabel}>{feetNames[index].toUpperCase()}</Text>
    <Text style={styles.footValue}>{number(value)}<Text style={styles.unit}> kg</Text></Text>
    {recommended && <Text style={styles.adjust}>ADJUST THIS FOOT</Text>}
  </View>
);

const Bubble = ({measurement}: {measurement: Measurement}) => {
  const x = Math.max(-42, Math.min(42, measurement.rollDeg * 80));
  const y = Math.max(-42, Math.min(42, measurement.pitchDeg * 80));
  return <View style={styles.bubbleBox}>
    <View style={styles.crossHorizontal}/><View style={styles.crossVertical}/>
    <View style={[styles.bubble, {transform: [{translateX: x}, {translateY: y}]}]}/>
    <Text style={styles.axisTop}>FRONT</Text>
  </View>;
};

const Metric = ({label, value, warning}: {label: string; value: string; warning?: boolean}) => (
  <View style={styles.metric}><Text style={styles.metricLabel}>{label}</Text><Text style={[styles.metricValue, warning && styles.warning]}>{value}</Text></View>
);

const LiveScreen = ({measurement, onTare, onCapture}: {measurement: Measurement; onTare: () => void; onCapture: () => void}) => {
  const aligned = Math.abs(measurement.pitchDeg) < project.tiltLimit && Math.abs(measurement.rollDeg) < project.tiltLimit && measurement.imbalancePercent < project.imbalanceLimit;
  return <>
    <View style={[styles.verdict, aligned ? styles.goodBorder : styles.warnBorder]}>
      <Text style={styles.verdictEyebrow}>{measurement.stable ? 'STABLE READING' : 'WAITING FOR STABILITY'}</Text>
      <Text style={[styles.verdictText, {color: aligned ? palette.green : palette.amber}]}>{aligned ? 'Machine is balanced' : 'Adjustment required'}</Text>
      <Text style={styles.verdictDetail}>{number(measurement.totalKg)} kg total supported load</Text>
    </View>
    <View style={styles.footGrid}>{measurement.feet.map((value, index) => <FootCard key={index} index={index as FootIndex} value={value} recommended={index === measurement.recommendedFoot}/>)}</View>
    <View style={styles.panelRow}>
      <Bubble measurement={measurement}/>
      <View style={styles.metrics}>
        <Metric label="PITCH" value={`${number(measurement.pitchDeg, 2)}°`}/>
        <Metric label="ROLL" value={`${number(measurement.rollDeg, 2)}°`}/>
        <Metric label="LOAD IMBALANCE" value={`${number(measurement.imbalancePercent)}%`} warning={measurement.imbalancePercent >= project.imbalanceLimit}/>
        <Metric label="VIBRATION RMS" value={`${number(measurement.vibrationRmsG, 3)} g`}/>
      </View>
    </View>
    <View style={styles.actions}>
      <TouchableOpacity style={styles.secondaryButton} onPress={onTare}><Text style={styles.secondaryText}>Tare pucks</Text></TouchableOpacity>
      <TouchableOpacity style={styles.primaryButton} onPress={onCapture}><Text style={styles.primaryText}>Save capture</Text></TouchableOpacity>
    </View>
  </>;
};

const GuideScreen = ({measurement}: {measurement: Measurement}) => {
  const turns = measurement.recommendedTurns;
  const foot = feetNames[measurement.recommendedFoot];
  return <>
    <View style={styles.guideHero}><Text style={styles.step}>NEXT ADJUSTMENT</Text><Text style={styles.guideTitle}>Raise {foot}</Text>
      <Text style={styles.turns}>{number(turns, 2)} <Text style={styles.turnUnit}>turns clockwise</Text></Text>
      <Text style={styles.guideCopy}>This estimate combines the measured tilt, {project.widthMm} × {project.depthMm} mm footprint, and {project.screwPitchMm} mm leveling-screw pitch. Stop when the live indicator enters the center ring.</Text>
    </View>
    <View style={styles.panel}><Text style={styles.panelTitle}>SAFE PROCEDURE</Text>
      {['Lock out moving equipment before touching a foot.', 'Loosen the selected jam nut while supporting the frame.', `Raise the ${foot} foot gradually; do not exceed one turn per pass.`, 'Tighten the jam nut, let the machine settle, then capture again.'].map((text, i) =>
        <View style={styles.procedure} key={text}><Text style={styles.procedureNumber}>{i + 1}</Text><Text style={styles.procedureText}>{text}</Text></View>)}
    </View>
    <View style={styles.panel}><Text style={styles.panelTitle}>CENTER OF LOAD</Text><Text style={styles.largeMetric}>{number(measurement.centerXmm)} mm X · {number(measurement.centerYmm)} mm Y</Text>
      <Text style={styles.guideCopy}>A persistent off-center result after leveling may indicate an unsupported frame, soft foot, displaced load, or structural distortion.</Text></View>
  </>;
};

const CapturesScreen = ({captures}: {captures: Capture[]}) => <>
  <Text style={styles.sectionTitle}>{project.name}</Text><Text style={styles.guideCopy}>Commissioning evidence is stored locally with stability and overload flags.</Text>
  {captures.length === 0 ? <View style={styles.empty}><Text style={styles.emptyTitle}>No captures yet</Text><Text style={styles.guideCopy}>Return to Live and save a stable reading.</Text></View> : captures.map(c =>
    <View style={styles.capture} key={c.id}><View><Text style={styles.captureTitle}>{c.label}</Text><Text style={styles.captureTime}>{new Date(c.receivedAt).toLocaleString()}</Text></View>
      <View><Text style={styles.captureScore}>{number(c.imbalancePercent)}%</Text><Text style={styles.captureStatus}>{c.stable ? 'STABLE' : 'MOVING'}</Text></View></View>)}
</>;

export default function App() {
  const [tab, setTab] = useState<Tab>('Live');
  const [phase, setPhase] = useState(0);
  const [measurement, setMeasurement] = useState(() => demoMeasurement(0));
  const [captures, setCaptures] = useState<Capture[]>([]);
  const [notice, setNotice] = useState('');
  useEffect(() => { const timer = setInterval(() => { setPhase(p => p + 0.08); }, 500); return () => clearInterval(timer); }, []);
  useEffect(() => setMeasurement(demoMeasurement(phase)), [phase]);
  const tabs = useMemo<Tab[]>(() => ['Live', 'Guide', 'Captures'], []);
  const send = (frame: Uint8Array, message: string) => { void frame; setNotice(message); setTimeout(() => setNotice(''), 1600); };
  const capture = () => { setCaptures(old => [{...measurement, id: `${measurement.sequence}-${Date.now()}`, label: `Balance run ${old.length + 1}`}, ...old]); send(commands.capture(), 'Capture saved'); };
  return <SafeAreaView style={styles.safe}><StatusBar barStyle="light-content"/><ScrollView contentContainerStyle={styles.content}>
    <Header connected={false} battery={measurement.batteryMv}/>
    <View style={styles.tabs}>{tabs.map(item => <TouchableOpacity key={item} style={[styles.tab, tab === item && styles.activeTab]} onPress={() => setTab(item)}><Text style={[styles.tabText, tab === item && styles.activeTabText]}>{item}</Text></TouchableOpacity>)}</View>
    {notice !== '' && <Text style={styles.notice}>{notice}</Text>}
    {tab === 'Live' && <LiveScreen measurement={measurement} onTare={() => send(commands.tare(), 'Tare command sent')} onCapture={capture}/>} 
    {tab === 'Guide' && <GuideScreen measurement={measurement}/>} 
    {tab === 'Captures' && <CapturesScreen captures={captures}/>} 
    <Text style={styles.footer}>Designed and authored by jayis1 · {project.name}</Text>
  </ScrollView></SafeAreaView>;
}

const styles = StyleSheet.create({
  safe: {flex: 1, backgroundColor: palette.bg}, content: {padding: 18, paddingBottom: 50}, header: {flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 22},
  brand: {color: palette.cyan, fontWeight: '900', fontSize: 20, letterSpacing: 2}, subtitle: {color: palette.muted, fontSize: 11, marginTop: 2},
  connection: {backgroundColor: '#24313a', borderRadius: 20, paddingHorizontal: 11, paddingVertical: 7}, connectionOn: {backgroundColor: '#164831'}, connectionText: {color: palette.ink, fontWeight: '800', fontSize: 10},
  tabs: {flexDirection: 'row', borderBottomWidth: 1, borderColor: palette.line, marginBottom: 16}, tab: {paddingVertical: 11, paddingHorizontal: 18}, activeTab: {borderBottomWidth: 3, borderColor: palette.cyan}, tabText: {color: palette.muted, fontWeight: '700'}, activeTabText: {color: palette.ink}, notice: {backgroundColor: palette.cyan, color: palette.bg, textAlign: 'center', padding: 9, borderRadius: 7, marginBottom: 12, fontWeight: '800'},
  verdict: {backgroundColor: palette.panel, padding: 18, borderRadius: 14, borderLeftWidth: 5, marginBottom: 12}, goodBorder: {borderColor: palette.green}, warnBorder: {borderColor: palette.amber}, verdictEyebrow: {color: palette.muted, fontSize: 10, fontWeight: '900', letterSpacing: 1}, verdictText: {fontSize: 25, fontWeight: '900', marginTop: 4}, verdictDetail: {color: palette.ink, marginTop: 5},
  footGrid: {flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between'}, foot: {width: '49%', minHeight: 98, backgroundColor: palette.panel, borderWidth: 1, borderColor: palette.line, borderRadius: 12, padding: 12, marginBottom: 8}, footRecommended: {borderColor: palette.amber, borderWidth: 2}, footLabel: {color: palette.muted, fontSize: 9, fontWeight: '800'}, footValue: {color: palette.ink, fontSize: 24, fontWeight: '800', marginTop: 6}, unit: {fontSize: 11, color: palette.muted}, adjust: {color: palette.amber, fontSize: 8, marginTop: 6, fontWeight: '900'},
  panelRow: {flexDirection: 'row', backgroundColor: palette.panel, borderRadius: 14, marginTop: 4, padding: 14}, bubbleBox: {width: 132, height: 132, borderWidth: 2, borderColor: palette.line, borderRadius: 66, alignItems: 'center', justifyContent: 'center', overflow: 'hidden'}, crossHorizontal: {position: 'absolute', width: 120, height: 1, backgroundColor: palette.line}, crossVertical: {position: 'absolute', width: 1, height: 120, backgroundColor: palette.line}, bubble: {width: 24, height: 24, borderRadius: 12, backgroundColor: palette.cyan, borderWidth: 4, borderColor: '#a2fff9'}, axisTop: {position: 'absolute', top: 7, color: palette.muted, fontSize: 7}, metrics: {flex: 1, marginLeft: 16}, metric: {marginBottom: 10}, metricLabel: {color: palette.muted, fontSize: 8, fontWeight: '800'}, metricValue: {color: palette.ink, fontWeight: '800', fontSize: 16}, warning: {color: palette.amber},
  actions: {flexDirection: 'row', marginTop: 14}, primaryButton: {flex: 1, marginLeft: 6, backgroundColor: palette.cyan, padding: 15, borderRadius: 10, alignItems: 'center'}, secondaryButton: {flex: 1, marginRight: 6, borderWidth: 1, borderColor: palette.cyan, padding: 15, borderRadius: 10, alignItems: 'center'}, primaryText: {color: palette.bg, fontWeight: '900'}, secondaryText: {color: palette.cyan, fontWeight: '900'},
  guideHero: {backgroundColor: palette.panel, padding: 22, borderRadius: 15, marginBottom: 12}, step: {color: palette.amber, fontSize: 10, fontWeight: '900', letterSpacing: 2}, guideTitle: {color: palette.ink, fontSize: 27, fontWeight: '900', marginTop: 8}, turns: {color: palette.cyan, fontSize: 40, fontWeight: '900', marginVertical: 10}, turnUnit: {fontSize: 15, color: palette.ink}, guideCopy: {color: palette.muted, lineHeight: 20}, panel: {backgroundColor: palette.panel, padding: 17, borderRadius: 13, marginBottom: 12}, panelTitle: {color: palette.ink, fontWeight: '900', fontSize: 12, letterSpacing: 1}, procedure: {flexDirection: 'row', alignItems: 'center', marginTop: 14}, procedureNumber: {color: palette.bg, backgroundColor: palette.cyan, width: 24, height: 24, textAlign: 'center', paddingTop: 3, borderRadius: 12, fontWeight: '900'}, procedureText: {color: palette.ink, marginLeft: 11, flex: 1, lineHeight: 18}, largeMetric: {color: palette.cyan, fontWeight: '900', fontSize: 21, marginVertical: 10},
  sectionTitle: {color: palette.ink, fontSize: 25, fontWeight: '900'}, empty: {backgroundColor: palette.panel, padding: 30, borderRadius: 14, alignItems: 'center', marginTop: 18}, emptyTitle: {color: palette.ink, fontWeight: '900', fontSize: 17}, capture: {backgroundColor: palette.panel, borderRadius: 12, padding: 16, marginTop: 10, flexDirection: 'row', justifyContent: 'space-between'}, captureTitle: {color: palette.ink, fontWeight: '800'}, captureTime: {color: palette.muted, marginTop: 4, fontSize: 11}, captureScore: {color: palette.green, fontWeight: '900', fontSize: 18, textAlign: 'right'}, captureStatus: {color: palette.muted, fontSize: 8, fontWeight: '900'}, footer: {color: palette.muted, fontSize: 10, textAlign: 'center', marginTop: 28}
});
