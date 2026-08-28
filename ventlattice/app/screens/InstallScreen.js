// VentLattice install screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function InstallScreen({ install }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Install quality'),
    React.createElement(Text, { style: styles.body }, `Magnetic coupling: ${install.magneticCoupling}`),
    React.createElement(Text, { style: styles.body }, `Nozzle alignment: ${install.nozzleAlignment}`),
    React.createElement(Text, { style: styles.body }, `Vent coverage: ${install.coverage}`),
    React.createElement(Text, { style: styles.tip }, `Tip: ${install.tip}`)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 10 },
  body: { color: '#d8f4f4', marginBottom: 6 },
  tip: { color: '#95d0ff', marginTop: 8 }
});

module.exports = InstallScreen;
