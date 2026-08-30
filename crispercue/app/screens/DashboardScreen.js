// CrisperCue dashboard screen
// Author: jayis1
import React from 'react';
import { StyleSheet, Text, View } from 'react-native';
import MetricCard from '../components/MetricCard';

export default function DashboardScreen({ bins, selected, health }) {
  const averageFreshness = Math.round(bins.reduce((sum, bin) => sum + bin.freshness, 0) / bins.length);
  const atRiskBins = bins.filter((bin) => bin.spoilageRisk > 0.45).length;

  return (
    <View>
      <MetricCard label="Fleet freshness" value={`${averageFreshness}%`} hint={`Selected: ${selected.name} · ${health.label}`} accent={health.accent} />
      <MetricCard label="At-risk drawers" value={`${atRiskBins}`} hint="Drawers flagged for recipe rescue or aggressive purge." accent="#FFD166" />
      <MetricCard label="Now do this" value={selected.name} hint={selected.recommendation} accent="#72DDF7" />
      <View style={styles.timeline}>
        <Text style={styles.heading}>Why CrisperCue matters</Text>
        <Text style={styles.body}>CrisperCue is designed to catch invisible freshness drift before you can smell or see it. It combines gas sensing, weight change, optical trends, and door-open exposure to recommend when to eat, repackage, freeze, or share produce. This gives households and small commercial kitchens a real decision surface instead of a vague feeling that something in the drawer is probably going bad.</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  timeline: { backgroundColor: '#0F1E29', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#1A3342' },
  heading: { color: '#F6FBFF', fontSize: 18, fontWeight: '700', marginBottom: 10 },
  body: { color: '#D8E8EE', lineHeight: 20 },
});
