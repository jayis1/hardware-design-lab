// DrainVeil card component
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

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
  card: { backgroundColor: '#132029', borderRadius: 12, padding: 12, borderWidth: 1, borderColor: '#1f3642', gap: 6, minWidth: 140 },
  label: { color: '#8eb0c2', fontSize: 12, textTransform: 'uppercase' },
  value: { color: '#f3fbff', fontSize: 22, fontWeight: '700' },
  caption: { color: '#b4cad5', lineHeight: 18, fontSize: 12 }
});

module.exports = MetricCard;
