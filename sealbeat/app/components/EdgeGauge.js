// SealBeat edge gauge
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function scoreColor(score) {
  if (score >= 0.8) return '#39d98a';
  if (score >= 0.6) return '#f8c146';
  return '#ff6d7a';
}

function EdgeGauge(props) {
  const width = `${Math.max(6, Math.round(props.score * 100))}%`;
  return React.createElement(
    View,
    { style: styles.row },
    React.createElement(Text, { style: styles.label }, props.label),
    React.createElement(View, { style: styles.track },
      React.createElement(View, { style: [styles.fill, { width, backgroundColor: scoreColor(props.score) }] })
    ),
    React.createElement(Text, { style: styles.value }, `${Math.round(props.score * 100)}%`)
  );
}

const styles = StyleSheet.create({
  row: { marginBottom: 12 },
  label: { color: '#dceff3', marginBottom: 6, fontSize: 13 },
  track: { height: 10, backgroundColor: '#20353d', borderRadius: 99, overflow: 'hidden' },
  fill: { height: 10, borderRadius: 99 },
  value: { color: '#8fb4bb', marginTop: 4, fontSize: 12 }
});

module.exports = EdgeGauge;
