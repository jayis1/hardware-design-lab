// DrainVeil event timeline screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function EventTimelineScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Event Timeline'),
    ...props.events.map((event, index) => React.createElement(
      View,
      { key: `event-${index}`, style: styles.event },
      React.createElement(Text, { style: styles.time }, event.time),
      React.createElement(Text, { style: styles.eventTitle }, event.title),
      React.createElement(Text, { style: styles.detail }, event.detail)
    ))
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  event: { backgroundColor: '#132029', borderRadius: 12, padding: 12, borderWidth: 1, borderColor: '#1e3340', gap: 4 },
  time: { color: '#7fb5ff', fontWeight: '700' },
  eventTitle: { color: '#eef6ff', fontSize: 16, fontWeight: '700' },
  detail: { color: '#b8ced9', lineHeight: 18 }
});

module.exports = EventTimelineScreen;
