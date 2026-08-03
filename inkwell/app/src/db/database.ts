// database.ts — SQLite persistence for Inkwell sessions and strokes
//
// Stores sessions (date, duration, stroke count, mean pressure) and the
// raw stroke segments (lossless) so sessions can be replayed, exported,
// and searched after the fact. The database is the app-side mirror of the
// pen's flash journal: on reconnect the app pulls missing segments and
// inserts them here, ensuring nothing is ever lost.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import SQLite from 'react-native-sqlite-storage';

export type SessionRecord = {
  id: number;
  startedAt: number;      // epoch ms
  durationMs: number;
  strokeCount: number;
  meanPressureMN: number;
  note: string;
};

export type StrokeRecord = {
  id: number;
  sessionId: number;
  seq: number;
  tsMs: number;
  xUm: number;            // accumulated absolute position
  yUm: number;
  pressureMN: number;
  flags: number;
};

let db: SQLite.SQLiteDatabase | null = null;

export async function openDatabase(): Promise<SQLite.SQLiteDatabase> {
  if (db) return db;
  db = await SQLite.openDatabase({ name: 'inkwell.db', location: 'default' });
  await db.executeSql(`
    CREATE TABLE IF NOT EXISTS sessions (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      started_at INTEGER NOT NULL,
      duration_ms INTEGER NOT NULL DEFAULT 0,
      stroke_count INTEGER NOT NULL DEFAULT 0,
      mean_pressure_mN INTEGER NOT NULL DEFAULT 0,
      note TEXT DEFAULT ''
    );
  `);
  await db.executeSql(`
    CREATE TABLE IF NOT EXISTS strokes (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      session_id INTEGER NOT NULL,
      seq INTEGER NOT NULL,
      ts_ms INTEGER NOT NULL,
      x_um INTEGER NOT NULL,
      y_um INTEGER NOT NULL,
      pressure_mN INTEGER NOT NULL,
      flags INTEGER NOT NULL,
      FOREIGN KEY(session_id) REFERENCES sessions(id)
    );
  `);
  await db.executeSql(`CREATE INDEX IF NOT EXISTS idx_strokes_session ON strokes(session_id);`);
  await db.executeSql(`CREATE INDEX IF NOT EXISTS idx_strokes_seq ON strokes(seq);`);
  return db;
}

export async function startSession(): Promise<number> {
  const d = await openDatabase();
  const now = Date.now();
  const [res] = await d.executeSql(
    'INSERT INTO sessions (started_at) VALUES (?);', [now]);
  return res.insertId;
}

export async function endSession(id: number, durationMs: number): Promise<void> {
  const d = await openDatabase();
  await d.executeSql(
    'UPDATE sessions SET duration_ms = ? WHERE id = ?;', [durationMs, id]);
}

export async function insertStroke(
  sessionId: number, seq: number, tsMs: number,
  xUm: number, yUm: number, pressureMN: number, flags: number): Promise<void> {
  const d = await openDatabase();
  await d.executeSql(
    'INSERT OR IGNORE INTO strokes (session_id, seq, ts_ms, x_um, y_um, pressure_mN, flags) ' +
    'VALUES (?, ?, ?, ?, ?, ?, ?);',
    [sessionId, seq, tsMs, xUm, yUm, pressureMN, flags]);
}

export async function listSessions(): Promise<SessionRecord[]> {
  const d = await openDatabase();
  const [res] = await d.executeSql('SELECT * FROM sessions ORDER BY started_at DESC;');
  const rows: SessionRecord[] = [];
  for (let i = 0; i < res.rows.length; i++) {
    const r = res.rows.item(i);
    rows.push({
      id: r.id,
      startedAt: r.started_at,
      durationMs: r.duration_ms,
      strokeCount: r.stroke_count,
      meanPressureMN: r.mean_pressure_mN,
      note: r.note,
    });
  }
  return rows;
}

export async function getSessionStrokes(sessionId: number): Promise<StrokeRecord[]> {
  const d = await openDatabase();
  const [res] = await d.executeSql(
    'SELECT * FROM strokes WHERE session_id = ? ORDER BY seq ASC;', [sessionId]);
  const rows: StrokeRecord[] = [];
  for (let i = 0; i < res.rows.length; i++) {
    const r = res.rows.item(i);
    rows.push({
      id: r.id, sessionId: r.session_id, seq: r.seq, tsMs: r.ts_ms,
      xUm: r.x_um, yUm: r.y_um, pressureMN: r.pressure_mN, flags: r.flags,
    });
  }
  return rows;
}

export async function getLastSeq(): Promise<number> {
  const d = await openDatabase();
  const [res] = await d.executeSql('SELECT MAX(seq) AS m FROM strokes;');
  if (res.rows.length === 0) return 0;
  return res.rows.item(0).m || 0;
}

export async function deleteSession(id: number): Promise<void> {
  const d = await openDatabase();
  await d.executeSql('DELETE FROM strokes WHERE session_id = ?;', [id]);
  await d.executeSql('DELETE FROM sessions WHERE id = ?;', [id]);
}