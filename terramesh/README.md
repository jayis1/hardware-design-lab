# Terramesh — Subterranean Geotechnical Monitoring Mesh

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## 1. Overview

Terramesh is a distributed, low-power, subterranean sensor mesh designed for continuous geotechnical monitoring of soil slopes, embankments, retaining walls, bridge abutments, and building foundations. Each Terramesh node is a self-contained, IP68-sealed instrument that buries into the ground at depths from 0.5 m to 5 m and measures the key physical parameters that precede catastrophic ground failure: soil moisture content, pore water pressure, micro-tilt (inclination), seismic vibration, and ambient temperature.

The nodes form a self-healing LoRa mesh network, relaying data hop-by-hop to a surface gateway that uplinks via LTE-M (NB-IoT fallback) to a cloud dashboard. With a battery life of 5–10 years on three D-cell LiSOCl₂ primary cells, Terramesh enables long-term, maintenance-free monitoring of geohazard-prone areas where wired instrumentation is impractical or cost-prohibitive.

### 1.1 Why Terramesh?

Landslides kill thousands of people annually and cause tens of billions of dollars in infrastructure damage. Existing monitoring solutions fall into three categories, all inadequate:

| Approach | Limitation |
|----------|------------|
| **Manual survey** (inclinometers, piezometers read by hand) | Expensive, infrequent (monthly), misses rapid-onset events |
| **Wired sensor networks** (vibrating-wire piezometers, tiltmeters) | High installation cost, vulnerable to cable severing during ground movement |
| **Satellite InSAR** | Only measures surface deformation, cm-scale resolution, revisit time days to weeks |

Terramesh fills the gap: a dense, buried, wireless mesh that measures subsurface precursors (pore pressure rise, moisture saturation, micro-tilt) in real time, with per-node cost low enough to deploy at scale (50+ nodes per slope).

### 1.2 Key Innovation

The core innovation is the **multi-depth pore pressure / moisture correlation algorithm** running on each node. By measuring pore water pressure at two depths (shallow: 0.5 m, deep: 2.0 m) and correlating with soil moisture and tilt, the node can distinguish between:

- **Normal rainfall infiltration** (shallow pressure rises, deep pressure stable, tilt < 0.1°)
- **Warning: perched water table formation** (deep pressure rises, moisture saturates, tilt 0.1–0.5°)
- **Critical: basal shear surface lubrication** (both depths elevated, tilt accelerates > 0.5°/hr)

This on-node classification reduces false alarms and enables the mesh to autonomously increase reporting frequency from once per hour to once per 10 seconds when precursors are detected, without cloud dependency.

---

## 2. Hardware Specifications

### 2.1 Core Components

| Component | Part Number | Purpose |
|-----------|-------------|---------|
| **MCU** | STM32U5A5ZJT6Q | Ultra-low-power Cortex-M33 @ 160 MHz, 4 MB Flash, 2.5 MB SRAM, TrustZone, AES/SHA hardware crypto |
| **LoRa Transceiver** | Semtech SX1262 | 868/915 MHz ISM, +22 dBm, -148 dBm sensitivity, long-range mesh link |
| **LTE-M Module** | Quectel BG95-M3 | LTE Cat M1 / NB-IoT / EGPRS, global bands, GNSS assist, ultra-low-power PSM + eDRX |
| **Accelerometer** | ADXL372 | High-g (200 g), low-noise, 3-axis MEMS, 3200 Hz ODR, for seismic/vibration event detection |
| **Tilt Sensor** | SCL3300-D01 | 3-axis inclinometer, ±90° range, 0.001° resolution, SPI, automotive-grade |
| **Environmental** | BME688 | Temperature, humidity, barometric pressure, gas (VOC) — for atmospheric compensation |
| **Pore Pressure** | Custom MEMS piezoresistive | Two depths: 0–500 kPa, 0.1% FS accuracy, I²C interface via ADS1120 16-bit ADC |
| **Soil Moisture** | Capacitance probe (custom) | Fringe-effect capacitance measurement at 70 MHz, 0–100% VWC, ±2% accuracy |
| **Flash Storage** | MX25R6435F | 64 Mb serial NOR flash, deep power-down, for data logging during LTE outage |
| **RTC** | RV-3028-C7 | Ultra-low-power (45 nA) real-time clock, ±3 ppm, I²C, backup supercap |
| **Power Management** | MAX20361 | Ultra-low-quiescent boost converter + battery monitor, MPPT for solar trickle |

