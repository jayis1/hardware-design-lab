# CarbonFlux — Open-Source Soil CO₂ Flux & Respiration Monitor

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

![PCB](https://img.shields.io/badge/PCB-120%C3%9780mm-blue)
![MCU](https://img.shields.io/badge/MCU-STM32U5A9-orange)
![Sensor](https://img.shields.io/badge/CO₂-SCD41-brightgreen)
![LoRa](https://img.shields.io/badge/LoRa-SX1262-yellow)
![License](https://img.shields.io/badge/License-MIT-green)

> **A low-power, open-hardware stationary soil respiration chamber connected to a compact measurement console. CarbonFlux continuously measures CO₂ concentration, soil temperature, volumetric water content, barometric pressure, and PAR (photosynthetically active radiation) to compute soil CO₂ efflux (µmol CO₂·m⁻²·s⁻¹) — the fundamental metric for carbon cycle science, agricultural carbon sequestration verification, and ecosystem ecology.**

---

## Table of Contents

1. [Overview & Purpose](#1-overview--purpose)
2. [How It Works](#2-how-it-works)
3. [Hardware Specifications](#3-hardware-specifications)
4. [Architecture & Block Diagram](#4-architecture--block-diagram)
5. [Firmware Design](#5-firmware-design)
6. [Companion Application](#6-companion-application)
7. [Data & Calibration](#7-data--calibration)
8. [Use Cases & Target Audience](#8-use-cases--target-audience)
9. [Directory Structure](#9-directory-structure)
10. [Getting Started](#10-getting-started)

---

## 1. Overview & Purpose

**CarbonFlux** is a complete, open-hardware solution for measuring soil CO₂ efflux — the rate at which carbon dioxide is released from the soil surface into the atmosphere. Soil respiration is the second-largest terrestrial carbon flux (after photosynthesis), releasing ~60–80 Pg C/year globally. Understanding this flux at fine spatial and temporal scales is critical for:

- **Carbon credit verification** — agricultural carbon farming projects need measured, not modelled, flux data
- **Climate change research** — soil carbon feedbacks are the largest uncertainty in climate projections
- **Precision agriculture** — soil respiration correlates with microbial activity, root health, and decomposition
- **Ecosystem ecology** — partitioning heterotrophic vs. autotrophic respiration

Existing commercial solutions (LI-COR LI-8100A, PP Systems EGM-5, Campbell Scientific) cost $10,000–$35,000 per unit and are **closed, proprietary black boxes**. CarbonFlux aims to provide **>95% of the scientific capability at <5% of the cost** (~$350 BOM), fully open from sensor to cloud.

### Key Innovations

1. **Automated closed-chamber cycling** — a low-power servo-operated chamber lid opens between measurements for equilibration, then closes for a 120–300 second accumulation period, after which flux is computed from the linear rise of CO₂ concentration.

2. **Multi-depth soil profile** — optional vertical array of soil moisture/temperature sensors at 5 cm, 15 cm, and 30 cm depths for partitioning flux sources.

3. **LoRaWAN uplink** — data transmitted over long-range radio (up to 15 km line-of-sight) to a local gateway or directly to The Things Network, enabling battery-powered deployment in remote fields.

4. **On-device flux computation** — the STM32U5A9 microcontroller runs a real-time linear regression on CO₂ accumulation data, computes corrected flux (accounting for temperature, pressure, chamber volume), and transmits only final measurements — not raw time series.

5. **Solar-ready power architecture** — designed for continuous unattended operation with a 6 W solar panel and 12 Ah LiFePO₄ battery, drawing ~45 mW average in measurement mode.

---

## 2. How It Works

### Measurement Principle

CarbonFlux uses the **closed dynamic chamber method** (also called non-steady-state flow-through):

1. **Equilibration (open)** — The chamber lid is opened for ≥60 seconds, allowing the chamber headspace to equilibrate with ambient air.

2. **Closure & accumulation (closed)** — The lid seals shut. CO₂ respired from the soil accumulates inside the fixed-volume headspace (typically 4.5 L for the standard 20 cm diameter collar).

3. **Measurement** — An NDIR (non-dispersive infrared) CO₂ sensor (Sensirion SCD41) samples concentration every 2 seconds. The rate of concentration increase (dC/dt) is proportional to the soil CO₂ efflux.

4. **Flux calculation** — After 120–300 seconds of accumulation (depending on expected flux rate), the firmware fits a linear regression to the CO₂ concentration vs. time data and computes:

   ```
   F = (dC/dt) × (V × P) / (A × R × T)
   ```

   Where:
   - F = soil CO₂ efflux (µmol·m⁻²·s⁻¹)
   - dC/dt = rate of change of CO₂ concentration (µmol·mol⁻¹·s⁻¹)
   - V = chamber headspace volume (m³)
   - A = soil surface area enclosed (m²)
   - P = barometric pressure (kPa)
   - T = air temperature in chamber (K)
   - R = ideal gas constant (8.314462 L·kPa·mol⁻¹·K⁻¹)

5. **Return to step 1** — lid opens, chamber vents, next cycle begins.

### Measurement Modes

| Mode | Description | Power | Typical Duty Cycle |
|------|-------------|-------|--------------------|
| **Continuous** | Full flux measurement every 15–60 min | ~45 mW avg | 24/7, solar-powered |
| **Burst** | 1 Hz sampling for 30 min, triggered by rain/irrigation | ~120 mW avg | Event-driven |
| **Standby** | Deep sleep, wake on RTC once per day | ~15 µW | Long-term deployment |
| **Calibration** | Zero/span gas calibration routine | ~200 mW | Manual trigger |

---

## 3. Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **CO₂ Sensor** | Sensirion SCD41 — NDIR, ±(40 ppm + 5%) accuracy, 0–40,000 ppm, I²C |
| **Soil Moisture** | Sensirion SHT31+ TEROS-11 compatible, SDI-12 or analog |
| **Soil Temperature** | DS18B20 (3×, at 5/15/30 cm depths), 1-Wire, ±0.5°C |
| **Barometric Pressure** | Infineon DPS310 — ±0.002 hPa (high precision), I²C |
| **PAR Sensor** | Apogee SQ-420 or compatible quantum sensor, 0–3000 µmol·m⁻²·s⁻¹, analog 0–2.5V |
| **Air Temperature** | Texas Instruments TMP117 — ±0.1°C, I²C |
| **MCU** | ST STM32U5A9ZG — Cortex-M33 @ 160 MHz, 2 MB Flash, 1.5 MB SRAM TrustZone |
| **LoRa Radio** | SEMTECH SX1262 — 868/915 MHz, +22 dBm, LoRaWAN 1.0.4 |
| **RTC** | Micro Crystal RV-8803-C7 — ±3 ppm, I²C, backup battery |
| **ADC** | 3× ADC channels (12-bit) on STM32U5 for PAR + spare analog |
| **Chamber Actuator** | SG90 micro servo (lid open/close), PWM |
| **Power System** | Solar MPPT (CN3791) + LiFePO₄ 12.8V 12Ah + TPS63070 buck-boost 3.3V |
| **Data Storage** | 16 MB W25Q128JV SPI flash — 2+ years of hourly data |
| **Connectivity** | LoRaWAN (primary), USB-C (configuration + debug), BLE 5.2 nRF52840 (optional local fetch) |
| **Collars** | 20 cm diameter PVC, 10 cm insertion depth (standard), 4.5 L headspace |
| **Enclosure** | IP65 weatherproof (chamber console), IP68 submersible (soil sensor stake) |
| **Operating Temperature** | -20°C to +60°C |
| **BOM Cost (1K volume)** | ~$350 USD |
| **PCB** | 120 × 80 mm, 4-layer FR-4, ENIG finish |

---

## 4. Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                     ABOVE GROUND (Console)                          │
│                                                                     │
│  ┌──────────┐   ┌──────────────┐   ┌──────────┐   ┌──────────────┐ │
│  │ Solar    │──▶│ CN3791 MPPT  │──▶│ LiFePO₄  │──▶│ TPS63070     │ │
│  │ Panel 6W │   │ Charger      │   │ 12.8V12Ah│   │ Buck-Boost   │ │
│  └──────────┘   └──────────────┘   └──────────┘   │ 3.3V / 5V    │ │
│                                                    └──────────────┘ │
│                                                                     │
│  ┌──────────┐   ┌──────────────────────────────────────────────────┐│
│  │ SCD41    │──▶│                                                ││
│  │ CO₂ NDIR │   │                   STM32U5A9ZG                  ││
│  └──────────┘   │              Cortex-M33 @ 160 MHz              ││
│                 │              2 MB Flash / 1.5 MB SRAM          ││
│  ┌──────────┐   │                                                ││
│  │ DPS310   │──▶│  ┌─────────────────┐  ┌──────────────────────┐ ││
│  │ Baro     │   │  │ Flux Engine     │  │ LoRaWAN Stack        │ ││
│  └──────────┘   │  │ (Linear Regress)│  │ (SX1262 + TCXO)      │ ││
│                 │  └─────────────────┘  └──────────┬───────────┘ ││
│  ┌──────────┐   │  ┌─────────────────┐            │             ││
│  │ TMP117   │──▶│  │ Data Logger     │            │             ││
│  │ Air Temp │   │  │ (W25Q128 SPI)   │            ▼             ││
│  └──────────┘   │  └─────────────────┘    ┌──────────────┐      ││
│                 │                         │ SMA Antenna  │      ││
│  ┌──────────┐   │  ┌─────────────────┐    │ 868/915 MHz  │      ││
│  │ SG90     │◀──│  │ Chamber Control │    └──────────────┘      ││
│  │ Servo    │   │  │ (PWM + Stall    │                          ││
│  └──────────┘   │  │  Detection)     │                          ││
│                 │  └─────────────────┘                          ││
│  ┌──────────┐   │  ┌─────────────────┐                          ││
│  │ RV-8803  │◀──│  │ USB-C (CDC+DFU) │──────────────────────────┤│
│  │ RTC      │   │  └─────────────────┘        USB-C             ││
│  └──────────┘   └──────────────────────────────────────────────────┘
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
                              │
                  ┌───────────┴───────────┐
                  │   6-Wire Shielded     │
                  │   (1-Wire, 12C, 12V)  │
                  └───────────┬───────────┘
                              │
┌─────────────────────────────────────────────────────────────────────┐
│                     BELOW GROUND (Sensor Stake)                     │
│                                                                     │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐            │
│  │ DS18B20      │   │ DS18B20      │   │ DS18B20      │            │
│  │ Soil Temp    │   │ Soil Temp    │   │ Soil Temp    │            │
│  │ @ 5 cm       │   │ @ 15 cm      │   │ @ 30 cm      │            │
│  └──────────────┘   └──────────────┘   └──────────────┘            │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Soil Moisture Probe (TEROS-11 / analog capacitance)         │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Apogee SQ-420 PAR Sensor (0–2.5V analog)                    │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Bus Architecture

| Bus | Devices | Speed | Notes |
|-----|---------|-------|-------|
| **I²C1** | SCD41, DPS310, TMP117, RV-8803 | 400 kHz | 3.3V only, shared bus with individual EN lines |
| **1-Wire** | DS18B20 × 3 | 15 kbps | Parasitic power on same line, unique ROM IDs |
| **SPI1** | SX1262 (LoRa) | 8 MHz | DIO0–DIO3 interrupt mapping |
| **SPI2** | W25Q128 (Flash) | 40 MHz | Quad-SPI supported |
| **ADC1** | PAR sensor (IN1), Battery voltage (IN2), Spare (IN3) | 12-bit | Internal VREF, 3.3V reference |
| **TIM2_CH1** | SG90 servo PWM | 50 Hz | 1–2 ms pulse for 0–180° |

---

## 5. Firmware Design

### Architecture Overview

The firmware follows a **super-loop with cooperative multitasking** pattern, driven by a 1 kHz SysTick scheduler. The STM32U5A9ZG was chosen over the STM32L4/L5 for its high-performance Cortex-M33 with TrustZone (hardware isolation for security keys), massive SRAM (1.5 MB for data buffering), and ultra-low-power **Stop 2 mode** (2.5 µW with full SRAM retention).

### Firmware Modules

| Module | File | Description |
|--------|------|-------------|
| **System** | `main.c` | Scheduler, state machine, initialization |
| **Board** | `board.h` | Pin mappings, peripheral clocks, PCB version |
| **Registers** | `registers.h` | MMIO register definitions, bitfield unions |
| **CO₂ Sensor** | `drivers/scd41.c` | SCD41 I²C driver with FIFO, self-calibration, CRC |
| **Barometer** | `drivers/dps310.c` | DPS310 precision mode, temperature compensation |
| **Soil Temp** | `drivers/ds18b20.c` | 1-Wire implementation, parasitic power handling |
| **LoRa** | `drivers/sx1262.c` | SX1262 LoRa modem driver, LoRaWAN stack |
| **Flash** | `drivers/w25q128.c` | SPI/QSPI flash wear-levelling, ring buffer |
| **RTC** | `drivers/rv8803.c` | RV-8803 RTC driver, alarm scheduling |
| **Servo** | `drivers/servo.c` | PWM servo control, stall detection |
| **ADC** | `drivers/adc.c` | ADC oversampling, averaging, VREF calibration |
| **Flux Engine** | `drivers/flux_engine.c` | Linear regression on CO₂ data, flux computation |
| **Power Mgmt** | `drivers/power.c` | Battery SOC, solar MPPT monitoring, sleep scheduling |

### State Machine

```
                    ┌─────────────┐
                    │   POWER UP  │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
        ┌──────────▶│  INITIALIZE │◀──────────────────┐
        │           └──────┬──────┘                   │
        │                  │                          │
        │        ┌─────────┴──────────┐               │
        │        ▼                    ▼               │
        │ ┌──────────┐      ┌──────────────────┐      │
        │ │ CALIBRATE│      │    EQUILIBRATE   │      │
        │ │ (manual) │      │ (lid open, 60 s) │      │
        │ └────┬─────┘      └────────┬─────────┘      │
        │      │                     │                 │
        │      ▼                     ▼                 │
        │ ┌──────────┐      ┌──────────────────┐      │
        │ │ ZERO/SPAN│      │     ACCUMULATE   │      │
        │ │ (manual) │      │ (lid closed,     │      │
        │ └────┬─────┘      │  CO₂ sampling)   │      │
        │      │            └────────┬─────────┘      │
        │      │                     │                 │
        │      │                     ▼                 │
        │      │            ┌──────────────────┐      │
        │      │            │   COMPUTE FLUX   │      │
        │      │            │ (linear regress) │      │
        │      │            └────────┬─────────┘      │
        │      │                     │                 │
        │      │                     ▼                 │
        │      │            ┌──────────────────┐      │
        │      │            │     LOG + TX     │      │
        │      │            │ (flash storage & │      │
        │      │            │  LoRaWAN upload) │      │
        │      │            └────────┬─────────┘      │
        │      │                     │                 │
        │      │            ┌────────┴────────┐        │
        │      │            ▼                 ▼        │
        │      │     ┌──────────┐    ┌──────────────┐ │
        │      │     │ DEEP SLEEP│    │  BURST MODE │ │
        │      │     │ (15–60 m)│    │ (30 min 1Hz)│ │
        │      │     └────┬─────┘    └──────┬───────┘ │
        │      │          │                 │         │
        │      └──────────┴─────────────────┘         │
        │                                             │
        └─────────────────────────────────────────────┘
```

### Data Logging Format

Binary records (32 bytes each) stored in W25Q128 ring buffer:

```
Offset  Size  Field
0       4     Timestamp (Unix UTC, seconds)
4       4     CO₂ concentration (ppm, float × 1000)
8       4     Soil CO₂ flux (µmol·m⁻²·s⁻¹, float × 1000)
12      2     Soil temperature @5cm (°C, int16 × 100)
14      2     Soil temperature @15cm (°C, int16 × 100)
16      2     Soil temperature @30cm (°C, int16 × 100)
18      2     Volumetric water content (% × 100)
20      2     Barometric pressure (hPa × 10)
22      2     Air temperature (°C, int16 × 100)
24      2     PAR (µmol·m⁻²·s⁻¹, uint16)
26      1     Battery SOC (%)
27      1     Flags (bit0: rain, bit1: calibration, bit2: error)
28      4     Reserved
32     TOTAL
```

### Key Design Decisions

1. **SCD41 over SCD30/SCD40** — The SCD41 offers 0–40,000 ppm range (vs. 0–5,000 for SCD30), critical for accumulation chamber measurements that can reach 5,000+ ppm. I²C with built-in CRC ensures data integrity on long cable runs.

2. **Separate barometer (DPS310) vs. internal** — External DPS310 provides ±0.002 hPa precision vs. the ±0.5 hPa of most MCU-integrated barometers. This matters because flux calculation includes a pressure term, and errors compound.

3. **Cortex-M33 with TrustZone** — The STM32U5 series provides hardware-isolated secure enclave for LoRaWAN root keys and calibration coefficients, protecting the most valuable IP (calibration data and network credentials) even if physical access is compromised.

4. **Quad-SPI flash for wear leveling** — The W25Q128 supports 40 MHz QSPI reads with memory-mapped mode, enabling direct-code-execution potential. A ring buffer with wear leveling extends flash lifetime to >20 years at hourly logging.

5. **Servo over solenoid** — A continuous-rotation micro servo with position feedback draws <3 mA when holding position vs. 100–200 mA for a latching solenoid. Stall detection on the servo allows fail-safe lid opening on power loss.

6. **LoRaWAN over NB-IoT/Cat-M1** — LoRaWAN provides sub-50 mW transmission, no SIM fees, and reaches kilometer-range in agricultural fields. Data rate is sufficient: each hourly flux record is ~50 bytes, fitting easily in a single uplink packet.

---

## 6. Companion Application

The CarbonFlux companion app (React Native for iOS/Android) provides:

### Screens

| Screen | Description |
|--------|-------------|
| **Dashboard** | Live flux reading, chamber state, battery, last transmission |
| **Timeseries** | Flux rate over configurable time windows (1h, 1d, 1w, 1m) |
| **Sensor Detail** | Real-time raw sensor values (CO₂, temp, moisture, PAR, pressure) |
| **Configuration** | Chamber volume, measurement interval, LoRaWAN settings, calibration |
| **Map** | Geolocation of deployed units with color-coded flux markers |
| **Data Export** | CSV/JSON export, cloud upload, email |

### BLE Local Fetch

The companion app connects via BLE 5.2 (nRF52840 on board) for:
- Real-time sensor streaming during installation/testing
- Historical data download (full flash dump over BLE at ~50 kB/s)
- Calibration routine initiation
- Firmware update over the air (DFU via BLE)

### LoRaWAN Integration

Data is also transmitted over LoRaWAN to The Things Network or a local ChirpStack gateway. The app can pull data from these cloud backends via REST API for remote monitoring.

---

## 7. Data & Calibration

### Calibration Routine

CarbonFlux supports two-point calibration:

1. **Zero** — Ambient air flushed through soda lime scrubber (~0 ppm CO₂)
2. **Span** — Certified 5000 ppm CO₂ gas cylinder

Calibration coefficients are stored in the secure flash region and encrypted with device-unique key. The companion app guides the user through the calibration process.

### Data Quality Flags

Each record includes quality metadata:
- **R² of linear fit** — threshold ≥0.95 for valid flux
- **Number of samples in regression** — minimum 30 points (60 seconds)
- **Chamber leak detection** — pressure differential across lid seal
- **Temperature drift warning** — >2°C change during accumulation invalidates measurement
- **Rain flag** — precipitation detected by capacitance sensor on lid

---

## 8. Use Cases & Target Audience

### Primary Use Cases

| Use Case | Description | Key Metric |
|----------|-------------|------------|
| **Carbon Credit Verification** | Measure actual soil C sequestration in regenerative agriculture projects | ±0.1 µmol·m⁻²·s⁻¹ |
| **Crop Research** | Partition heterotrophic vs. autotrophic respiration under different tillage/cover crop treatments | Sub-daily flux patterns |
| **Climate Change Ecology** | Monitor soil carbon feedbacks in warming experiments and permafrost regions | Annual flux sums |
| **Wetland Methane Studies** | Co-deploy with CH₄ sensor for full greenhouse gas budget | CO₂:CH₄ ratio |
| **Compost & Biochar** | Quantify CO₂ evolution from composting operations and biochar-amended soils | Cumulative emissions |
| **Agricultural Extension** | Low-cost tool for farm advisors to benchmark soil health improvement | Baseline vs. treatment |

### Target Audience

- **Soil scientists and ecologists** — need affordable, replicable flux measurements
- **Carbon credit project developers** — require verifiable, continuous monitoring data
- **Precision agriculture researchers** — want sub-field resolution on soil respiration
- **DIY environmental monitoring community** — open hardware for citizen science
- **Universities** — teaching tool for ecosystem ecology and instrumentation courses

### Competitive Landscape

| Product | Price | Open? | CO₂ Only | Data Logging | Remote Comms | Battery |
|---------|-------|-------|----------|--------------|--------------|---------|
| LI-COR LI-8100A | ~$25,000 | No | Yes | Yes | Optional | Yes |
| PP Systems EGM-5 | ~$12,000 | No | Yes | Yes | Optional | Yes |
| Campbell Sci CMP3 | ~$15,000 | No | No | Yes | Yes | No |
| **CarbonFlux** | **~$350** | **Yes** | **Yes** | **Yes** | **LoRaWAN** | **Yes** |

---

## 9. Directory Structure

```
carbonflux/
├── README.md                       ← This file
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
├── firmware/
│   ├── main.c                      ← Main state machine, scheduler, init
│   ├── board.h                     ← Pin mappings, clock config, PCB rev
│   ├── registers.h                 ← MMIO register definitions
│   ├── Makefile                    ← arm-none-eabi-gcc build system
│   ├── linker.ld                   ← STM32U5A9ZG linker script
│   └── drivers/
│       ├── scd41.c / .h            ← SCD41 CO₂ sensor I²C driver
│       ├── dps310.c / .h           ← DPS310 barometer driver
│       ├── ds18b20.c / .h          ← DS18B20 1-Wire temperature
│       ├── sx1262.c / .h           ← SX1262 LoRa modem driver
│       ├── w25q128.c / .h          ← W25Q128 SPI flash driver
│       ├── rv8803.c / .h           ← RV-8803 RTC driver
│       ├── servo.c / .h            ← SG90 PWM servo driver
│       ├── adc.c / .h              ← ADC oversampling and PAR driver
│       ├── flux_engine.c / .h      ← Linear regression flux computation
│       └── power.c / .h            ← Power management and sleep
├── kicad/
│   ├── carbonflux.kicad_pro        ← KiCad project file
│   ├── carbonflux.kicad_sch        ← Schematic
│   └── carbonflux.kicad_pcb        ← PCB layout
└── app/
    ├── package.json
    ├── App.js                      ← Root React Native app
    ├── components/
    │   ├── BLEContext.js           ← BLE provisioning and data streaming
    │   ├── FluxChart.js            ← Realtime flux chart component
    │   └── StatusCard.js           ← Device status summary card
    ├── screens/
    │   ├── DashboardScreen.js      ← Live flux, chamber state, battery
    │   ├── TimeseriesScreen.js     ← Historical flux over configurable window
    │   ├── SensorDetailScreen.js   ← All raw sensor values
    │   ├── ConfigurationScreen.js  ← Device settings & calibration
    │   └── DataExportScreen.js     ← Download CSV/JSON, cloud sync
    └── utils/
        └── protocol.js             ← BLE / serial data protocol decoder
```

---

## 10. Getting Started

### Building the Firmware

```bash
# Prerequisites: arm-none-eabi-gcc, stlink-tools, openocd
cd carbonflux/firmware
make clean && make
# Flash via ST-Link
make flash
```

### Building the App

```bash
cd carbonflux/app
npm install
npx react-native run-android  # or run-ios
```

### KiCad

Open `kicad/carbonflux.kicad_pro` in KiCad 8.0 or later. Generate BOM and PCB fabrication outputs via the integrated Fab Toolkit.

### License Summary

| Component | License |
|-----------|---------|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| Python tools & scripts | MIT |
| React Native companion app | MIT |

**Author: jayis1** — All original designs, firmware, and documentation.