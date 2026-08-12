# GrainGuard — In-Silo Stored-Grain Condition & Spoilage Early-Warning Sentinel

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)
**Version:** 1.0.0
**Date:** 2026-08-12

![PCB](https://img.shields.io/badge/PCB-Ø38mm%20probe-blue) ![MCU](https://img.shields.io/badge/MCU-STM32WL55JC-orange) ![Sensors](https://img.shields.io/badge/Sensors-CO₂%20%2B%209×T%20%2B%20RH%20%2B%20acoustic-green) ![Radio](https://img.shields.io/badge/RF-LoRa%20Sub--GHz%20mesh-purple) ![Power](https://img.shields.io/badge/Power-3.6V%20LiSOCl₂-yellow) ![Author](https://img.shields.io/badge/Author-jayis1-orange)

---

## 1. Purpose and Overview

GrainGuard is a novel, open-hardware, **multi-sensor probe sentinel for
continuous in-silo stored-grain condition monitoring and early spoilage
warning**. It is a slender (38 mm diameter, 2.4 m long) sensor probe that is
inserted vertically into a grain silo, grain bin, or flat-storage pile,
where it simultaneously measures the four physical signatures that precede
spoilage — **CO₂ concentration, temperature profile (9 zones),
relative humidity / equilibrium moisture content, and acoustic emission
from insect activity** — and fuses them on-device into a **Spoilage Risk
Index** that is transmitted via a self-healing **LoRa mesh** to a gateway
and React Native companion app, giving grain elevators, farmers, and
commodity warehouses days-to-weeks of advance notice before mold,
heating, or insect infestation ruins a stored crop.

### The Problem

Post-harvest grain losses to spoilage, mold, and insects are a global
catastrophe that hides in plain sight. The Food and Agriculture
Organization estimates that **10–20 % of all harvested grain** is lost
during storage — roughly **300 million tonnes per year**, enough to feed
over 1 billion people. In developed nations, a single "hot spot" in a
10,000-tonne steel silo can destroy tens of thousands of dollars of
wheat, corn, or barley in days. In developing nations, where smallholder
farmers store grain in bags or mud bins, the losses are even more
devastating — often a family's entire annual surplus.

The root cause is that spoilage is **invisible until it is too late**.
Mold growth (Aspergillus, Penicillium, Fusarium) and insect infestation
(Sitophilus granarius — the granary weevil; Tribolium castaneum — the
red flour beetle; Rhyzopertha dominica — the lesser grain borer) begin in
localized micro-environments: a pocket of higher moisture, a slow leak in
a silo roof, a region of compaction. By the time the spoilage is visible
on the grain surface or detectable as a musty odor, the damage is
extensive and spreading.

Current monitoring is either:

| Approach | Limitation |
|----------|------------|
| **Manual probing with a grain thermometer** | Labor-intensive, infrequent (weekly–monthly), only samples the top 1 m, misses deep hot spots |
| **Permanent thermocouple cables** suspended in silo | Only temperature; no CO₂, no moisture, no acoustic insect detection; expensive to install; no wireless |
| **CO₂ spot-check with handheld meter** | Labor-intensive; single point; no continuous monitoring |
| **Commercial wireless temperature cables** (e.g., BinManager) | Temperature + moisture only; no CO₂; no acoustic insect detection; proprietary closed systems; $100s–$1000s per silo |
| **Proactive fumigation** | Toxic (phosphine), applied blindly without knowing if insects are actually present; resistance is increasing |

No existing solution combines **multi-zone temperature, CO₂, moisture,
and acoustic insect detection** in a single wireless probe with on-device
spoilage-risk fusion and a mesh network that scales from a single farm
bin to an elevator with dozens of silos. GrainGuard closes that gap.

### Core Innovation

GrainGuard's novelty rests on four pillars:

1. **Acoustic insect detection in the grain bulk.** Insects feeding
   inside grain kernels produce characteristic ultrasonic emission
   bursts (20–80 kHz, 0.5–5 ms duration, 10–200 events/s for a
   moderate infestation). GrainGuard uses a piezoelectric acoustic
   emission (AE) sensor coupled to the probe wall and a dedicated
   analog front end (40 kHz band-pass, 60 dB gain, envelope detector)
   sampled at 192 kS/s by the MCU's ADC, with on-device event detection
   and classification. This detects infestation **weeks before** insects
   are visible or CO₂ rises — the earliest possible warning.

2. **Nine-zone distributed temperature profiling.** Instead of a single
   point measurement, GrainGuard has nine DS18B20 1-Wire digital
  temperature sensors spaced every 25 cm along the 2.4 m probe,
   providing a full thermal profile of the grain column. Temperature
   gradients reveal convection cells, localized heating from mold
   respiration, and the position of the grain surface.

3. **CO₂ as the universal spoilage sentinel.** All biological spoilage
   — mold, bacteria, insects — produces CO₂ through respiration.
   GrainGuard uses a Sensirion SCD41 NDIR CO₂ sensor (400–5000 ppm,
   ±40 ppm) sampled every 15 minutes. A rising CO₂ trend is the most
   reliable single indicator that *something* is metabolizing the
   grain, even before temperature rises measurably. The SCD41 is
   factory-calibrated, maintenance-free, and consumes only 3 mA during
   measurement.

4. **Equilibrium moisture content (EMC) computation.** From the
   measured temperature and relative humidity (SHT45 sensor,
   ±1.8 %RH), GrainGuard computes the grain's equilibrium moisture
   content using the modified Henderson equation (parameterized for
   wheat, corn, barley, rice, oats, and soybeans). This translates
   ambient RH% — which managers find unintuitive — into "% moisture
   content of the grain," the number that actually determines storage
   stability. Grain above 13.5 % MC (wheat) or 15.5 % MC (corn) is at
   risk; GrainGuard flags it automatically.

### How It Works

The probe is inserted vertically into the grain (or built into the silo
wall at construction). Once powered, it enters a low-power duty cycle:

- Every 15 minutes: read all 9 temperature zones, RH, compute EMC.
- Every 15 minutes: read CO₂ (NDIR measurement cycle).
- Every 6 hours: acoustic insect scan (5-minute listening window at
  192 kS/s, envelope detection, event count + classification).
- Every 30 minutes: transmit fused Spoilage Risk Index + raw data via
  LoRa to the mesh.

The on-device **Spoilage Risk Index (SRI)** is a 0–100 score fusing:

| Signal | Weight | Threshold logic |
|--------|--------|-----------------|
| CO₂ trend (15-min Δ and absolute) | 35 % | > 1000 ppm = risk; > 2000 ppm = high |
| Temperature gradient (max zone ΔT) | 25 % | > 2 °C gradient = risk |
| Temperature absolute vs setpoint | 15 % | > 30 °C = risk (tropical grains), > 25 °C (temperate) |
| EMC above safe threshold | 15 % | > 13.5 % MC wheat, > 15.5 % MC corn |
| Acoustic insect event rate | 10 % | > 10 events/min = infestation likely |

SRI > 40 triggers a **Caution** alert; SRI > 70 triggers **Critical**,
prompting aeration, turning, or fumigation.

---

## 2. Why This Is Novel

No existing open-hardware or commercial device combines:

- **Acoustic emission insect detection** (the earliest infestation
  signal, detectable before CO₂ rises) with
- **Multi-zone temperature profiling** (9-point thermal map, not a
  single point) with
- **NDIR CO₂ sensing** (the universal metabolic spoilage indicator)
  with
- **EMC computation from T+RH** (grain-specific, not generic RH%)
  with
- **On-device spoilage-risk fusion** (SRI, not just raw data dumps)
  with
- **Self-healing LoRa mesh** (scales from one bin to an elevator,
  no per-silo gateway needed).

Existing commercial systems (BinManager, OPI Grain Bin Manager, Agri
Dry) provide temperature cables and sometimes moisture, but none
include CO₂ or acoustic insect detection. Academic literature on
acoustic insect detection in grain exists (Hickin, 1984; Mankin, 2002)
but has never been productized into a deployable multi-sensor probe.
GrainGuard is the first integrated, open, field-deployable system to
unite all four sensing modalities with on-device fusion and mesh
networking.

---

## 3. Hardware Specifications

### 3.1 Mechanical & Form Factor

| Parameter | Value |
|----------|-------|
| **Probe diameter** | 38 mm (1.5 in — fits standard silo probe ports) |
| **Probe length** | 2.4 m (modular: 4× 0.6 m segments, screw-together) |
| **Material** | 6061-T6 anodized aluminum tube, food-grade, IP68 |
| **Weight** | 1.1 kg (full assembly) |
| **Operating temperature** | -30 °C to +65 °C |
| **Ingress protection** | IP68 (2 m / 30 days); dust-tight (silos are dusty) |
| **Mounting** | Suspended from silo roof cable, or free-standing in flat storage |

### 3.2 Microcontroller & Radio

| Component | Part | Function |
|-----------|------|----------|
| **MCU + Radio** | STM32WL55JC (Cortex-M4 @ 48 MHz + LoRa SX126x sub-GHz, 64 KB Flash, 32 KB RAM) | Main processor + LoRa transceiver (single-chip solution) |
| **TCXO** | 32 MHz (on-chip, ±0.5 ppm after cal) | LoRa frequency reference |
| **RTC** | Internal (backed by supercap) | Duty-cycle timing |

The STM32WL55 was chosen because it integrates an ARM Cortex-M4
application core and a LoRa SX1262-class radio in a single QFN package,
dramatically reducing BOM count and board area. The M4 core provides
enough DSP horsepower for acoustic event detection (192 kS/s ADC +
envelope computation + event classifier) while drawing only 12 mA
active and 1.2 µA in STOP2 (RTC + SRAM retention).

### 3.3 Sensors

| Sensor | Part | Parameter | Range | Accuracy |
|--------|------|-----------|-------|----------|
| **CO₂** | Sensirion SCD41 (NDIR, photoacoustic) | CO₂ concentration | 400–5000 ppm | ±40 ppm ±5 % |
| **Temperature (9 zones)** | DS18B20 (1-Wire, ×9, parasitic power) | Temperature, 9 points | -55 to +125 °C | ±0.5 °C |
| **RH/T (bulk air)** | Sensirion SHT45 | Relative humidity + temperature | 0–100 %RH, -40 to +125 °C | ±1.8 %RH, ±0.1 °C |
| **Acoustic emission** | Murata 7BB-20-6L0 piezo disc + custom AFE | Insect ultrasonic AE | 20–80 kHz | N/A (event counter) |

### 3.4 Acoustic Front End

The acoustic insect-detection channel uses a piezoelectric disc
(Murata 7BB-20-6L0, 20 mm, 6.3 kHz resonance — but sensitive to
in-band AE at 20–80 kHz via the broadband response) bonded to the
inside of the aluminum probe wall. The signal passes through:

1. **Charge amplifier** (OPA2376, 10 MΩ feedback, 10 pF) — converts
   piezo charge to voltage, high-Z input.
2. **Band-pass filter** (4th-order Sallen-Key, 20–80 kHz, OPA2380) —
   rejects audible noise and out-of-band mechanical vibration.
3. **Gain stage** (60 dB programmable, PGA controlled by MCU).
4. **Envelope detector** (ADL5511 RMS detector, 1 Hz–10 GHz) —
   produces a slow envelope for ADC sampling at 1 kS/s, plus a fast
   raw output (192 kS/s) for spectral analysis when needed.
5. **MCU ADC** (12-bit, 192 kS/s) — samples the envelope for event
   detection; raw signal available for FFT on demand.

### 3.5 Power

| Parameter | Value |
|-----------|-------|
| **Battery** | 3.6V LiSOCl₂ D-cell (Saft LSH-20, 13.0 Ah) |
| **Battery life** | 3 years (15-min T/RH/CO₂ cycle, 6-hr acoustic scan) |
| **Regulator** | TPS7A02 (3.3V, 1 µA Iq) + LP5912 (1.2V for MCU core) |
| **Backup** | 0.47F supercap (RTC + SRAM retention during battery swap) |
| **Low-power modes** | STOP2 (1.2 µA), Standby (0.5 µA) |

### 3.6 Connectivity

| Interface | Details |
|-----------|---------|
| **LoRa** | 868 MHz (EU) / 915 MHz (US), SX1262 on-chip, +22 dBm, SF7–SF12, BW 125–500 kHz |
| **Mesh protocol** | Self-healing flooding-mesh, up to 8 hops, AES-128 |
| **BLE** | n/a (battery life; BLE omitted; commissioning via NFC tag) |
| **NFC** | ST25DV-I2C (passive, for serial number + commissioning URL) |
| **Gateway** | One gateway per site; Ethernet or LTE-M uplink to cloud |

### 3.7 Storage

| Component | Part | Function |
|-----------|------|----------|
| **Flash** | W25R80 (8 MB SPI NOR) | Raw data log (30-day ring buffer at 15-min rate) |
| **EEPROM** | 24LC02 (2 Kbit I²C) | Calibration + serial number + commissioning |

### 3.8 Full Bill of Materials (Summary)

| Ref | Part | Qty | Function |
|-----|------|-----|----------|
| U1 | STM32WL55JCI6 | 1 | MCU + LoRa radio |
| U2 | SCD41 | 1 | NDIR CO₂ sensor |
| U3 | SHT45 | 1 | RH + temperature |
| U4–U12 | DS18B20 | 9 | Distributed temperature (1-Wire) |
| U13 | OPA2380 | 2 | AE band-pass filter |
| U14 | OPA2376 | 1 | Charge amplifier |
| U15 | ADL5511 | 1 | Envelope detector |
| U16 | TPS7A02 | 1 | 3.3V LDO |
| U17 | LP5912 | 1 | 1.2V LDO |
| U18 | W25R80 | 1 | 8 MB SPI flash |
| U19 | 24LC02 | 1 | EEPROM |
| U20 | ST25DV | 1 | NFC tag |
| BAT1 | LSH-20 | 1 | 3.6V LiSOCl₂ D-cell |
| Y1 | 32.768 kHz crystal | 1 | RTC |
| AE1 | 7BB-20-6L0 | 1 | Piezoelectric AE sensor |

---

## 4. Architecture and Block Diagram

```
  ┌─────────────────────────────────────────────────────────────────┐
  │                         GrainGuard Probe                         │
  │                                                                  │
  │  ┌──────────┐   I²C    ┌─────────────────────────────────────┐  │
  │  │  SCD41   │─────────►│                                     │  │
  │  │ (CO₂)    │          │                                     │  │
  │  └──────────┘          │       STM32WL55JC                   │  │
  │                        │   ┌───────────────┐                  │  │
  │  ┌──────────┐   I²C    │   │  Cortex-M4    │   ┌──────────┐   │  │
  │  │  SHT45   │─────────►│   │  @ 48 MHz     │   │  SX1262  │   │  │
  │  │ (RH+T)   │          │   │  64KB Flash   │──►│  LoRa    │───┼──► Antenna
  │  └──────────┘          │   │  32KB SRAM     │   │ 868/915  │   │  │
  │                        │   └───────────────┘   └──────────┘   │  │
  │  ┌──────────┐  1-Wire  │        │  │  │  │        │          │  │
  │  │ DS18B20  │─────────►│  ┌─────┘  │  │  └────────┘          │  │
  │  │   ×9     │          │  │SPI    │ADC│                       │  │
  │  │ (T zones)│          │  │       │   │                       │  │
  │  └──────────┘          │  ▼       ▼   │                       │  │
  │                        │ ┌────┐ ┌────┐│                       │  │
  │  ┌──────────┐ Charge   │ │W25R│ │ADC│◄─ Envelope              │  │
  │  │ 7BB-20   │─ Amp ────►│ │80  │ │   │  │                       │  │
  │  │ (AE piezo)│          │ │Flash││   │  │                       │  │
  │  └──────────┘          │ └────┘ └────┘  │                       │  │
  │         │              │   │            │                       │  │
  │         ▼              │   │  I²C       │                       │  │
  │  ┌──────────┐  BPF     │   ▼            │                       │  │
  │  │ OPA2380  │  20-80k  │ ┌──────┐ ┌──────┐                     │  │
  │  │ ×2       │──────────►│ │EEPROM│ │ NFC │                      │  │
  │  └──────────┘  Gain    │ │24LC02│ │ST25DV│                      │  │
  │         │              │ └──────┘ └──────┘                     │  │
  │         ▼              │                                          │  │
  │  ┌──────────┐  Envelope│   ┌──────────────────┐                 │  │
  │  │ ADL5511  │──────────►│   │  TPS7A02 + LP5912│◄── LSH-20 LiSOCl₂│  │
  │  │  RMS Det │          │   │  3.3V / 1.2V LDO │     3.6V D-cell │  │
  │  └──────────┘          │   └──────────────────┘                 │  │
  │                        └─────────────────────────────────────┘  │
  └─────────────────────────────────────────────────────────────────┘
                                       │
                                    LoRa Mesh
                                       │
                              ┌────────▼────────┐
                              │  Site Gateway   │
                              │ (Ethernet/LTE-M)│
                              └────────┬────────┘
                                       │
                              ┌────────▼────────┐
                              │   Companion App  │
                              │  (React Native)  │
                              └─────────────────┘
```

---

## 5. Firmware Design

The firmware is written in portable C targeting the STM32WL55JC.
It is interrupt-driven with a cooperative scheduler; no RTOS is used
(battery and flash budget). The architecture is layered:

```
  ┌─────────────────────────────────────────────────┐
  │              main.c (application loop)           │
  ├───────────────────────────────────────────────────┤
  │  Drivers:                                        │
  │  ├── co2.c       (SCD41 I²C driver)             │
  │  ├── temp.c      (DS18B20 1-Wire, 9 sensors)     │
  │  ├── humid.c     (SHT45 I²C driver)             │
  │  ├── acoustic.c  (AE ADC + event detector)       │
  │  ├── emc.c       (equilibrium moisture content)  │
  │  ├── sri.c       (Spoilage Risk Index fusion)    │
  │  ├── loramesh.c  (LoRa mesh MAC + AES-128)       │
  │  ├── storage.c   (W25R80 flash ring buffer)      │
  │  └── power.c     (low-power + RTC scheduler)     │
  ├───────────────────────────────────────────────────┤
  │  board.h / registers.h (HW definitions)          │
  └─────────────────────────────────────────────────┘
```

### Design Decisions

1. **No RTOS.** The duty cycle is dominated by sleep (99.97 % of the
   time). A bare-metal cooperative scheduler with a 15-minute RTC
   wake-up is simpler, smaller, and more predictable than FreeRTOS.

2. **Parasitic 1-Wire for DS18B20.** Eliminates a separate power wire
   in the probe cable; only DATA + GND needed. Conversion current
   (1.5 mA per sensor) is supplied parasitically; strong pull-up
   during conversion.

3. **Envelope detection instead of full FFT.** The ADL5511 produces
   a low-bandwidth envelope of the 20–80 kHz AE signal. Sampling
   this at 1 kS/s consumes 100× less power than sampling the raw
   signal at 192 kS/s. Full-band FFT is available on-demand for
   confirmation scans.

4. **CO₂ measurement every 15 minutes, not continuous.** The SCD41
   draws 3 mA during a 5-second measurement. At 15-min intervals this
   averages 17 µA — acceptable. Continuous mode would drain the
   battery in months.

5. **Mesh flooding with duplicate suppression.** Each probe relays
   packets from others, with a 3-second random back-off and a
   "recently seen" cache to prevent loops. This scales to 50+ probes
   per site with no manual routing configuration.

6. **AES-128 hardware encryption.** The STM32WL55 has a hardware AES
   accelerator; all mesh traffic is authenticated (CMAC) and encrypted
   (CTR). Prevents spoofed spoilage-silent attacks on commercial
   elevators.

### Acoustic Insect Detection Algorithm

The firmware implements a two-stage detection:

1. **Envelope event detection** (runs every 6 hours, 5-minute window):
   - Sample envelope ADC at 1 kS/s.
   - Compute short-term (10 ms) and long-term (500 ms) RMS.
   - If short-term RMS > long-term RMS × 3 for ≥ 2 ms, register an
     "event" with timestamp, peak amplitude, and duration.
   - Count events over the 5-minute window; compute events/min.

2. **On-demand spectral confirmation** (runs only if events/min > 10):
   - Switch ADC to raw-signal mode at 192 kS/s for 10 seconds.
   - Compute 512-point FFT, identify dominant frequency.
   - Match against known insect signatures:
     - Sitophilus granarius: 26–30 kHz bursts
     - Tribolium castaneum: 42–48 kHz
     - Rhyzopertha dominica: 55–65 kHz
   - Classify species if confidence > 70 %.

---

## 6. Equilibrium Moisture Content (EMC)

GrainGuard computes EMC from measured T and RH using the **modified
Henderson equation**:

```
        1
  MC = ─── × [ -ln(1 - RH) ]^(1/n)
        K × A
```

where RH is fractional (0–1), and K, A, n are grain-specific constants:

| Grain | K | A | n | Safe MC (%) |
|-------|---|---|---|-------------|
| Wheat | 0.00046 | 1.0 | 5.3 | 13.5 |
| Corn | 0.00086 | 1.0 | 4.2 | 15.5 |
| Barley | 0.00051 | 1.0 | 5.0 | 14.0 |
| Rice | 0.00048 | 1.0 | 5.5 | 13.0 |
| Oats | 0.00063 | 1.0 | 4.6 | 14.0 |
| Soybean | 0.00200 | 1.0 | 3.0 | 13.0 |

The grain type is set via the NFC commissioning tag or the companion
app. The firmware stores all six sets of constants and selects at
runtime.

---

## 7. Application / Software Interface

The companion app is a **React Native** application (iOS + Android)
that connects to a site gateway over Wi-Fi or cellular. It provides:

- **Dashboard**: Map of all silos with color-coded SRI (green/yellow/red).
- **Silo detail**: 9-zone temperature profile chart, CO₂ trend,
  moisture content, acoustic event rate, insect species (if detected).
- **Alerts**: Push notifications for Caution (SRI > 40) and Critical
  (SRI > 70), with recommended action (aerate, turn, fumigate, inspect).
- **Configuration**: Set grain type per silo, safe MC threshold,
  alert thresholds, measurement interval.
- **Commissioning**: Scan the NFC tag on a new probe to register it;
  assign to a silo via the app.
- **Data export**: CSV / JSON download of full historical logs for
  regulatory compliance (grain elevator audits).

### Protocol

The gateway exposes a REST + WebSocket API. The app subscribes to
WebSocket updates for real-time SRI changes. The binary mesh packet
format (18 bytes) is:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | Protocol version (0x01) |
| 1 | 1 | Probe serial (0–254) |
| 2 | 2 | UTC timestamp (minutes since epoch, mod 2^16) |
| 4 | 2 | CO₂ (ppm, ÷10) |
| 6 | 1 | SRI (0–100) |
| 7 | 1 | Max zone temp (°C, offset +128) |
| 8 | 1 | Min zone temp (°C, offset +128) |
| 9 | 1 | Max-min ΔT (°C) |
| 10 | 1 | RH% |
| 11 | 1 | EMC% (×10) |
| 12 | 1 | Acoustic events/min |
| 13 | 1 | Insect species ID (0=none) |
| 14 | 2 | Battery voltage (mV) |
| 16 | 2 | AES-128 CMAC (truncated) |

---

## 8. Use Cases and Target Audience

### 8.1 On-Farm Grain Storage (Smallholder to Mid-Scale)

A farmer with one or more 500–5000 bu bins inserts a GrainGuard probe
at fill time. The app shows green when grain is stable. If moisture
migrates after a roof leak or temperature inversion, the SRI rises
and the app alerts the farmer to run the aeration fan — saving the
bin before spoilage.

### 8.2 Commercial Grain Elevators

An elevator with 20+ silos deploys one probe per silo (or 2–3 per
large silo). The LoRa mesh self-organizes; one gateway covers the
entire facility. The elevator operator monitors all silos from a
single dashboard, prioritizing aeration and fumigation by actual
risk rather than on a fixed schedule — saving energy and phosphine.

### 8.3 Commodity Warehouses & Food Processors

Stored-grain processors (flour mills, breweries, feed mills) must
prove grain quality for food safety audits. GrainGuard's logged data
(temperature, CO₂, moisture, insect activity) provides a defensible
compliance record.

### 8.4 Developing-Nation Smallholder Storage

A low-cost variant (fewer temperature zones, no CO₂) can be deployed
in smallholder bag-storage systems, where losses are highest. The
acoustic insect detection alone — warning of infestation before the
grain is visibly damaged — can save a family's annual food supply.

### 8.5 Research & Extension

Agricultural researchers use GrainGuard's raw data to study spoilage
kinetics, insect population dynamics, and the efficacy of aeration
and fumigation strategies.

---

## 9. Performance & Specifications Summary

| Parameter | Value |
|-----------|-------|
| Temperature zones | 9 (25 cm spacing) |
| Temperature accuracy | ±0.5 °C |
| Temperature range | -55 to +125 °C |
| CO₂ range | 400–5000 ppm |
| CO₂ accuracy | ±40 ppm ±5 % |
| RH range | 0–100 % |
| RH accuracy | ±1.8 % |
| EMC computation | 6 grain types |
| Acoustic band | 20–80 kHz |
| Acoustic sensitivity | Events ≥ 2 ms, 3× background |
| Insect classification | 3 species (≥70 % confidence) |
| LoRa range | 2 km line-of-sight, 500 m in-silo |
| Mesh nodes | Up to 50+ per gateway |
| Battery life | 3 years (LiSOCl₂ D-cell) |
| Measurement interval | 15 min (T/RH/CO₂), 6 hr (acoustic) |
| SRI update | Every 15 min |
| Alert latency | < 30 min (mesh hop + gateway) |
| Data retention | 30 days on-probe flash, unlimited in cloud |
| Operating temp | -30 to +65 °C |
| Ingress protection | IP68 |

---

## 10. Repository Structure

```
grainguard/
├── README.md                   — This document
├── firmware/
│   ├── Makefile                — ARM GCC build
│   ├── board.h                 — Pin definitions, clock config
│   ├── registers.h             — STM32WL55 register map
│   ├── main.c                  — Application + scheduler
│   └── drivers/
│       ├── co2.c / co2.h       — SCD41 NDIR CO₂ driver
│       ├── temp.c / temp.h     — DS18B20 1-Wire (9 sensors)
│       ├── humid.c / humid.h   — SHT45 RH+T driver
│       ├── acoustic.c / acoustic.h — AE insect detector
│       ├── emc.c / emc.h       — Equilibrium moisture content
│       ├── sri.c / sri.h       — Spoilage Risk Index fusion
│       ├── loramesh.c / loramesh.h — LoRa mesh MAC
│       ├── storage.c / storage.h — W25R80 flash logging
│       └── power.c / power.h   — Low-power scheduling
├── kicad/
│   ├── grainguard.kicad_sch    — Schematic
│   ├── grainguard.kicad_pcb    — PCB layout
│   └── grainguard.kicad_pro    — Project file
└── app/
    ├── App.js                  — React Native root
    ├── package.json            — Dependencies
    └── src/
        ├── screens/            — Dashboard, SiloDetail, Alerts, Config
        ├── services/           — GrainGuardContext (BLE/cloud)
        ├── components/          — SRI gauge, temp profile chart
        └── utils/               — Protocol parser
```

---

## 11. License

- **Hardware** (KiCad schematics, PCB): CERN-OHL-S v2
- **Firmware** (C source): GPL-2.0
- **Companion App** (React Native): MIT

All authored by **jayis1**. Copyright © 2026 jayis1. All rights reserved.

---

*GrainGuard: listening to the grain before it spoils.*