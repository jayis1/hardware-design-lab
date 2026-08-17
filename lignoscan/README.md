# LignoScan — Portable Acoustic Tomography Scanner for Tree Decay Detection

![LignoScan](https://img.shields.io/badge/PCB-120x80mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H733-orange) ![Sensors](https://img.shields.io/badge/Sensors-16x%20Ultrasonic-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.2%20%2B%20USB--C-purple) ![License](https://img.shields.io/badge/License-MIT-yellow)

**Author:** jayis1
**Copyright:** © 2026 jayis1. All rights reserved.
**License:** CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)

---

## 1. Overview

**LignoScan** is a portable, open-source **acoustic tomography instrument** designed for arborists, urban foresters, and tree-risk assessors. It non-destructively detects internal decay, hollows, cracks, and structural defects in living tree trunks by measuring the propagation velocity of ultrasonic stress waves through the wood cross-section.

Traditional decay assessment relies on increment borers (invasive, slow, damages the tree) or single-point resistance drilling (only samples one radius). Commercial acoustic tomographs exist but cost $8,000–$25,000 and use closed-source software with proprietary transducers. LignoScan brings this capability to the open-source community at a fraction of the cost, with a full open toolchain from firmware to mobile app.

### How It Works

1. An arborist places **8–16 magnetic-coupled ultrasonic transducers** around the circumference of the tree trunk at the height of interest, spaced evenly.
2. Each transducer is pressed against the bark using a spring-loaded strap. Magnetic couplers ensure consistent acoustic coupling.
3. The LignoScan unit sequentially pulses each transducer with a high-voltage spike, generating an ultrasonic stress wave that propagates through the wood.
4. All other transducers act as receivers, and the system measures the **time-of-flight (ToF)** of the first-arriving wave for every transmitter→receiver pair.
5. From the full matrix of ToF measurements (up to N×(N−1) pairs for N sensors), the onboard processor computes **slowness** (inverse velocity) for each ray path.
6. A **2D tomographic reconstruction** (Simultaneous Algebraic Reconstruction Technique — SART) divides the trunk cross-section into a polar grid and iteratively solves for the wave velocity in each cell, producing a color-coded map of sound wood vs. decayed/hollow regions.
7. Results are streamed via BLE to the companion mobile app, which renders the tomogram, logs measurements with GPS coordinates, and generates PDF inspection reports.

### Why It Matters

Urban tree failure causes **over $1 billion in property damage and dozens of fatalities annually** in the United States alone. Many hazardous trees have internal decay that is invisible from the outside. LignoScan enables rapid, non-invasive screening of high-value or high-risk trees — those near buildings, roads, playgrounds, and pedestrian areas — empowering municipalities, arborists, and conservationists to make data-driven risk-management decisions without harming the tree.

---

## 2. Key Features

| Feature | Description |
|---------|-------------|
| **16-channel ultrasonic array** | Supports 8–16 magnetic-coupled 60 kHz transducers around the trunk circumference |
| **Sub-microsecond timing** | TDC-GP22 time-to-digital converter provides 22 ps resolution for precise ToF measurement |
| **Onboard tomographic reconstruction** | SART algorithm runs on STM32H733 Cortex-M7 @ 280 MHz; full 12-sensor scan reconstructs in < 3 seconds |
| **Non-destructive** | No drilling, no coring — transducers couple to bark via magnetic clamps on a ratchet strap |
| **BLE 5.2 wireless** | Streams tomograms and raw data to mobile app in real time |
| **GPS tagging** | Every scan is geotagged for mapping tree inventories |
| **SD card logging** | All raw waveforms and ToF matrices stored as CSV for post-processing |
| **OLED status display** | 128×64 OLED shows scan progress, battery, sensor count, and quality metrics |
| **USB-C** | Charging, data download, and firmware updates |
| **IP65 weatherproof** | Operates in rain and dust — field-ready for forestry work |
| **8-hour battery life** | 2200 mAh Li-Po supports a full day of inspections (≈200 scans) |
| **Open-source toolchain** | Hardware (CERN-OHL), firmware (GPL), app (MIT) — fully hackable |

---

## 3. Hardware Specifications

