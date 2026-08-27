// SealBeat app entrypoint
// Author: jayis1
const React = require('react');
const { SafeAreaView, ScrollView, StyleSheet, Text, View } = require('react-native');
const DashboardScreen = require('./screens/DashboardScreen');
const SealMapScreen = require('./screens/SealMapScreen');
const DoorCyclesScreen = require('./screens/DoorCyclesScreen');
const ThermalRecoveryScreen = require('./screens/ThermalRecoveryScreen');
const MaintenanceScreen = require('./screens/MaintenanceScreen');
const DeviceScreen = require('./screens/DeviceScreen');
const SetupScreen = require('./screens/SetupScreen');
const { sampleSealBeatData } = require('./utils/protocol');

function App() {
  const session = sampleSealBeatData();
  return React.createElement(
    SafeAreaView,
    { style: styles.root },
    React.createElement(
      ScrollView,
      { contentContainerStyle: styles.container },
      React.createElement(Text, { style: styles.title }, 'SealBeat'),
      React.createElement(Text, { style: styles.subtitle }, 'Refrigerator and freezer seal intelligence • Author: jayis1'),
      React.createElement(View, { style: styles.banner },
        React.createElement(Text, { style: styles.bannerTitle }, session.summary.status),
        React.createElement(Text, { style: styles.bannerText }, session.summary.message)
      ),
      React.createElement(DashboardScreen, { data: session.dashboard }),
      React.createElement(SealMapScreen, { sealMap: session.sealMap }),
      React.createElement(DoorCyclesScreen, { cycles: session.cycles }),
      React.createElement(ThermalRecoveryScreen, { recovery: session.recovery }),
      React.createElement(MaintenanceScreen, { tasks: session.tasks }),
      React.createElement(DeviceScreen, { device: session.device }),
      React.createElement(SetupScreen, { setup: session.setup })
    )
  );
}

const styles = StyleSheet.create({
  root: { flex: 1, backgroundColor: '#091316' },
  container: { padding: 16, gap: 16 },
  title: { color: '#effcff', fontSize: 30, fontWeight: '700' },
  subtitle: { color: '#95b8bd', marginBottom: 8 },
  banner: { backgroundColor: '#12252b', borderRadius: 14, padding: 14, borderWidth: 1, borderColor: '#214149' },
  bannerTitle: { color: '#dffef5', fontSize: 18, fontWeight: '700', marginBottom: 6 },
  bannerText: { color: '#b7d0d5', lineHeight: 20 }
});

module.exports = App;
