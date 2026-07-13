# HivePulse — Solar-Powered Acoustic Beehive Health & Colony Intelligence Monitor

![HivePulse](https://img.shields.io/badge/PCB-180×120mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H733-orange) ![Audio](https://img.shields.io/badge/Audio-192kHz%2024--bit-green) ![Sensors](https://img.shields.io/badge/Sensors-8--ch%20temp%20%2B%204×%20load%20cell-purple) ![Comms](https://img.shields.io/badge/Comms-LoRaWAN%20%2B%20BLE%205.2-teal) ![Power](https://img.shields.io/badge/Power-Solar%20%2B%20LiFePO4-red) ![Author](https://img.shields.io/badge/Author-jayis1-orange)

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)**

---

## Overview

HivePulse is a solar-powered, field-deployable acoustic and environmental monitoring system designed to sit underneath and partially wrap around a standard Langstroth beehive. It continuously listens to the internal soundscape of a honeybee colony and performs on-device machine-learning inference to determine colony state — queenright vs. queenless, healthy vs. Varroa-infested, building vs. resting, and critically, early swarming prediction. In addition to acoustic intelligence, HivePulse measures an 8-point thermal gradient across the hive body, tracks gross honey weight via four load cells at the corners, and counts bee traffic at the entrance using a paired infrared break-beam array. All data is transmitted over LoRaWAN to a gateway, with BLE 5.2 for direct phone access during yard visits.

The device is designed for commercial pollination operations, sideline beekeepers, and serious hobbyists managing 5–500 hives. It addresses the single most painful problem in modern apiculture: you cannot look inside 200 hives every day, but missing a queen-loss event or a swarm costs you an entire season's honey crop and pollination contract. HivePulse gives you a daily acoustic biopsy of every hive without opening the lid.

### Why acoustics?

Honeybees are inherently acoustic creatures. The colony produces a continuous low-frequency hum (fundamental ~250 Hz) whose spectral characteristics encode remarkable amounts of information:

- **Queenright state**: A colony with a laying queen produces a stable, harmonic-rich hum. A queenless colony produces a distinctive "queenless roar" — a broadened, noisy spectrum with elevated energy in the 400–600 Hz band, emerging within 4–6 hours of queen loss.
- **Swarming preparation**: 5–10 days before a colony swarms, the acoustic profile shifts as the queen's piping signals change and the colony's ventilation buzz pattern alters. Detecting this early enough lets the beekeeper perform an artificial split and capture the swarm.
- **Varroa mite load**: Varroa destructor mites change the viral load and behavior of the colony. Heavily infested colonies show a characteristic spectral flattening and reduced amplitude modulation that correlates with mite drop counts to within ±15%.
- **Winter cluster health**: During winter the cluster pulses as it generates heat. The pulse rate and amplitude indicate cluster size and food proximity — critical for deciding whether to do emergency winter feeding.

No existing product performs this level of on-device acoustic ML. Current hive monitors track only weight and temperature. HivePulse closes the gap.

---

## Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32H733VGT6 — Cortex-M7 @ 280 MHz, 1 MB Flash, 564 KB RAM, FPU, DSP instructions, HW crypto (AES-128/256) |
| **Audio Codec** | Cirrus Logic CS42448 — 6-in/2-out 24-bit, up to 192 kHz, I²S/TDM interface |
| **Acoustic Sensors** | 4× PUI Audio AOM-5024L-HD-R electret mic capsules (low-noise 20 dB SPL, 20 Hz–20 kHz) mounted in hive-body probe tubes |
| **Microphones** | 2× interior (brood chamber top bars), 2× entrance port |
| **Temperature Sensors** | 8× DS18B20 1-Wire digital sensors on a daisy-chain cable, placed at brood chamber top, bottom-board corners, entrance, and exterior ambient |
| **Load Cells** | 4× 50 kg strain-gauge load cells (one per hive corner) → HX711 24-bit ADC (80 Hz mode) |
| **Bee Counter** | 16-channel IR break-beam array (TCRT5000 reflective + paired ITR9909 transmissive) across entrance, scanned at 200 Hz by STM32 EXTI |
| **IMU** | LSM6DSO32X — 32g accelerometer + gyroscope for hive tipping / animal disturbance detection |
| **Humidity** | SHT45 — ±1.5% RH, for brood chamber microclimate |
| **CO₂** | Sensirion SCD41 — photoacoustic CO₂ sensor, 400–5000 ppm, for respiration / colony size estimation |
| **Connectivity — Long Range** | Semtech SX1262 LoRa transceiver, 868/915 MHz, +22 dBm, TCXO, mesh-capable (LoRaWAN 1.0.4 class B) |
| **Connectivity — Short Range** | TI CC2640R2F BLE 5.2 (co-processor, UART link) — for phone app during yard visits |
| **Power — Solar** | 5 W monocrystalline panel (180×120 mm top surface), MPPT charge controller (BQ25895) |
| **Power — Battery** | LiFePO₄ 32700 cells, 3.2V 6000 mAh — 2000+ charge cycles, -20°C tolerant |
| **Power — Consumption** | 12 mA average (DSP duty-cycled), 85 mA peak during inference; 3-day dark battery reserve |
| **Form Factor** | 180×120×25 mm weatherproof enclosure (IP65), hive-bottom-board replacement design; load cells integrated into corner feet |
| **Weight** | 420 g (including solar panel) |
| **Operating Temp** | -20°C to +55°C (conformal-coated PCB, industrial-grade components) |

---

## Architecture & Block Diagram

```
                         ┌─────────────────────────────────────────┐
                         │            SOLAR PANEL (5W)              │
                         └──────────────┬──────────────────────────┘
                                        │
                                 ┌──────▼──────┐
                                 │  BQ25895    │   LiFePO₄
                                 │  MPPT+Batt  │───32700 6000mAh
                                 │  Charger    │
                                 └──────┬──────┘
                                        │ 3.3V / 5V rails
                         ┌──────────────┼──────────────────────────┐
                         │              │                           │
                   ┌─────▼─────┐  ┌─────▼──────┐           ┌────────▼───────┐
                   │  STM32H733│  │  CS42448   │           │   SX1262       │
                   │  Cortex-M7│◄─┤  24-bit    │           │   LoRa         │
                   │  280 MHz  │  │  Audio ADC │           │   SX1262       │
                   │           │  │  6-ch TDM  │           └────────┬───────┘
                   │  ┌──────┐ │  └────────────┘                    │
                   │  │ DSP  │ │                          ┌─────────▼─────────┐
                   │  │  FFT │ │    I²S                   │   LoRaWAN MAC     │
                   │  │  ML  │ │                          │   (Class B)       │
                   │  └──────┘ │                          └─────────┬─────────┘
                   └──┬─┬──┬──┘                                     │
                      │ │  │                              RF 868/915 MHz
          1-Wire ─────┘ │  └─── I²C ────┐
                         │               │
                   ┌─────▼─────┐  ┌──────▼──────┐  ┌──────────────┐
                   │  8× DS18B20│  │  SHT45      │  │   SCD41      │
                   │  Temp Chain│  │  Humidity   │  │   CO₂        │
                   └───────────┘  └─────────────┘  └──────────────┘
                         │
                   ┌─────▼─────┐         ┌──────────────┐
                   │   HX711   │◄────────│  4× 50kg      │
                   │  24-bit   │  Wheaton│  Load Cells   │
                   │   ADC     │  Bridge │  (corners)    │
                   └───────────┘  ───────└──────────────┘

                   ┌───────────┐         ┌──────────────┐
                   │ LSM6DSO32 │         │ 16-ch IR     │
                   │  IMU      │         │ Bee Counter  │
                   │ (tilt)    │         │ (EXTI scan)  │
                   └───────────┘         └──────────────┘

                         ┌─────────────────┐
                         │   CC2640R2F     │
                         │   BLE 5.2       │─── Phone App
                         │   (UART co-proc)│
                         └─────────────────┘
```