### 3.1 Microcontroller

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32H733VIT6 — ARM Cortex-M7 |
| **Clock** | 280 MHz |
| **Flash** | 1 MB (external 8 MB QSPI flash for waveform storage) |
| **RAM** | 564 KB SRAM + 128 KB DTCM |
| **FPU** | Double-precision hardware FPU (critical for SART math) |
| **DSP** | CMSIS-DSP for FIR filtering and cross-correlation |

The STM32H733 was chosen for its combination of high clock speed, double-precision FPU (essential for the iterative tomographic reconstruction), generous SRAM for the slowness matrix, and integrated BLE-capable UART. The hardware FPU accelerates the SART inner loop by 8–10× compared to software float.

### 3.2 Time-to-Digital Converter

| Parameter | Value |
|-----------|-------|
| **TDC** | TI (previously acam) TDC-GP22 |
| **Resolution** | 22 ps (picosecond) |
| **Range** | 500 ns – 4 ms |
| **Interface** | SPI @ 20 MHz |
| **Channels** | 2 (START + STOP), multiplexed across 16 sensors |

The TDC-GP22 measures the time between the transmit pulse and the first received signal crossing a threshold. With 22 ps resolution and typical wood wave velocities of 1500–4000 m/s, the spatial resolution of the tomogram is approximately 0.03–0.09 mm in the time domain, which translates to practical cell resolution of 2–5 cm in the reconstructed image after accounting for multi-path and grain effects.

### 3.3 Ultrasonic Transducer Array

| Parameter | Value |
|-----------|-------|
| **Transducer** | Custom 60 kHz piezo-composite disc, 14 mm diameter |
| **Bandwidth** | 50–80 kHz (−6 dB) |
| **Coupling** | Magnetic — neodymium disc magnet in sensor housing mates to steel strap clamps |
| **Housing** | 3D-printed PETG, IP65 with O-ring seal |
| **Cable** | Shielded twisted-pair, 2 m, M8 connector |
| **Quantity** | 8–16 sensors (auto-detected via cable ID resistors) |
| **Excitation** | ±200 V spike from MOSFET H-bridge, programmable pulse width 1–10 µs |

The 60 kHz frequency is chosen as a compromise between attenuation (lower frequencies penetrate further through wet wood) and spatial resolution (higher frequencies give sharper tomograms). 60 kHz provides good penetration in trunks up to 80 cm diameter while maintaining adequate resolution.

### 3.4 Analog Front End

| Stage | Description |
|-------|-------------|
| **Protection** | TVS diodes + series resistors protect inputs from the ±200 V transmit pulse |
| **Pre-amp** | OPA657 low-noise JFET op-amp, gain = 20× (26 dB) |
| **Bandpass filter** | 4th-order active Butterworth, center 60 kHz, BW 30 kHz |
| **Variable gain** | AD8331 VGA, 0–48 dB programmable, controlled by DAC |
| **Comparator** | Fast ADCMP601 (1 ns propagation delay) for threshold crossing → TDC STOP |
| **ADC** | 12-bit, 5 MSPS for waveform capture (optional raw logging) |

### 3.5 Connectivity

| Interface | Usage |
|-----------|-------|
| **BLE 5.2** | Nordic nRF52833 module via UART @ 1 Mbps; mobile app communication |
| **USB-C** | USB 2.0 Full-Speed; charging (5 V/2 A), data download, DFU firmware update |
| **SD card** | MicroSD slot, SPI mode, FAT32; raw data + waveform logging |
| **GPS** | u-blox NEO-M9N, 1.8 m accuracy, 25 Hz; geotags every scan |

### 3.6 Power

| Parameter | Value |
|-----------|-------|
| **Battery** | 2200 mAh 3.7 V Li-Po (8.14 Wh) |
| **Charging** | MCP73871, USB-C 5 V/2 A input, 4.2 V CC/CV charge |
| **Regulation** | Buck: 3.3 V (MCU, BLE, GPS), 5 V (AFE); Boost: ±12 V → ±200 V pulse (via transformer) |
| **Power management** | Auto-sleep after 5 min idle; sensor mux power-gated between scans |
| **Battery life** | ~8 hours active scanning (~200 scans), ~72 hours standby |
| **Fuel gauge** | MAX17048, I²C, ±1% accuracy |

### 3.7 Form Factor

