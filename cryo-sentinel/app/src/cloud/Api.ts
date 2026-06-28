/*
 * src/cloud/Api.ts — Cloud REST + WebSocket client for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 *
 * The backend is a thin FastAPI service over Postgres + TimescaleDB. This
 * module wraps the REST endpoints and the WebSocket alarm stream, and falls
 * back to a local mock when the backend URL is not configured (useful for
 * demos and for the open-source release where the user has not yet stood
 * up the cloud).
 */
import { DewarReading, AuditRecord, EscalationEntry } from '../types';

const API_BASE = (process?.env?.EXPO_PUBLIC_CRYO_API ?? '');
const WS_URL   = (process?.env?.EXPO_PUBLIC_CRYO_WS  ?? '');

/* ---- REST ----------------------------------------------------------- */
export async function getFleet(): Promise<DewarReading[]> {
  if (!API_BASE) return mockFleet();
  const r = await fetch(`${API_BASE}/v1/dewars`, { headers: jsonHeaders() });
  if (!r.ok) throw new Error(`fleet HTTP ${r.status}`);
  return (await r.json()) as DewarReading[];
}

export async function getDewarHistory(
  dewarId: string, rangeHours: number
): Promise<DewarReading[]> {
  if (!API_BASE) return mockHistory(dewarId, rangeHours);
  const r = await fetch(
    `${API_BASE}/v1/dewars/${dewarId}/history?hours=${rangeHours}`,
    { headers: jsonHeaders() });
  if (!r.ok) throw new Error(`history HTTP ${r.status}`);
  return (await r.json()) as DewarReading[];
}

export async function getAuditLog(dewarId: string): Promise<AuditRecord[]> {
  if (!API_BASE) return mockAudit(dewarId);
  const r = await fetch(`${API_BASE}/v1/dewars/${dewarId}/log`,
    { headers: jsonHeaders() });
  if (!r.ok) throw new Error(`log HTTP ${r.status}`);
  return (await r.json()) as AuditRecord[];
}

export async function getEscalation(dewarId: string): Promise<EscalationEntry[]> {
  if (!API_BASE) return mockEscalation();
  const r = await fetch(`${API_BASE}/v1/dewars/${dewarId}/escalation`,
    { headers: jsonHeaders() });
  if (!r.ok) throw new Error(`escalation HTTP ${r.status}`);
  return (await r.json()) as EscalationEntry[];
}

export async function putEscalation(
  dewarId: string, chain: EscalationEntry[]
): Promise<void> {
  if (!API_BASE) return;
  await fetch(`${API_BASE}/v1/dewars/${dewarId}/escalation`, {
    method: 'PUT',
    headers: jsonHeaders(),
    body: JSON.stringify(chain),
  });
}

export async function ackAlarm(
  dewarId: string, technicianId: string, note: string
): Promise<void> {
  if (!API_BASE) return;
  await fetch(`${API_BASE}/v1/dewars/${dewarId}/ack`, {
    method: 'POST',
    headers: jsonHeaders(),
    body: JSON.stringify({ technicianId, note }),
  });
}

/* ---- WebSocket alarm stream ----------------------------------------- */
type AlarmHandler = (dewarId: string, state: string, reason: string) => void;

export function openAlarmStream(onAlarm: AlarmHandler): () => void {
  if (!WS_URL) {
    // Demo mode: emit a synthetic alarm every 30 s so the inbox is alive.
    const t = setInterval(() => {
      onAlarm('DW-004', 'CRITICAL', 'level_low_crit');
    }, 30000);
    return () => clearInterval(t);
  }
  const ws = new WebSocket(WS_URL);
  ws.onmessage = (ev) => {
    try {
      const m = JSON.parse(ev.data);
      onAlarm(m.dewarId, m.state, m.reason);
    } catch { /* ignore malformed */ }
  };
  return () => ws.close();
}

function jsonHeaders(): HeadersInit {
  return { 'Content-Type': 'application/json' };
}

