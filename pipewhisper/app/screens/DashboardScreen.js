// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const MetricCard = require('../components/MetricCard');
const ScoreBar = require('../components/ScoreBar');

function DashboardScreen(props) {
  const data = props.data;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Branch health dashboard'),
    React.createElement(Text, { style: styles.subtitle }, `${data.branch} • ${data.profile} • Author: jayis1`),
    React.createElement(View, { style: styles.row },
      React.createElement(MetricCard, { label: 'Leak', value: `${data.leakConfidence}%`, caption: 'Persistent drip confidence' }),
      React.createElement(MetricCard, { label: 'Freeze', value: `${data.freezeRisk}%`, caption: 'Overnight freeze exposure' })
    ),
    React.createElement(View, { style: styles.row },
      React.createElement(MetricCard, { label: 'Battery', value: `${data.battery}%`, caption: 'Device battery remaining' }),
      React.createElement(MetricCard, { label: 'Health', value: data.healthIndex, caption: 'Overall branch health index' })
    ),
    React.createElement(ScoreBar, { label: 'Install quality', value: data.installQuality }),
    React.createElement(ScoreBar, { label: 'Hammer severity', value: data.hammerSeverity }),
    React.createElement(Text, { style: styles.summary }, `Summary: ${data.summary}`)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8fb8b6', marginTop: 4, marginBottom: 14 },
  row: { flexDirection: 'row', gap: 10, marginBottom: 10 },
  summary: { color: '#dffaf5', marginTop: 12, lineHeight: 20 }
});

module.exports = DashboardScreen;
