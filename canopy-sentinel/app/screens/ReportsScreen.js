// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function ReportsScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.heading }, 'Reports'),
    props.reports.map((report) => React.createElement(
      View,
      { key: report.id, style: styles.card },
      React.createElement(Text, { style: styles.title }, report.title),
      React.createElement(Text, { style: styles.body }, report.summary)
    ))
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  heading: { color: '#d7f7e6', fontSize: 22, fontWeight: '700' },
  card: { backgroundColor: '#13201a', borderRadius: 12, padding: 12, borderWidth: 1, borderColor: '#294233' },
  title: { color: '#f4fff8', fontWeight: '700' },
  body: { color: '#9dc0ab', marginTop: 5 }
});

module.exports = ReportsScreen;
