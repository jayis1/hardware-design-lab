// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function MetricTile(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.label }, props.label),
    React.createElement(Text, { style: styles.value }, String(props.value)),
    React.createElement(Text, { style: styles.caption }, props.caption)
  );
}

const styles = StyleSheet.create({
  card: {
    flex: 1,
    minWidth: 140,
    backgroundColor: '#172427',
    borderRadius: 14,
    padding: 14,
    marginBottom: 10,
    borderWidth: 1,
    borderColor: '#264248'
  },
  label: { color: '#8cb9bf', fontSize: 13, marginBottom: 8 },
  value: { color: '#f0fffb', fontSize: 26, fontWeight: '700' },
  caption: { color: '#91aab0', fontSize: 12, marginTop: 6 }
});

module.exports = MetricTile;