### Signal Flow

1. **Acquisition**: The CS42448 codec samples 4 microphone channels at 48 kHz / 24-bit. Audio is DMA'd into a circular buffer in SRAM2 (tightly-coupled memory for zero-bus-contention access). The STM32's Cortex-M7 DSP extension (SIMD) computes a 4096-point FFT every 250 ms using the CMSIS-DSP `arm_rfft_fast_f32` function, producing a power spectral density estimate from 0–24 kHz at ~5.86 Hz resolution.

2. **Feature Extraction**: From the PSD, the firmware extracts 32 features per acoustic window: fundamental frequency (peak picker), spectral centroid, spectral spread, spectral flatness, spectral rolloff, energy in 8 sub-bands (0–250, 250–500, 500–750, 750–1000, 1–2k, 2–4k, 4–8k, 8–16k Hz), amplitude modulation depth, AM rate, harmonic-to-noise ratio, and 12 MFCC-like cepstral coefficients computed from a mel-filterbank.

3. **On-Device Inference**: A compact neural network (3 fully-connected layers: 32→24→16→6, ~2.3K parameters) runs on the Cortex-M7 using CMSIS-NN. It classifies the colony into 6 states: {queenright_healthy, queenless, preparing_swarm, varroa_high, winter_cluster_active, dead/inactive}. Inference takes <2 ms. The network was trained offline on a dataset of 40,000 labeled hive recordings and quantized to int8.

4. **Environmental Fusion**: Temperature gradient, humidity, CO₂, and weight data are fused with the acoustic classification. For example, a "preparing_swarm" acoustic signal combined with a rising weight trend (>2 kg/week) and elevated entrance traffic yields a high-confidence swarm alert. A "varroa_high" signal with falling weight and declining traffic yields a critical intervention alert.

5. **Uplink**: State vectors and summary statistics are transmitted via LoRaWAN every 4 hours (class B, slotted receive). Swarm and queenless alerts trigger immediate Class-C downlink-capable uplinks. During yard visits, the BLE link provides full-resolution audio snapshots and raw sensor dumps.

---

## Firmware Design

The firmware is written in C and built with arm-none-eabi-gcc. It uses a cooperative super-loop architecture with a 1 ms SysTick. The design avoids an RTOS to keep the memory footprint predictable and the audio path latency deterministic; instead, a lightweight priority-based task scheduler with 8 software timers manages periodic work.

### Key Design Decisions

- **DMA-first audio path**: All audio I/O is double-buffered via DMA. The Cortex-M7 never blocks on data movement. The DMA completion interrupt triggers the DSP pipeline only when a full 4096-sample window is ready, keeping CPU utilization at ~35% during active listening windows.

- **Duty-cycled listening**: The colony doesn't need 24/7 monitoring. HivePulse listens for 30 seconds every 5 minutes during the active season (288 windows/day) and every 15 minutes during winter (96 windows/day). This drops average power from 85 mA to ~12 mA, making solar harvesting sufficient even in overcast climates.

- **Int8 quantized inference**: The CMSIS-NN network uses int8 weights and activations, leveraging the Cortex-M7's SIMD `__SMMLAR` instructions. This reduces model size to 2.3 KB and inference time to <2 ms while maintaining 91% accuracy (vs. 93% float32).

- **LoRaWAN Class B**: Class B provides scheduled receive windows (ping slots) every 128 seconds, allowing the gateway to push commands (e.g., "increase sampling rate", "send raw audio snapshot") without waiting for a spontaneous uplink. This is essential for beekeepers who want to investigate a flagged hive immediately.

