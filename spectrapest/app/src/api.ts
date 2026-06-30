/**
 * api.ts — REST API client for SpectraPest gateway
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Communicates with the SpectraPest gateway node's REST API (served by
 * the ESP32-C3 co-processor over WiFi). Provides methods for fetching
 * detections, node status, pest pressure heatmaps, and configuration.
 */

import { DetectionEvent, SpectraPestNode, PestPressureGrid, GatewayStatus, AlertConfig } from './types';

const API_VERSION = 'v1';
const DEFAULT_TIMEOUT_MS = 10000;

export class SpectraPestAPI {
  private baseUrl: string;

  constructor(baseUrl: string = 'http://192.168.4.1') {
    this.baseUrl = baseUrl.replace(/\/$/, '');
  }

  private async request<T>(
    path: string,
    options: RequestInit = {},
    timeoutMs: number = DEFAULT_TIMEOUT_MS
  ): Promise<T> {
    const url = `${this.baseUrl}/api/${API_VERSION}${path}`;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), timeoutMs);

    try {
      const response = await fetch(url, {
        ...options,
        signal: controller.signal,
        headers: {
          'Content-Type': 'application/json',
          ...options.headers,
        },
      });

      if (!response.ok) {
        throw new Error(`API error ${response.status}: ${response.statusText}`);
      }

      return await response.json() as T;
    } catch (error) {
      if (error instanceof DOMException && error.name === 'AbortError') {
        throw new Error(`Request timed out after ${timeoutMs}ms`);
      }
      throw error;
    } finally {
      clearTimeout(timeout);
    }
  }

  async getGatewayStatus(): Promise<GatewayStatus> {
    return this.request<GatewayStatus>('/status');
  }

  async getNodes(): Promise<SpectraPestNode[]> {
    return this.request<SpectraPestNode[]>('/nodes');
  }

  async getNode(nodeId: string): Promise<SpectraPestNode> {
    return this.request<SpectraPestNode>(`/nodes/${nodeId}`);
  }

  async getNodeDetections(
    nodeId: string,
    limit: number = 50,
    offset: number = 0
  ): Promise<DetectionEvent[]> {
    return this.request<DetectionEvent[]>(
      `/nodes/${nodeId}/detections?limit=${limit}&offset=${offset}`
    );
  }

  async getDetections(
    from?: Date,
    to?: Date,
    species?: string
  ): Promise<DetectionEvent[]> {
    const params = new URLSearchParams();
    if (from) params.set('from', from.toISOString());
    if (to) params.set('to', to.toISOString());
    if (species) params.set('species', species);

    const query = params.toString() ? `?${params.toString()}` : '';
    return this.request<DetectionEvent[]>(`/detections${query}`);
  }

  async getHeatmap(
    species?: string,
    date?: Date
  ): Promise<PestPressureGrid> {
    const params = new URLSearchParams();
    if (species) params.set('species', species);
    if (date) params.set('date', date.toISOString().split('T')[0]);

    const query = params.toString() ? `?${params.toString()}` : '';
    return this.request<PestPressureGrid>(`/heatmap${query}`);
  }

  async updateNodeConfig(
    nodeId: string,
    config: Partial<SpectraPestNode['config']>
  ): Promise<void> {
    await this.request(`/nodes/${nodeId}/config`, {
      method: 'POST',
      body: JSON.stringify(config),
    });
  }

  async triggerOTA(nodeId: string): Promise<void> {
    await this.request(`/nodes/${nodeId}/ota`, {
      method: 'POST',
    });
  }

  async getAlerts(): Promise<AlertConfig[]> {
    return this.request<AlertConfig[]>('/alerts');
  }

  async createAlert(alert: Omit<AlertConfig, 'id' | 'created_at'>): Promise<AlertConfig> {
    return this.request<AlertConfig>('/alerts', {
      method: 'POST',
      body: JSON.stringify(alert),
    });
  }

  async deleteAlert(alertId: string): Promise<void> {
    await this.request(`/alerts/${alertId}`, {
      method: 'DELETE',
    });
  }

  async exportData(
    format: 'csv' | 'geojson' | 'isobus',
    from?: Date,
    to?: Date
  ): Promise<string> {
    const params = new URLSearchParams();
    params.set('format', format);
    if (from) params.set('from', from.toISOString());
    if (to) params.set('to', to.toISOString());

    const url = `${this.baseUrl}/api/${API_VERSION}/export?${params.toString()}`;
    const response = await fetch(url);
    return await response.text();
  }
}

export const defaultApi = new SpectraPestAPI();