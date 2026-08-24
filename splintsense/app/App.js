// SplintSense app entrypoint
// Author: jayis1
const React = require('react');
const { SafeAreaView, ScrollView, StyleSheet, Text } = require('react-native');
const HomeScreen = require('./screens/HomeScreen');
const TrendScreen = require('./screens/TrendScreen');
const AlertScreen = require('./screens/AlertScreen');
const DeviceScreen = require('./screens/DeviceScreen');
const ClinicianScreen = require('./screens/ClinicianScreen');
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
      React.createElement(Text, { style: styles.title }, 'SplintSense'),
      React.createElement(Text, { style: styles.subtitle }, 'Smart orthopedic recovery liner • Author: jayis1'),
      React.createElement(HomeScreen, { data: session.live }),
      React.createElement(TrendScreen, { trends: session.trends }),
      React.createElement(AlertScreen, { alerts: session.alerts }),
      React.createElement(DeviceScreen, { device: session.device }),
      React.createElement(ClinicianScreen, { data: session.clinician }),
      React.createElement(SettingsScreen, { settings: session.settings })
    )
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#0d1518' },
  container: { padding: 16, gap: 16 },
  title: { color: '#f0fffb', fontSize: 30, fontWeight: '700' },
  subtitle: { color: '#8cb9bf', marginBottom: 8 }
});

module.exports = App;
