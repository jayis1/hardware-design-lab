// VentLattice metric card
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function MetricCard({ label, value, tone }) {
  return React.createElement(
    View,
    [styles.card, tone === 'warn' ? styles.warn : null, tone === 'good' ? styles.good : null],
    React.createElement(Text, { style: styles.label }, label),
    React.createElement(Text, { style: styles.value }, value)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#112126', borderRadius: 14, padding: 14, minWidth: 140, marginBottom: 10 },
  warn: { borderWidth: 1, borderColor: '#f4b860' },
  good: { borderWidth: 1, borderColor: '#4bd4a9' },
  label: { color: '#8cb3b3', fontSize: 12, textTransform: 'uppercase', marginBottom: 4 },
  value: { color: '#f4ffff', fontSize: 22, fontWeight: '700' }
});

module.exports = MetricCard;
