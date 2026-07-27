/**
 * database.ts — SQLite local history store for LithoCore cell test results.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import * as SQLite from 'expo-sqlite';
import React, { createContext, useContext, ReactNode } from 'react';

export interface CellRecord {
  id?: number;
  timestamp: number;
  cellLabel: string;
  sohScore: number;
  degradation: number;
  verdict: number;
  chemistryIdx: number;
  ocvMv: number;
  tempDc: number;
  dcirMohm: number;
  selfDischargeUvPerMin: number;
  rsMohm: number;
  rseiMohm: number;
  cseimF: number;
  rctMohm: number;
  cdlmF: number;
  sigma: number;
  fitValid: number;
  sweepDataJson: string;  // JSON-serialized array of SweepPoints
}

interface DatabaseContextType {
  db: SQLite.SQLiteDatabase | null;
  saveResult: (record: Omit<CellRecord, 'id'>) => Promise<number>;
  getHistory: () => Promise<CellRecord[]>;
  getResult: (id: number) => Promise<CellRecord | null>;
  deleteResult: (id: number) => Promise<void>;
  clearAll: () => Promise<void>;
  getPackCells: (packId: string) => Promise<CellRecord[]>;
}

const DatabaseContext = createContext<DatabaseContextType | null>(null);

export function DatabaseProvider({ children }: { children: ReactNode }) {
  const [db, setDb] = React.useState<SQLite.SQLiteDatabase | null>(null);

  React.useEffect(() => {
    const initDb = async () => {
      const database = await SQLite.openDatabaseAsync('lithocore.db');
      await database.execAsync(`
        CREATE TABLE IF NOT EXISTS cell_results (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          timestamp INTEGER NOT NULL,
          cell_label TEXT DEFAULT '',
          soh_score INTEGER NOT NULL,
          degradation INTEGER NOT NULL,
          verdict INTEGER NOT NULL,
          chemistry_idx INTEGER NOT NULL,
          ocv_mv INTEGER NOT NULL,
          temp_dc INTEGER NOT NULL,
          dcir_mohm INTEGER NOT NULL,
          self_discharge_uv_per_min INTEGER NOT NULL,
          rs_mohm INTEGER NOT NULL,
          rsei_mohm INTEGER NOT NULL,
          csei_mf INTEGER NOT NULL,
          rct_mohm INTEGER NOT NULL,
          cdl_mf INTEGER NOT NULL,
          sigma INTEGER NOT NULL,
          fit_valid INTEGER NOT NULL,
          sweep_data_json TEXT DEFAULT '[]'
        );

        CREATE TABLE IF NOT EXISTS packs (
          id TEXT PRIMARY KEY,
          name TEXT NOT NULL,
          topology TEXT NOT NULL,
          created_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS pack_cells (
          pack_id TEXT NOT NULL,
          cell_result_id INTEGER NOT NULL,
          slot_position INTEGER NOT NULL,
          FOREIGN KEY (pack_id) REFERENCES packs(id),
          FOREIGN KEY (cell_result_id) REFERENCES cell_results(id),
          PRIMARY KEY (pack_id, cell_result_id)
        );

        CREATE INDEX IF NOT EXISTS idx_cell_results_timestamp
          ON cell_results(timestamp DESC);
      `);
      setDb(database);
    };
    initDb();
  }, []);

  const saveResult = async (record: Omit<CellRecord, 'id'>): Promise<number> => {
    if (!db) return -1;
    const result = await db.runAsync(
      `INSERT INTO cell_results (
        timestamp, cell_label, soh_score, degradation, verdict,
        chemistry_idx, ocv_mv, temp_dc, dcir_mohm,
        self_discharge_uv_per_min, rs_mohm, rsei_mohm, csei_mf,
        rct_mohm, cdl_mf, sigma, fit_valid, sweep_data_json
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
      [
        record.timestamp, record.cellLabel, record.sohScore,
        record.degradation, record.verdict, record.chemistryIdx,
        record.ocvMv, record.tempDc, record.dcirMohm,
        record.selfDischargeUvPerMin, record.rsMohm, record.rseiMohm,
        record.cseimF, record.rctMohm, record.cdlMohm, record.sigma,
        record.fitValid, record.sweepDataJson,
      ]
    );
    return result.lastInsertRowId as number;
  };

  const getHistory = async (): Promise<CellRecord[]> => {
    if (!db) return [];
    const rows = await db.getAllAsync(
      `SELECT * FROM cell_results ORDER BY timestamp DESC`
    );
    return rows as CellRecord[];
  };

  const getResult = async (id: number): Promise<CellRecord | null> => {
    if (!db) return null;
    const row = await db.getFirstAsync(
      `SELECT * FROM cell_results WHERE id = ?`, [id]
    );
    return (row as CellRecord) || null;
  };

  const deleteResult = async (id: number): Promise<void> => {
    if (!db) return;
    await db.runAsync(`DELETE FROM cell_results WHERE id = ?`, [id]);
  };

  const clearAll = async (): Promise<void> => {
    if (!db) return;
    await db.runAsync(`DELETE FROM cell_results`);
  };

  const getPackCells = async (packId: string): Promise<CellRecord[]> => {
    if (!db) return [];
    const rows = await db.getAllAsync(
      `SELECT cr.* FROM cell_results cr
       JOIN pack_cells pc ON cr.id = pc.cell_result_id
       WHERE pc.pack_id = ?
       ORDER BY pc.slot_position`, [packId]
    );
    return rows as CellRecord[];
  };

  return (
    <DatabaseContext.Provider
      value={{ db, saveResult, getHistory, getResult, deleteResult, clearAll, getPackCells }}
    >
      {children}
    </DatabaseContext.Provider>
  );
}

export function useDatabase(): DatabaseContextType {
  const ctx = useContext(DatabaseContext);
  if (!ctx) throw new Error('useDatabase must be used within DatabaseProvider');
  return ctx;
}