// VentLattice automation screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function AutomationScreen({ automation }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Automation handoff'),
    React.createElement(Text, { style: styles.body }, `Target platform: ${automation.platform}`),
    React.createElement(Text, { style: styles.body }, `Recommended scene: ${automation.scene}`),
    React.createElement(Text, { style: styles.body }, `Thermostat note: ${automation.note}`),
    React.createElement(Text, { style: styles.body }, `Smart vent hint: ${automation.smartVentHint}`)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 10 },
  body: { color: '#d8f4f4', marginBottom: 6 }
});

module.exports = AutomationScreen;
