/**
 * Theme.js — Color theme and design tokens for Vortex-SDR app
 * Dark theme optimized for outdoor/field use
 */

export const theme = {
  colors: {
    primary: '#00E676',      // Green accent (spectrum trace color)
    primaryDark: '#00C853',
    secondary: '#2979FF',     // Blue accent (waterfall)
    accent: '#FF6D00',       // Orange accent (peaks/markers)
    background: '#0D0D0D',   // Near-black background
    surface: '#1A1A2E',      // Dark surface
    surfaceLight: '#252540',  // Lighter surface
    border: '#333355',      // Border color
    text: '#E0E0E0',         // Primary text
    textSecondary: '#9E9E9E', // Secondary text
    textDim: '#616161',      // Dimmed text
    error: '#FF1744',        // Error red
    warning: '#FFD600',      // Warning yellow
    success: '#00E676',      // Success green
    gridLine: '#1A1A3E',     // Grid line color
    gridLineMajor: '#2A2A4E', // Major grid line
    waterfallHot: '#FF0000', // Waterfall hot color
    waterfallWarm: '#FFFF00', // Waterfall warm color
    waterfallCool: '#0000FF', // Waterfall cool color
    waterfallCold: '#000033', // Waterfall cold color
  },
  spacing: {
    xs: 4,
    sm: 8,
    md: 12,
    lg: 16,
    xl: 24,
    xxl: 32,
  },
  borderRadius: {
    sm: 4,
    md: 8,
    lg: 12,
    xl: 16,
  },
  fontSize: {
    xs: 10,
    sm: 12,
    md: 14,
    lg: 16,
    xl: 20,
    xxl: 24,
    title: 28,
    display: 36,
  },
  fontWeight: {
    regular: '400',
    medium: '500',
    semiBold: '600',
    bold: '700',
  },
};

export default theme;