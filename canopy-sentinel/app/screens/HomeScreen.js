// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const MetricCard = require('../components/MetricCard');
const RiskGauge = require('../components/RiskGauge');
const ThermalGrid = require('../components/ThermalGrid');

function HomeScreen(props) {
  const live = props.data;
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.heading }, 'Live Scan'),
    React.createElement(RiskGauge, { score: live.riskScore, level: live.riskLevel }),
    React.createElement(View, { style: styles.row },
      React.createElement(MetricCard, { label: 'Dew Margin', value: live.dewMargin + ' °C', caption: 'leaf temp vs dew point' }),
      React.createElement(MetricCard, { label: 'Wetness', value: live.wetness + '%', caption: 'persistence normalized' })
    ),
    React.createElement(View, { style: styles.row },
      React.createElement(MetricCard, { label: 'Spore Index', value: live.sporeIndex.toFixed(1), caption: 'fluorescence events' }),
      React.createElement(MetricCard, { label: 'Stagnation', value: live.stagnation + '%', caption: 'low-airflow canopy pocket' })
    ),
    React.createElement(ThermalGrid, { frame: live.thermalPreview })
  );
}

const styles = StyleSheet.create({
  section: { gap: 12 },
  heading: { color: '#d7f7e6', fontSize: 22, fontWeight: '700' },
  row: { flexDirection: 'row', gap: 12, flexWrap: 'wrap' }
});

module.exports = HomeScreen;
