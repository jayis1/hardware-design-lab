// DrainVeil app entrypoint
// Author: jayis1
const React = require('react');
const { SafeAreaView, ScrollView, StyleSheet, Text, View } = require('react-native');
const DashboardScreen = require('./screens/DashboardScreen');
const DrainMapScreen = require('./screens/DrainMapScreen');
const EventTimelineScreen = require('./screens/EventTimelineScreen');
const ChemistryScreen = require('./screens/ChemistryScreen');
const MaintenanceScreen = require('./screens/MaintenanceScreen');
const DeviceScreen = require('./screens/DeviceScreen');
const SetupScreen = require('./screens/SetupScreen');
const { sampleDrainVeilData } = require('./utils/protocol');

function App() {
  const session = sampleDrainVeilData();
  return React.createElement(
    SafeAreaView,
    { style: styles.root },
    React.createElement(
      ScrollView,
      { contentContainerStyle: styles.container },
      React.createElement(Text, { style: styles.title }, 'DrainVeil'),
      React.createElement(Text, { style: styles.subtitle }, 'Hidden drain-line health intelligence • Author: jayis1'),
      React.createElement(
        View,
        { style: styles.banner },
        React.createElement(Text, { style: styles.bannerTitle }, session.summary.status),
        React.createElement(Text, { style: styles.bannerText }, session.summary.message)
      ),
      React.createElement(DashboardScreen, { data: session.dashboard }),
      React.createElement(DrainMapScreen, { nodes: session.nodes }),
      React.createElement(EventTimelineScreen, { events: session.events }),
      React.createElement(ChemistryScreen, { chemistry: session.chemistry }),
      React.createElement(MaintenanceScreen, { tasks: session.tasks }),
      React.createElement(DeviceScreen, { device: session.device }),
      React.createElement(SetupScreen, { setup: session.setup })
    )
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#0c1216' },
  container: { padding: 16, gap: 16 },
  title: { color: '#eff8ff', fontSize: 30, fontWeight: '700' },
  subtitle: { color: '#8ca5b5', marginBottom: 8 },
  banner: { backgroundColor: '#15212a', borderRadius: 14, padding: 14, borderWidth: 1, borderColor: '#243947' },
  bannerTitle: { color: '#e7fff7', fontSize: 18, fontWeight: '700', marginBottom: 6 },
  bannerText: { color: '#bfd4df', lineHeight: 20 }
});

module.exports = App;
