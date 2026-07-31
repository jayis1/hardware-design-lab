/**
 * database.ts — SQLite database for Synthand presets and history.
 *
 * Stores MIDI mapping presets, calibration snapshots, and session
 * history using react-native-sqlite-storage.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import SQLite from 'react-native-sqlite-storage';
import { MappingData, CalibrationData } from '../ble/protocol';

SQLite.enablePromise(true);

let db: SQLite.SQLiteDatabase | null = null;

/**
 * Initialize the database — create tables if they don't exist.
 * Author: jayis1
 */
export async function initDatabase(): Promise<void> {
  db = await SQLite.openDatabase({
    name: 'synthand.db',
    location: 'default',
  });

  await db.executeSql(`
    CREATE TABLE IF NOT EXISTS presets (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL,
      mapping_json TEXT NOT NULL,
      created_at INTEGER NOT NULL,
      updated_at INTEGER NOT NULL
    );
  `);

  await db.executeSql(`
    CREATE TABLE IF NOT EXISTS calibration_history (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      calibration_json TEXT NOT NULL,
      created_at INTEGER NOT NULL
    );
  `);

  await db.executeSql(`
    CREATE TABLE IF NOT EXISTS session_log (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      session_start INTEGER NOT NULL,
      session_end INTEGER,
      note_count INTEGER DEFAULT 0,
      gesture_count INTEGER DEFAULT 0,
      notes TEXT
    );
  `);
}

/**
 * Save a mapping preset.
 * Author: jayis1
 */
export async function savePreset(name: string, mapping: MappingData): Promise<number> {
  if (!db) await initDatabase();
  const now = Date.now();
  const json = JSON.stringify(mapping);
  const [result] = await db!.executeSql(
    'INSERT INTO presets (name, mapping_json, created_at, updated_at) VALUES (?, ?, ?, ?)',
    [name, json, now, now]
  );
  return result.insertId;
}

/**
 * Load all presets.
 */
export async function loadPresets(): Promise<Array<{ id: number; name: string; mapping: MappingData }>> {
  if (!db) await initDatabase();
  const [result] = await db!.executeSql('SELECT id, name, mapping_json FROM presets ORDER BY updated_at DESC');
  const presets: Array<{ id: number; name: string; mapping: MappingData }> = [];
  for (let i = 0; i < result.rows.length; i++) {
    const row = result.rows.item(i);
    presets.push({
      id: row.id,
      name: row.name,
      mapping: JSON.parse(row.mapping_json),
    });
  }
  return presets;
}

/**
 * Delete a preset by ID.
 */
export async function deletePreset(id: number): Promise<void> {
  if (!db) await initDatabase();
  await db!.executeSql('DELETE FROM presets WHERE id = ?', [id]);
}

/**
 * Save a calibration snapshot.
 * Author: jayis1
 */
export async function saveCalibration(calib: CalibrationData): Promise<number> {
  if (!db) await initDatabase();
  const now = Date.now();
  const json = JSON.stringify(calib);
  const [result] = await db!.executeSql(
    'INSERT INTO calibration_history (calibration_json, created_at) VALUES (?, ?)',
    [json, now]
  );
  return result.insertId;
}

/**
 * Get the latest calibration.
 */
export async function getLatestCalibration(): Promise<CalibrationData | null> {
  if (!db) await initDatabase();
  const [result] = await db!.executeSql(
    'SELECT calibration_json FROM calibration_history ORDER BY created_at DESC LIMIT 1'
  );
  if (result.rows.length > 0) {
    return JSON.parse(result.rows.item(0).calibration_json);
  }
  return null;
}

/**
 * Start a new session log.
 */
export async function startSession(): Promise<number> {
  if (!db) await initDatabase();
  const now = Date.now();
  const [result] = await db!.executeSql(
    'INSERT INTO session_log (session_start) VALUES (?)',
    [now]
  );
  return result.insertId;
}

/**
 * End a session log.
 * Author: jayis1
 */
export async function endSession(
  id: number,
  noteCount: number,
  gestureCount: number,
  notes: string
): Promise<void> {
  if (!db) await initDatabase();
  const now = Date.now();
  await db!.executeSql(
    'UPDATE session_log SET session_end = ?, note_count = ?, gesture_count = ?, notes = ? WHERE id = ?',
    [now, noteCount, gestureCount, notes, id]
  );
}