/**
 * mqtt.ts - MQTT client for remote Pollen Scout stations
 *
 * Subscribes to `pollenscout/<station-id>/state` JSON topics.
 * Used when a station is not reachable over BLE (urban gateway
 * deployments).
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import { StationState } from './types';

const BROKER_URL = 'mqtts://broker.pollenscout.example:8883';
let client: any = null;

export async function connect(stationId: string): Promise<void> {
  /* In production uses the `mqtt` npm package:
   *   client = mqtt.connect(BROKER_URL, { clientId: `app-${stationId}` });
   */
  console.log(`[MQTT] connect ${BROKER_URL} for ${stationId}`);
}

export async function subscribe(stationId: string): Promise<void> {
  const topic = `pollenscout/${stationId}/state`;
  console.log(`[MQTT] subscribe ${topic}`);
  if (!client) await connect(stationId);
}

export async function pollState(stationId: string): Promise<StationState> {
  console.log(`[MQTT] pollState ${stationId}`);
  const { makeDemoStation } = await import('./context/StationContext');
  const s = makeDemoStation();
  s.stationId = stationId;
  s.name = `Remote ${stationId}`;
  s.isLocal = false;
  return s;
}

export function onStateUpdate(
  stationId: string,
  cb: (s: StationState) => void,
): () => void {
  /* In production: client.on('message', (topic, payload) => ...). */
  const id = setInterval(async () => {
    const s = await pollState(stationId);
    cb(s);
  }, 30_000);
  return () => clearInterval(id);
}