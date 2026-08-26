// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function settingRow(label, value) {
  return React.createElement(View, { key: label, style: styles.row },
    React.createElement(Text, { style: styles.label }, label),
    React.createElement(Text, { style: styles.value }, value)
  );
}

function SettingsScreen(props) {
  const settings = props.settings;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Settings'),
    React.createElement(Text, { style: styles.subtitle }, 'Local-first controls by jayis1'),
    settingRow('Quiet hours', settings.quietHours),
    settingRow('Leak sensitivity', settings.leakSensitivity),
    settingRow('Freeze sensitivity', settings.freezeSensitivity),
    settingRow('Automation mode', settings.automationMode),
    settingRow('BLE diagnostics', settings.bleDiagnostics)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8fb8b6', marginTop: 4, marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8, borderBottomWidth: 1, borderBottomColor: '#1d3438' },
  label: { color: '#dbfaf6' },
  value: { color: '#6ae0cd', fontWeight: '600' }
});

module.exports = SettingsScreen;
