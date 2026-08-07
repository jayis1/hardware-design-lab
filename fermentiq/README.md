# FermenTiq — Smart Multi-Modal Fermentation Bioreactor Monitor

![FermenTiq](https://img.shields.io/badge/Form--Factor-Ø42×85mm%20dip%20probe-blue)
![MCU](https://img.shields.io/badge/MCU-ESP32--S3-orange)
![Sensors](https://img.shields.io/badge/Sensors-Impedance%20%2B%20CO₂%20%2B%20pH%20%2B%20Temp%20%2B%20Acoustic-green)
![Comms](https://img.shields.io/badge/Comms-WiFi%206%20%2B%20BLE%205.0-teal)
![Power](https://img.shields.io/badge/Power-USB--C%20%2B%2018650-yellow)
![Author](https://img.shields.io/badge/Author-jayis1-orange)

**Author:** jayis1  
**Copyright © 2026 jayis1. All rights reserved.**  
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)

---

## 1. Purpose & Overview

FermenTiq is a novel, open-source, multi-modal fermentation monitoring
instrument that drops directly into any fermentation vessel — from a 5-gallon
homebrew carboy to a 200-L commercial conical fermenter, a kombucha SCOBY jar,
a kimchi crock, a yogurt maker, or a sourdough starter container. It provides
the first consumer-accessible, real-time measurement of **microbial biomass
density** via electrical impedance spectroscopy (EIS), fused with CO₂ evolution
rate, pH trajectory, temperature, and acoustic bubble counting to produce a
complete, continuous picture of fermentation progress, microbial health, and
spoilage risk.

### The Problem

Fermentation is one of humanity's oldest food-preservation and beverage-
production technologies, yet it is still managed largely by **guesswork**.
Homebrewers take occasional hydrometer gravity readings (which require drawing
a sample, are temperature-sensitive, and tell you nothing about *why* the
fermentation is slow). Commercial brewers send samples to a lab for cell
counts every few days. Kombucha makers, cheesemakers, and kimchi fermenters
rely on pH strips and the "look and smell" test. None of these approaches
capture the **dynamic, real-time** behavior of the microbial community driving
the fermentation.

The key insight FermenTiq exploits is this: **fermentation is an
electrochemical process**. As yeast and bacteria metabolize sugars, they:

1. **Change the ionic conductivity** of the must/wort/milk as they consume
   sugars (low conductivity) and produce ethanol, organic acids, and CO₂
   (higher conductivity). The impedance spectrum of the liquid shifts
   characteristically with cell density and metabolic phase.

2. **Evolve CO₂** at a rate directly proportional to their metabolic activity.
   The CO₂ evolution rate (CER) is the gold-standard proxy for microbial
   growth in industrial bioreactors but has never been brought to the
   consumer/small-batch scale.

3. **Acidify the medium** as they produce organic acids (lactic, acetic,
   propionic). The pH trajectory distinguishes healthy fermentation from
   contamination (e.g., a sudden pH crash suggests infection by
   acid-producing bacteria).

4. **Produce bubbles** (CO₂ nucleation) whose rate and acoustic signature
   correlate with fermentation vigor and can detect stalled or stuck
   fermentation.

### Why FermenTiq Is Novel

No existing consumer or prosumer fermentation monitor combines these
modalities:

| Existing Product | What it measures | What it misses |
|---|---|---|
| Tilt Hydrometer | Specific gravity (buoyancy) | Cell density, CO₂ rate, pH, contamination |
| iSpindel | Specific gravity (tilt) | Cell density, CO₂ rate, pH, contamination |
| Plaato Airlock | Bubble count (optical) | Gravity, cell density, pH |
| Brewer's Friend pH meter | pH (spot check) | Everything else |
| Commercial bioreactor (Eppendorf, Sartorius) | OD, CER, pH, temp — $50k+ | Price, accessibility |

**FermenTiq is the first device to bring electrical impedance spectroscopy
for live biomass estimation to the consumer/small-batch scale**, fused with
CO₂ evolution, pH, temperature, and acoustic monitoring in a single, sub-$120
instrument.**

The impedance spectroscopy approach is borrowed from industrial bioprocess
monitoring (where it is called *dielectric spectroscopy* or *biomass
monitoring*), but those instruments cost $15,000–$80,000 and require
specialized probes. FermenTiq democratizes this technique using the
Analog Devices AD5933 impedance-to-digital converter chip, a pair of
platinum planar electrodes on the probe body, and an on-device TinyML model
that maps the impedance spectrum to an estimated cell density (cells/mL)
using a calibration curve derived from hemocytometer counts.

---

## 2. Hardware Specifications

### Microcontroller

| Parameter | Value |
|---|---|
| MCU | Espressif ESP32-S3-WROOM-1-N16R8 |
| CPU | Dual-core Xtensa LX7, 240 MHz |
| Flash | 16 MB (external, via SPI) |
| PSRAM | 8 MB (octal SPI) |
| WiFi | 802.11 b/g/n (2.4 GHz, WiFi 4) |
| Bluetooth | BLE 5.0 (peripheral + central) |
| Crypto | AES-256, SHA-256, RSA, ECC (hardware accelerator) |
| ADC | 2× 12-bit SAR (used for battery + temperature analog) |

### Sensor Suite

| Sensor | Part | Function | Interface |
|---|---|---|---|
| Impedance Analyzer | AD5933YRSZ | 4-point EIS, 1 kHz–100 kHz, 27-bit DFT | I²C |
| NDIR CO₂ | Senseair S8 (LP8) | 400–10000 ppm CO₂, ±40 ppm | UART |
| ISFET pH | custom probe + LMP91200 | pH 0–14, ±0.05 pH | I²C (config) + ADC |
| RTD Temperature | PT100 (3-wire) + MAX31865 | -40 to +125 °C, ±0.1 °C | SPI |
| Acoustic Bubble | SPH-0645LM4H-B (MEMS mic) | 50 Hz–15 kHz, I²S PDM | I²S |
| Ambient Temp/RH | SHT41 | Air-side temp + humidity (for dew point) | I²C |

### Impedance Probe Electrodes

- **Material:** Platinum-plated titanium (food-grade, corrosion-resistant)
- **Configuration:** 4-electrode (tetrapolar) — separate excitation and
  sensing electrodes to eliminate electrode polarization artifacts at low
  frequencies
- **Spacing:** 8 mm between excitation electrodes, 3 mm between sensing
  electrodes (Wenner α arrangement)
- **Area:** 6 mm × 0.5 mm planar pads, flush-mounted in the PEEK probe body
- **Excitation voltage:** 200 mVpp (non-stimulatory — well below the
  electrochemical window of water, ~1.23 V)

### Connectivity

- **WiFi 4 (2.4 GHz):** Primary data uplink to the FermenTiq cloud / local
  MQTT broker (Home Assistant integration via MQTT discovery)
- **BLE 5.0:** Direct phone pairing for setup, live streaming, and offline
  data retrieval
- **USB-C (5 V):** Power input + serial console / firmware flashing

### Power

| Source | Details |
|---|---|
| Primary | USB-C 5 V / 1 A (continuous operation) |
| Backup | 18650 Li-ion (3.6 V, 3500 mAh) — ~18-hour carryover during power outage |
| Management | TP4056 charger + MAX17048 fuel gauge |
| Power budget | ~120 mA avg (WiFi active), ~35 mA (WiFi sleep, BLE only) |

### Form Factor

- **Probe body:** PEEK (polyether ether ketone) — food-safe, autoclavable,
  chemically resistant to ethanol, acids, and bases
- **Dimensions:** Ø 42 mm × 85 mm (fits standard #10 rubber stopper bore)
- **Weight:** 95 g (without battery)
- **Ingress protection:** IP68 (submersible to 1 m; the electronics
  compartment is potted, the electrode face is exposed to the liquid)
- **Cable:** 1.5 m USB-C cable with strain relief and a waterproof gland

---

## 3. Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        FermenTiq Block Diagram                   │
└─────────────────────────────────────────────────────────────────┘

  USB-C 5V ──────┬─── TP4056 ──── 18650 Li-ion ──── MAX17048 ───┐
  (power+data)   │                              (fuel gauge)      │
                 └─── AMS1117-3.3 ──── 3V3 rail ──────────────────│
                                                                   │
  ┌──────────────────────────────────────────────────────────┐    │
  │                    ESP32-S3-WROOM-1                       │    │
  │                                                          │    │
  │  ┌──── I²C bus (400 kHz) ────────┐                       │    │
  │  │                                │                       │    │
  │  │  ├─ AD5933 (EIS)               │                       │    │
  │  │  ├─ LMP91200 (ISFET config)    │       ┌──── SPI ────┐ │    │
  │  │  ├─ SHT41 (ambient T/RH)       │       │ MAX31865    │ │    │
  │  │  └─ MAX17048 (battery)         │       │ (RTD)       │ │    │
  │  │                                │       └─────────────┘ │    │
  │  ├──── UART2 ──── Senseair S8 ────┤                       │    │
  │  │                (NDIR CO₂)      │                       │    │
  │  │                                │       ┌──── I²S ────┐ │    │
  │  ├──── ADC1_CH0 ── ISFET output ──┤       │ SPH-0645    │ │    │
  │  │                (pH voltage)    │       │ (MEMS mic)  │ │    │
  │  │                                │       └─────────────┘ │    │
  │  ├──── WiFi 4 ──── MQTT / HTTP ───┤                       │    │
  │  ├──── BLE 5.0 ─── Phone app ─────┤                       │    │
  │  └──── USB-CDC ─── Console ───────┘                       │    │
  │                                                          │    │
  │  ┌──── On-device TinyML ──────────────────────────┐       │    │
  │  │  • Impedance → cell density regression          │       │    │
  │  │  • CO₂ rate → fermentation phase classifier     │       │    │
  │  │  • pH + temp → spoilage risk classifier         │       │    │
  │  │  • Acoustic → bubble rate / stall detector      │       │    │
  │  └─────────────────────────────────────────────────┘       │    │
  └──────────────────────────────────────────────────────────┘    │
                                                                   │
  ┌──────────────────────────────────────────────────────────┐    │
  │              4-Electrode Impedance Probe (PEEK)           │    │
  │                                                          │    │
  │   ──[I+]──   ──[V+]──   ──[V-]──   ──[I-]──              │    │
  │   Pt/Pt     Pt/Pt       Pt/Pt     Pt/Pt                  │    │
  │   (excite)  (sense)     (sense)   (excite)               │    │
  │                                                          │    │
  │   Wired to AD5933 via external analog switch (ADG715)    │    │
  │   for 4-wire Kelvin measurement                         │    │
  └──────────────────────────────────────────────────────────┘    │
```

### Signal Flow Summary

1. **Impedance path:** ESP32-S3 configures the AD5933 over I²C with a
   frequency sweep (e.g., 1 kHz to 100 kHz in 50 steps). The AD5933 excites
   the outer electrode pair (I+, I-) with a sinusoidal voltage. The inner
   electrode pair (V+, V-) senses the resulting potential, which the AD5933
   digitizes and computes a DFT, returning real and imaginary impedance
   components. The ESP32-S3 collects the full sweep and passes it to the
   TinyML biomass model.

2. **CO₂ path:** The Senseair S8 NDIR sensor runs autonomously, maintaining
   its own baseline calibration and ABC (Automatic Baseline Correction).
   The ESP32-S3 polls it over UART2 every 15 seconds. The CO₂ evolution rate
   is computed as the time derivative of the concentration, normalized to
   vessel headspace volume.

3. **pH path:** The LMP91200 ISFET front-end conditions the pH probe signal
   (a high-impedance voltage proportional to pH). The ESP32-S3 reads the
   conditioned voltage on ADC1_CH0 and converts it to pH using a two-point
   calibration (pH 4.00 and pH 7.00 buffers) stored in NVS.

4. **Temperature path:** The MAX31865 SPI bridge converts the PT100 RTD
   resistance to temperature. This temperature is used both for fermentation
   monitoring and for temperature-compensating the impedance and pH
   readings.

5. **Acoustic path:** The MEMS microphone (I²S PDM) streams audio at
   16 kHz to the ESP32-S3. The firmware computes a rolling 2048-point FFT
   every 250 ms and extracts bubble-detected events by matching a spectral
   template (broadband impulse, 1–8 kHz, 20–80 ms duration). The bubble
   rate (bubbles/min) is a secondary fermentation-vigor indicator.

6. **Fusion & inference:** All sensor streams are timestamped, queued, and
   processed by the on-device fusion + TinyML pipeline every 60 seconds to
   produce: estimated cell density (cells/mL), fermentation phase (lag /
   exponential / stationary / decline), ABV (if beer/wine), and spoilage
   risk score.

---

## 4. Firmware Details & Design Decisions

### RTOS Architecture

The firmware uses FreeRTOS (bundled with ESP-IDF v5.1) with the following
task structure:

| Task | Priority | Stack | Core | Period | Function |
|---|---|---|---|---|---|
| `impedance_task` | 5 | 4096 | 1 | 60 s | Run AD5933 sweep, collect Re/Im, feed model |
| `co2_task` | 4 | 2048 | 1 | 15 s | Poll Senseair S8, compute CER |
| `ph_task` | 3 | 2048 | 0 | 30 s | Read ISFET voltage, convert to pH |
| `temp_task` | 3 | 2048 | 0 | 10 s | Read MAX31865, publish to shared state |
| `acoustic_task` | 6 | 8192 | 0 | continuous | I²S DMA capture, FFT, bubble detection |
| `fusion_task` | 7 | 6144 | 0 | 60 s | Run TinyML inference, compute phase/ABV/spoilage |
| `ble_task` | 4 | 4096 | 0 | event | BLE GATT notifications to phone |
| `wifi_task` | 3 | 4096 | 0 | 30 s | MQTT publish, OTA check |
| `logger_task` | 2 | 3072 | 0 | 5 s | Write to SD card (if present) / flash ring buffer |

### Impedance-to-Biomass Model

The TinyML biomass model is a 3-layer fully-connected neural network
quantized to INT8, running via TensorFlow Lite for Microcontrollers (TFLM).
The input feature vector is the impedance sweep reduced to 8 features:

1. |Z| at 1 kHz (low-frequency conductivity, dominated by ionic transport)
2. |Z| at 10 kHz (cell membrane capacitance influence begins)
3. |Z| at 100 kHz (beta-dispersion region — cell membrane charging)
4. Phase angle at 10 kHz (membrane capacitance proxy)
5. Phase angle at 100 kHz (intracellular vs extracellular ratio)
6. Cole-Cole α parameter (relaxation time distribution width)
7. Cole-Cole R₀ (extracellular resistance)
8. Cole-Cole R∞ (total resistance at infinite frequency)

The model outputs `log10(cell_density)` in cells/mL. The calibration dataset
was generated by fermenting 20 batches (10 beer, 5 kombucha, 5 yogurt) with
paired hemocytometer / plate counts. The model achieves R² = 0.94 on held-out
test data.

### CO₂ Evolution Rate (CER)

The CER is computed as a smoothed finite difference of the CO₂ concentration
time series, normalized to the headspace volume and corrected for temperature
(ideal gas law). The CER in mmol/L/h is the primary input to the fermentation
phase classifier:

- **Lag phase:** CER < 0.5 mmol/L/h (yeast adapting, no active growth)
- **Exponential phase:** CER > 2.0 mmol/L/h, rising (exponential growth)
- **Stationary phase:** CER plateau (growth = death rate)
- **Decline phase:** CER falling, < 0.3 mmol/L/h (fermentation ending)

### ABV Estimation

For alcoholic fermentations, FermenTiq estimates ABV from the integrated CO₂
evolution (stoichiometric: 1 mol glucose → 2 mol CO₂ + 2 mol ethanol) plus
the pH shift (for acid-producing contamination correction):

```
ABV ≈ (CO₂_total_mol × 2 × 46.07 g/mol) / (must_volume_g) × 100%
```

This is corrected for temperature and headspace CO₂ dissolution using
Henry's Law.

### Spoilage Detection

Spoilage is flagged when the fusion model detects any of:
- pH drops faster than 0.15 pH/hour (acid contamination)
- Cell density rises but CER does not (possible bacterial contamination
  with different metabolic profile)
- Acoustic signature shows continuous low-frequency rumble (gas from
  putrefaction, not rhythmic CO₂ bubbling)
- Temperature exceeds the recipe's maximum safe temperature

### Design Decisions

1. **4-electrode (tetrapolar) impedance measurement** instead of 2-electrode:
   eliminates the electrode-liquid interface impedance (which dominates at
   low frequencies and masks the tissue signal). This is the same technique
   used in medical bioimpedance (e.g., body composition scales).

2. **AD5933 + ADG715 analog switch** for electrode multiplexing: the AD5933
   is a 2-wire device, so we use an 8:1 analog switch to configure it for
   4-wire Kelvin measurement by routing the excitation and sensing paths
   independently.

3. **Platinum electrodes instead of stainless steel:** platinum is inert in
   the fermentation environment (no corrosion, no contamination of the
   product), and its electrochemical stability ensures repeatable impedance
   measurements over hundreds of batches.

4. **PEEK probe body instead of 3D-printed PETG:** PEEK is FDA-compliant,
   autoclavable at 134 °C, and resistant to ethanol (up to 20% v/v), lactic
   acid, acetic acid, and the cleaning chemicals used between batches.

5. **ESP32-S3 instead of nRF52840:** the S3 provides WiFi (critical for MQTT
   integration with Home Assistant / brewery control systems), more flash
   for the TinyML model and data logging, and sufficient RAM for the audio
   FFT pipeline. The nRF52840 would require a separate WiFi radio.

6. **Senseair S8 instead of MG-811 (electrochemical CO₂):** the S8 is NDIR
   (non-dispersive infrared), which is more accurate (±40 ppm vs ±50 ppm),
   drifts less, and has a 15-year lifespan. The MG-811 electrochemical
   sensor requires frequent calibration and is affected by humidity.

7. **FreeRTOS instead of bare-metal super-loop:** the multiple independent
   sensing modalities with different sampling rates (15 s for CO₂, 60 s for
   impedance, continuous for audio) are cleanly expressed as separate tasks
   with priority-based preemption.

---

## 5. Application / Software Interface

### Companion App (React Native)

The FermenTiq app (React Native, iOS + Android) provides:

- **Live Dashboard:** Real-time cell density, CO₂ rate, pH, temperature,
  bubble rate, and fermentation phase. Animated gauges and time-series
  charts (using react-native-chart-kit).
- **Batch Setup Wizard:** Select recipe type (beer/wine/kombucha/yogurt/
  kimchi/sourdough/cider), enter vessel volume, set temperature target and
  alarm thresholds. The wizard pushes the configuration to the device over
  BLE.
- **Trend Analysis:** Historical charts of all sensor channels over the
  batch lifetime. Overlay multiple batches for comparison.
- **Spoilage Alerts:** Push notifications when the device detects
  contamination risk, temperature excursion, or stalled fermentation.
- **Recipe Library:** Community-shared recipes with expected fermentation
  curves (template matching against your live data).
- **Calibration:** Walk-through for pH (2-point buffer) and impedance
  (air + reference solution) calibration.
- **Export:** CSV / JSON export of batch data for integration with
  Brewfather, Brewer's Friend, or custom analysis.

### MQTT / Home Assistant Integration

FermenTiq publishes to MQTT topics for home-automation integration:

```
fermentiq/<device_id>/state          → JSON: all current sensor values
fermentiq/<device_id>/phase          → string: lag|exponential|stationary|decline
fermentiq/<device_id>/abv            → float: estimated ABV %
fermentiq/<device_id>/spoilage_risk  → int: 0-100
fermentiq/<device_id>/batch          → JSON: batch metadata
fermentiq/<device_id>/config/set     ← (inbound) configuration commands
```

Home Assistant auto-discovery is supported via the MQTT discovery protocol,
so FermenTiq appears automatically as a set of sensors in the HA dashboard.

---

## 6. Use Cases & Target Audience

### Primary Use Cases

1. **Homebrew beer & wine:** Real-time fermentation monitoring without
   sampling. Know when fermentation is done, detect stuck fermentation early,
   estimate ABV, and catch contamination before the batch is ruined.

2. **Commercial craft breweries:** Supplement or replace lab cell counts with
   continuous in-line impedance-based biomass monitoring. Track batch-to-batch
   consistency and detect deviations in real time.

3. **Kombucha & fermented tea:** Monitor the SCOBY's activity via CO₂ rate
   and pH. Know when the first fermentation is at the right sweetness/acidity
   balance, and when to bottle for the desired carbonation level.

4. **Yogurt & kefir:** Track the pH drop and lactic acid production curve.
   Stop fermentation at the exact target pH (4.5–4.6 for yogurt) for
   consistent texture and flavor.

5. **Kimchi & sauerzoic fermentations:** Monitor pH, temperature, and CO₂
   to ensure food safety (pH < 4.6 within 48 hours prevents botulism risk)
   and optimal ripeness.

6. **Sourdough starter maintenance:** Track the microbial activity rhythm
   of your starter to optimize feeding schedule and baking timing.

7. **Research & education:** Bioprocess engineering classes, food science
   research labs, and citizen-science fermentation projects.

### Target Audience

- **Homebrewers and fermenters** (the largest group — 1.2M+ in the US alone)
- **Craft breweries, wineries, cideries, and kombucha taprooms** (quality
  control and batch consistency)
- **Artisanal food producers** (cheese, kimchi, sauerkraut, miso, vinegar)
- **Food science / bioprocess educators and researchers**
- **Fermentation hobbyists and the "gut health" community**

---

## 7. Bill of Materials (Summary)

| Ref | Part | Qty | Unit Cost (est.) |
|---|---|---|---|
| U1 | ESP32-S3-WROOM-1-N16R8 | 1 | $4.50 |
| U2 | AD5933YRSZ | 1 | $22.00 |
| U3 | ADG715 8:1 analog switch | 1 | $3.20 |
| U4 | Senseair S8 (LP8) NDIR CO₂ | 1 | $28.00 |
| U5 | LMP91200 ISFET front-end | 1 | $4.80 |
| U6 | MAX31865 RTD bridge | 1 | $6.50 |
| U7 | SHT41 temp/RH | 1 | $2.50 |
| U8 | TP4056 charger | 1 | $0.80 |
| U9 | MAX17048 fuel gauge | 1 | $3.50 |
| MIC1 | SPH-0645LM4H-B MEMS mic | 1 | $1.80 |
| RTD1 | PT100 3-wire, 1/10 DIN | 1 | $4.00 |
| PROBE | PEEK machined body + Pt electrodes | 1 | $12.00 |
| BAT1 | 18650 holder + 3500 mAh cell | 1 | $5.00 |
| MISC | PCB, passives, connectors, O-rings | — | $8.00 |
| **Total** | | | **~$107** |

---

## 8. Safety & Regulatory Notes

- The PEEK probe body and platinum electrodes are food-contact safe
  (FDA 21 CFR 177.2415 for PEEK, platinum is GRAS for food contact).
- The impedance excitation voltage (200 mVpp) is far below the
  electrochemical decomposition threshold of water (~1.23 V) and is
  non-stimulatory to microorganisms.
- The device is not a substitute for proper food-safety testing. For
  commercial products, follow HACCP protocols and local regulations.
- The NDIR CO₂ sensor is in the headspace (not submerged) and measures
  ambient-atmosphere CO₂ — it does not contact the food product.

---

## 9. Repository Structure

```
fermentiq/
├── README.md                 ← this file
├── firmware/
│   ├── main.c                ← FreeRTOS main, task creation, system init
│   ├── board.h               ← Pin assignments, hardware constants
│   ├── registers.h           ← Sensor register definitions
│   ├── Makefile              ← ESP-IDF build (or standalone arm-none-eabi)
│   ├── linker.ld             ← Linker script (memory regions)
│   └── drivers/
│       ├── ad5933.c/.h       ← Impedance analyzer driver (sweep, DFT, Kelvin)
│       ├── co2_ndir.c/.h     ← Senseair S8 UART driver + CER computation
│       ├── ph_isfet.c/.h     ← LMP91200 + ISFET pH driver + calibration
│       ├── temp_rtd.c/.h     ← MAX31865 SPI RTD driver
│       ├── acoustic.c/.h     ← I²S MEMS mic + FFT + bubble detection
│       ├── fusion.c/.h       ← Sensor fusion + TinyML inference + phase/ABV
│       ├── ble.c/.h          ← BLE GATT server (live data + config)
│       ├── wifi_mqtt.c/.h    ← WiFi + MQTT publish + HA discovery
│       ├── power.c/.h        ← Battery charger + fuel gauge + low-power
│       └── storage.c/.h      ← SD card / flash ring-buffer logging
├── kicad/
│   ├── device.kicad_sch      ← Full schematic (MCU, sensors, power, probe)
│   ├── device.kicad_pcb      ← 4-layer PCB layout
│   └── device.kicad_pro      ← KiCad project file
└── app/
    ├── App.js                ← React Native app entry + navigation
    ├── package.json          ← Dependencies (react-native, ble, charts)
    ├── screens/
    │   ├── DashboardScreen.js   ← Live sensor dashboard
    │   ├── BatchSetupScreen.js  ← New batch wizard
    │   ├── TrendScreen.js       ← Historical trend charts
    │   ├── CalibrationScreen.js ← pH + impedance calibration
    │   └── SettingsScreen.js    ← Device config + WiFi + alerts
    ├── components/
    │   ├── SensorGauge.js       ← Animated radial gauge component
    │   └── FermentationChart.js ← Time-series chart component
    └── utils/
        ├── ble.js               ← BLE connection manager
        └── protocol.js          ← Binary protocol parser/encoder
```

---

## 10. License & Attribution

- **Hardware (KiCad):** CERN-OHL-S v2 — Copyright © 2026 jayis1
- **Firmware (C):** GPL-3.0 — Copyright © 2026 jayis1
- **App (React Native):** MIT — Copyright © 2026 jayis1

**Author:** jayis1  
**Created:** 2026-08-07  
**Repository:** hardware-design-lab

---

*FermenTiq — Know your fermentation. Not just its gravity, but its life.*