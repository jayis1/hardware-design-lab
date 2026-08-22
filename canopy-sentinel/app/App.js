// Canopy Sentinel app entrypoint
// Author: jayis1
const React = require('react');
const { SafeAreaView, ScrollView, StyleSheet, Text, View } = require('react-native');
const HomeScreen = require('./screens/HomeScreen');
const SessionScreen = require('./screens/SessionScreen');
const DeviceScreen = require('./screens/DeviceScreen');
const ReportsScreen = require('./screens/ReportsScreen');
const SettingsScreen = require('./screens/SettingsScreen');
const { sampleSessionData } = require('./utils/protocol');

function App() {
  const session = sampleSessionData();
  return React.createElement(
    SafeAreaView,
    { style: styles.root },
    React.createElement(
      ScrollView,
      { contentContainerStyle: styles.container },
      React.createElement(Text, { style: styles.title }, 'Canopy Sentinel'),
      React.createElement(Text, { style: styles.subtitle }, 'Author: jayis1'),
      React.createElement(HomeScreen, { data: session.live }),
      React.createElement(SessionScreen, { sessions: session.history }),
      React.createElement(DeviceScreen, { device: session.device }),
      React.createElement(ReportsScreen, { reports: session.reports }),
      React.createElement(SettingsScreen, { settings: session.settings }),
      React.createElement(View, { style: styles.footer },
        React.createElement(Text, { style: styles.footerText }, 'Canopy Sentinel open hardware workflow by jayis1'))
    )
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#0c1310' },
  container: { padding: 16, gap: 16 },
  title: { color: '#d7f7e6', fontSize: 28, fontWeight: '700' },
  subtitle: { color: '#8ad6a8', marginBottom: 8 },
  footer: { paddingVertical: 24 },
  footerText: { color: '#8ea89a', textAlign: 'center' }
});

module.exports = App;
