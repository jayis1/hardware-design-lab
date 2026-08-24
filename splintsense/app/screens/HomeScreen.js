// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const MetricTile = require('../components/MetricTile');
const RiskBar = require('../components/RiskBar');

function HomeScreen(props) {
  const data = props.data;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Recovery snapshot'),
    React.createElement(Text, { style: styles.subtitle }, `${data.profile} • Author: jayis1`),
    React.createElement(
      View,
      { style: styles.row },
      React.createElement(MetricTile, { label: 'RSI', value: data.rsi, caption: 'Recovery stability index' }),
      React.createElement(MetricTile, { label: 'Fit', value: `${data.fitScore}%`, caption: 'Brace fit quality' })
    ),
    React.createElement(
      View,
      { style: styles.row },
      React.createElement(MetricTile, { label: 'Battery', value: `${data.battery}%`, caption: 'Pod battery remaining' }),
      React.createElement(MetricTile, { label: 'Moisture', value: `${data.moisture}%`, caption: 'Current burden estimate' })
    ),
    React.createElement(RiskBar, { label: 'Odor risk', value: data.odorRisk }),
    React.createElement(Text, { style: styles.alert }, `Active alert: ${data.activeAlert}`),
    React.createElement(Text, { style: styles.haptic }, `Suggested haptic pattern: ${data.hapticMode}`)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#132024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#264248' },
  title: { color: '#effdff', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8cb9bf', marginTop: 4, marginBottom: 14 },
  row: { flexDirection: 'row', gap: 10 },
  alert: { color: '#ffd8ab', marginTop: 10, fontWeight: '600' },
  haptic: { color: '#82d0df', marginTop: 8 }
});

module.exports = HomeScreen;
