// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const ScoreBar = require('../components/ScoreBar');

function InstallScreen(props) {
  const install = props.install;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Install assistant'),
    React.createElement(Text, { style: styles.subtitle }, 'Clamp fit validation • Author: jayis1'),
    React.createElement(ScoreBar, { label: 'Acoustic coupling', value: install.acousticCoupling }),
    React.createElement(ScoreBar, { label: 'Clamp tension', value: install.clampTension }),
    React.createElement(ScoreBar, { label: 'Orientation confidence', value: install.orientation }),
    React.createElement(Text, { style: styles.note }, install.note)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8fb8b6', marginTop: 4, marginBottom: 10 },
  note: { color: '#defaf5', marginTop: 12, lineHeight: 20 }
});

module.exports = InstallScreen;
