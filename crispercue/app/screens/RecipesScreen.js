// CrisperCue recipe planning screen
// Author: jayis1
import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

export default function RecipesScreen({ selected }) {
  return (
    <View>
      <Text style={styles.heading}>Recipe rescue queue</Text>
      {selected.recipeIdeas.map((idea, index) => (
        <View key={idea} style={styles.ideaCard}>
          <Text style={styles.ideaIndex}>0{index + 1}</Text>
          <View style={styles.ideaText}>
            <Text style={styles.ideaTitle}>{idea}</Text>
            <Text style={styles.ideaBody}>CrisperCue picked this idea because it matches the texture, sweetness, and remaining mass in {selected.name}. The goal is not just to avoid waste; it is to convert a freshness warning into an immediate cooking plan.</Text>
          </View>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  heading: { color: '#F6FBFF', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  ideaCard: {
    backgroundColor: '#10212D',
    borderRadius: 16,
    padding: 14,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#1A3342',
    flexDirection: 'row',
  },
  ideaIndex: { color: '#72DDF7', fontSize: 22, fontWeight: '700', width: 36 },
  ideaText: { flex: 1 },
  ideaTitle: { color: '#F6FBFF', fontWeight: '700', fontSize: 16, marginBottom: 4 },
  ideaBody: { color: '#D8E8EE', lineHeight: 20 },
});