/* ---- Mock data (no backend configured) ------------------------------ */
function mockFleet(): DewarReading[] {
  const now = Date.now() / 1000;
  return [
    { dewarId: 'DW-001', label: 'IVF Lab A', state: 'OK', reason: 'none',
      levelPct: 78, levelRatePerHour: 0.1, rtdTopC: -120, rtdMidC: -170,
      rtdBotC: -192, gradientSlope: 72, tiltDeg: 1.2, vibRmsG: 0.01,
      ambientC: 21, ambientRh: 42, battPct: 99, mainsPresent: true,
      lidOpen: false, enclosureOpen: false, lastHeartbeatEpoch: now,
      serial: 'TW-XC-3492' },
    { dewarId: 'DW-002', label: 'IVF Lab B', state: 'WATCH', reason: 'mains_lost',
      levelPct: 64, levelRatePerHour: 0.3, rtdTopC: -118, rtdMidC: -168,
      rtdBotC: -190, gradientSlope: 72, tiltDeg: 1.5, vibRmsG: 0.02,
      ambientC: 22, ambientRh: 45, battPct: 88, mainsPresent: false,
      lidOpen: false, enclosureOpen: false, lastHeartbeatEpoch: now - 120,
      serial: 'TW-XC-3493' },
    { dewarId: 'DW-003', label: 'Stem-cell bank 1', state: 'WARN',
      reason: 'lid_open', levelPct: 41, levelRatePerHour: 1.2,
      rtdTopC: -90, rtdMidC: -150, rtdBotC: -185, gradientSlope: 95,
      tiltDeg: 3.0, vibRmsG: 0.05, ambientC: 24, ambientRh: 50,
      battPct: 95, mainsPresent: true, lidOpen: true,
      enclosureOpen: false, lastHeartbeatEpoch: now - 300,
      serial: 'CR-180-007' },
    { dewarId: 'DW-004', label: 'Stem-cell bank 2', state: 'CRITICAL',
      reason: 'level_low_crit', levelPct: 17, levelRatePerHour: 2.4,
      rtdTopC: -60, rtdMidC: -110, rtdBotC: -160, gradientSlope: 100,
      tiltDeg: 2.0, vibRmsG: 0.03, ambientC: 23, ambientRh: 48,
      battPct: 91, mainsPresent: true, lidOpen: false,
      enclosureOpen: false, lastHeartbeatEpoch: now - 60,
      serial: 'CR-180-008' },
  ];
}

function mockHistory(_id: string, hours: number): DewarReading[] {
  const out: DewarReading[] = [];
  const now = Date.now() / 1000;
  const n = Math.min(96, hours * 4);   // 15-min spacing
  for (let i = n; i >= 0; i--) {
    const t = now - i * 900;
    const level = 70 - 0.08 * (n - i) + Math.sin(i / 6) * 1.5;
    out.push({
      dewarId: _id, label: '', state: level < 35 ? 'WARN' : 'OK',
      reason: level < 35 ? 'level_low_warn' : 'none',
      levelPct: level, levelRatePerHour: 0.2,
      rtdTopC: -120 + (1 - level / 100) * 60,
      rtdMidC: -170 + (1 - level / 100) * 30,
      rtdBotC: -192 + (1 - level / 100) * 10,
      gradientSlope: 72 - (1 - level / 100) * 30,
      tiltDeg: 1.0 + Math.sin(i / 8) * 0.3,
      vibRmsG: 0.01 + Math.random() * 0.02,
      ambientC: 21 + Math.sin(i / 12) * 1.5,
      ambientRh: 44 + Math.sin(i / 10) * 4,
      battPct: 95 - (n - i) * 0.05,
      mainsPresent: true, lidOpen: false, enclosureOpen: false,
      lastHeartbeatEpoch: t, serial: '',
    });
  }
  return out;
}

function mockAudit(_id: string): AuditRecord[] {
  const now = Math.floor(Date.now() / 60000);
  return [
    { seq: 101, epochMin: now - 720, event: 'BOOT', aux: 0 },
    { seq: 102, epochMin: now - 719, event: 'CAL_DONE', aux: 4 },
    { seq: 103, epochMin: now - 120, event: 'STATE', aux: 0x00010009 },
    { seq: 104, epochMin: now - 60,  event: 'ALARM_SEND', aux: 0x00010001 },
    { seq: 105, epochMin: now - 30,  event: 'ALARM_ACK', aux: 1234 },
    { seq: 106, epochMin: now - 15,  event: 'HEARTBEAT', aux: 0 },
    { seq: 107, epochMin: now - 5,   event: 'LID_OPEN', aux: 0 },
  ];
}

function mockEscalation(): EscalationEntry[] {
  return [
    { technicianId: '1234', name: 'Dr. Alice Tanaka', order: 1,
      push: true, sms: true, email: true, call: false, delayMin: 0 },
    { technicianId: '1235', name: 'Bob Chen (on-call)', order: 2,
      push: true, sms: true, email: true, call: true, delayMin: 10 },
    { technicianId: '1236', name: 'Lab Director', order: 3,
      push: true, sms: false, email: true, call: true, delayMin: 30 },
  ];
}