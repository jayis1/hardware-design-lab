// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function MetricCard(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.label }, props.label),
    React.createElement(Text, { style: styles.value }, props.value),
    React.createElement(Text, { style: styles.caption }, props.caption || '')
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#13201a',
    borderRadius: 12,
    padding: 12,
    minWidth: 140,
    borderWidth: 1,
    borderColor: '#294233'
  },
  label: { color: '#92c9a9', fontSize: 12, textTransform: 'uppercase' },
  value: { color: '#f4fff8', fontSize: 22, fontWeight: '700', marginTop: 6 },
  caption: { color: '#94a99a', marginTop: 4, fontSize: 12 }
});

module.exports = MetricCard;
