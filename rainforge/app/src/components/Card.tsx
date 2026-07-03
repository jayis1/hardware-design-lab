/**
 * Card.tsx — Reusable card component for RainForge UI
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

interface CardProps {
  title?: string;
  children?: React.ReactNode;
  accent?: string;
}

export function Card({ title, children, accent = '#4fc3f7' }: CardProps) {
  return (
    <View style={[styles.card, { borderLeftColor: accent }]}>
      {title && <Text style={styles.title}>{title}</Text>}
      {children}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#0d1e3a',
    borderRadius: 8,
    padding: 16,
    marginVertical: 8,
    borderLeftWidth: 3,
    elevation: 2,
  },
  title: {
    fontSize: 13,
    fontWeight: '700',
    color: '#4fc3f7',
    marginBottom: 8,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
});