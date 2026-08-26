// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function ReportScreen(props) {
  const report = props.report;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Maintenance report'),
    React.createElement(Text, { style: styles.subtitle }, `Prepared by PipeWhisper • Author: ${report.author}`),
    React.createElement(Text, { style: styles.heading }, 'Summary'),
    React.createElement(Text, { style: styles.body }, report.summary),
    React.createElement(Text, { style: styles.heading }, 'Recommended actions'),
    report.actions.map((text, index) => React.createElement(Text, { key: `bullet-${index}`, style: styles.bullet }, `• ${text}`))
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8fb8b6', marginTop: 4, marginBottom: 10 },
  heading: { color: '#6ce4d0', marginTop: 8, marginBottom: 6, fontWeight: '700' },
  body: { color: '#eafdf8', lineHeight: 20 },
  bullet: { color: '#ddfaf3', lineHeight: 22, marginTop: 4 }
});

module.exports = ReportScreen;
