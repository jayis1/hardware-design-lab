// VentLattice app entrypoint
// Author: jayis1
const React = require('react');
const { SafeAreaView, ScrollView, StyleSheet, Text } = require('react-native');
const DashboardScreen = require('./screens/DashboardScreen');
const AirflowMapScreen = require('./screens/AirflowMapScreen');
const BalanceScreen = require('./screens/BalanceScreen');
const AlertsScreen = require('./screens/AlertsScreen');
const InstallScreen = require('./screens/InstallScreen');
const AutomationScreen = require('./screens/AutomationScreen');
const DeviceScreen = require('./screens/DeviceScreen');
const { sampleVentData } = require('./utils/protocol');

function App() {
  const session = sampleVentData();
  return React.createElement(
    SafeAreaView,
    { style: styles.root },
    React.createElement(
      ScrollView,
      { contentContainerStyle: styles.container },
      React.createElement(Text, { style: styles.title }, 'VentLattice'),
      React.createElement(Text, { style: styles.subtitle }, 'Register airflow intelligence module • Author: jayis1'),
      React.createElement(DashboardScreen, { live: session.live }),
      React.createElement(AirflowMapScreen, { map: session.map }),
      React.createElement(BalanceScreen, { rooms: session.rooms }),
      React.createElement(AlertsScreen, { alerts: session.alerts }),
      React.createElement(InstallScreen, { install: session.install }),
      React.createElement(AutomationScreen, { automation: session.automation }),
      React.createElement(DeviceScreen, { device: session.device })
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