| Parameter | Value |
|-----------|-------|
| **Enclosure** | IP65 polycarbonate, 120 × 80 × 35 mm |
| **Weight** | 280 g (scanner unit) + 45 g per sensor |
| **Operating temp** | −10 °C to +50 °C |
| **Display** | 0.96" OLED, 128×64, SPI |
| **Controls** | 3-button (Scan, Mode, Power) + rotary encoder for sensor count adjustment |
| **Mounting** | Belt clip + lanyard; sensors on ratchet strap around trunk |

---

## 4. Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        LignoScan Scanner                     │
│                                                              │
│  ┌──────────┐    SPI     ┌──────────────┐    UART            │
│  │ STM32H733│◄──────────►│  TDC-GP22    │         ┌────────┐│
│  │  280 MHz │           │  22 ps TDC    │         │nRF52833││
│  │  Cortex-M7│          └──────┬───────┘         │ BLE5.2 ││
│  │  DP-FPU  │                 │ STOP             └───┬────┘│
│  │          │                 │                      │     │
│  │  QSPI    │     ┌───────────▼──────────────┐      │     │
│  │  Flash   │     │   Analog Front End        │      │     │
│  │  8 MB    │     │  ┌─────┐  ┌──────┐       │      │     │
│  │          │     │  │Preamp│→│BPF60k│→VGA→  │      │     │
│  │  SD card │     │  │OPA657│  │Butter│  AD8331     │     │
│  │  (SPI)   │     │  └──▲───┘  └──────┘   │   │     │     │
│  │          │     │     │              └───┘   │     │     │
│  │  GPS     │     │     │           Comparator  │     │     │
│  │  NEO-M9N │     │  16:1 MUX    ADCMP601       │     │     │
│  │  (UART)  │     │  HC4067        │            │     │     │
│  │          │     │     │           STOP───►TDC │     │     │
│  │  OLED    │     │     │                      │     │     │
│  │  (SPI)   │     │     ▼                      │     │     │
│  │          │     │  ┌──────────────┐          │     │     │
│  │  Fuel    │     │  │ TX MUX + HV  │          │     │     │
│  │  Gauge   │     │  │ H-Bridge     │          │     │     │
│  │  MAX17048│     │  │ ±200V Spike  │          │     │     │
│  │  (I²C)   │     │  └──────┬───────┘          │     │     │
│  │          │     │         │ TX               │     │     │
│  └────┬─────┘     └─────────┼──────────────────┘     │     │
│       │                     │                        │     │
│       │ 16 sensor channels  │                        │     │
│       ▼                     ▼                        ▼     │
│  ┌────────────────────────────────────────┐   ┌──────────┐│
│  │       16× M8 Sensor Connectors          │   │ USB-C    ││
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐      │   │ Charge + ││
│  │  │S0││S1││S2││S3││S4││S5││S6││S7│      │   │ Data +   ││
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘      │   │ DFU      ││
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐      │   └──────────┘│
│  │  │S8││S9││10││11││12││13││14││15│      │               │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘      │               │
│  └────────────────────────────────────────┘               │
│                                                             │
│  Power: MCP73871 Charger ← USB-C 5V                         │
│         2200 mAh Li-Po → Buck 3.3V / 5V → Boost ±200V       │
│         MAX17048 Fuel Gauge                                 │
└─────────────────────────────────────────────────────────────┘
         │ BLE 5.2
         ▼
