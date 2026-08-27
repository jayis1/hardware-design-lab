// SealBeat dashboard screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');
const MetricCard = require('../components/MetricCard');

function DashboardScreen(props) {
  const data = props.data;
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Dashboard'),
    React.createElement(View, { style: styles.grid },
      React.createElement(MetricCard, { label: 'Seal score', value: `${data.sealScore}%`, caption: 'Composite closure integrity across all four edges.' }),
      React.createElement(MetricCard, { label: 'Safety score', value: `${data.safetyScore}%`, caption: 'Thermal safety margin after recent close cycles.' }),
      React.createElement(MetricCard, { label: 'Hinge wear', value: `${data.hingeWear}%`, caption: 'Trend score derived from skew, bounce, and vibration.' }),
      React.createElement(MetricCard, { label: 'Battery', value: `${data.battery}%`, caption: 'Projected to exceed six months in current traffic profile.' }),
      React.createElement(MetricCard, { label: 'Recovery', value: `${data.lastRecovery}s`, caption: 'Time constant for the latest warm-edge recovery event.' }),
      React.createElement(MetricCard, { label: 'Night openings', value: `${data.nightOpenings}`, caption: 'Open cycles recorded in the overnight quiet window.' })
    )
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d' },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10, justifyContent: 'space-between' }
});

module.exports = DashboardScreen;