### 2.2 Sensor Specifications

| Parameter | Sensor | Range | Resolution | Accuracy | Sample Rate |
|-----------|--------|-------|------------|----------|-------------|
| Pore pressure (shallow) | MEMS + ADS1120 | 0–500 kPa | 0.0076 kPa | ±0.1% FS | 1 Hz |
| Pore pressure (deep) | MEMS + ADS1120 | 0–500 kPa | 0.0076 kPa | ±0.1% FS | 1 Hz |
| Soil moisture | Capacitance probe | 0–100% VWC | 0.1% | ±2% | 0.1 Hz |
| Tilt (X/Y) | SCL3300-D01 | ±90° | 0.001° | ±0.01° | 10 Hz |
| Vibration (3-axis) | ADXL372 | ±200 g | 0.1 mg/LSB | ±10% | 3200 Hz max |
| Temperature | BME688 | -40 to +85°C | 0.01°C | ±0.5°C | 1 Hz |
| Barometric pressure | BME688 | 300–1100 hPa | 0.18 Pa | ±0.12 hPa | 1 Hz |

### 2.3 Connectivity

| Interface | Protocol | Purpose |
|-----------|----------|---------|
| LoRa (mesh) | SX1262, 868/915 MHz | Inter-node mesh relay, up to 2 km line-of-sight per hop |
| LTE-M / NB-IoT | BG95-M3 | Gateway uplink to cloud, GNSS position assist |
| USB-C | USB 2.0 FS | Configuration, firmware update, debug (surface access only) |
| NFC (passive) | ST25DV04K | Near-field commissioning, read node ID / status with phone |
| Debug SWD | 5-pin Tag-Connect | Production programming and debug |

### 2.4 Power

| Parameter | Value |
|-----------|-------|
| **Primary cells** | 3 × D-cell LiSOCl₂ (3.6 V, 19 Ah each) in series-parallel: 7.2 V, 38 Ah |
| **Supercapacitor** | 5 F, 2.7 V (for LTE burst current) |
| **Solar trickle** | 2 W polycrystalline panel (surface gateway only) |
| **Quiescent current** | 2.5 µA (deep sleep, RTC + wake-on-tilt) |
| **Active average** | 45 µA (1 sample per 10 min, LoRa TX every 30 min) |
| **LTE burst** | 250 mA for 2 s (gateway only, once per 6 hours) |
| **Battery life** | 5–10 years (node), indefinite (gateway with solar) |
| **Boost converter** | MAX20361, 85–95% efficiency, 1.8–5.5 V input range |

### 2.5 Mechanical / Environmental

| Parameter | Value |
|-----------|-------|
| **Enclosure** | 316L stainless steel tube, 120 mm × 80 mm × 50 mm |
| **Sealing** | IP68 (20 m water depth, 72 hr), double O-ring + potting |
| **Operating temp** | -30°C to +70°C (storage -40°C to +85°C) |
| **Burial depth** | 0.5 m to 5 m (cable to surface antenna) |
| **Weight** | 1.2 kg (with batteries) |
| **Mounting** | Driven rod or excavated pit, backfilled with native soil |
| **Antenna** | External whip (LoRa, ¼-wave, 8.6 cm @ 868 MHz) on 3 m cable to surface |

---

## 3. Architecture

### 3.1 System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TERRAMESH NODE                               │
│                                                                     │
│  ┌──────────┐    ┌──────────────┐    ┌──────────────────────────┐ │
│  │  Solar    │    │  MAX20361    │    │  STM32U5A5ZJT6Q           │ │
│  │  Panel    │───▶│  PMIC/Boost  │───▶│  Cortex-M33 @ 160 MHz     │ │
│  │  (opt.)   │    │  + Batt Mgt │    │  4 MB Flash / 2.5 MB SRAM │ │
│  └──────────┘    └──────────────┘    │  TrustZone / AES / SHA     │ │
│                                        └───────────┬──────────────┘ │
│  ┌──────────┐                                     │                 │
│  │ 3× D-cell│                                     │                 │
│  │ LiSOCl₂  │                                     │                 │
│  └──────────┘                                     │                 │
│                                                   │                 │
│  ┌─────────────────────────────────────────────────┼─────────────┐  │
│  │  SENSOR INTERFACE BUS (I²C + SPI + GPIO)       │             │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐      │             │  │
│  │  │ BME688   │  │ SCL3300 │  │ ADXL372  │      │             │  │
│  │  │ Env. Sens│  │ Tilt    │  │ Accel.   │      │             │  │
│  │  └──────────┘  └──────────┘  └──────────┘      │             │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐      │             │  │
│  │  │ ADS1120  │  │ ADS1120  │  │ Moisture │      │             │  │
│  │  │ Pore P.1 │  │ Pore P.2 │  │ Capac.   │      │             │  │
│  │  └──────────┘  └──────────┘  └──────────┘      │             │  │
│  └─────────────────────────────────────────────────┘             │  │
│                                                                     │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐    │
│  │ SX1262   │    │ BG95-M3  │    │ MX25R64  │    │ RV-3028  │    │
│  │ LoRa     │    │ LTE-M    │    │ NOR Flash│    │ RTC      │    │
│  └────┬─────┘    └────┬─────┘    └──────────┘    └──────────┘    │
│       │               │                                            │
│  ┌────┴─────┐    ┌────┴─────┐                                     │
│  │ 868/915  │    │ LTE Ant. │                                     │
│  │ MHz Ant. │    │ (int./ext)│                                     │
│  └──────────┘    └──────────┘                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 Mesh Topology

