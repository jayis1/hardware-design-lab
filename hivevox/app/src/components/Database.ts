/*
 * components/Database.ts — SQLite local store for HiveVox app
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Wraps expo-sqlite to persist hive sensor readings, alerts, and queued
 * downlinks. In production this syncs with a cloud backend (The Things
 * Network webhook → cloud function → device). For offline-first use,
 * all data is mirrored locally.
 */
import * as SQLite from 'expo-sqlite';
import type { HiveData, AlertEntry } from '../App';

const DB_NAME = 'hivevox.db';

export class HiveDatabase {
  private db: SQLite.SQLiteDatabase;

  constructor() {
    this.db = SQLite.openDatabase(DB_NAME);
    this.init();
  }

  private async init(): Promise<void> {
    await this.db.transactionAsync(async (tx) => {
      await tx.executeSql(
        `CREATE TABLE IF NOT EXISTS hives (
          deveui TEXT PRIMARY KEY,
          name TEXT NOT NULL,
          state INTEGER DEFAULT 6,
          dominant_hz INTEGER DEFAULT 0,
          r_hi INTEGER DEFAULT 0,
          cv_loud INTEGER DEFAULT 0,
          brood_temp_c INTEGER DEFAULT 0,
          humidity INTEGER DEFAULT 0,
          weight_g INTEGER DEFAULT 0,
          battery_mv INTEGER DEFAULT 0,
          flags INTEGER DEFAULT 0,
          last_seen INTEGER DEFAULT 0,
          rssi INTEGER DEFAULT 0
        )`
      );
      await tx.executeSql(
        `CREATE TABLE IF NOT EXISTS history (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          deveui TEXT NOT NULL,
          timestamp INTEGER NOT NULL,
          state INTEGER,
          dominant_hz INTEGER,
          brood_temp_c INTEGER,
          weight_g INTEGER,
          battery_mv INTEGER
        )`
      );
      await tx.executeSql(
        `CREATE TABLE IF NOT EXISTS alerts (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          deveui TEXT NOT NULL,
          hive_name TEXT,
          type TEXT,
          message TEXT,
          timestamp INTEGER,
          acknowledged INTEGER DEFAULT 0
        )`
      );
      await tx.executeSql(
        `CREATE TABLE IF NOT EXISTS downlink_queue (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          deveui TEXT NOT NULL,
          payload BLOB,
          queued_at INTEGER
        )`
      );
      await tx.executeSql(
        `CREATE INDEX IF NOT EXISTS idx_history_deveui ON history(deveui, timestamp)`
      );
      await tx.executeSql(
        `CREATE INDEX IF NOT EXISTS idx_alerts_ack ON alerts(acknowledged, timestamp)`
      );
    });
  }

  async getAllHives(): Promise<HiveData[]> {
    const result = await this.db.executeSql(
      `SELECT * FROM hives ORDER BY name`
    );
    return result.rows.map(this.rowToHive);
  }

  async getHiveHistory(deveui: string, limit: number = 500): Promise<any[]> {
    const result = await this.db.executeSql(
      `SELECT * FROM history WHERE deveui = ? ORDER BY timestamp DESC LIMIT ?`,
      [deveui, limit]
    );
    return result.rows;
  }

  async getUnacknowledgedAlerts(): Promise<AlertEntry[]> {
    const result = await this.db.executeSql(
      `SELECT * FROM alerts WHERE acknowledged = 0 ORDER BY timestamp DESC`
    );
    return result.rows.map(this.rowToAlert);
  }

  async addHive(deveui: string, name: string): Promise<void> {
    await this.db.executeSql(
      `INSERT OR REPLACE INTO hives (deveui, name) VALUES (?, ?)`,
      [deveui, name]
    );
  }

