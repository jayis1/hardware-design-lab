// VentLattice dashboard screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');
const MetricCard = require('../components/MetricCard');
const GaugeBar = require('../components/GaugeBar');

function DashboardScreen({ live }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Room dashboard'),
    React.createElement(Text, { style: styles.sub }, live.summary),
    React.createElement(
      View,
      { style: styles.row },
      React.createElement(MetricCard, { label: 'Service score', value: `${live.serviceScore.toFixed(0)} / 100`, tone: live.serviceScore < 60 ? 'warn' : 'good' }),
      React.createElement(MetricCard, { label: 'Airflow', value: `${live.airflowCfm.toFixed(1)} CFM` }),
      React.createElement(MetricCard, { label: 'Supply / Room', value: `${live.supplyTemp.toFixed(1)}° / ${live.roomTemp.toFixed(1)}°` })
    ),
    React.createElement(GaugeBar, { label: 'Stale-air risk', value: live.staleAirRisk * 100, max: 100, accent: '#f4b860' }),
    React.createElement(GaugeBar, { label: 'Comfort waste', value: live.comfortWaste * 100, max: 100, accent: '#8a79ff' }),
    React.createElement(GaugeBar, { label: 'Maintenance priority', value: live.maintenancePriority * 100, max: 100, accent: '#ff6d7f' })
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 4 },
  sub: { color: '#89b7b7', marginBottom: 14 },
  row: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 }
});

module.exports = DashboardScreen;
