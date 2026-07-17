/**
 * api.ts — HTTP/WebSocket client for the Stridophone device.
 *
 * Author : jayis1
 * License: MIT
 *
 * All endpoints documented in README §5 are surfaced here. The client
 * auto-discovers devices via mDNS (the native zeroconf module is wired
 * but optional; for the Expo preview we fall back to a manually entered IP).
 */
export interface DeviceInfo {
  name: string;
  fwVersion: string;
  battery: number;
  uptime: number;
  wifiUp: boolean;
}

export interface StridoEvent {
  id: number;
  t: number;
  sp: number;
  conf: number;
  az: number;
  T: number;
  RH: number;
  species?: string;
}

export interface SpectrogramRow {
  ts: number;
  bins: number[];   // 32 log-scaled dB values (0..255)
}

export interface DeviceConfig {
  sensitivity: number;       // 0..100
  confidenceThreshold: number;
  wifiSsid?: string;
  wifiPass?: string;
  alertRules: AlertRule[];
}

export interface AlertRule {
  speciesId: number;
  pulseRateMin?: number;
  durationMin?: number;      // minutes the condition must hold
  notify: boolean;
}

const SPECIES_NAMES: Record<number, string> = {
  0: 'ambient', 1: 'Gryllus campestris', 2: 'Acheta domesticus',
  3: 'Oecanthus spp.', 4: 'Tibicen spp.', 5: 'Magicicada spp.',
  6: 'Locusta migratoria', 7: 'Schistocerca gregaria',
  8: 'Reticulitermes flavipes', 9: 'Coptotermes formosanus',
  10: 'Camponotus spp.', 11: 'Hylotrupes bajulus',
  12: 'Anobium punctatum', 13: 'Sitophilus granarius',
  14: 'Sitophilus oryzae', 15: 'Tribolium castaneum',
  16: 'Plodia interpunctella', 17: 'Cydia pomonella',
  18: 'Lygus spp.', 19: 'Empoasca spp.', 20: 'Nezara viridula',
  21: 'Halictus spp. (benign)', 22: 'Achroia grisella',
  23: 'Tineola bisselliella', 24: 'Lasius spp.', 25: 'unknown',
};

export function speciesName(id: number): string {
  return SPECIES_NAMES[id] ?? `species#${id}`;
}

export class StridoClient {
  baseUrl: string;
  private ws: WebSocket | null = null;

  constructor(ip: string) {
    this.baseUrl = `http://${ip}`;
  }

  async getInfo(): Promise<DeviceInfo> {
    const r = await fetch(`${this.baseUrl}/api/info`);
    return r.json();
  }

  async getEvents(since: number = 0, limit: number = 100): Promise<StridoEvent[]> {
    const r = await fetch(`${this.baseUrl}/api/events?since=${since}&limit=${limit}`);
    const arr = await r.json();
    return arr.map((e: any) => ({ ...e, species: speciesName(e.sp) }));
  }

  async getWavUrl(id: number): Promise<string> {
    return `${this.baseUrl}/api/wav/${id}`;
  }

  async getConfig(): Promise<DeviceConfig> {
    const r = await fetch(`${this.baseUrl}/api/config`);
    return r.json();
  }

  async putConfig(cfg: Partial<DeviceConfig>): Promise<void> {
    await fetch(`${this.baseUrl}/api/config`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(cfg),
    });
  }

  async ota(file: Blob): Promise<void> {
    await fetch(`${this.baseUrl}/api/ota`, { method: 'POST', body: file });
  }

  /** Open a WebSocket that delivers live events + spectrogram rows. */
  openStream(onEvent: (e: StridoEvent) => void,
             onSpec:  (s: SpectrogramRow) => void): void {
    const wsUrl = this.baseUrl.replace(/^http/, 'ws') + '/api/stream';
    this.ws = new WebSocket(wsUrl);
    this.ws.onmessage = (ev) => {
      try {
        if (typeof ev.data === 'string') {
          const obj = JSON.parse(ev.data);
          if (obj.type === 'event') onEvent({ ...obj, species: speciesName(obj.sp) });
          else if (obj.type === 'spec') onSpec({ ts: obj.ts, bins: obj.bins });
        }
      } catch { /* ignore malformed frames */ }
    };
  }

  closeStream(): void {
    this.ws?.close();
    this.ws = null;
  }
}