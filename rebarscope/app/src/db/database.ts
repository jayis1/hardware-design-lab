/**
 * Database — SQLite storage for surveys
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import * as SQLite from 'expo-sqlite';

const db = SQLite.openDatabase('rebarscope.db');

export interface SurveyRecord {
  id: number;
  site_name: string;
  created_at: number;
  config_json: string;
  hash_chain: string;
}

export interface PointRecord {
  id: number;
  survey_id: number;
  x_mm: number;
  y_mm: number;
  heading_deg: number;
  hcp_mv: number;
  hcp_class: number;
  rho_ohm_m: number;
  rho_class: number;
  cover_mm: number;
  rebar_diam_mm: number;
  timestamp_ms: number;
  hash_hex: string;
}

export function initDatabase(): Promise<void> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          `CREATE TABLE IF NOT EXISTS surveys (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            site_name TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            config_json TEXT NOT NULL,
            hash_chain TEXT NOT NULL
          );`
        );
        tx.executeSql(
          `CREATE TABLE IF NOT EXISTS points (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            survey_id INTEGER NOT NULL,
            x_mm REAL NOT NULL,
            y_mm REAL NOT NULL,
            heading_deg REAL NOT NULL,
            hcp_mv REAL NOT NULL,
            hcp_class INTEGER NOT NULL,
            rho_ohm_m REAL NOT NULL,
            rho_class INTEGER NOT NULL,
            cover_mm REAL NOT NULL,
            rebar_diam_mm REAL NOT NULL,
            timestamp_ms INTEGER NOT NULL,
            hash_hex TEXT NOT NULL,
            FOREIGN KEY (survey_id) REFERENCES surveys(id)
          );`
        );
        tx.executeSql(
          `CREATE INDEX IF NOT EXISTS idx_points_survey ON points(survey_id);`
        );
      },
      (err) => reject(err),
      () => resolve()
    );
  });
}

export function createSurvey(siteName: string, configJson: string): Promise<number> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          'INSERT INTO surveys (site_name, created_at, config_json, hash_chain) VALUES (?, ?, ?, ?);',
          [siteName, Date.now(), configJson, ''],
          (_, result) => resolve(result.insertId),
          (_, err) => {
            reject(err);
            return false;
          }
        );
      },
      (err) => reject(err)
    );
  });
}

export function addPoint(p: PointRecord): Promise<void> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          `INSERT INTO points (survey_id, x_mm, y_mm, heading_deg, hcp_mv, hcp_class,
                              rho_ohm_m, rho_class, cover_mm, rebar_diam_mm,
                              timestamp_ms, hash_hex)
           VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);`,
          [
            p.survey_id, p.x_mm, p.y_mm, p.heading_deg, p.hcp_mv, p.hcp_class,
            p.rho_ohm_m, p.rho_class, p.cover_mm, p.rebar_diam_mm,
            p.timestamp_ms, p.hash_hex,
          ],
          () => resolve(),
          (_, err) => {
            reject(err);
            return false;
          }
        );
      },
      (err) => reject(err)
    );
  });
}

export function getPoints(surveyId: number): Promise<PointRecord[]> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          'SELECT * FROM points WHERE survey_id = ? ORDER BY timestamp_ms;',
          [surveyId],
          (_, result) => {
            const points: PointRecord[] = [];
            for (let i = 0; i < result.rows.length; i++) {
              points.push(result.rows.item(i));
            }
            resolve(points);
          },
          (_, err) => {
            reject(err);
            return false;
          }
        );
      },
      (err) => reject(err)
    );
  });
}

export function updateHashChain(surveyId: number, hashHex: string): Promise<void> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          'UPDATE surveys SET hash_chain = ? WHERE id = ?;',
          [hashHex, surveyId],
          () => resolve(),
          (_, err) => {
            reject(err);
            return false;
          }
        );
      },
      (err) => reject(err)
    );
  });
}

export function getSurvey(surveyId: number): Promise<SurveyRecord | null> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          'SELECT * FROM surveys WHERE id = ?;',
          [surveyId],
          (_, result) => {
            if (result.rows.length > 0) resolve(result.rows.item(0));
            else resolve(null);
          },
          (_, err) => {
            reject(err);
            return false;
          }
        );
      },
      (err) => reject(err)
    );
  });
}

export function listSurveys(): Promise<SurveyRecord[]> {
  return new Promise((resolve, reject) => {
    db.transaction(
      (tx) => {
        tx.executeSql(
          'SELECT * FROM surveys ORDER BY created_at DESC;',
          [],
          (_, result) => {
            const surveys: SurveyRecord[] = [];
            for (let i = 0; i < result.rows.length; i++) {
              surveys.push(result.rows.item(i));
            }
            resolve(surveys);
          },
          (_, err) => {
            reject(err);
            return false;
          }
        );
      },
      (err) => reject(err)
    );
  });
}