┌────────────────────┐
│  Mobile App (RN)   │
│  • Tomogram view   │
│  • Scan management │
│  • GPS map         │
│  • PDF reports     │
│  • Tree inventory  │
└────────────────────┘
```

### Signal Flow Summary

1. **TX path:** MCU triggers HV H-bridge → ±200 V spike on selected sensor (via 16:1 TX mux) → piezo generates ultrasonic wave in wood.
2. **RX path:** Wave arrives at all other sensors → MUX selects one receiver → OPA657 pre-amp → 60 kHz BPF → AD8331 VGA → ADCMP601 comparator → STOP pulse to TDC-GP22.
3. **Timing:** TDC measures START→STOP interval = time-of-flight. The MCU sweeps through all N−1 receiver channels per transmitter, then advances to the next transmitter.
4. **Reconstruction:** MCU assembles N×N ToF matrix → computes ray paths (chords through trunk) → runs SART to produce 2D velocity map → classifies cells as sound / moderate / severe decay.
5. **Output:** Tomogram + classification sent via BLE to app; raw data logged to SD card with GPS timestamp.

---

## 5. Firmware Design

### 5.1 Architecture

The firmware is written in portable C (C11) targeting the STM32H733 using bare-metal register-level programming with CMSIS-Core. No HAL dependency — direct register access for deterministic timing and minimal overhead.

**Source files:**

| File | Purpose | Lines |
|------|---------|-------|
| `main.c` | System init, state machine, scan orchestration | ~250 |
| `board.h` | Pin assignments, clock config, board constants | ~120 |
| `registers.h` | STM32H733 register definitions & bit masks | ~180 |
| `drivers/tdc.c/.h` | TDC-GP22 SPI driver, calibration, ToF measurement | ~220 |
| `drivers/mux.c/.h` | 16-channel TX/RX mux control via GPIO expanders | ~140 |
| `drivers/hv.c/.h` | HV pulse generation, H-bridge control, safety interlocks | ~160 |
| `drivers/afe.c/.h` | VGA gain control, comparator threshold, signal quality | ~180 |
| `drivers/tomography.c/.h` | SART reconstruction, ray tracing, decay classification | ~320 |
| `drivers/ble.c/.h` | nRF52833 UART protocol, packet framing, BLE GATT bridge | ~200 |
| `drivers/gps.c/.h` | NEO-M9N NMEA parser, geotagging | ~150 |
| `drivers/sdlog.c/.h` | SD card SPI driver, FAT32 file operations, CSV logging | ~200 |
| `drivers/display.c/.h` | OLED SSD1306 driver, scan progress UI | ~140 |
| `drivers/power.c/.h` | Battery monitoring, charging control, sleep modes | ~120 |

**Total firmware: ~2400 lines of C**

### 5.2 State Machine

```
                  ┌──────────┐
                  │   BOOT   │
                  └────┬─────┘
                       ▼
                  ┌──────────┐
        ┌────────►│   IDLE   │◄────────┐
        │         └────┬─────┘         │
        │              │ SCAN button   │
        │              ▼               │
        │         ┌──────────┐         │
        │         │ CALIBRATE│         │
        │         └────┬─────┘         │
        │              │               │
        │              ▼               │
        │         ┌──────────┐         │
        │         │  ACQUIRE │         │
        │         └────┬─────┘         │
        │              │               │
        │              ▼               │
        │         ┌──────────┐         │
        │         │ RECONSTRUCT        │
        │         └────┬─────┘         │
        │              │               │
        │              ▼               │
        │         ┌──────────┐         │
        │         │  TRANSMIT│         │
        │         └────┬─────┘         │
        │              │               │
        └──────────────┴───────────────┘