```
                    ┌──────────┐
                    │  Cloud   │
                    │ Dashboard│
                    └────┬─────┘
                         │ LTE-M
                    ┌────┴─────┐
                    │ Gateway  │  (surface, solar-powered)
                    │ Node #0  │
                    └────┬─────┘
                         │ LoRa
              ┌──────────┼──────────┐
              │          │          │
         ┌────┴────┐ ┌───┴───┐ ┌───┴───┐
         │ Node #1 │ │Node #2│ │Node #3│  (buried, battery)
         │ (slope  │ │(mid)  │ │(toe)  │
         │  crest) │ └───┬───┘ └───┬───┘
         └─────────┘     │         │
                    ┌─────┴───┐ ┌──┴──────┐
                    │ Node #4 │ │ Node #5 │
                    │ (deep)  │ │ (deep)  │
                    └─────────┘ └─────────┘
```

Each node acts as a store-and-forward repeater. The mesh uses a TDMA-based schedule with 8 time slots per 60-second frame. Nodes synchronize via the gateway's beacon. If a node loses contact with the gateway, it enters "dark mode" — logging data to flash and attempting relay through any neighbor it can hear.

### 3.3 Power Domains

| Domain | Voltage | Switched | Components |
|--------|---------|----------|------------|
| **V_BATT** | 3.0–7.2 V | No | LiSOCl₂ cells, supercap |
| **V_CORE** | 1.8 V | No | STM32U5 backup domain, RTC |
| **V_3V3** | 3.3 V | Yes (PMIC) | MCU core, sensors, LoRa, flash |
| **V_LTE** | 3.8 V | Yes (PMIC) | BG95-M3 (gateway only) |
| **V_REF** | 2.5 V | Yes | ADS1120 reference |

---

## 4. Firmware Design

### 4.1 Architecture

The firmware is built on a **super-loop with interrupt-driven state machine** architecture, optimized for ultra-low-power operation. The main loop spends >99.9% of time in STOP2 sleep mode (2.5 µA), waking only on:

1. **RTC alarm** — periodic measurement cycle (default: every 10 minutes)
2. **Tilt interrupt** — motion/tilt exceeds threshold (rapid-onset detection)
3. **LoRa RX** — incoming mesh packet (relay or command)
4. **LTE timer** — gateway scheduled uplink

### 4.2 State Machine

```
                    ┌─────────────┐
                    │   SLEEP     │◄──────────────────────────────┐
                    │ (STOP2,     │                                │
                    │  2.5 µA)    │                                │
                    └──────┬──────┘                                │
                           │ wake source                           │
                           ▼                                        │
                    ┌─────────────┐                                │
              ┌────▶│  SENSE      │                                │
              │     │ Read all    │                                │
              │     │ sensors     │                                │
              │     └──────┬──────┘                                │
              │            │                                        │
              │            ▼                                        │
              │     ┌─────────────┐   no anomaly   ┌─────────────┐ │
              │     │  CLASSIFY   │───────────────▶│  LOG + SLEEP│─┘
              │     │ On-node ML  │                │ (store to   │
              │     │ threshold   │                │  flash)     │
              │     └──────┬──────┘                └─────────────┘
              │            │ anomaly detected
              │            ▼
              │     ┌─────────────┐
              │     │  FAST_MODE  │
              │     │ 10 sec cycle│
              │     └──────┬──────┘
              │            │
              │            ▼
              │     ┌─────────────┐
              │     │  TX_QUEUE   │
              │     │ Enqueue msg │
              │     └──────┬──────┘
              │            │
              │            ▼
              │     ┌─────────────┐
              │     │  TX_MESH    │
              │     │ LoRa send   │
              │     │ (TDMA slot) │
              │     └──────┬──────┘
              │            │
              └────────────┘
```

