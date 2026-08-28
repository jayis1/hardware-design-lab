// VentLattice gauge bar
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function GaugeBar({ label, value, max, accent }) {
  const width = `${Math.max(4, Math.min(100, (value / max) * 100))}%`;
  return React.createElement(
    View,
    { style: styles.wrap },
    React.createElement(Text, { style: styles.label }, `${label} · ${value.toFixed(1)}/${max}`),
    React.createElement(
      View,
      { style: styles.track },
      React.createElement(View, { style: [styles.fill, { width, backgroundColor: accent || '#59d3ff' }] })
    )
  );
}

const styles = StyleSheet.create({
  wrap: { marginBottom: 12 },
  label: { color: '#c8e5e5', marginBottom: 6 },
  track: { height: 10, backgroundColor: '#173038', borderRadius: 999 },
  fill: { height: 10, borderRadius: 999 }
});

module.exports = GaugeBar;
