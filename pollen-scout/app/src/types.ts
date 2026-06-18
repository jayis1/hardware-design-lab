/**
 * types.ts - Shared TypeScript types for the Pollen Scout app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

export interface TaxaReading {
  classId: number;
  name: string;
  confidence: number;     /* 0..1 */
  concentration: number;  /* grains/m³ */
}

export interface WeatherReading {
  temperatureC: number;
  humidityPct: number;
  pressurePa: number;
  uvIndex: number;
  windSpeedMps: number;
  windDirDeg: number;
  pm1_0: number;
  pm2_5: number;
  pm10: number;
}

export interface DeviceHealth {
  batteryMv: number;
  solarMv: number;
  chargePct: number;
  charging: boolean;
  flowLpm: number;
  sdUsagePct: number;
  lastUplinkMs: number;
  firmwareVersion: string;
  captureCount: number;
  roiCountTotal: number;
}

export interface StationState {
  stationId: string;
  name: string;
  isLocal: boolean;          /* BLE-reachable */
  lastUpdateMs: number;
  taxa: TaxaReading[];
  forecast: number[][];      /* [6 taxa][72 hours] */
  weather: WeatherReading;
  health: DeviceHealth;
}

export interface AlertRule {
  id: string;
  classId: number;
  thresholdGrainsM3: number;
  enabled: boolean;
  lastTriggeredMs: number;
}

export const TAXA_NAMES: string[] = [
  'Alnus (alder)',        'Ambrosia (ragweed)',   'Artemisia (mugwort)',
  'Betula (birch)',       'Carpinus (hornbeam)',  'Castanea (chestnut)',
  'Cedrus (cedar)',       'Corylus (hazel)',      'Cupressaceae (cypress)',
  'Fagus (beech)',        'Fraxinus (ash)',       'Juglans (walnut)',
  'Olea (olive)',         'Picea (spruce)',       'Pinus (pine)',
  'Plantago (plantain)',  'Platanus (plane)',     'Populus (poplar)',
  'Quercus (oak)',        'Rumex (sorrel)',       'Salix (willow)',
  'Tilia (linden)',       'Ulmus (elm)',          'Urtica (nettle)',
  'Acer (maple)',         'Aesculus',             'Carya (hickory)',
  'Sphagnum (moss)',      'Tsuga (hemlock)',      'Tilia cordata',
  'Poaceae (grass)',      'Lolium (ryegrass)',    'Phleum (timothy)',
  'Dactylis (orchardgrass)', 'Festuca (fescue)',  'Poa (bluegrass)',
  'Cladosporium',         'Alternaria',           'Aspergillus',
  'Penicillium',          'Epicoccum',
];

export const POLLEN_INDEX_LABELS = ['Low', ' Moderate', 'High', 'Very High'] as const;

export function pollenIndex(grainsM3: number): number {
  if (grainsM3 < 15)  return 0;
  if (grainsM3 < 50)  return 1;
  if (grainsM3 < 200) return 2;
  return 3;
}