  /* Called when a new LoRaWAN uplink arrives (from TTN webhook poller). */
  async ingestUplink(deveui: string, payload: Uint8Array, rssi: number): Promise<void> {
    if (payload.length < 12) return;

    const state = payload[0];
    const dominantHz = payload[1] | (payload[2] << 8);
    const rHi = payload[3];
    const cvLoud = payload[4];
    const broodTempCenti = payload[5] | (payload[6] << 8);
    const humidity = payload[7];
    const weightDg = payload[8] | (payload[9] << 8);
    const batteryMv = payload[10] * 20;
    const flags = payload[11];
    const now = Math.floor(Date.now() / 1000);

    // Update hives table
    await this.db.executeSql(
      `UPDATE hives SET state=?, dominant_hz=?, r_hi=?, cv_loud=?,
        brood_temp_c=?, humidity=?, weight_g=?, battery_mv=?, flags=?,
        last_seen=?, rssi=? WHERE deveui=?`,
      [state, dominantHz, rHi, cvLoud, broodTempCenti, humidity,
       weightDg * 10, batteryMv, flags, now, rssi, deveui]
    );

    // Insert history point
    await this.db.executeSql(
      `INSERT INTO history (deveui, timestamp, state, dominant_hz, brood_temp_c, weight_g, battery_mv)
       VALUES (?, ?, ?, ?, ?, ?, ?)`,
      [deveui, now, state, dominantHz, broodTempCenti, weightDg * 10, batteryMv]
    );

    // Generate alerts based on state + flags
    const hive = await this.db.executeSql(`SELECT name FROM hives WHERE deveui=?`, [deveui]);
    const hiveName = hive.rows[0]?.name || deveui;

    if (state === 1) {
      await this.insertAlert(deveui, hiveName, 'queenless',
        'Queenless colony detected — bees producing queenless roar (>380 Hz).');
    }
    if (state === 2) {
      await this.insertAlert(deveui, hiveName, 'swarm_prep',
        'Swarming preparation detected — pulsing 110 Hz tone. Add supers or prepare to split.');
    }
    if (state === 4) {
      await this.insertAlert(deveui, hiveName, 'dead',
        'Colony appears dead or empty — no acoustic energy detected.');
    }
    if (flags & 0x04) {
      await this.insertAlert(deveui, hiveName, 'probe_fault',
        'Temperature probe fault — a DS18B20 may be disconnected.');
    }
    if (batteryMv < 2500) {
      await this.insertAlert(deveui, hiveName, 'low_battery',
        `Low battery: ${batteryMv} mV — check solar panel connection.`);
    }

    // Weight-drop detection: compare to previous reading
    const prev = await this.db.executeSql(
      `SELECT weight_g FROM history WHERE deveui=? ORDER BY timestamp DESC LIMIT 1,1`,
      [deveui]
    );
    if (prev.rows.length > 0) {
      const prevW = prev.rows[0].weight_g;
      const drop = prevW - (weightDg * 10);
      if (drop > 1500) {  // >1.5 kg drop
        await this.insertAlert(deveui, hiveName, 'weight_drop',
          `Sudden weight drop: ${drop} g — possible swarm departure or robbing.`);
      }
    }
  }

  private async insertAlert(deveui: string, hiveName: string,
                            type: string, message: string): Promise<void> {
    const now = Math.floor(Date.now() / 1000);
    // Dedup: don't insert same-type alert within 1 hour
    await this.db.executeSql(
      `INSERT INTO alerts (deveui, hive_name, type, message, timestamp)
       SELECT ?, ?, ?, ?, ? WHERE NOT EXISTS (
         SELECT 1 FROM alerts WHERE deveui=? AND type=? AND timestamp > ? AND acknowledged=0
       )`,
      [deveui, hiveName, type, message, now, deveui, type, now - 3600]
    );
  }

  async acknowledgeAlert(id: number): Promise<void> {
    await this.db.executeSql(`UPDATE alerts SET acknowledged=1 WHERE id=?`, [id]);
  }

  async queueDownlink(deveui: string, payload: number[]): Promise<void> {
    const now = Math.floor(Date.now() / 1000);
    const buf = new Uint8Array(payload);
    await this.db.executeSql(
      `INSERT INTO downlink_queue (deveui, payload, queued_at) VALUES (?, ?, ?)`,
      [deveui, buf, now]
    );
  }

  async getQueuedDownlinks(): Promise<any[]> {
    const result = await this.db.executeSql(
      `SELECT * FROM downlink_queue ORDER BY queued_at ASC`
    );
    return result.rows;
  }

  async clearQueuedDownlink(id: number): Promise<void> {
    await this.db.executeSql(`DELETE FROM downlink_queue WHERE id=?`, [id]);
  }

  private rowToHive(row: any): HiveData {
    return {
      deveui: row.deveui,
      name: row.name,
      state: row.state,
      dominantHz: row.dominant_hz,
      rHi: row.r_hi,
      cvLoud: row.cv_loud,
      broodTempC: row.brood_temp_c / 100,
      humidity: row.humidity,
      weightG: row.weight_g,
      batteryMv: row.battery_mv,
      flags: row.flags,
      lastSeen: row.last_seen,
      rssi: row.rssi,
    };
  }

  private rowToAlert(row: any): AlertEntry {
    return {
      id: row.id,
      deveui: row.deveui,
      hiveName: row.hive_name,
      type: row.type,
      message: row.message,
      timestamp: row.timestamp,
      acknowledged: row.acknowledged === 1,
    };
  }
}