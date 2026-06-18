// utils/theme.js — HydroFluor app theme
// Author: jayis1  License: MIT

export const theme = {
  colors: {
    bg:        '#0b1d2a',
    surface:  '#102838',
    accent:    '#1e88e5',
    accentDim: '#0d4d8c',
    good:      '#43a047',
    warn:      '#fb8c00',
    alarm:     '#e53935',
    text:      '#e0f7fa',
    textDim:   '#80a7b3',
    gridLine:  '#143747',
    fluorLow:  '#0d2b45',
    fluorHigh: '#1e88e5',
  },
  fonts: {
    mono:    'monospace',
    sans:    'System',
    title:   'System',
  },
  spacing: { xs: 4, sm: 8, md: 16, lg: 24, xl: 32 },
  radius:  { sm: 4, md: 8, lg: 12 },
};

export const ANALYTES = [
  { key: 'cdom', label: 'CDOM',  unit: 'ppb',   alarm: 500,   color: '#8d6e63' },
  { key: 'chla', label: 'Chl-a', unit: 'µg/L',  alarm: 100,   color: '#43a047' },
  { key: 'phyc', label: 'Phyc',  unit: 'µg/L',  alarm: 50,    color: '#1e88e5' },
  { key: 'oil',  label: 'Oil',   unit: 'ppb',   alarm: 20,    color: '#fb8c00' },
  { key: 'turb', label: 'Turb',  unit: 'NTU',   alarm: 500,   color: '#9e9d24' },
];