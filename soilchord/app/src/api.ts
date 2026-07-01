/**
 * api.ts — gateway API client for Soilchord Field
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Talks to the MQTT broker / REST gateway that bridges LoRa uplinks into
 * the cloud. We use fetch against a configurable gateway URL; the gateway
 * in turn decodes the binary uplink frames (see proto.ts) and exposes them
 * as JSON.
 */
import { decodeUplink, UplinkFrame, encodeDownlink, DL_CMD } from './proto';

const GATEWAY_URL = 'https://gw.soilchord.jayis1.example/api';

export interface Probe {
  id: string;
  name: string;
  site: string;
  lastSeq: number;
  lastTime: number;
  batteryMv: number;
  urgent: boolean;
  latest?: UplinkFrame;
}

export async function listProbes(): Promise<Probe[]> {
  try {
    const r = await fetch(`${GATEWAY_URL}/probes`, { method: 'GET' });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    return (await r.json()) as Probe[];
  } catch (e) {
    // Offline: return empty; the UI handles the empty state.
    return [];
  }
}

export async function getProbeHistory(probeId: string, since: number): Promise<UplinkFrame[]> {
  try {
    const r = await fetch(`${GATEWAY_URL}/probes/${probeId}/history?since=${since}`, { method: 'GET' });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    const raw = (await r.json()) as { frames: number[][] };
    return raw.frames.map((arr) => decodeUplink(new Uint8Array(arr))).filter((f): f is UplinkFrame => f !== null);
  } catch (e) {
    return [];
  }
}

export async function sendDownlink(probeId: string, cmd: number, arg: number): Promise<boolean> {
  try {
    const payload = encodeDownlink(cmd, arg);
    const r = await fetch(`${GATEWAY_URL}/probes/${probeId}/downlink`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/octet-stream' },
      body: payload as unknown as BodyInit,
    });
    return r.ok;
  } catch (e) {
    return false;
  }
}

export async function setIntervalCmd(probeId: string, seconds: number): Promise<boolean> {
  return sendDownlink(probeId, DL_CMD.SET_INTERVAL, seconds);
}

export async function calibrateBaseline(probeId: string): Promise<boolean> {
  return sendDownlink(probeId, DL_CMD.CALIBRATE_BASELINE, 0);
}

export async function requestBackfill(probeId: string, fromSeq: number): Promise<boolean> {
  return sendDownlink(probeId, DL_CMD.REQUEST_BACKFILL, fromSeq);
}

export async function forceMeasure(probeId: string): Promise<boolean> {
  return sendDownlink(probeId, DL_CMD.FORCE_MEASURE, 0);
}