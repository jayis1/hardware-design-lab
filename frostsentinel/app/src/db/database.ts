// src/db/database.ts — Local SQLite database for FrostSentinel app
//
// Stores mesh node data, historical time-series, and frost events
// for offline viewing and CSV export.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import SQLite from 'react-native-sqlite-storage';

const DB_NAME = 'frostsentinel.db';
const DB_VERSION = '1.0';
const DB_DISPLAY_NAME = 'FrostSentinel Database';
const DB_SIZE = 200000;

export interface NodeRecord {
  nodeId: number;
  label: string;
  meshRole: number;
  lastRfri: number;
  lastTwet: number;
  lastDeltaRad: number;
  lastAeStatus: number;
  lastSeen: number;  // epoch seconds
}

export interface SampleRecord {
  timestamp: number;
  nodeId: number;
  rfri: number;
  tair: number;
  tsky: number;
  twet: number;
  deltaRad: number;
  leafWet: number;
  aeStatus: number;
  aeEnergy: number;
  batteryPct: number;
}

class Database {
  private db: SQLite.SQLiteDatabase | null = null;

  async open(): Promise<void> {
    this.db = await SQLite.openDatabase(
      DB_NAME, DB_VERSION, DB_DISPLAY_NAME, DB_SIZE
    );
    await this.createTables();
  }

  private async createTables(): Promise<void> {
    if (!this.db) return;

    await this.db.executeSql(`
      CREATE TABLE IF NOT EXISTS nodes (
        node_id    INTEGER PRIMARY KEY,
        label      TEXT DEFAULT '',
        mesh_role  INTEGER DEFAULT 0,
        last_rfri  REAL DEFAULT 0,
        last_twet  REAL DEFAULT 0,
        last_delta REAL DEFAULT 0,
        last_ae    INTEGER DEFAULT 0,
        last_seen  INTEGER DEFAULT 0
      )
    `);

    await this.db.executeSql(`
      CREATE TABLE IF NOT EXISTS samples (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp   INTEGER NOT NULL,
        node_id     INTEGER NOT NULL,
        rfri        REAL,
        tair        REAL,
        tsky        REAL,
        twet        REAL,
        delta_rad   REAL,
        leaf_wet    INTEGER,
        ae_status   INTEGER,
        ae_energy   INTEGER,
        battery_pct INTEGER
      )
    `);

    await this.db.executeSql(`
      CREATE INDEX IF NOT EXISTS idx_samples_node_time
      ON samples (node_id, timestamp DESC)
    `);

    await this.db.executeSql(`
      CREATE TABLE IF NOT EXISTS events (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp   INTEGER NOT NULL,
        node_id     INTEGER NOT NULL,
        event_type  TEXT NOT NULL,
        rfri        REAL,
        ae_confirmed INTEGER DEFAULT 0,
        description TEXT
      )
    `);
  }

  async upsertNode(node: NodeRecord): Promise<void> {
    if (!this.db) return;
    await this.db.executeSql(
      `INSERT OR REPLACE INTO nodes
       (node_id, label, mesh_role, last_rfri, last_twet, last_delta, last_ae, last_seen)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [node.nodeId, node.label, node.meshRole, node.lastRfri,
       node.lastTwet, node.lastDeltaRad, node.lastAeStatus, node.lastSeen]
    );
  }

  async insertSample(s: SampleRecord): Promise<void> {
    if (!this.db) return;
    await this.db.executeSql(
      `INSERT INTO samples
       (timestamp, node_id, rfri, tair, tsky, twet, delta_rad, leaf_wet,
        ae_status, ae_energy, battery_pct)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [s.timestamp, s.nodeId, s.rfri, s.tair, s.tsky, s.twet,
       s.deltaRad, s.leafWet, s.aeStatus, s.aeEnergy, s.batteryPct]
    );
  }

  async getNodes(): Promise<NodeRecord[]> {
    if (!this.db) return [];
    const [results] = await this.db.executeSql('SELECT * FROM nodes ORDER BY node_id');
    const nodes: NodeRecord[] = [];
    for (let i = 0; i < results.rows.length; i++) {
      const r = results.rows.item(i);
      nodes.push({
        nodeId: r.node_id,
        label: r.label,
        meshRole: r.mesh_role,
        lastRfri: r.last_rfri,
        lastTwet: r.last_twet,
        lastDeltaRad: r.last_delta,
        lastAeStatus: r.last_ae,
        lastSeen: r.last_seen,
      });
    }
    return nodes;
  }

  async getSamples(nodeId: number, limit: number = 288): Promise<SampleRecord[]> {
    if (!this.db) return [];
    const [results] = await this.db.executeSql(
      'SELECT * FROM samples WHERE node_id = ? ORDER BY timestamp DESC LIMIT ?',
      [nodeId, limit]
    );
    const samples: SampleRecord[] = [];
    for (let i = 0; i < results.rows.length; i++) {
      const r = results.rows.item(i);
      samples.push({
        timestamp: r.timestamp,
        nodeId: r.node_id,
        rfri: r.rfri,
        tair: r.tair,
        tsky: r.tsky,
        twet: r.twet,
        deltaRad: r.delta_rad,
        leafWet: r.leaf_wet,
        aeStatus: r.ae_status,
        aeEnergy: r.ae_energy,
        batteryPct: r.battery_pct,
      });
    }
    return samples.reverse(); // chronological order for plotting
  }

  async insertEvent(timestamp: number, nodeId: number, eventType: string,
                    rfri: number, aeConfirmed: boolean, description: string): Promise<void> {
    if (!this.db) return;
    await this.db.executeSql(
      `INSERT INTO events (timestamp, node_id, event_type, rfri, ae_confirmed, description)
       VALUES (?, ?, ?, ?, ?, ?)`,
      [timestamp, nodeId, eventType, rfri, aeConfirmed ? 1 : 0, description]
    );
  }

  async getEvents(limit: number = 50): Promise<any[]> {
    if (!this.db) return [];
    const [results] = await this.db.executeSql(
      'SELECT * FROM events ORDER BY timestamp DESC LIMIT ?', [limit]
    );
    const events: any[] = [];
    for (let i = 0; i < results.rows.length; i++) {
      events.push(results.rows.item(i));
    }
    return events;
  }

  async exportToCSV(): Promise<string> {
    if (!this.db) return '';
    const [results] = await this.db.executeSql(
      'SELECT * FROM samples ORDER BY timestamp ASC'
    );
    let csv = 'timestamp,node_id,rfri,tair,tsky,twet,delta_rad,leaf_wet,ae_status,ae_energy,battery_pct\n';
    for (let i = 0; i < results.rows.length; i++) {
      const r = results.rows.item(i);
      csv += `${r.timestamp},${r.node_id},${r.rfri.toFixed(4)},${r.tair.toFixed(2)},${r.tsky.toFixed(2)},${r.twet.toFixed(2)},${r.delta_rad.toFixed(2)},${r.leaf_wet},${r.ae_status},${r.ae_energy},${r.battery_pct}\n`;
    }
    return csv;
  }

  async close(): Promise<void> {
    if (this.db) {
      await this.db.close();
      this.db = null;
    }
  }
}

export default new Database();