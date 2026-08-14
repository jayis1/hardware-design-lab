/*
 * api.ts — MusselWatch gateway client
 *
 * Talks to the MusselWatch LoRaWAN gateway (TTN / ChirpStack HTTP
 * integration or a simple MQTT-to-HTTP bridge).  Polls node telemetry
 * and alerts and pushes node configuration back to the gateway.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

import { AlertEvent, Node, Telemetry, ChannelState, AppConfig } from './types';
import { ANOMALY_CLAMP, ANOMALY_STALL } from './types';

const DEFAULT_URL = 'https://musselwatch.jayis1.dev/api';

/* ---- In-memory mock state (used when no gateway is reachable) ---- */

const MOCK_NODES: Node[] = [
  {
    nodeId: 'a1',
    label: 'Riverside A',
    river: 'River Itchen',
    reach: 'Reach 3 — upstream of treatment plant',
    lastSeenS: 30,
    species: 'Unio pictorum',
    telemetry: {
      nodeId: 'a1',
      seq: 1042,
      batteryMv: 3980,
      solarMv: 4100,
      waterTempC10: 142,
      chargerState: 1,
      flags: 0,
      activeChannels: 0xff,
      maxAnomaly: 8,
      uptimeS: 486200,
      receivedAt: Date.now(),
    },
    channels: buildMockChannels(8, 'a1'),
  },
  {
    nodeId: 'b7',
    label: 'Outlet B',
    river: 'River Itchen',
    reach: 'Reach 7 — downstream of treatment plant',
    lastSeenS: 45,
    species: 'Anodonta anatina',
    telemetry: {
      nodeId: 'b7',
      seq: 988,
      batteryMv: 3720,
      solarMv: 5400,
      waterTempC10: 146,
      chargerState: 2,
      flags: 0x02,
      activeChannels: 0x3f,
      maxAnomaly: 65,
      uptimeS: 312900,
      receivedAt: Date.now(),
    },
    channels: buildMockChannels(6, 'b7'),
  },
];

function buildMockChannels(n: number, seed: string): ChannelState[] {
  const speciesPool = ['Unio pictorum', 'Anodonta anatina', 'Margaritifera margaritifera'];
  const out: ChannelState[] = [];
  for (let i = 0; i < n; i++) {
    const flag = (seed === 'b7' && i === 2) ? ANOMALY_CLAMP : 0;
    const score = flag ? 65 : Math.floor(Math.random() * 20) + 5;
    out.push({
      channel: i,
      rawHall: 2100 + Math.floor(Math.random() * 400),
      rawBaseline: 2050,
      gapeUm: flag ? 20 : 300 + Math.floor(Math.random() * 600),
      activityScore: flag ? 2 : 15 + Math.floor(Math.random() * 30),
      anomalyFlag: flag,
      anomalyScore: score,
      lastEventS: flag ? 120 : 0,
      species: speciesPool[i % speciesPool.length],
      location: `Cage ${String.fromCharCode(65 + i)}`,
    });
  }
  return out;
}

const MOCK_ALERTS: AlertEvent[] = [
  {
    id: 'al-001',
    nodeId: 'b7',
    channel: 2,
    level: 'warning',
    anomalyFlag: ANOMALY_CLAMP,
    gapeUm: 20,
    activityScore: 2,
    waterTempC10: 146,
    timestamp: Date.now() - 1000 * 60 * 4,
    acknowledged: false,
    note: 'Sustained shell closure on 1 mussel for > 60 s.',
  },
  {
    id: 'al-002',
    nodeId: 'a1',
    channel: 5,
    level: 'advisory',
    anomalyFlag: ANOMALY_STALL,
    gapeUm: 410,
    activityScore: 0,
    waterTempC10: 142,
    timestamp: Date.now() - 1000 * 60 * 35,
    acknowledged: true,
    note: 'Gape variability dropped to zero; possible sediment smothering.',
  },
];

/* ---- Client class ----------------------------------------------- */

export class MusselWatchClient {
  private baseUrl: string;
  private useMock: boolean;

  constructor(config: AppConfig) {
    this.baseUrl = config.gatewayUrl || DEFAULT_URL;
    this.useMock = this.baseUrl.startsWith('mock');
  }

  async getNodes(): Promise<Node[]> {
    if (this.useMock) {
      await delay(200);
      return MOCK_NODES;
    }
    try {
      const r = await fetch(`${this.baseUrl}/nodes`, { method: 'GET' });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      return (await r.json()) as Node[];
    } catch (e) {
      console.warn('getNodes failed, falling back to mock:', e);
      return MOCK_NODES;
    }
  }

  async getAlerts(): Promise<AlertEvent[]> {
    if (this.useMock) {
      await delay(150);
      return MOCK_ALERTS;
    }
    try {
      const r = await fetch(`${this.baseUrl}/alerts`, { method: 'GET' });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      return (await r.json()) as AlertEvent[];
    } catch (e) {
      console.warn('getAlerts failed, falling back to mock:', e);
      return MOCK_ALERTS;
    }
  }

  async acknowledgeAlert(id: string, note: string): Promise<boolean> {
    if (this.useMock) {
      const a = MOCK_ALERTS.find((x) => x.id === id);
      if (a) { a.acknowledged = true; a.note = note; }
      return true;
    }
    const r = await fetch(`${this.baseUrl}/alerts/${id}/ack`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ note }),
    });
    return r.ok;
  }

  async getChannelHistory(nodeId: string, channel: number, hours: number): Promise<number[]> {
    if (this.useMock) {
      await delay(100);
      // synthetic 4-hour gape trace with diurnal rhythm
      const n = hours * 60;
      const out: number[] = [];
      for (let i = 0; i < n; i++) {
        const t = i / 60;
        const diurnal = 300 + 200 * Math.sin((t / 24) * Math.PI * 2);
        const noise = (Math.random() - 0.5) * 80;
        out.push(Math.max(0, Math.round(diurnal + noise)));
      }
      return out;
    }
    const r = await fetch(
      `${this.baseUrl}/nodes/${nodeId}/channels/${channel}/history?hours=${hours}`,
      { method: 'GET' });
    return (await r.json()) as number[];
  }

  async setNodeLabel(nodeId: string, label: string): Promise<boolean> {
    if (this.useMock) {
      const n = MOCK_NODES.find((x) => x.nodeId === nodeId);
      if (n) n.label = label;
      return true;
    }
    const r = await fetch(`${this.baseUrl}/nodes/${nodeId}/label`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ label }),
    });
    return r.ok;
  }

  async requestCalibration(nodeId: string): Promise<boolean> {
    if (this.useMock) return true;
    const r = await fetch(`${this.baseUrl}/nodes/${nodeId}/calibrate`, {
      method: 'POST',
    });
    return r.ok;
  }
}

function delay(ms: number): Promise<void> {
  return new Promise((res) => setTimeout(res, ms));
}