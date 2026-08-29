// DrainVeil chemistry screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function row(label, value) {
  return React.createElement(
    View,
    { style: styles.row, key: label },
    React.createElement(Text, { style: styles.label }, label),
    React.createElement(Text, { style: styles.value }, value)
  );
}

function ChemistryScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Chemistry and Odor'),
    row('H2S', props.chemistry.h2s),
    row('VOC', props.chemistry.voc),
    row('Humidity', props.chemistry.humidity),
    row('Biofilm', props.chemistry.biofilm),
    React.createElement(Text, { style: styles.narrative }, props.chemistry.narrative)
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  row: { flexDirection: 'row', justifyContent: 'space-between', backgroundColor: '#132029', borderRadius: 10, padding: 12 },
  label: { color: '#9eb7c8', fontWeight: '600' },
  value: { color: '#f2f9ff', fontWeight: '700' },
  narrative: { color: '#bed2dd', lineHeight: 20, backgroundColor: '#101b22', borderRadius: 12, padding: 12 }
});

module.exports = ChemistryScreen;
