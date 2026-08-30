// CrisperCue freshness analytics screen
// Author: jayis1
import React from 'react';
import { StyleSheet, Text, View } from 'react-native';
import MetricCard from '../components/MetricCard';

export default function FreshnessScreen({ selected, health }) {
  return (
    <View>
      <Text style={styles.heading}>Freshness analytics</Text>
      <MetricCard label="Health state" value={health.label} hint={`Ethylene ${selected.ethylene} ppm · CO₂ ${selected.co2} ppm`} accent={health.accent} />
      <MetricCard label="Spoilage risk" value={`${Math.round(selected.spoilageRisk * 100)}%`} hint="Derived from gas accumulation, optical decay, and mass loss." accent="#FF8A65" />
      <MetricCard label="Best intervention" value="Recipe rescue" hint={selected.recommendation} accent="#72DDF7" />
      <View style={styles.noteBox}>
        <Text style={styles.noteTitle}>Interpretation</Text>
        <Text style={styles.note}>A high ethylene reading means climacteric fruit is ripening faster than the bin is venting. A rising CO₂ trend paired with falling freshness usually means respiration is outrunning airflow. The app uses those signals to estimate when produce crosses from ideal texture into “good for cooking only.”</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  heading: { color: '#F6FBFF', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  noteBox: { backgroundColor: '#0F1E29', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#1A3342' },
  noteTitle: { color: '#F6FBFF', fontWeight: '700', marginBottom: 8 },
  note: { color: '#D8E8EE', lineHeight: 20 },
});
