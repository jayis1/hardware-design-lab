// SealBeat seal map screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');
const EdgeGauge = require('../components/EdgeGauge');

function SealMapScreen(props) {
  const sealMap = props.sealMap;
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Seal map'),
    React.createElement(Text, { style: styles.subtitle }, `Primary leak vector: ${sealMap.leakSide}`),
    React.createElement(EdgeGauge, { label: 'Top edge', score: sealMap.top }),
    React.createElement(EdgeGauge, { label: 'Latch edge', score: sealMap.latch }),
    React.createElement(EdgeGauge, { label: 'Bottom edge', score: sealMap.bottom }),
    React.createElement(EdgeGauge, { label: 'Hinge edge', score: sealMap.hinge }),
    React.createElement(Text, { style: styles.note }, sealMap.note)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d' },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 8 },
  subtitle: { color: '#8eb1b8', marginBottom: 12 },
  note: { color: '#b9d2d7', lineHeight: 20, marginTop: 8 }
});

module.exports = SealMapScreen;
