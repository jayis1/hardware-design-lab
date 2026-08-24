// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function SettingsScreen(props) {
  const settings = props.settings;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Settings'),
    React.createElement(Text, { style: styles.line }, `Vibration strength: ${settings.vibrationStrength}`),
    React.createElement(Text, { style: styles.line }, `Quiet hours: ${settings.quietHours}`),
    React.createElement(Text, { style: styles.line }, `Scan cadence: ${settings.scanCadence}`),
    React.createElement(Text, { style: styles.line }, `Profile mode: ${settings.profileMode}`),
    React.createElement(Text, { style: styles.footer }, 'Companion app workflow and interface by jayis1')
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#132024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#264248' },
  title: { color: '#effdff', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  line: { color: '#b8d7dc', marginBottom: 6 },
  footer: { color: '#7ea4ab', marginTop: 10 }
});

module.exports = SettingsScreen;
