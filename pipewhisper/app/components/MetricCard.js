// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function MetricCard(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.label }, props.label),
    React.createElement(Text, { style: styles.value }, String(props.value)),
    React.createElement(Text, { style: styles.caption }, props.caption)
  );
}

const styles = StyleSheet.create({
  card: { flex: 1, backgroundColor: '#122226', borderRadius: 14, padding: 14, borderWidth: 1, borderColor: '#234349' },
  label: { color: '#8fc1c0', fontSize: 12, textTransform: 'uppercase', letterSpacing: 1 },
  value: { color: '#f2fffd', fontSize: 24, fontWeight: '700', marginTop: 8 },
  caption: { color: '#9db7b8', marginTop: 6, fontSize: 12 }
});

module.exports = MetricCard;
