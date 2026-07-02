/**
 * api.ts — TensilGuard Field gateway API client
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Talks to the bridge gateway (one per structure, e.g. a Raspberry Pi with
 * a LoRa HAT) over REST + WebSocket. The gateway holds the decoded LoRa
 * packets and serves history. The app also works fully offline against
 * the local SQLite cache; when connectivity returns, the store syncs.
 */

import { DecodedUplink, decodeUplink } from './proto';

export interface NodeSummary {
  nodeId: string;
  label: string;
  cableId: string;
  tMagKn: number;
  tVibKn: number;
  dtKn: number;
  f1Hz: number;
  tempC: number;
  batteryPct: number;
  panelMv: number;
  rssiDb: number;
  hops: number;
  urgent: boolean;
  aeCount: number;
  lastSeen: number; // unix seconds
  flags: number;
}

export interface NodeHistory {
  timestamps: number[];
  tMag: number[];
  tVib: number[];
  dt: number[];
  temp: number[];
  battery: number[];
  f1: number[];
}

export interface AeEventLog {
  unixTime: number;
  peakUv: number;
  riseUs: number;
  durUs: number;
  centroidKhz: number;
  score: number;
  classification: number;
  nodeId: string;
}

export interface MeshLink {
  fromNode: string;
  toNode: string;
  rssiDb: number;
  snrDb: number;
  hops: number;
}

const GATEWAY_URL = 'http://gateway.local:8080';
const WS_URL = 'ws://gateway.local:8080/ws';

/** Fetch the current node summary list from the gateway. */
export async function fetchNodes(): Promise<NodeSummary[]> {
  try {
    const r = await fetch(`${GATEWAY_URL}/api/nodes`, { method: 'GET' });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    return await r.json() as NodeSummary[];
  } catch (e) {
    // offline — return empty; the store will serve cached data
    return [];
  }
}

/** Fetch history for a single node over a time range. */
export async function fetchNodeHistory(
  nodeId: string,
  fromUnix: number,
  toUnix: number,
): Promise<NodeHistory | null> {
  try {
    const r = await fetch(
      `${GATEWAY_URL}/api/nodes/${nodeId}/history?from=${fromUnix}&to=${toUnix}`,
      { method: 'GET' },
    );
    if (!r.ok) return null;
    return await r.json() as NodeHistory;
  } catch {
    return null;
  }
}

/** Fetch recent AE events for a node. */
export async function fetchAeEvents(nodeId: string, limit = 50): Promise<AeEventLog[]> {
  try {
    const r = await fetch(`${GATEWAY_URL}/api/nodes/${nodeId}/ae?limit=${limit}`);
    if (!r.ok) return [];
    return await r.json() as AeEventLog[];
  } catch {
    return [];
  }
}

/** Fetch mesh link quality table. */
export async function fetchMeshLinks(): Promise<MeshLink[]> {
  try {
    const r = await fetch(`${GATEWAY_URL}/api/mesh/links`);
    if (!r.ok) return [];
    return await r.json() as MeshLink[];
  } catch {
    return [];
  }
}

/** Send a downlink command (e.g. set interval) via the gateway. */
export async function sendDownlink(nodeId: string, command: Uint8Array): Promise<boolean> {
  try {
    const body = JSON.stringify({
      nodeId,
      payload: Array.from(command),
    });
    const r = await fetch(`${GATEWAY_URL}/api/nodes/${nodeId}/downlink`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body,
    });
    return r.ok;
  } catch {
    return false;
  }
}

/** Export measurement history as CSV. */
export async function exportCsv(nodeId: string, fromUnix: number, toUnix: number): Promise<string> {
  try {
    const r = await fetch(
      `${GATEWAY_URL}/api/nodes/${nodeId}/export?from=${fromUnix}&to=${toUnix}&format=csv`,
    );
    if (!r.ok) return '';
    return await r.text();
  } catch {
    return '';
  }
}

/**
 * WebSocket subscription for real-time uplinks. Calls onPacket for every
 * decoded uplink the gateway forwards. Returns a close function.
 */
export function subscribeLive(onPacket: (uplink: DecodedUplink) => void): () => void {
  let ws: WebSocket | null = null;
  let closed = false;
  const connect = () => {
    if (closed) return;
    ws = new WebSocket(WS_URL);
    ws.onmessage = (ev) => {
      try {
        const data = JSON.parse(ev.data as string);
        const raw = new Uint8Array(data.payload);
        const decoded = decodeUplink(raw);
        if (decoded) onPacket(decoded);
      } catch { /* ignore malformed */ }
    };
    ws.onclose = () => {
      if (!closed) setTimeout(connect, 3000); // auto-reconnect
    };
  };
  connect();
  return () => { closed = true; ws?.close(); };
}