```

1. **BOOT:** Initialize clocks, peripherals, read sensor cable IDs, load calibration from flash.
2. **IDLE:** OLED shows battery, sensor count, GPS fix status. Awaiting button press.
3. **CALIBRATE:** Measure cable delays (cross-talk compensation) — one transducer is tapped manually; system records baseline propagation in air for cable-length normalization.
4. **ACQUIRE:** For each of N transmitters, fire HV pulse and measure ToF to all N−1 receivers. Average 16 shots per pair for noise reduction. Build N×N ToF matrix.
5. **RECONSTRUCT:** Run SART algorithm (50 iterations) on the slowness matrix. Classify each cell. Compute overall decay severity score.
6. **TRANSMIT:** Send tomogram + metadata via BLE to app. Log raw data to SD card with GPS coordinates. Return to IDLE.

### 5.3 Tomographic Reconstruction (SART)

The reconstruction uses the **Simultaneous Algebraic Reconstruction Technique**, adapted for circular geometry:

1. The trunk cross-section is discretized into a **polar grid** (radial × angular cells), typically 8 radial × 16 angular = 128 cells for a 12-sensor scan.
2. For each transmitter→receiver pair, a **ray path** is computed as the straight chord connecting the two sensor positions on the trunk perimeter.
3. For each ray, the **ray sum** (expected travel time) is the sum of (cell slowness × chord length through that cell) over all cells the ray passes through.
4. SART iteratively corrects the slowness of each cell by distributing the residual (measured − computed travel time) back along the ray, weighted by ray length.
5. After convergence (typically 30–50 iterations), the slowness map is converted to velocity and classified:
   - **Velocity > 2500 m/s:** Sound wood (green)
   - **1500–2500 m/s:** Moderate decay (yellow)
   - **< 1500 m/s:** Severe decay / hollow (red)

The double-precision FPU on the STM32H733 is critical here — single-precision accumulators lose significant digits after 30+ iterations with 128 cells. Benchmark: 50 iterations × 132 rays × 128 cells = ~845K FLOPs, completing in ~1.2 s at 280 MHz with FPU.

### 5.4 Signal Quality & Error Handling

- **Auto-gain:** VGA gain is automatically adjusted per receiver channel to normalize signal amplitude before threshold crossing — compensates for varying bark thickness and coupling quality.
- **Outlier rejection:** ToF measurements more than 3σ from the pair average (across 16 shots) are discarded.
- **Coupling check:** If signal amplitude is below threshold for any channel, the OLED flags "Sensor X: poor coupling" and the app prompts the user to reseat that sensor.
- **Cable ID:** Each sensor cable has a unique ID resistor (0–15 via resistor divider); the MCU reads this on connection to auto-detect sensor count and positions.
- **Safety:** HV is interlocked — no pulse is generated unless the scan button is physically pressed and all sensor cables are detected. A hardware watchdog disables the HV supply if the MCU stops responding.

---

## 6. Mobile Application

The companion app is built in **React Native** (Expo) for iOS and Android. It provides:

### Screens

| Screen | Function |
|--------|----------|
| **HomeScreen** | Connection status, battery, scan button, quick-start guide |
| **TomogramScreen** | Real-time 2D velocity map with color-coded decay classification; pinch-to-zoom, tap cell for velocity value |
| **ScanListScreen** | History of scans with date, GPS, tree ID, severity score |
| **TreeInventoryScreen** | Map view of all scanned trees; color-coded pins by severity; filter by risk level |
| **ReportScreen** | Generates PDF inspection report with tomogram, GPS, severity, recommendations |
| **SettingsScreen** | Sensor count, reconstruction iterations, decay thresholds, units (metric/imperial), data export |

### Communication Protocol

BLE GATT service `0000LIGN-0000-1000-8000-00805F9B34FB`:
- **Characteristic `SCAN_TRIGGER`:** Write to start scan (1 byte = sensor count)
- **Characteristic `SCAN_STATUS`:** Notify — scan progress, state, errors
- **Characteristic `TOF_MATRIX`:** Notify — raw ToF data (N×N×4 bytes, float32)
- **Characteristic `TOMOGRAM`:** Notify — reconstructed velocity map (cells × 4 bytes) + classification (cells × 1 byte)
- **Characteristic `GPS_DATA`:** Notify — lat, lon, altitude, HDOP, timestamp
- **Characteristic `DEVICE_INFO`:** Read — firmware version, battery, serial

### Data Format

Raw data is logged to SD card as CSV:
```
# LignoScan Raw Data — 2026-08-17T14:32:00Z
# Tree_ID: OAK-042, GPS: 47.6062,-122.3321
# Sensors: 12, Diameter: 45.0cm, Height: 1.3m
# Author: jayis1
TX,RX,ToF_ns,Amplitude_mV,Quality
0,1,152340,820,GOOD
0,2,289100,650,GOOD
0,3,412800,430,GOOD
...
```

---

## 7. Use Cases & Target Audience

### Primary Users

1. **Certified Arborists (ISA Certified):** Routine tree-risk assessments for private clients, municipalities, and insurance claims. LignoScan provides objective, documented evidence of internal condition — far more convincing than visual assessment alone.

2. **Urban Forestry Departments:** Screening city street trees for hazard potential. A single arborist with LignoScan can assess 30–50 trees per day, building a GIS-linked inventory of tree health that informs removal/benefit decisions.

3. **Utility Vegetation Managers:** Assessing trees near power lines for internal decay that could cause catastrophic failure during storms. Non-destructive testing is critical when removal permits are contested.

4. **Botanical Gardens & Arboreta:** Monitoring valuable specimen trees (e.g., heritage oaks, sequoias) for early decay detection without invasive sampling that could introduce pathogens.

5. **Tree Preservation Orders (UK/EU):** When development threatens protected trees, LignoScan provides quantified structural integrity data for legal proceedings.

6. **Research & Education:** University forestry programs, wood science labs, and citizen-science projects studying decay dynamics in urban forests.

### Typical Workflow

1. Arborist identifies a tree of concern (visual symptoms: fungal fruiting bodies, cavities, lean).
2. Selects a test height (typically 1.3 m, or at the point of visible defect).
3. Measures trunk circumference, wraps the sensor strap around the trunk.
4. Attaches 8–16 ultrasonic sensors to the magnetic strap clamps at evenly spaced positions.
5. Enters tree ID and species in the app, presses SCAN on the LignoScan unit.
6. Device performs the full scan in ~15 seconds (12 sensors × 11 receivers × 16 averages).
7. Reconstruction completes in ~3 seconds; tomogram appears on the phone.
8. Arborist reviews the color-coded map: green = sound, yellow = moderate decay, red = severe decay/hollow.
9. App computes a **Tomographic Decay Index (TDI)** — percentage of cross-section with velocity below sound-wood threshold — and recommends action (monitor / prune / remove).
10. PDF report is generated and emailed to the client or uploaded to the tree inventory database.

---

## 8. Comparison to Alternatives

| Method | Cost | Invasive? | Coverage | Speed | Resolution |
|--------|------|-----------|----------|-------|------------|
| **LignoScan** | ~$500 BOM | No (surface coupling) | Full cross-section | ~20 s/scan | 2–5 cm cells |
| Increment borer | $150–$400 | Yes (core sample) | Single radial line | 5 min/tree | High (core) |
| Resistograph | $3,000–$8,000 | Yes (fine drill) | Single radius | 2 min/drill | 0.1 mm radial |
| Picus Sonic Tomograph | $15,000–$25,000 | No | Full cross-section | ~5 min/scan | 2–5 cm cells |
| Radar (GPR) | $10,000–$40,000 | No | Limited (moisture confound) | ~2 min/scan | Variable |

LignoScan matches the measurement principle of the gold-standard Picus at ~3% of the cost, with open-source software and a modern mobile interface.

---

## 9. Bill of Materials (Key Components)

| Ref | Part | Qty | Est. Cost |
|-----|------|-----|-----------|
| U1 | STM32H733VIT6 | 1 | $14.00 |
| U2 | TDC-GP22 | 1 | $18.00 |
| U3 | nRF52833 Module | 1 | $6.00 |
| U4 | NEO-M9N GPS | 1 | $25.00 |
| U5 | OPA657 (pre-amp) | 1 | $8.50 |
| U6 | AD8331 (VGA) | 1 | $12.00 |
| U7 | ADCMP601 (comparator) | 1 | $4.00 |
| U8 | MCP73871 (charger) | 1 | $3.00 |
| U9 | MAX17048 (fuel gauge) | 1 | $2.50 |
| U10 | SSD1306 OLED 0.96" | 1 | $3.00 |
| Q1–Q4 | IRFH MOSFETs (H-bridge) | 4 | $4.00 |
| T1 | Pulse transformer 1:10 | 1 | $5.00 |
| MUX1–2 | HC4067 16:1 analog mux | 2 | $2.00 |
| S1–S16 | 60 kHz piezo transducers | 16 | $48.00 |
| Misc | PCB, enclosure, connectors, passives | — | $25.00 |
| **Total (16-sensor kit)** | | | **~$185** |

---

## 10. License & Attribution

- **Hardware (KiCad schematics, PCB, mechanical):** CERN-OHL-S v2
- **Firmware (C source):** GPL-2.0
- **Mobile App (React Native):** MIT

**Author:** jayis1
**Copyright:** © 2026 jayis1. All rights reserved.

All designs, firmware, code, and documentation are credited to jayis1. This project is open-source to advance the field of non-destructive tree assessment and make professional arboricultural tools accessible to all.

---

## 11. Future Enhancements

- **3D tomography:** Stack multiple sensor rings at different heights to reconstruct 3D decay volumes.
- **AI decay classification:** On-device neural network (via STM32Cube.AI) to classify decay type (brown rot, white rot, soft rot) from velocity patterns.
- **Automatic species calibration:** Built-in database of species-specific sound-wood velocities (oak ~3600 m/s, pine ~2800 m/s, maple ~3200 m/s) for more accurate thresholds.
- **Stress wave directionality:** Multi-axis transducers to detect anisotropic grain effects and improve reconstruction accuracy.
- **Cloud sync:** Optional cloud backup and fleet management for municipal forestry teams.
- **Drone-mounted variant:** For assessing canopy branches in tall trees without climbing.

---

*LignoScan — seeing inside trees without cutting them open.*
*Designed by jayis1, 2026.*