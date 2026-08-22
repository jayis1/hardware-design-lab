// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function SettingsScreen(props) {
  const settings = props.settings;
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.heading }, 'Settings'),
    React.createElement(View, { style: styles.card },
      React.createElement(Text, { style: styles.item }, 'Crop preset: ' + settings.cropPreset),
      React.createElement(Text, { style: styles.item }, 'Auto-sync: ' + (settings.autoSync ? 'enabled' : 'disabled')),
      React.createElement(Text, { style: styles.item }, 'Thermal smoothing: ' + settings.thermalSmoothing),
      React.createElement(Text, { style: styles.item }, 'Spore threshold: ' + settings.sporeThreshold),
      React.createElement(Text, { style: styles.item }, 'Author metadata: jayis1')
    )
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  heading: { color: '#d7f7e6', fontSize: 22, fontWeight: '700' },
  card: { backgroundColor: '#13201a', borderRadius: 12, padding: 12, borderWidth: 1, borderColor: '#294233' },
  item: { color: '#c8e7d6', marginBottom: 6 }
});

module.exports = SettingsScreen;
