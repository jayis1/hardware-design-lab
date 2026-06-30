/**
 * types.ts — Shared TypeScript types for SpectraPest Field app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

export interface DetectionEvent {
  id: string;
  node_id: string;
  timestamp: string;       /* ISO 8601 */
  gps: {
    lat: number;
    lon: number;
  };
  species: string;
  species_id: number;
  confidence: number;      /* 0-1 */
  severity: 'none' | 'low' | 'med' | 'high';
  features: {
    wingbeat_hz: number;
    spectral_ndvi: number;
    spectral_ndre: number;
    damage_area_pct: number;
    temp_c: number;
    humidity_pct: number;
    co2_ppm: number;
  };
  thumbnail_b64?: string;
}

export interface SpectraPestNode {
  id: string;
  label: string;
  node_addr: number;
  field_name: string;
  gps: {
    lat: number;
    lon: number;
  };
  last_seen: string;       /* ISO 8601 */
  battery_pct: number;
  solar_charging: boolean;
  firmware_version: string;
  config: {
    detection_interval_sec: number;
    lora_frequency_hz: number;
    mesh_role: 'leaf' | 'router' | 'gateway';
  };
  latest_detection?: DetectionEvent;
}

export interface AlertConfig {
  id: string;
  species: string;
  severity_threshold: 'low' | 'med' | 'high';
  proximity_m: number;
  enabled: boolean;
  created_at: string;
}

export interface PestPressureGrid {
  cells: PestPressureCell[];
  bounds: {
    north: number;
    south: number;
    east: number;
    west: number;
  };
  grid_size_m: number;
  timestamp: string;
}

export interface PestPressureCell {
  lat: number;
  lon: number;
  pressure: number;       /* 0-1, normalized detection density */
  dominant_species: string;
  detection_count: number;
}

export interface GatewayStatus {
  online: boolean;
  node_count: number;
  total_detections: number;
  pest_detections: number;
  uptime_sec: number;
  last_update: string;
}

export const SPECIES_NAMES: Record<number, string> = {
  0: 'Aphis gossypii (Cotton Aphid)',
  1: 'Aphis glycines (Soybean Aphid)',
  2: 'Myzus persicae (Green Peach Aphid)',
  3: 'Bemisia tabaci (Silverleaf Whitefly)',
  4: 'Trialeurodes vaporariorum (Greenhouse Whitefly)',
  5: 'Frankliniella occidentalis (Western Flower Thrips)',
  6: 'Tetranychus urticae (Two-spotted Spider Mite)',
  7: 'Panonychus ulmi (European Red Mite)',
  8: 'Helicoverpa armigera (Cotton Bollworm)',
  9: 'Spodoptera frugiperda (Fall Armyworm)',
  10: 'Spodoptera exigua (Beet Armyworm)',
  11: 'Spodoptera litura (Tobacco Cutworm)',
  12: 'Ostrinia nubilalis (European Corn Borer)',
  13: 'Cydia pomonella (Codling Moth)',
  14: 'Lobesia botrana (Grapevine Moth)',
  15: 'Plutella xylostella (Diamondback Moth)',
  16: 'Pectinophora gossypiella (Pink Bollworm)',
  17: 'Leptinotarsa decemlineata (Colorado Potato Beetle)',
  18: 'Diabrotica virgifera (Western Corn Rootworm)',
  19: 'Epilachna varivestis (Mexican Bean Beetle)',
  20: 'Liriomyza sativae (Vegetable Leafminer)',
  21: 'Liriomyza trifolii (American Serpentine Leafminer)',
  22: 'Bactrocera dorsalis (Oriental Fruit Fly)',
  23: 'Ceratitis capitata (Mediterranean Fruit Fly)',
  24: 'Rhagoletis pomonella (Apple Maggot)',
  25: 'Halyomorpha halys (Brown Marmorated Stink Bug)',
  26: 'Nezara viridula (Southern Green Stink Bug)',
  27: 'Euschistus servus (Brown Stink Bug)',
  28: 'Lygus lineolaris (Tarnished Plant Bug)',
  29: 'Lycorma delicatula (Spotted Lanternfly)',
  30: 'Anoplophora glabripennis (Asian Longhorned Beetle)',
  31: 'Leptinotarsa decemlineata (Colorado Potato Beetle)',
  32: 'Popillia japonica (Japanese Beetle)',
  33: 'Diaphorina citri (Asian Citrus Psyllid)',
  34: 'Bactericera cockerelli (Potato Psyllid)',
  35: 'Homalodisca vitripennis (Glassy-winged Sharpshooter)',
  36: 'Empoasca fabae (Potato Leafhopper)',
  37: 'Nephotettix virescens (Rice Green Leafhopper)',
  38: 'Nilaparvata lugens (Brown Planthopper)',
  39: 'Sogatella furcifera (White-backed Planthopper)',
  40: 'Laodelphax striatellus (Small Brown Planthopper)',
  41: 'Sitobion avenae (English Grain Aphid)',
  42: 'Schizaphis graminum (Greenbug)',
  43: 'Rhopalosiphum padi (Bird Cherry-oat Aphid)',
  44: 'Diuraphis noxia (Russian Wheat Aphid)',
  45: 'Mayetiola destructor (Hessian Fly)',
  46: 'Sitodiplosis mosellana (Wheat Midge)',
  47: 'Contarinia nasturtii (Swede Midge)',
  48: 'Delia radicum (Cabbage Root Fly)',
  49: 'Delia antiqua (Onion Maggot)',
  50: 'Phyllotreta cruciferae (Flea Beetle)',
  51: 'Epitrix cucumeris (Potato Flea Beetle)',
  52: 'Cerotoma trifurcata (Bean Leaf Beetle)',
  53: 'Colias eurytheme (Alfalfa Caterpillar)',
  54: 'Autographa californica (Alfalfa Looper)',
  55: 'Trichoplusia ni (Cabbage Looper)',
  56: 'Pieris rapae (Small Cabbage White)',
  57: 'Pieris brassicae (Large White)',
  58: 'Mamestra brassicae (Cabbage Moth)',
  59: 'Agrotis ipsilon (Black Cutworm)',
  60: 'No pest detected',
};

export const SEVERITY_COLORS: Record<string, string> = {
  none: '#81C784',
  low: '#FFF176',
  med: '#FFB74D',
  high: '#E57373',
};

export const SEVERITY_LABELS: Record<string, string> = {
  none: 'None',
  low: 'Low',
  med: 'Medium',
  high: 'High',
};