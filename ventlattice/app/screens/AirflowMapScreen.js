// VentLattice airflow map screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');
const GaugeBar = require('../components/GaugeBar');

function AirflowMapScreen({ map }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Airflow map'),
    map.map((entry, index) => React.createElement(
      View,
      { key: `${entry.period}-${index}`, style: styles.item },
      React.createElement(Text, { style: styles.label }, `${entry.period} · ${entry.status}`),
      React.createElement(GaugeBar, { label: 'Delivered airflow', value: entry.cfm, max: 120, accent: entry.status === 'restricted' ? '#ff7b7b' : '#4bd4a9' })
    ))
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 10 },
  item: { marginBottom: 10 },
  label: { color: '#d6f5f5', marginBottom: 4 }
});

module.exports = AirflowMapScreen;