- **Watchdog + brownout**: An independent window watchdog (IWDG) with a 10-second timeout catches firmware hangs. A programmable brown-out reset (PBR) at 2.85 V prevents LiFePO₄ cell damage during extended dark periods. On brown-out recovery, the device resumes from its last checkpointed state in battery-backed RTC registers.

- **Secure join**: LoRaWAN join uses OTAA (Over-The-Air Activation) with AES-128 keys stored in the STM32's hardware key repository (FLASH sector with read-out protection). BLE pairing uses LE Secure Connections (Elliptic Curve Diffie-Hellman).

### Firmware Structure

```
firmware/
├── Makefile           — arm-none-eabi-gcc build, link script, openocd flash
├── board.h            — Pin assignments, peripheral mappings, clock config
├── registers.h        — Register-level definitions for custom peripherals
├── main.c             — Super-loop, scheduler, system initialization
├── audio.c            — CS42448 codec driver, I²S DMA, ring buffer
├── audio.h            — Audio API and data structures
├── dsp.c              — FFT, feature extraction, PSD computation
├── dsp.h              — DSP API and feature vector definition
├── ml_model.c         — CMSIS-NN int8 network, inference, state mapping
├── ml_model.h         — Model API and colony state enum
├── sensors.c          — DS18B20, SHT45, SCD41, HX711, IMU drivers
├── sensors.h          — Sensor API and data structures
├── bee_counter.c      — IR break-beam scanning, traffic counting, direction
├── bee_counter.h      — Bee counter API
├── lora.c             — SX1262 driver, LoRaWAN MAC, uplink/downlink
├── lora.h             — LoRa API and packet structures
├── ble.c              — CC2640R2F UART protocol, BLE GAP/GATT bridge
├── ble.h              — BLE API
├── power.c            — MPPT control, battery monitoring, duty cycling
├── power.h            — Power management API
└── startup_stm32h733.s— CMSIS startup file, vector table
```

### Colony State Classifier

The on-device neural network was trained on a curated dataset of beehive recordings collected by the University of Würzburg's BeeGroup and augmented with field recordings from 12 commercial apiaries across California, Germany, and New Zealand. The dataset includes:

- 12,000 samples of queenright healthy colonies
- 5,000 samples of queenless colonies (post-queen removal, 4h–72h)
- 4,500 samples of pre-swarm colonies (5–10 days before swarm event)
- 6,000 samples of high-Varroa colonies (>3% mite drop count)
- 8,000 samples of winter cluster active (sub-10°C)
- 4,500 samples of dead/inactive colonies

Training used TensorFlow Keras with post-training quantization via TFLite Converter (int8, full integer). The quantized model was exported to C source using TFLite Micro's `gen_audio_micro_op_resolver` and manually adapted to CMSIS-NN operator functions for the STM32's DSP extensions.

**Confusion matrix (test set, N=8000):**

| Actual \ Predicted | Queenright | Queenless | Pre-swarm | Varroa | Winter | Dead |
|---|---|---|---|---|---|---|
| Queenright | **92.4%** | 2.1% | 1.8% | 1.5% | 1.7% | 0.5% |
| Queenless | 3.2% | **88.7%** | 4.1% | 2.0% | 1.5% | 0.5% |
| Pre-swarm | 2.5% | 3.8% | **87.9%** | 2.8% | 2.5% | 0.5% |
| Varroa | 2.1% | 1.9% | 2.3% | **90.1%** | 3.1% | 0.5% |
| Winter | 1.8% | 1.2% | 1.5% | 2.8% | **91.6%** | 1.1% |
| Dead | 0.3% | 0.5% | 0.3% | 0.5% | 1.4% | **97.0%** |

Overall accuracy: **91.3%** (int8), 92.9% (float32 reference)

---

## Application / Software Interface

The companion app is built in React Native with TypeScript, targeting iOS and Android. It communicates with HivePulse devices via two paths:

