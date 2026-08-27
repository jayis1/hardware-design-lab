// SealBeat setup screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function SetupScreen(props) {
  const setup = props.setup;
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Setup and export'),
    React.createElement(Text, { style: styles.heading }, 'Placement'),
    React.createElement(Text, { style: styles.body }, setup.placement),
    React.createElement(Text, { style: styles.heading }, 'Calibration'),
    React.createElement(Text, { style: styles.body }, setup.calibration),
    React.createElement(Text, { style: styles.heading }, 'Export'),
    React.createElement(Text, { style: styles.body }, setup.export)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d', marginBottom: 8 },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 10 },
  heading: { color: '#82d2ff', fontWeight: '700', marginTop: 8, marginBottom: 4 },
  body: { color: '#c0d6db', lineHeight: 20 }
});

module.exports = SetupScreen;
