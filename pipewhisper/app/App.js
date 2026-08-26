// PipeWhisper app entrypoint
// Author: jayis1
const React = require('react');
const { SafeAreaView, ScrollView, StyleSheet, Text } = require('react-native');
const DashboardScreen = require('./screens/DashboardScreen');
const FingerprintScreen = require('./screens/FingerprintScreen');
const TimelineScreen = require('./screens/TimelineScreen');
const InstallScreen = require('./screens/InstallScreen');
const ReportScreen = require('./screens/ReportScreen');
const SettingsScreen = require('./screens/SettingsScreen');
const DeviceScreen = require('./screens/DeviceScreen');
const { samplePipeData } = require('./utils/protocol');

function App() {
  const session = samplePipeData();
  return React.createElement(
    SafeAreaView,
    { style: styles.root },
    React.createElement(
      ScrollView,
      { contentContainerStyle: styles.container },
      React.createElement(Text, { style: styles.title }, 'PipeWhisper'),
      React.createElement(Text, { style: styles.subtitle }, 'Clamp-on plumbing health sentinel • Author: jayis1'),
      React.createElement(DashboardScreen, { data: session.live }),
      React.createElement(FingerprintScreen, { fingerprints: session.fingerprints }),
      React.createElement(TimelineScreen, { timeline: session.timeline }),
      React.createElement(InstallScreen, { install: session.install }),
      React.createElement(ReportScreen, { report: session.report }),
      React.createElement(DeviceScreen, { device: session.device }),
      React.createElement(SettingsScreen, { settings: session.settings })
    )
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#081417' },
  container: { padding: 16, gap: 16 },
  title: { color: '#ecfffe', fontSize: 30, fontWeight: '700' },
  subtitle: { color: '#87b7b6', marginBottom: 8 }
});

module.exports = App;
