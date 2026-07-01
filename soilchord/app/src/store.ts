/**
 * store.ts — lightweight global store + SQLite mirror
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * We keep a simple in-memory store with subscriptions and mirror everything
 * to SQLite for offline operation. No Redux — this is a field app with a
 * small state surface.
 */
import { useState, useEffect, useCallback } from 'react';
import * as SQLite from 'expo-sqlite';
import { Probe, listProbes, getProbeHistory } from './api';
import { UplinkFrame } from './proto';

const db = SQLite.openDatabase('soilchord.db');

let sProbes: Probe[] = [];
let sHistory: Record<string, UplinkFrame[]> = {};
let sAlerts: { probeId: string; chord: number; seq: number; kind: string; ts: number }[] = [];

type Listener = () => void;
const listeners = new Set<Listener>();
function emit() { listeners.forEach((l) => l()); }

export function useStore() {
  const [, setTick] = useState(0);
  useEffect(() => {
    const l = () => setTick((t) => t + 1);
    listeners.add(l);
    return () => { listeners.delete(l); };
  }, []);
  return { probes: sProbes, history: sHistory, alerts: sAlerts, refresh };
}

export async function refresh() {
  const probes = await listProbes();
  sProbes = probes;
  for (const p of probes) {
    const hist = await getProbeHistory(p.id, Date.now() - 7 * 24 * 3600 * 1000);
    sHistory[p.id] = hist;
    // New alerts
    for (const f of hist) {
      f.chords.forEach((c, i) => {
        if (c.flags & 0x04) {
          sAlerts.push({ probeId: p.id, chord: i, seq: f.seq, kind: 'alert', ts: f.unixTime * 1000 });
        }
      });
    }
    sAlerts = sAlerts.filter((a, idx) => sAlerts.findIndex((b) => b.seq === a.seq && b.chord === a.chord && b.probeId === a.probeId) === idx);
  }
  persist();
  emit();
}

export function getProbe(id: string): Probe | undefined {
  return sProbes.find((p) => p.id === id);
}

export function getHistory(id: string): UplinkFrame[] {
  return sHistory[id] ?? [];
}

export function getAlerts(): typeof sAlerts {
  return sAlerts;
}

function persist() {
  db.transaction((tx) => {
    tx.executeSql(
      `CREATE TABLE IF NOT EXISTS frames (
        probe_id TEXT, seq INTEGER, ts INTEGER, payload TEXT,
        PRIMARY KEY (probe_id, seq)
      );`
    );
    for (const p of sProbes) {
      for (const f of sHistory[p.id] ?? []) {
        tx.executeSql(
          `INSERT OR REPLACE INTO frames (probe_id, seq, ts, payload) VALUES (?,?,?,?);`,
          [p.id, f.seq, f.unixTime, JSON.stringify(f)]
        );
      }
    }
  });
}

export function riskScoreForProbe(probeId: string): number {
  const hist = sHistory[probeId] ?? [];
  if (hist.length < 2) return 0;
  const recent = hist.slice(-12);
  let score = 0;
  // Q-drop across consecutive cycles (levee piping signature)
  for (let i = 1; i < recent.length; i++) {
    for (let c = 0; c < 6; c++) {
      const dq = recent[i].chords[c].qFreq - recent[i - 1].chords[c].qFreq;
      if (dq < -10) score += 8;
      if (dq < -25) score += 12;
    }
  }
  // Absolute low-Q (saturated)
  const last = recent[recent.length - 1];
  for (let c = 0; c < 6; c++) {
    if (last.chords[c].qFreq < 5) score += 4;
  }
  // Alert flags
  for (const f of recent) {
    for (const c of f.chords) {
      if (c.flags & 0x04) score += 5;
    }
  }
  return Math.min(100, score);
}

export function moistureEstimate(qFreq: number, soilType: string): number {
  // Transfer functions (rough, calibration-dependent) — percent VWC
  // Soil-specific curves: qFreq (dry ~80) → 0% VWC ; qFreq ~2 → ~50% VWC
  const dryQ = 80;
  const wetQ = 2;
  const maxVwc: Record<string, number> = {
    sand: 35, loam: 45, silt: 50, clay: 55,
  };
  const mx = maxVwc[soilType] ?? 45;
  const x = (dryQ - qFreq) / (dryQ - wetQ);
  return Math.max(0, Math.min(1, x)) * mx;
}