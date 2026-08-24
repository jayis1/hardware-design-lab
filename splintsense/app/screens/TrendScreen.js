// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function renderSeries(label, values, color) {
  return React.createElement(
    View,
    { style: styles.seriesBlock, key: label },
    React.createElement(Text, { style: styles.seriesTitle }, label),
    React.createElement(
      View,
      { style: styles.barRow },
      values.map((value, index) => React.createElement(View, {
        key: `${label}-${index}`,
        style: [styles.bar, { height: 18 + value * 2, backgroundColor: color }]
      }))
    ),
    React.createElement(Text, { style: styles.caption }, values.join('  •  '))
  );
}

function TrendScreen(props) {
  const trends = props.trends;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Trend window'),
    renderSeries('Pressure asymmetry', trends.pressureAsymmetry, '#6fd6ff'),
    renderSeries('Moisture burden', trends.moistureBurden, '#59d19a'),
    renderSeries('VOC rise', trends.vocRise, '#ffb347'),
    renderSeries('Impact counts', trends.impactCounts, '#ff6a5c')
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#132024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#264248' },
  title: { color: '#effdff', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  seriesBlock: { marginBottom: 16 },
  seriesTitle: { color: '#d2eef3', marginBottom: 8, fontWeight: '600' },
  barRow: { flexDirection: 'row', alignItems: 'flex-end', gap: 8 },
  bar: { width: 18, borderRadius: 5 },
  caption: { color: '#85aeb6', marginTop: 8, fontSize: 12 }
});

module.exports = TrendScreen;