### 4.3 On-Node Classification Algorithm

The classification engine runs a lightweight decision tree that evaluates five features:

```
1. ΔP_shallow = P_pore_shallow - P_pore_shallow_baseline
2. ΔP_deep    = P_pore_deep - P_pore_deep_baseline
3. Δtilt      = |tilt - tilt_baseline|
4. Δmoisture  = VWC - VWC_baseline
5. tilt_rate  = Δtilt / Δtime (over last 3 samples)

Decision tree:
  IF ΔP_deep > 15 kPa AND Δmoisture > 10%:
    → WARNING (perched water table forming)
    IF tilt_rate > 0.5°/hr:
      → CRITICAL (shear surface lubrication likely)
  ELSE IF ΔP_shallow > 10 kPa AND Δtilt > 0.1°:
    → WARNING (rainfall infiltration + minor movement)
  ELSE IF tilt_rate > 1.0°/hr:
    → CRITICAL (rapid movement, possible failure)
  ELSE:
    → NORMAL
```

### 4.4 LoRa Mesh Protocol

| Field | Size | Description |
|-------|------|-------------|
| Preamble | 8 bytes | Sync word, SF, CR |
| DestAddr | 2 bytes | Destination node ID (0xFFFF = broadcast) |
| SrcAddr | 2 bytes | Source node ID |
| SeqNum | 2 bytes | Monotonic sequence, dedup |
| HopCount | 1 byte | TTL, decremented at each relay |
| MsgType | 1 byte | DATA, ACK, CMD, BEACON, REGISTER |
| Payload | 0–51 bytes | Sensor data or command |
| CRC32 | 4 bytes | Over entire packet |

### 4.5 Data Logging

Each measurement record is 32 bytes:

```
struct __attribute__((packed)) sensor_record {
    uint32_t timestamp;       // Unix epoch seconds
    uint16_t pore_press_shallow;  // 0.01 kPa units
    uint16_t pore_press_deep;     // 0.01 kPa units
    uint16_t moisture;            // 0.01% VWC
    int16_t  tilt_x;              // 0.001° units
    int16_t  tilt_y;              // 0.001° units
    int16_t  accel_x;             // mg units
    int16_t  accel_y;             // mg units
    int16_t  accel_z;             // mg units
    int16_t  temperature;         // 0.01°C units
    uint16_t pressure;            // Pa units
    uint8_t  classification;     // 0=NORMAL, 1=WARNING, 2=CRITICAL
    uint8_t  battery_mv;          // mV / 20
};
```

At 1 record per 10 minutes, 64 Mb flash holds ~5.7 years of data.

---

## 5. Application / Software Interface

### 5.1 Companion App (React Native)

The Terramesh mobile app provides:

- **Dashboard** — Map view of all nodes with color-coded status (green=normal, yellow=warning, red=critical)
- **Node Detail** — Time-series plots of all sensor channels, classification history, battery status
- **Alerts** — Push notifications for WARNING and CRITICAL classifications
- **Commissioning** — NFC tap to read node ID, set depth, assign to site
- **Gateway Config** — LTE APN settings, uplink interval, mesh channel
- **Export** — CSV/GeoJSON export of sensor data

### 5.2 Cloud API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/v1/nodes` | GET | List all nodes with latest status |
| `/api/v1/nodes/:id` | GET | Node detail and history |
| `/api/v1/nodes/:id/data` | GET | Time-series sensor data |
| `/api/v1/alerts` | GET | Active and historical alerts |
| `/api/v1/sites` | GET/POST | Manage sites (slopes, structures) |
| `/api/v1/export` | GET | Export data as CSV/GeoJSON |

### 5.3 USB Protocol

The USB CDC-ACM virtual serial port provides:

| Command | Description |
|---------|-------------|
| `AT+ID` | Get node ID |
| `AT+STATUS` | Get current sensor readings |
| `AT+LOG?<n>` | Read last n log records |
| `AT+CONFIG=<key>=<val>` | Set configuration parameter |
| `AT+DFU` | Enter DFU mode for firmware update |
| `AT+RESET` | Soft reset |

---

## 6. Use Cases and Target Audience

