// VentLattice alerts screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function AlertsScreen({ alerts }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Alerts and recommendations'),
    alerts.map((alert, index) => React.createElement(
      View,
      { key: `${alert.code}-${index}`, style: styles.alertCard },
      React.createElement(Text, { style: styles.code }, `${alert.code} · ${alert.severity}`),
      React.createElement(Text, { style: styles.detail }, alert.detail),
      React.createElement(Text, { style: styles.action }, `Action: ${alert.action}`)
    ))
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 10 },
  alertCard: { backgroundColor: '#142930', borderRadius: 12, padding: 12, marginBottom: 10 },
  code: { color: '#ffd39c', fontWeight: '700', marginBottom: 4 },
  detail: { color: '#d5eded', marginBottom: 6 },
  action: { color: '#91d2c7' }
});

module.exports = AlertsScreen;
