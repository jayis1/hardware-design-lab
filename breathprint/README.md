# BreathPrint — Portable Breath VOC Analyzer & Metabolic Health Monitor

![BreathPrint](https://img.shields.io/badge/PCB-72x42mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H733-orange) ![Sensors](https://img.shields.io/badge/Sensors-8--channel%20MOX-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.2%20%2B%20USB--C-purple) ![License](https://img.shields.io/badge/License-MIT-yellow)

**Author: jayis1**

> A pocket-sized breath analysis device that captures your metabolic fingerprint from a single exhale. Eight metal-oxide gas sensors detect volatile organic compounds (VOCs) correlated with ketosis, gut microbiome activity, fermentation byproducts, lipid metabolism, and inflammatory markers — then streams a personalized breath profile to your phone.

---

## Table of Contents

1. [Overview](#overview)
2. [Why Breath Analysis?](#why-breath-analysis)
3. [Hardware Specifications](#hardware-specifications)
4. [Architecture & Block Diagram](#architecture--block-diagram)
5. [Sensor Array & Biomarker Mapping](#sensor-array--biomarker-mapping)
6. [Firmware Details](#firmware-details)
7. [Application / Software Interface](#application--software-interface)
8. [Use Cases & Target Audience](#use-cases--target-audience)
9. [Calibration & Data Model](#calibration--data-model)
10. [Power Budget](#power-budget)
11. [Safety & Regulatory Notes](#safety--regulatory-notes)
12. [Bill of Materials](#bill-of-materials)
13. [License](#license)

---

## Overview

BreathPrint is the first open-hardware, pocket-sized breath volatile organic compound (VOC) analyzer designed for consumer and research metabolic health monitoring. By analyzing the chemical composition of a single exhaled breath sample across eight orthogonal gas sensors, BreathPrint computes a **breath fingerprint** — a multi-dimensional profile that correlates with metabolic state, dietary patterns, gut microbiome activity, and certain physiological conditions.

Unlike lab-grade breath analyzers (mass spectrometers, SIFT-MS, GC-MS) that cost $50,000+ and require trained operators, BreathPrint is:

- **Pocketable** — 72 × 42 × 14 mm, 32 g
- **Affordable** — sub-$80 BOM
- **Instant** — results in under 15 seconds per breath sample
- **Personal** — BLE companion app tracks your breath fingerprint over time, correlates with diet and exercise logs, and surfaces actionable insights
- **Open** — full schematics, firmware, and app source under MIT license

### What BreathPrint Measures

Each of the 8 metal-oxide (MOX) gas sensors in the array responds to a broad but overlapping set of VOCs. Through multi-sensor fusion and machine learning (running on-device and in the companion app), BreathPrint estimates:

| Breath Marker | Sensor Channel | What It Indicates |
|---|---|---|
| **Acetone** | CH-A (BME688 #1, tuned) | Ketosis level / fat-burning rate |
| **Hydrogen (H₂)** | Electrochemical H₂ | Carbohydrate malabsorption, SIBO, FODMAP fermentation |
| **Methane (CH₄)** | NDIR CH₄ | Constipation-predominant gut type, methanogen archaea |
| **Ethanol** | CH-B (MOX alcohol-selective) | Alcohol metabolism, fermentative gut dysbiosis |
| **Isoprene** | CH-C (BME688 #2, tuned) | Cholesterol biosynthesis activity, oxidative stress |
| **Ammonia (NH₃)** | MOX NH₃-tuned | Kidney function, H. pylori indicator |
| **Hydrogen sulfide (H₂S)** | MOX H₂S-tuned | Protein fermentation, inflammatory bowel markers |
| **Broad VOC index** | BME688 IAQ composite | Overall air quality, breath freshness, environmental contamination |

### How a Breath Sample Works

1. User taps **"Analyze Breath"** in the app or presses the device button.
2. Device activates the sensor heater profiles (warm-up: 5 s).
3. User exhales through the disposable mouthpiece into the sampling chamber.
4. Sensors settle for 10 s while the MCU samples all 8 channels at 20 Hz with 16-bit resolution.
5. On-device feature extraction computes peak amplitude, rise time, steady-state, and decay for each channel.
6. A lightweight on-device model (random forest, quantized) classifies the breath profile and estimates ketone, H₂, and methane concentrations.
7. Results stream over BLE to the app, which logs the sample, overlays it on historical trends, and provides personalized insights.

---

## Why Breath Analysis?

Human exhaled breath contains over 3,500 volatile organic compounds. Many are produced by metabolic processes and can serve as non-invasive biomarkers:

- **Acetone** in breath is directly correlated with blood ketone levels (β-hydroxybutyrate). Breath acetone is the single most validated non-invasive ketosis biomarker, with correlations r > 0.9 against blood ketone meters in fasting and ketogenic diet studies.
- **Hydrogen and methane** are produced exclusively by gut microbiota fermentation of carbohydrates. Breath H₂/CH₄ testing is the gold standard for diagnosing small intestinal bacterial overgrowth (SIBO), irritable bowel syndrome (IBS) subtyping, and carbohydrate intolerance (lactose, fructose, sorbitol).
- **Isoprene** is the most abundant hydrocarbon in human breath and reflects cholesterol biosynthesis flux. It's being studied as an oxidative stress and cardiovascular risk marker.
- **Ammonia** levels correlate with kidney function (urea cycle efficiency) and can indicate H. pylori infection.

Despite this, breath analysis remains confined to clinical labs. BreathPrint democratizes it: a device you carry in your pocket that gives you a metabolic snapshot every morning, tracks how your diet changes your gut fermentation, and helps you understand your body without needles.

---

## Hardware Specifications

### MCU & Processing

| Parameter | Value |
|---|---|
| Microcontroller | STM32H733VGT6 — ARM Cortex-M7, 312 MHz |
| Flash | 1 MB (dual-bank, with bank swap for OTA) |
| RAM | 240 KB DTCM + 512 KB SRAM + 64 KB cache TCM |
| FPU | Single + double precision |
| DSP | CMSIS-DSP, hardware PID filter |
| Crypto | AES-256, SHA-256 hardware accelerator |

The STM32H733 was chosen for its high clock speed (needed for real-time multi-sensor fusion and feature extraction), dual-bank flash for over-the-air firmware updates, and integrated AES/SHA for secure BLE pairing and data-at-rest encryption on the SD card.

### Sensor Array

| # | Sensor | Target Gas | Technology | Interface |
|---|---|---|---|---|
| 1 | Bosch BME688 #1 | Acetone (tuned heater profile) | MOX, 4 heater modes | I²C (0x77) |
| 2 | Bosch BME688 #2 | Isoprene (tuned heater profile) | MOX, 4 heater modes | I²C (0x78 via addr jumper) |
| 3 | Sensirion SCD41 | CO₂ (reference) | Photoacoustic NDIR | I²C (0x62) |
| 4 | SenseAir S8 (miniaturized NDIR) | Methane (CH₄) | NDIR 3.3 µm | UART 9600 baud |
| 5 | Spec Sensors DGS-Ethanol | Ethanol | Electrochemical | I²C (0x48) |
| 6 | Spec Sensors DGS-NH3 | Ammonia (NH₃) | Electrochemical | I²C (0x49) |
| 7 | Spec Sensors DGS-H2S | Hydrogen Sulfide (H₂S) | Electrochemical | I²C (0x4A) |
| 8 | Membrapor H2-500 | Hydrogen (H₂) | Electrochemical | Analog → ADC |

All sensors are housed in a PTFE-coated stainless-steel sampling chamber (8 mL volume) with a single-use disposable mouthpiece (anti-bacterial filter, one-way valve).

### Connectivity

| Interface | Standard | Purpose |
|---|---|---|
| BLE 5.2 | ST BlueNRG-MS (SPBTLE-RC) | Phone app data transfer, OTA firmware |
| USB-C | USB 2.0 Full-Speed | Charging, data export, firmware flashing, serial console |
| SD Card | microSD UHS-I (SPI mode) | On-device logging (FAT32), firmware update images |

### Power

| Parameter | Value |
|---|---|
| Battery | 500 mAh Li-Po (3.7 V nominal) |
| Charger | MCP73831, 500 mA charge rate via USB-C |
| Regulator | TPS63031 buck-boost (3.3 V, 500 mA) |
| Battery life | ~120 breath samples per charge |
| Standby | 14 days (sensor heaters off) |
| Charge time | ~1.5 hours via USB-C (5 V / 500 mA) |

### Form Factor

| Dimension | Value |
|---|---|
| PCB | 72 × 42 mm, 4-layer, 0.8 mm thickness |
| Device (with case) | 72 × 42 × 14 mm |
| Weight | 32 g (with battery) |
| Mouthpiece | Standard 15 mm ID breath mouthpiece, disposable |
| Enclosure | Injection-molded PC/ABS, medical-grade silicone seal |

### Environmental

| Parameter | Value |
|---|---|
| Operating temperature | 10 °C to 40 °C (breath temp range) |
| Storage temperature | -20 °C to 60 °C |
| Humidity | 10%–95% RH (non-condensing) |
| Altitude | Compensated via BMP390 pressure sensor |

---

## Architecture & Block Diagram

```
 ┌─────────────────────────────────────────────────────────────────┐
 │                       BREATHPRINT BLOCK DIAGRAM                 │
 │                                                                 │
 │   ┌──────────┐     ┌──────────────────────┐     ┌──────────┐   │
 │   │ Mouthpiece│───▶│ Sampling Chamber     │───▶│ Exhaust  │   │
 │   │ (disposable│    │ 8 mL PTFE-coated     │    │ Pump +   │   │
 │   │  + filter) │    │                      │    │ Valve    │   │
 │   └──────────┘     │ ┌──┐ ┌──┐ ┌──┐ ┌──┐│     └──────────┘   │
 │                     │ │B1│ │B2│ │SC│ │S8││                     │
 │                     │ │88│ │88│ │41│ │  ││                     │
 │                     │ └──┘ └──┘ └──┘ └──┘│                     │
 │                     │ ┌──┐ ┌──┐ ┌──┐ ┌──┐│                     │
 │                     │ │Et│ │NH│ │HS│ │H2││                     │
 │                     │ │OH│ │3 │ │  │ │  ││                     │
 │                     │ └──┘ └──┘ └──┘ └──┘│                     │
 │                     │ ┌────┐              │                     │
 │                     │ │BMP │ (pressure)   │                     │
 │                     │ │390 │              │                     │
 │                     │ └────┘              │                     │
 │                     └──────────┬───────────┘                    │
 │                                │                                │
 │              ┌─────────────────┼─────────────────┐             │
 │              │     I²C Bus     │   ADC    UART    │             │
 │              │  (400 kHz)      │         (9600)   │             │
 │              ▼                 ▼         ▼        │             │
 │   ┌──────────────────────────────────────────────┐│             │
 │   │           STM32H733VGT6 (Cortex-M7)          ││             │
 │   │                                              ││             │
 │   │  ┌────────┐  ┌─────────┐  ┌──────────────┐  ││             │
 │   │  │Sensor  │  │Feature  │  │ ML Inference │  ││             │
 │   │  │Sampling│─▶│Extract  │─▶│ (Quantized   │  ││             │
 │   │  │ (20 Hz)│  │(peak/   │  │  RF, 8→6    │  ││             │
 │   │  │        │  │ slope/  │  │  classes)   │  ││             │
 │   │  │        │  │ decay)  │  │             │  ││             │
 │   │  └────────┘  └─────────┘  └──────┬──────┘  ││             │
 │   │                                     │       ││             │
 │   │  ┌────────┐  ┌─────────┐  ┌────────▼──────┐││             │
 │   │  │ SD Log │  │ USB-C   │  │ BLE 5.2      │││             │
 │   │  │ (FAT32)│  │ FS Dev  │  │ (SPBTLE-RC)  │││             │
 │   │  └────────┘  └─────────┘  └──────────────┘││             │
 │   └──────────────────────────────────────────────┘│             │
 └─────────────────────────────────────────────────────────────────┘

 Power: 500 mAh Li-Po → MCP73831 charger → TPS63031 3.3V buck-boost
 Buttons: 1× capacitive touch (analyze), 1× tactile (power/reset)
 Display: 0.96" OLED (SSD1306, 128×64) via SPI
 LEDs: RGB status LED (NeoPixel) + white sample-in-progress LED
```

### Bus Topology

| Bus | Devices | Speed |
|---|---|---|
| I²C1 | BME688 #1, BME688 #2, SCD41, DGS-Ethanol, DGS-NH3, DGS-H2S, BMP390 | 400 kHz |
| UART3 | SenseAir S8 (CH₄ NDIR) | 9600 baud |
| ADC1 | Membrapor H2-500 (analog) | 16-bit, 20 Hz |
| SPI2 | SSD1306 OLED display | 10 MHz |
| SPI1 | microSD card | 25 MHz |
| SPI3 | W25Q64 external flash (ML model storage) | 50 MHz |
| USART2 | BLE module (SPBTLE-RC) HCI | 115200 baud |
| USB | USB-C Full-Speed | 12 Mbps |

### Power Domains

| Domain | Voltage | Devices |
|---|---|---|
| VBAT | 3.7 V (Li-Po) | Charger input, fuel gauge |
| VCC | 3.3 V (buck-boost) | MCU, all sensors, display, BLE |
| VSENS | 3.3V (switched via load switch) | Sensor heaters (can be disconnected in sleep) |
| VUSB | 5.0 V (USB-C) | Charger only |

---

## Sensor Array & Biomarker Mapping

### Sensor Fusion Strategy

No single MOX sensor is perfectly selective. The BreathPrint approach uses:

1. **Orthogonal sensor technologies**: MOX, electrochemical, NDIR, and photoacoustic sensors have fundamentally different cross-sensitivity profiles, providing true orthogonal measurements.
2. **Heater profiling**: The BME688 sensors cycle through 4 heater temperature profiles per measurement, creating a 4-dimensional response per sensor that separates acetone from isoprene and other interferents.
3. **Environmental compensation**: The BMP390 pressure sensor and SCD41 CO₂ reading are used as reference channels. CO₂ serves as a breath-quality indicator (dead space vs. alveolar air — a valid sample requires CO₂ > 3.5%).
4. **Humidity correction**: The BME688 integrated humidity sensor and SCD41 humidity channel provide real-time humidity compensation for MOX readings.

### Biomarker Calibration

Each BreathPrint unit undergoes a two-point factory calibration:

- **Zero point**: Clean air baseline (20.9% O₂, 400 ppm CO₂, ~50% RH, 21 °C)
- **Span point**: Known concentration gas mixture (1000 ppm acetone, 100 ppm H₂, 50 ppm CH₄, etc.)

The calibration coefficients are stored in the W25Q64 external flash and are unique per device. The companion app also supports user-performed ambient air recalibration to track sensor drift over time.

### On-Device Machine Learning

The ML pipeline runs entirely on the STM32H733:

1. **Feature extraction** (C, ~2 ms per sample): For each of 8 sensor channels, compute:
   - Baseline-corrected peak amplitude
   - Rise time (10% → 90% of peak)
   - Steady-state plateau value
   - Decay tau (exponential fit)
   - Area under curve (trapezoidal)
   = 40 features total

2. **Inference** (CMSIS-DSP, ~15 ms): A quantized random forest (int8, 64 trees, max depth 8) classifies the breath profile into:
   - Metabolic state: {Fasted/ketotic, Post-meal, Post-exercise, Baseline, Unknown}
   - Estimated acetone concentration (ppm)
   - Estimated H₂ concentration (ppm)
   - Estimated CH₄ concentration (ppm)
   - Breath quality flag: {Valid, Dead-space, Contaminated, Retry}

   The model weights are stored in external QSPI flash and loaded into SRAM at boot.

---

## Firmware Details

### Architecture

The firmware is bare-metal C (no RTOS) using a super-loop architecture with timer-driven sampling. This was chosen over an RTOS for:

- **Deterministic sensor timing**: MOX heater profiles require precise timing (±1 ms) that's easier to guarantee without context switching.
- **Lower power**: No RTOS tick, no idle task overhead.
- **Small footprint**: The entire firmware fits in < 128 KB flash, leaving 896 KB for the ML model and calibration tables.

### File Structure

```
firmware/
├── Makefile              — ARM GCC toolchain build, size optimization
├── board.h               — Pin assignments, peripheral mappings, clock config
├── registers.h           — STM32H733 register definitions, bit masks
├── linker.ld             — Memory layout, dual-bank OTA support
├── main.c                — Super-loop, state machine, sample orchestration
└── drivers/
    ├── sensor_array.c/h  — Multi-sensor sampling, heater profiling, calibration
    ├── i2c_drv.c/h       — I²C driver with DMA, multi-device bus management
    ├── adc_drv.c/h       — 16-bit ADC for analog H₂ sensor, oversampling
    ├── ble.c/h           — BLE 5.2 GATT profile, notification streaming
    ├── sdlog.c/h         — FAT32 logging, CSV breath records
    ├── display.c/h       — SSD1306 OLED driver, status screens
    ├── ml_infer.c/h      — Quantized random forest inference engine
    ├── breath_state.c/h  — Sample state machine, breath quality validation
    ├── power.c/h         — Battery monitoring, sleep modes, charger control
    └── flash_drv.c/h     — W25Q64 SPI flash for model storage
```

### Sampling State Machine

The firmware implements a 6-state machine for each breath sample:

```
 IDLE → WARMUP → SAMPLE → EXHALE_WAIT → ANALYZE → RESULT → IDLE
```

1. **IDLE**: Sensor heaters off, BLE connected, display shows last result. Power: ~8 mA.
2. **WARMUP** (5 s): Sensor heaters ramp to target profiles. Display shows "Breathe into mouthpiece." Power: ~45 mA.
3. **SAMPLE** (user exhales): Pressure/flow sensor detects exhalation start. All 8 channels sampled at 20 Hz. One-way valve prevents backflow. Power: ~55 mA.
4. **EXHALE_WAIT** (10 s): Sample chamber equilibrates. Continued sampling captures the full response curve. Exhaust pump activates to flush after measurement.
5. **ANALYZE** (~50 ms): Feature extraction + ML inference. Display shows "Analyzing..."
6. **RESULT**: Results displayed on OLED and streamed via BLE. Sample logged to SD card. Returns to IDLE after 15 s or button press.

### Calibration & Drift Compensation

- Baseline tracking: Ambient air is sampled automatically every 30 minutes when idle to track sensor drift.
- Temperature compensation: BMP390 temperature reading used to correct MOX sensor baseline (Arrhenius model).
- Humidity compensation: BME688 humidity reading used to correct MOX resistance (linear model, calibrated per-device).
- Cross-sensitivity matrix: Stored in flash, applied during feature extraction to deconvolve overlapping sensor responses.

### Design Decisions

- **No RTOS**: Bare metal ensures deterministic 20 Hz sampling without jitter from context switches. Timer interrupts handle sampling, main loop handles state transitions.
- **Dual-bank flash**: Bank A runs firmware, Bank B receives OTA updates. The bootloader swaps banks on next boot with rollback if the new firmware fails to initialize within 10 s.
- **External flash for ML model**: The random forest model (64 trees × 8 depth × int8) is ~120 KB — too large for comfortable inlining in firmware flash. Storing it in the W25Q64 allows model updates without firmware reflashing.
- **FAT32 on SD**: Using FAT32 (not a custom format) ensures the SD card can be removed and breath logs opened directly on any computer — important for users who want to export data to their healthcare provider.
- **Disposable mouthpiece**: Hygiene is critical for a breath device. The mouthpiece includes a bacterial/viral filter and one-way valve. Standardized 15 mm ID fits common breath testing consumables.

---

## Application / Software Interface

### Companion App (React Native)

The BreathPrint app runs on iOS and Android via React Native with BLE communication.

**Screens:**

| Screen | Function |
|---|---|
| **HomeScreen** | Current metabolic state, last breath fingerprint, quick-analyze button |
| **AnalyzeScreen** | Live sensor readout during sampling, animated breath guide |
| **TrendScreen** | Historical trends: acetone (ketosis), H₂, CH₄, VOC index over time |
| **DietScreen** | Food log with correlation analysis against breath markers |
| **InsightsScreen** | AI-generated insights: "Your H₂ spiked after dairy — possible lactose sensitivity" |
| **CalibrationScreen** | Device calibration status, ambient recalibration, sensor health |
| **SettingsScreen** | Device pairing, data export, units, privacy controls |

**BLE Protocol:**

The app communicates with BreathPrint over a custom GATT service (UUID `0x1B7F0000-...`):

| Characteristic | UUID suffix | Direction | Purpose |
|---|---|---|---|
| Command | 0x0001 | Phone → Device | Start sample, calibrate, set config |
| Sample Data | 0x0002 | Device → Phone | Real-time sensor stream (notifications) |
| Result | 0x0003 | Device → Phone | Final analysis result (notification) |
| Status | 0x0004 | Device → Phone | Battery, state, errors |
| Log Transfer | 0x0005 | Device → Phone | SD card log download |
| OTA Control | 0x0006 | Phone → Device | Firmware update trigger |

**Data Format:**

Each breath result is a 64-byte binary packet:

```
struct breath_result_t {
    uint32_t timestamp;       // Unix time
    uint8_t  metabolic_state; // 0=baseline, 1=ketotic, 2=post-meal, ...
    uint8_t  breath_quality;  // 0=valid, 1=dead-space, 2=contaminated
    uint16_t acetone_ppm;     // Estimated acetone (ppm × 100)
    uint16_t h2_ppm;          // Estimated hydrogen (ppm × 10)
    uint16_t ch4_ppm;         // Estimated methane (ppm × 10)
    uint16_t ethanol_ppm;     // Estimated ethanol (ppm × 100)
    uint16_t isoprene_ppb;    // Estimated isoprene (ppb)
    uint16_t nh3_ppm;         // Estimated ammonia (ppm × 10)
    uint16_t h2s_ppb;         // Estimated H₂S (ppb)
    uint16_t voc_index;       // BME688 IAQ index
    uint16_t co2_ppm;         // CO₂ reference (ppm)
    int16_t  temperature;     // °C × 10
    uint16_t humidity;        // % RH × 10
    uint16_t pressure;        // hPa
    uint8_t  battery_pct;     // 0-100
    uint8_t  sensor_health;   // Bitmask of sensor status
    uint8_t  reserved[8];     // Future expansion
} __attribute__((packed));
```

### Cloud Sync (Optional)

The app supports optional cloud sync (end-to-end encrypted) for:
- Cross-device data continuity
- Long-term trend analysis
- Anonymized research data contribution (opt-in)
- Healthcare provider data sharing (PDF report export)

---

## Use Cases & Target Audience

### Primary Use Cases

1. **Ketogenic Diet Monitoring**
   Track breath acetone as a proxy for blood ketones. No more finger pricks. Know exactly when you're in ketosis and how deep. See how different foods, exercise, and fasting affect your ketone production in real-time.

2. **Gut Health & SIBO Screening**
   Breath hydrogen and methane are the clinical gold standard for SIBO and IBS diagnosis. BreathPrint brings this capability home — track your gut fermentation patterns, identify trigger foods, and monitor treatment progress.

3. **Food Intolerance Discovery**
   Log what you eat, then breathe 2-4 hours later. The app correlates H₂ and CH₄ spikes with specific foods to identify lactose, fructose, or sorbitol intolerance — all without expensive clinical tests.

4. **Athletic Metabolic Tracking**
   Endurance athletes can track their fat-adaptation (acetone) and metabolic flexibility. See how training blocks shift your body from carb-burning to fat-burning. Optimize race-day fueling strategy based on your actual metabolic state.

5. **Dietitian & Nutritionist Tool**
   Professionals can use BreathPrint with clients to provide objective metabolic feedback. "Your breath shows you're not actually in ketosis despite following the diet" — data-driven coaching.

6. **Research & Clinical Trials**
   Low-cost, portable breath analysis for epidemiological studies, dietary intervention trials, and gut microbiome research. The open data format and SD card logging make it ideal for field studies.

### Target Audience

| Audience | How They Use It |
|---|---|
| **Biohackers & quantified-self enthusiasts** | Daily metabolic tracking, diet optimization |
| **Ketogenic & low-carb dieters** | Ketosis verification without blood testing |
| **IBS/gut health patients** | Home breath testing, trigger food identification |
| **Athletes & coaches** | Metabolic flexibility monitoring, fueling strategy |
| **Dietitians & nutritionists** | Client metabolic assessment, objective feedback |
| **Researchers** | Field breath analysis, longitudinal VOC studies |
| **Integrative medicine practitioners** | Non-invasive metabolic assessment tool |

---

## Calibration & Data Model

### Factory Calibration

Each device is calibrated at manufacture with certified gas standards:

- Zero air (hydrocarbon-free, 20.9% O₂, balance N₂)
- Span gases: 1000 ppm acetone, 200 ppm H₂, 100 ppm CH₄, 50 ppm ethanol, 5 ppm NH₃, 1 ppm H₂S, 200 ppb isoprene

Calibration coefficients (zero offset, span gain, cross-sensitivity matrix) are stored in W25Q64 flash with a CRC-32 checksum.

### Field Recalibration

Users can perform ambient air recalibration through the app:

1. Device is placed in clean outdoor air for 60 seconds.
2. Sensors sample ambient baseline.
3. New zero offsets are computed and stored (with timestamp for drift tracking).
4. App warns if drift exceeds 15% from factory calibration — indicates sensor replacement needed.

### Sensor Lifetime

| Sensor | Expected Lifetime | Replacement |
|---|---|---|
| BME688 (MOX) | 10+ years | SMD, replaceable |
| SCD41 (NDIR CO₂) | 10+ years | SMD |
| SenseAir S8 (NDIR CH₄) | 10+ years | Through-hole module |
| Spec DGS (electrochemical) | 2-3 years (consumable) | Module, user-replaceable |
| Membrapor H2-500 | 2-3 years (consumable) | Through-hole, user-replaceable |
| BMP390 | 10+ years | SMD |

The electrochemical sensors are consumable (the electrolyte depletes over time). The app tracks sensor age and alerts when replacement is due. Replacement sensors are available as a kit.

---

## Power Budget

| State | Current | Duration | Energy |
|---|---|---|---|
| Sleep (BLE connected, heaters off) | 2.5 mA | Continuous | 9.25 mWh/h |
| Idle (OLED on, heaters off) | 8 mA | Continuous | 29.6 mWh/h |
| Warmup (heaters ramping) | 45 mA | 5 s | 0.23 mWh |
| Sample (all sensors active) | 55 mA | 15 s | 0.68 mWh |
| Analyze (MCU full speed) | 65 mA | 0.1 s | 0.006 mWh |
| BLE TX (result streaming) | 12 mA | 2 s | 0.022 mWh |

**Per-sample energy**: ~0.94 mWh → **~530 samples per charge** (theoretical), **~120 samples** with idle display and BLE between samples (realistic).

**Charging**: USB-C 5 V / 500 mA → 500 mAh battery charges in ~1.5 hours.

---

## Safety & Regulatory Notes

### Safety

- **Non-invasive**: BreathPrint analyzes exhaled breath only — no skin contact, no needles, no ingestion.
- **Disposable mouthpiece**: Anti-bacterial/viral filter prevents cross-contamination between users.
- **Low voltage**: Internal circuits are 3.3 V max. No high-voltage components.
- **Battery protection**: MCP73831 charger has overvoltage, overcurrent, and thermal protection. Battery has integrated PTC fuse and NTC thermistor.

### Regulatory

BreathPrint is designed as a **general wellness / lifestyle device**, not a medical device. It is not intended to diagnose, treat, cure, or prevent any disease. Users with medical concerns should consult a healthcare professional.

The open-source nature allows researchers and clinicians to validate and extend the platform for specific medical applications under appropriate regulatory pathways (e.g., FDA 510(k) for specific breath tests like H. pylori or SIBO).

### Data Privacy

- All breath data is stored locally on the device (SD card) and phone.
- Cloud sync is opt-in and end-to-end encrypted (AES-256-GCM).
- No personal health data is collected by the manufacturer.
- Research data contribution is fully anonymized and opt-in.

---

## Bill of Materials

| # | Component | Part Number | Qty | Unit Cost | Total |
|---|---|---|---|---|---|
| 1 | MCU | STM32H733VGT6 | 1 | $8.50 | $8.50 |
| 2 | MOX Sensor | Bosch BME688 | 2 | $5.50 | $11.00 |
| 3 | CO₂ Sensor | Sensirion SCD41 | 1 | $9.20 | $9.20 |
| 4 | CH₄ NDIR | SenseAir S8 (mini) | 1 | $12.00 | $12.00 |
| 5 | Ethanol Sensor | Spec DGS-Ethanol | 1 | $4.50 | $4.50 |
| 6 | NH₃ Sensor | Spec DGS-NH3 | 1 | $4.50 | $4.50 |
| 7 | H₂S Sensor | Spec DGS-H2S | 1 | $4.50 | $4.50 |
| 8 | H₂ Sensor | Membrapor H2-500 | 1 | $8.00 | $8.00 |
| 9 | Pressure Sensor | Bosch BMP390 | 1 | $2.80 | $2.80 |
| 10 | BLE Module | ST SPBTLE-RC | 1 | $3.20 | $3.20 |
| 11 | OLED Display | 0.96" SSD1306 (128×64) | 1 | $2.50 | $2.50 |
| 12 | Flash | W25Q64JVSIQ (8 MB SPI) | 1 | $0.60 | $0.60 |
| 13 | microSD Socket | Hirose DM3AT | 1 | $1.20 | $1.20 |
| 14 | Charger IC | MCP73831T-2ACI/OT | 1 | $0.70 | $0.70 |
| 15 | Buck-Boost | TPS63031DSKR | 1 | $2.10 | $2.10 |
| 16 | Battery | 500 mAh Li-Po (402030) | 1 | $3.50 | $3.50 |
| 17 | USB-C Connector | GCT USB4085 | 1 | $0.80 | $0.80 |
| 16 | PCB (4-layer) | 72 × 42 mm | 1 | $2.00 | $2.00 |
| 17 | Passives | R, C, L, diodes | ~40 | — | $2.00 |
| 18 | Enclosure | PC/ABS injection molded | 1 | $3.00 | $3.00 |
| 19 | Mouthpiece assembly | Silicone + filter + valve | 1 | $1.50 | $1.50 |
| **Total** | | | | | **$77.90** |

---

## License

BreathPrint is released under the MIT License.

**Author: jayis1**

Copyright (c) 2026 jayis1

Permission is hereby granted, free of charge, to any person obtaining a copy of this hardware design, software, and associated documentation files (the "Design"), to deal in the Design without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Design, and to permit persons to whom the Design is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Design.

THE DESIGN IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE DESIGN OR THE USE OR OTHER DEALINGS IN THE DESIGN.

---

*BreathPrint — Your metabolic fingerprint, in every breath.*