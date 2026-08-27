// SealBeat thermal recovery screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function ThermalRecoveryScreen(props) {
  const recovery = props.recovery;
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Thermal recovery'),
    React.createElement(Text, { style: styles.metric }, `Warm-edge rebound: ${recovery.warmReboundC.toFixed(1)} °C`),
    React.createElement(Text, { style: styles.metric }, `Recovery time constant: ${recovery.tauSeconds}s`),
    React.createElement(Text, { style: styles.metric }, `Edge temperature: ${recovery.edgeTempC.toFixed(1)} °C`),
    React.createElement(Text, { style: styles.metric }, `Compartment temperature: ${recovery.compartmentTempC.toFixed(1)} °C`),
    React.createElement(Text, { style: [styles.badge, recovery.foodSafe ? styles.safe : styles.warn] }, recovery.foodSafe ? 'Food/medicine safe in current profile' : 'Safety margin at risk'),
    React.createElement(Text, { style: styles.commentary }, recovery.commentary)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d' },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 10 },
  metric: { color: '#b3ccd1', marginBottom: 6 },
  badge: { marginTop: 8, marginBottom: 10, alignSelf: 'flex-start', paddingHorizontal: 10, paddingVertical: 6, borderRadius: 99, fontWeight: '700' },
  safe: { backgroundColor: '#183b2a', color: '#8ef0b8' },
  warn: { backgroundColor: '#412222', color: '#ffc1c1' },
  commentary: { color: '#c4d9de', lineHeight: 20 }
});

module.exports = ThermalRecoveryScreen;