### 6.1 Primary Use Cases

| Use Case | Description | Typical Deployment |
|----------|-------------|-------------------|
| **Landslide early warning** | Monitor active landslide zones, issue alerts when pore pressure and tilt precursors exceed thresholds | 20–100 nodes on unstable slope, 1 gateway at safe location |
| **Embankment / dam monitoring** | Detect internal erosion, piping, and slope instability in earth dams and levees | 10–50 nodes along crest and toe, 2–3 gateways |
| **Bridge abutment / foundation** | Monitor scour and soil movement around bridge piers and abutments | 5–20 nodes per abutment, 1 gateway per bridge |
| **Construction site safety** | Monitor excavation walls, fill slopes, and adjacent structures during construction | 10–30 nodes, temporary deployment, solar gateways |
| **Mining pit wall stability** | Monitor open-pit mine walls for failure precursors | 30–100 nodes, ruggedized, explosion-proof variant |
| **Permafrost degradation** | Monitor thaw depth, pore pressure, and slope movement in Arctic infrastructure | 10–50 nodes, cold-rated batteries, satellite uplink |

### 6.2 Target Audience

- **Geotechnical engineers** — Design and interpret monitoring programs
- **Civil engineering firms** — Construction monitoring and risk management
- **Transportation authorities** — Highway and railway slope monitoring
- **Mining companies** — Pit wall stability and tailings dam monitoring
- **Municipalities** — Landslide-prone area early warning systems
- **Research institutions** — Geohazard research and model validation
- **Insurance / reinsurance** — Risk assessment and mitigation verification

### 6.3 Competitive Advantages

| Feature | Terramesh | Traditional Wired | Manual Survey | Satellite InSAR |
|---------|-----------|-------------------|---------------|-----------------|
| Subsurface measurement | ✅ Pore pressure, moisture, tilt at depth | ✅ (wired piezometers) | ✅ (manual readout) | ❌ Surface only |
| Real-time alerts | ✅ < 60 s latency | ✅ (if powered) | ❌ Weeks | ❌ Days |
| Per-node cost | ~$200 | ~$2,000+ (installed) | ~$500/visit | ~$10K/site |
| Deployment density | High (50+ nodes) | Low (5–10) | Very low (1–3) | N/A |
| Battery life | 5–10 years | Wired (needs power) | N/A | N/A |
| Self-healing mesh | ✅ | ❌ (star topology) | N/A | N/A |
| On-node ML classification | ✅ | ❌ | ❌ | ❌ |

---

## 7. Bill of Materials (Estimated)

| Item | Qty | Unit Cost | Total |
|------|-----|-----------|-------|
| STM32U5A5ZJT6Q | 1 | $12.50 | $12.50 |
| SX1262 | 1 | $4.20 | $4.20 |
| BG95-M3 (gateway only) | 1 | $18.00 | $18.00 |
| ADXL372 | 1 | $3.80 | $3.80 |
| SCL3300-D01 | 1 | $8.50 | $8.50 |
| BME688 | 1 | $3.20 | $3.20 |
| ADS1120 (×2) | 2 | $2.10 | $4.20 |
| MX25R6435F | 1 | $1.80 | $1.80 |
| RV-3028-C7 | 1 | $2.50 | $2.50 |
| MAX20361 | 1 | $3.90 | $3.90 |
| LiSOCl₂ D-cell (×3) | 3 | $8.00 | $24.00 |
| 316L enclosure + O-rings | 1 | $15.00 | $15.00 |
| PCB (4-layer, ENIG) | 1 | $8.00 | $8.00 |
| Passive components | ~50 | $0.05 | $2.50 |
| Antenna + cable | 1 | $4.00 | $4.00 |
| **Node total** | | | **$116.10** |
| **Gateway total** (adds BG95 + solar) | | | **$152.30** |

---

## 8. Development Status

| Component | Status |
|-----------|--------|
| Phase 1 — Conceptual Architecture | ✅ Complete |
| Phase 2 — Component Selection & Schematics | ✅ Complete |
| Phase 3 — PCB Blueprints & Layout | ✅ Complete |
| Phase 4 — Software Stack | ✅ Complete |
| Firmware (C) | ✅ Complete (500+ lines) |
| KiCad Schematic | ✅ Complete |
| KiCad PCB Layout | ✅ Complete |
| React Native App | ✅ Complete |

---

*Terramesh: Listening to the ground before it moves.*  
*Designed by jayis1 — all hardware, firmware, and software.*