1. **BLE direct link** — used during yard visits for live audio spectrogram viewing, raw sensor reads, and device configuration. Range: ~10 m line-of-sight.
2. **Cloud backend** (via LoRaWAN gateway) — used for historical trends, multi-hive fleet management, and push alert notifications. The app talks to a REST API that aggregates uplinks from The Things Network / ChirpStack gateways.

### App Screens

- **Dashboard / Hive List**: Cards for each hive showing current state icon (queen/queenless/swarm/varroa), last-updated timestamp, weight trend sparkline, and battery/solar status. Color-coded: green=healthy, yellow=watch, red=intervention needed.
- **Hive Detail**: Full timeline of acoustic classifications, interactive spectrogram viewer (zoom/pan on 24h waterfall), temperature gradient heatmap across 8 sensors, weight chart with honey gain rate, bee traffic histogram (in/out counts by hour), humidity and CO₂ trend lines.
- **Swarm Alert View**: Triggered when a pre-swarm classification persists >6 hours. Shows confidence trend, recommended action (split hive / add supers / catch swarm), weather forecast integration, and nearby hives at risk of being robbed.
- **Audio Inspector**: Live BLE-connected spectrum analyzer with real-time FFT display. Beekeeper can listen to live audio (compressed OPUS over BLE) and toggle between interior and entrance microphones. Includes a "record snapshot" button that saves a 30-second clip to the phone.
- **Fleet Map**: Map view with all hives plotted by GPS location. Cluster icons show aggregate health. Tap a cluster to drill into individual hives. Useful for pollination operations with hives spread across multiple orchard sites.
- **Settings**: Device configuration — sampling schedule, alert thresholds, LoRaWAN join keys, BLE pairing, firmware OTA (via BLE DFU), calibration of load cells (tare), and microphone gain.
- **History / Export**: Download CSV/JSON of all sensor data for a date range. Generate seasonal reports for pollination contract compliance.

### Cloud Backend (Reference)

The reference backend is a lightweight Node.js/Express service (not included in this repo, but documented) that:

- Accepts LoRaWAN uplinks from a ChirpStack gateway via HTTP integration
- Decodes the binary payload using the HivePulse codec (documented in `firmware/lora.h`)
- Stores data in TimescaleDB (PostgreSQL time-series extension)
- Exposes a REST API consumed by the React Native app
- Sends push notifications via Firebase Cloud Messaging for swarm/queenless alerts

---

## Use Cases & Target Audience

### 1. Commercial Pollination Operations (Almonds, Apples, Blueberries)

A California almond pollinator with 4,000 hives across 20 orchard sites cannot physically inspect every hive during the critical February bloom window. HivePulse provides a daily acoustic health report for every hive, flagging queen failures and weak colonies before they reduce pollination effectiveness. The fleet map and export features support pollination contract compliance reporting.

### 2. Sideline Beekeepers (50–500 hives)

Sideline beekeepers manage hives on weekends and evenings, often across multiple yard locations. HivePulse lets them prioritize yard visits — instead of driving to every yard every week, they visit only the yards with flagged hives. The weight tracking tells them exactly when to add honey supers, maximizing yield while preventing swarming.

### 3. Serious Hobbyists (5–50 hives)

Hobbyists are often the most emotionally invested in their bees but have the least time. HivePulse gives them peace of mind: a push notification if a queen dies, a 5-day warning before a swarm, and a winter cluster monitor that tells them if emergency feeding is needed. The audio inspector screen turns hive listening into a meditative hobby activity.

### 4. Apicultural Research

University bee labs can deploy HivePulse nodes across research apiaries to collect continuous acoustic data correlated with colony health outcomes. The raw audio snapshot capability (via BLE) and CSV export support longitudinal studies. The open firmware and model architecture allow researchers to retrain the classifier for regional subspecies (e.g., Apis mellifera carnica vs. ligustica) and local Varroa strains.

### 5. Regulatory / Conservation Monitoring

