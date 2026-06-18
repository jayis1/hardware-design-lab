/**
 * Card.tsx — reusable styled card component
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React, { ReactNode } from 'react';
import { View, StyleSheet } from 'react-native';

export function Card({ children, style }: { children: ReactNode; style?: any }) {
  return <View style={[styles.card, style]}>{children}</View>;
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#11182b',
    borderRadius: 12,
    padding: 16,
    marginVertical: 6,
    marginHorizontal: 10,
    borderWidth: 1,
    borderColor: '#1f2b47',
  },
});