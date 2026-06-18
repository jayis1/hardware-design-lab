// screens/CalibrationScreen.js — guided blank + standard calibration
// Author: jayis1  License: MIT
import React, { useState } from 'react';
import { View, Text, Button, Picker, StyleSheet, Alert } from 'react-native';
import { theme, ANALYTES } from '../utils/theme';
import { CALIBRATION_STANDARDS, ANALYTE_IDS, buildWorkflow } from '../utils/calibration';
import ble from '../utils/ble';

export default function CalibrationScreen() {
  const [analyte, setAnalyte] = useState('cdom');
  const [stepIdx, setStepIdx] = useState(0);
  const [running, setRunning] = useState(false);

  const steps = buildWorkflow(analyte);
  const step = steps[stepIdx];

  const doMeasure = async () => {
    if (step.phase === 'done') { setStepIdx(0); return; }
    setRunning(true);
    try {
      // Push the reference pair to the device. The firmware's
      // fluorometry_calibrate() adjusts the PLS offset for this analyte.
      await ble.calibrate(ANALYTE_IDS[analyte], step.refValue);
      Alert.alert('Calibration step complete',
        `${CALIBRATION_STANDARDS[analyte].label} @ ${step.label} recorded.`);
      setStepIdx(i => i + 1);
    } catch (e) {
      Alert.alert('Calibration error', String(e.message || e));
    } finally {
      setRunning(false);
    }
  };

  const reset = () => { setStepIdx(0); };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Calibration</Text>
      <Text style={styles.sub}>Guided blank + standard calibration for each analyte. Follow on-screen prompts and immerse the sonde in each standard in turn.</Text>

      <Text style={styles.fieldLabel}>Analyte</Text>
      <View style={styles.pickerWrap}>
        <Picker selectedValue={analyte} onValueChange={v => { setAnalyte(v); setStepIdx(0); }} style={styles.picker}>
          {ANALYTES.map(a => <Picker.Item key={a.key} label={CALIBRATION_STANDARDS[a.key].label} value={a.key} />)}
        </Picker>
      </View>

      <Text style={styles.fieldLabel}>Step {stepIdx + 1} of {steps.length}</Text>
      <View style={styles.stepCard}>
        <Text style={styles.phase}>{phaseLabel(step.phase)}</Text>
        <Text style={styles.stdLabel}>{step.label}</Text>
        {step.phase !== 'done' && (
          <Text style={styles.refValue}>Reference: {step.refValue} {ANALYTES.find(a => a.key === analyte).unit}</Text>
        )}
      </View>

      <View style={styles.buttonRow}>
        <Button title={step.phase === 'done' ? 'Restart' : 'Measure & record'} onPress={doMeasure} disabled={running} color={theme.colors.accent} />
        <Button title="Reset"    onPress={reset}    color={theme.colors.accentDim} />
      </View>

      <Text style={styles.progressLabel}>Workflow:</Text>
      {steps.map((s, i) => (
        <Text key={i} style={[styles.stepItem, { color: i === stepIdx ? theme.colors.accent : theme.colors.textDim, fontWeight: i === stepIdx ? '700' : '400' }]}>
          {i + 1}. {s.label}
        </Text>
      ))}
    </View>
  );
}

function phaseLabel(p) {
  switch (p) {
    case 'blank': return 'BLANK';
    case 'point': return 'STANDARD';
    case 'done':  return 'COMPLETE';
    default: return p;
  }
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: theme.colors.bg, padding: theme.spacing.md },
  title: { color: theme.colors.text, fontSize: 22, fontWeight: '700' },
  sub: { color: theme.colors.textDim, fontSize: 12, marginBottom: theme.spacing.md },
  fieldLabel: { color: theme.colors.textDim, fontSize: 11, textTransform: 'uppercase', marginTop: 8 },
  pickerWrap: { backgroundColor: theme.colors.surface, borderRadius: theme.radius.md, marginTop: 4 },
  picker: { color: theme.colors.text, height: 44 },
  stepCard: { backgroundColor: theme.colors.surface, borderRadius: theme.radius.md, padding: theme.spacing.md, marginVertical: theme.spacing.sm },
  phase: { color: theme.colors.accent, fontSize: 11, fontWeight: '700', letterSpacing: 1.5 },
  stdLabel: { color: theme.colors.text, fontSize: 18, fontWeight: '600', marginTop: 4 },
  refValue: { color: theme.colors.textDim, fontSize: 12, marginTop: 4, fontFamily: theme.fonts.mono },
  buttonRow: { flexDirection: 'row', gap: 8, justifyContent: 'center', marginVertical: 10 },
  progressLabel: { color: theme.colors.textDim, fontSize: 11, textTransform: 'uppercase', marginTop: 8, marginBottom: 4 },
  stepItem: { fontSize: 12, paddingVertical: 2, fontFamily: theme.fonts.mono },
});