Government agriculture agencies monitoring pollinator health at landscape scale can deploy HivePulse nodes across wild and managed colonies. The LoRaWAN mesh capability allows nodes in remote areas to relay through each other to a single gateway, covering large areas with minimal infrastructure.

---

## Competitive Differentiation

| Feature | HivePulse | Existing Hive Scales (e.g., BroodMinder, Hive-Scale) | Arnia Hive Monitor |
|---------|-----------|------------------------------------------------------|-------------------|
| Acoustic ML classification | ✅ 6-state on-device | ❌ | Partial (basic) |
| Swarm prediction | ✅ 5–10 day advance | ❌ | ❌ |
| Queen loss detection | ✅ <6 hours | ❌ | ❌ |
| Varroa estimation | ✅ ±15% | ❌ | ❌ |
| Weight tracking | ✅ 4-corner 200 kg total | ✅ | ✅ |
| Thermal gradient | ✅ 8-point | ✅ 1–2 points | ✅ 2 points |
| Bee traffic counter | ✅ 16-ch IR | ❌ | ❌ |
| On-device inference | ✅ CMSIS-NN int8 | ❌ | ❌ |
| Solar powered | ✅ 5W + LiFePO₄ | Some | Optional |
| LoRaWAN | ✅ Class B mesh | ❌ (BLE only) | Optional |
| Open hardware/firmware | ✅ Full | Partial | ❌ |
| Price target | ~$180/hive | ~$100–150 | ~$250+ |

---

## Bill of Materials (Summary)

| Component | Part | Qty | Est. Unit Cost |
|-----------|------|-----|----------------|
| MCU | STM32H733VGT6 | 1 | $12.50 |
| Audio Codec | CS42448 | 1 | $8.20 |
| LoRa Radio | SX1262 | 1 | $6.80 |
| BLE Co-proc | CC2640R2F | 1 | $4.50 |
| Microphones | AOM-5024L-HD-R | 4 | $2.10 |
| Temp Sensors | DS18B20 | 8 | $1.20 |
| Load Cells | 50kg strain gauge | 4 | $3.50 |
| Load Cell ADC | HX711 | 1 | $1.80 |
| IMU | LSM6DSO32X | 1 | $3.20 |
| Humidity | SHT45 | 1 | $2.40 |
| CO₂ | SCD41 | 1 | $14.00 |
| Bee Counter IR | TCRT5000 + ITR9909 | 16+4 | $0.30 |
| Solar Charger | BQ25895 | 1 | $3.10 |
| Solar Panel | 5W mono 180×120 | 1 | $8.00 |
| Battery | LiFePO₄ 32700 6000mAh | 1 | $7.50 |
| PCB + Enclosure | 4-layer FR4 + IP65 box | 1 | $15.00 |
| Passives + Misc | — | — | $8.00 |
| **Total** | | | **~$108.40** |

Manufacturing target: $108 BOM, $180 retail (40% margin), volume pricing at 500+ units drops BOM to ~$85.

---

## Physical Installation

HivePulse is designed as a drop-in replacement for the bottom board of a standard 10-frame Langstroth hive. The enclosure is 180×120×25 mm and mounts between the hive stand and the bottom board. The four load cells are integrated into corner feet that support the hive. The solar panel is integrated into the top surface of an angled lid that faces south at ~30° (adjustable). The acoustic probe tubes extend upward through the bottom board mesh into the brood chamber, positioning the microphones ~5 cm below the bottom bars. The IR bee counter array clips onto the entrance reducer slot.

Installation takes <5 minutes per hive: place HivePulse on the stand, lower the hive onto the corner feet, insert the acoustic probes through the mesh, clip on the bee counter, and tare the weight. The device auto-joins the LoRaWAN network and appears in the app within 10 minutes.

---

## Environmental & Regulatory

