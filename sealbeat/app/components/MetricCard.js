// SealBeat metric card
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function MetricCard(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.label }, props.label),
    React.createElement(Text, { style: styles.value }, props.value),
    React.createElement(Text, { style: styles.caption }, props.caption)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#112027', borderRadius: 14, padding: 12, flexBasis: '48%', borderWidth: 1, borderColor: '#1c3640' },
  label: { color: '#95b8bd', fontSize: 12, marginBottom: 8 },
  value: { color: '#f0fcff', fontSize: 24, fontWeight: '700', marginBottom: 6 },
  caption: { color: '#b3cad0', fontSize: 12, lineHeight: 18 }
});

module.exports = MetricCard;
