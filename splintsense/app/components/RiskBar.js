// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function colorForValue(value) {
  if (value >= 80) return '#ff6a5c';
  if (value >= 50) return '#ffb347';
  return '#59d19a';
}

function RiskBar(props) {
  const width = Math.max(6, Math.min(100, props.value));
  return React.createElement(
    View,
    { style: styles.wrap },
    React.createElement(Text, { style: styles.label }, props.label),
    React.createElement(
      View,
      { style: styles.track },
      React.createElement(View, {
        style: [styles.fill, { width: `${width}%`, backgroundColor: colorForValue(props.value) }]
      })
    ),
    React.createElement(Text, { style: styles.value }, `${props.value}%`)
  );
}

const styles = StyleSheet.create({
  wrap: { marginBottom: 14 },
  label: { color: '#d4ecef', marginBottom: 6, fontWeight: '600' },
  track: { height: 12, borderRadius: 999, backgroundColor: '#20363b', overflow: 'hidden' },
  fill: { height: 12, borderRadius: 999 },
  value: { color: '#87adb3', marginTop: 4, fontSize: 12 }
});

module.exports = RiskBar;
