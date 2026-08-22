// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function RiskGauge(props) {
  const score = props.score || 0;
  const level = props.level || 'low';
  const width = Math.max(0, Math.min(100, score));
  const color = level === 'critical' ? '#ff5d5d' : level === 'elevated' ? '#ff9f43' : level === 'moderate' ? '#f6d55c' : '#4cd964';
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Disease Risk'),
    React.createElement(View, { style: styles.bar },
      React.createElement(View, { style: [styles.fill, { width: width + '%', backgroundColor: color }] })),
    React.createElement(Text, { style: styles.value }, level.toUpperCase() + ' · ' + score.toFixed(1))
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#13201a', borderRadius: 12, padding: 14, borderWidth: 1, borderColor: '#294233' },
  title: { color: '#d7f7e6', fontSize: 16, fontWeight: '700', marginBottom: 10 },
  bar: { height: 14, borderRadius: 10, backgroundColor: '#22362b', overflow: 'hidden' },
  fill: { height: '100%' },
  value: { color: '#c8e7d6', marginTop: 10, fontWeight: '600' }
});

module.exports = RiskGauge;
