// DrainVeil node chip component
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function riskColor(level) {
  if (level >= 0.8) return '#d94b4b';
  if (level >= 0.6) return '#dd8e3e';
  if (level >= 0.35) return '#c9b147';
  return '#43b581';
}

function NodeChip(props) {
  return React.createElement(
    View,
    { style: [styles.chip, { borderColor: riskColor(props.risk) }] },
    React.createElement(Text, { style: styles.name }, props.name),
    React.createElement(Text, { style: [styles.value, { color: riskColor(props.risk) }] }, `Risk ${Math.round(props.risk * 100)}`),
    React.createElement(Text, { style: styles.detail }, props.detail)
  );
}

const styles = StyleSheet.create({
  chip: { backgroundColor: '#101c24', borderRadius: 12, borderWidth: 1.5, padding: 12, gap: 4 },
  name: { color: '#eef7ff', fontWeight: '700' },
  value: { fontWeight: '700' },
  detail: { color: '#abc1cd', lineHeight: 18, fontSize: 12 }
});

module.exports = NodeChip;