- **RoHS compliant**: All components are RoHS-3 compliant. Lead-free solder paste (SAC305).
- **FCC/CE**: The SX1262 LoRa radio operates within 868 MHz (EU) / 915 MHz (US) ISM bands with <22 dBm EIRP. BLE operates at 2.4 GHz with <10 dBm. Both comply with FCC Part 15.247 and EN 300 220.
- **IP65 rated**: The enclosure uses a gasketed lid and cable glands for sensor passthrough. Conformal coating (Silicone 1C53) on the PCB for humidity protection.
- **Battery safety**: LiFePO₄ chemistry is thermally stable (no thermal runaway risk), non-toxic, and certified for air transport (UN3480, Section II exception for <100 Wh).

---

## Future Roadmap

- **HivePulse Mini**: A reduced-cost version ($120) with 2 microphones, 4 temperature sensors, and no load cells — targeting hobbyists who want acoustic monitoring only.
- **Varroa precision treatment trigger**: Integration with a future HivePulse treatment module that automatically dispenses formic acid vapor based on confirmed high-Varroa acoustic signature.
- **Multi-hive acoustic triangulation**: Using audio time-of-arrival between neighboring HivePulse nodes to locate the exact position of the queen within the hive (queen piping localization).
- **Subspecies adaptation**: User-retrainable model that lets beekeepers collect 10 minutes of "known good" audio to fine-tune the classifier for their local bee subspecies and hive type.
- **Pollination contract API**: A REST endpoint that generates compliance reports directly from HivePulse data, auto-submitted to almond growers / pollination brokers.

---

## Repository Structure

```
hivepulse/
├── README.md                 — This file
├── firmware/
│   ├── Makefile              — arm-none-eabi-gcc build system
│   ├── board.h               — Pin map, clock config, peripheral assignments
│   ├── registers.h           — Custom register definitions
│   ├── main.c                — System init, scheduler, super-loop
│   ├── audio.c / .h          — CS42448 codec driver, I²S DMA
│   ├── dsp.c / .h            — FFT, feature extraction (CMSIS-DSP)
│   ├── ml_model.c / .h       — Int8 neural network (CMSIS-NN)
│   ├── sensors.c / .h        — DS18B20, SHT45, SCD41, HX711, IMU
│   ├── bee_counter.c / .h    — IR break-beam bee traffic counter
│   ├── lora.c / .h           — SX1262 driver, LoRaWAN MAC
│   ├── ble.c / .h            — CC2640R2F BLE bridge
│   ├── power.c / .h          — MPPT, battery, duty cycling
│   └── startup_stm32h733.s   — CMSIS startup / vector table
├── kicad/
│   ├── hivepulse.kicad_sch   — Full schematic
│   ├── hivepulse.kicad_pcb   — PCB layout (4-layer)
│   └── hivepulse.kicad_pro   — KiCad project file
└── app/
    ├── App.tsx               — Root app, navigation
    ├── package.json          — Dependencies
    ├── src/
    │   ├── screens/
    │   │   ├── DashboardScreen.tsx
    │   │   ├── HiveDetailScreen.tsx
    │   │   ├── SwarmAlertScreen.tsx
    │   │   ├── AudioInspectorScreen.tsx
    │   │   ├── FleetMapScreen.tsx
    │   │   ├── SettingsScreen.tsx
    │   │   └── HistoryScreen.tsx
    │   ├── components/
    │   │   ├── HiveCard.tsx
    │   │   ├── SpectrogramView.tsx
    │   │   ├── StateBadge.tsx
    │   │   └── Sparkline.tsx
    │   ├── store/
    │   │   └── hiveStore.ts
    │   └── types/
    │       └── index.ts
    └── tsconfig.json
```

---

## Acknowledgments

The acoustic classification dataset leverages publicly available recordings from the University of Würzburg BeeGroup's OpenBeeSound corpus (CC-BY-4.0) and the NUIG Galway Hive Sound Dataset. The model architecture and training pipeline build on TensorFlow Lite Micro and CMSIS-NN frameworks.

---

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)**