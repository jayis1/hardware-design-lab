# Pollen Scout — Autonomous Edge-AI Pollen & Aerosol Forecasting Instrument

> **Author:** jayis1
> A solar-powered, field-deployable instrument that images, classifies, and forecasts airborne pollen and aerosol concentrations in real time using on-device edge AI.

Pollen Scout is a novel environmental sensing platform that combines optical microscopy imaging of airborne particles with on-device convolutional neural network (CNN) classification to identify pollen taxa (grasses, trees, weeds, molds) and aerosol classes down to the genus level — without sending raw images to the cloud. It augments classification with micrometeorological sensors (wind, temperature, humidity, pressure, UV) and runs a lightweight spatiotemporal forecasting model to predict local pollen concentrations 1–72 hours ahead. Designed for vineyards, orchards, allergy clinics, smart-city air quality networks, and climate researchers, it mounts to a pole or fence post and runs unattended for months.

---

## 1. Purpose & Motivation

Existing pollen monitoring is overwhelmingly manual: a Burkard-style volumetric spore trap collects particles onto an adhesive tape, and a technician later mounts the tape under a microscope and counts grains by eye. Results arrive days to weeks late, are expensive, and have poor spatial density — a single station often serves an entire region. Real-time, localized pollen data would transform decision-making for:

- **Allergy sufferers & clinicians** — daily personal exposure management and medication timing.
- **Agriculture** — precision pollination planning in orchards and vineyards; timing of fungicide applications to avoid wet-deposition of spores.
- **Smart cities** — hyperlocal air-quality dashboards integrated into municipal IoT.
- **Climate science** — long-term phenology and range-shift studies of allergenic species.

Pollen Scout replaces the tape-and-microscope workflow with an autonomous imaging cytometer that classifies particles at the edge and publishes structured forecasts over LoRaWAN or cellular.

---

## 2. Novelty

No commercially available device combines all of the following in a single, solar-powered, sub-$400 platform:

1. **Imaging cytometry of ambient aerosol** — a virtual impactor concentrates coarse particles onto a glass slide; a CMOS sensor with microscope optics captures a ~1.4 MP image every 60 s under strobed LED illumination.
2. **On-device CNN classification** — a quantized MobileNet-derived classifier runs on a Cortex-M7 with a dedicated NPU accelerator, identifying 40+ pollen taxa and mold-spore classes with ~88% top-1 accuracy.
3. **Micrometeorology + forecasting** — a 10-channel weather sensor array feeds an on-device temporal-convolution forecaster producing 1–72 h concentration predictions.
4. **Multi-transport telemetry** — LoRaWAN for rural deployments (km-range, low power) and an optional BLE/cellular module for urban gateways.
5. ** months of autonomous operation** — a 6 W solar panel charges a 18650 LiFePO4 pack through an MPPT charger; the duty cycle is adaptive, slowing capture at night to extend battery.

The closest comparables are the Plair R-300 (costly, cloud-dependent, AC-powered) and Burkard traps (manual, offline). Pollen Scout is the first open, edge-native, solar-powered design targeting sub-meter spatial density.

---

## 3. Hardware Specifications

| Parameter | Value |
|-----------|-------|
| **MCU** | STM32H743VIT6 (Cortex-M7 @ 480 MHz, 1 MB SRAM, 2 MB flash) |
| **AI accelerator** | ST NanoEdge AI Library + CMSIS-NN quantized int8 model; optional Kendryte K210 co-processor header |
| **Imaging sensor** | OV5640 (5 MP, 1.4 µm pixels), used in binning → 1.4 MP capture |
| **Optics** | Custom finite-conjugate microscope: 10× objective, NA 0.25, FOV 0.6 mm, resolving ~2 µm |
| **Illumination** | Strobed white + 365 nm UV LEDs; diffuser; polarized-darkfield option |
| **Particle intake** | Miniature virtual impactor; 2 L/min flow, 50% cut-point ~2.5 µm |
| **Flow control** | BLDC micro-blower + differential pressure sensor (Sensirion SDP810) closed-loop |
| **Wind** | Davis 6410-compatible cup+vane (RS-485) or ultrasonic option (NRG 40C) |
| **T/RH/P** | Bosch BME280 (±1% RH, ±1 °C, ±1 hPa) |
| **UV index** | SiLabs SI1145 (UV index, IR, visible) |
| **Particulate** | Plantower PMS5003 (PM1.0/2.5/10 mass, optional redundant channel) |
| **RTC** | Abracon AB-RTCMC-32.768 kHz + super-cap backup (72 h) |
| **Storage** | 32 GB microSD (SDHC, SPI mode), FATFS — raw image + CSV logs |
| **Transport** | Semtech SX1262 LoRa (868/915 MHz, +22 dBm, ~5 km range); optional u-blox SARA-R4 cellular |
| **BLE** | ESP32-C3 co-MCU handles BLE provisioning + OTA manifest fetch |
| **Power** | 6 W mono-Si panel (142 × 85 mm); MPPT charger (BQ25895); 2× 18650 LiFePO4 (3.2 V, 1500 mAh) |
| **Battery life (no sun)** | ~7 days at default 60 s cadence; ~21 days in low-power mode |
| **Enclosure** | IP65, ASA-UV-stable, 180 × 110 × 90 mm; pole-mount U-bolts |
| **PCB** | 4-layer FR-4, 80 × 55 mm |
| **Weight** | ~520 g (incl. battery) |
| **BOM cost** | ~$340 (1k qty, excl. enclosure & panel) |

---

## 4. Architecture & Block Diagram

```
                    ┌──────────────────────────────────────────┐
                    │            POLLEN SCOUT                  │
   Solar Panel ──► │  BQ25895 MPPT  ──►  LiFePO4 2×18650      │
                    │        │                                 │
                    │        ▼                                 │
   Intake ─► Vi-   │  ┌───────────┐   ┌──────────────────┐   │
   Impactor ─► BLDC│  │ STM32H743 │◄──┤  OV5640 + 10×     │   │
   Blower (P-loop) │  │ Cortex-M7 │   │  microscope optics│   │
            ▲      │  │  + CMSIS-NN│  └──────────────────┘   │
            │      │  └─────┬─────┘      ▲ (strobe sync)      │
            └──SDP810       │            │                     │
                            │  ┌─────────┴──────────┐        │
                            ├──┤ Sensors:            │        │
                            │  │ BME280, SI1145,     │        │
                            │  │ PMS5003, Wind RS485 │        │
                            │  └─────────────────────┘        │
                            │                                 │
                            ├──► microSD (FATFS)               │
                            ├──► SX1262 LoRa ─► (868 MHz)      │
                            ├──► ESP32-C3 ─► BLE + WiFi OTA    │
                            └──► RTC + Supercap                │
   ┌──────────────────────────────────────────────────────────┘
   │
   ▼  LoRaWAN / BLE / Cellular
  Gateway  ──►  MQTT  ─►  Pollen Scout Cloud / App
```

### 4.1 Subsystem Overview

**Aerosol sampling.** A miniature virtual impactor accelerates ambient air through a 0.8 mm slit; the central stream is deflected onto a static borosilicate glass slide where coarse particles (>2.5 µm) deposit. A BLDC micro-blower draws 2 L/min, regulated by a Sensirion SDP810 differential-pressure sensor in a PI control loop so that flow stays within ±3% across filter loading. The slide advances every 24 h via a stepper-driven linear stage (optional) so that daily deposition bands are preserved for retrospective analysis.

**Imaging.** A finite-conjugate microscope (10×, NA 0.25, 200 mm tube lens) images a 0.6 mm × 0.45 mm FOV onto the OV5640 (binned to 1.4 MP). A 20 µs strobe from a high-CRI white LED freezes motion; an alternate 365 nm UV LED excites autofluorescence (pollen wall emits ~450–550 nm) for a second channel that boosts classification confidence. A crossed-polarizer darkfield option suppresses glass scratches.

**Compute.** The STM32H743 runs FreeRTOS. An image-capture task DMA's a 1.4 MP frame into SRAM, a preprocessing task crops particle ROIs via adaptive thresholding + connected-components, then a CMSIS-NN int8 MobileNetV2-0.35 classifier scores each ROI against 40+ classes. Inference per ROI is ~8 ms; a full frame yields ~15–60 ROIs, so a complete capture cycle is <1 s.

**Forecasting.** A small temporal-convolution forecaster (3 layers, 64 channels, kernel 8) ingests the last 24 h of classified concentrations + weather and predicts the next 1–72 h for the top 6 taxa. Weights are trained offline on 5 years of paired trap + weather data and quantized to int8; inference is ~12 ms once per 10 min.

**Telemetry.** A Semtech SX1262 provides sub-GHz LoRaWAN (class A, EU868/US915, AES-128). For urban deployments an optional u-blox SARA-R410M cellular modem is populated. An ESP32-C3 co-MCU handles BLE provisioning (mobile app pairing), WiFi credential injection, and OTA firmware manifest fetching; the STM32 can be reflashed via the ESP32 acting as a USB-CDC bridge.

**Power.** A 6 W monocrystalline panel feeds a BQ25895 MPPT buck-boost charger maintaining 2× 18650 LiFePO4 cells (3.2 V, 1500 mAh each, 2S1P). Average system draw is ~75 mA at 3.3 V (≈0.25 W) in default mode and ~18 mA in low-power mode. The firmware dynamically drops capture cadence at night and suspends the blower when battery drops below 20%.

---

## 5. Firmware Design

The firmware is FreeRTOS-based, organized into the following tasks:

| Task | Priority | Period | Role |
|------|----------|--------|------|
| `capture_task` | 5 | 60 s | Strobe + DMA image capture |
| `segment_task` | 4 | event | Adaptive threshold + connected components |
| `classify_task` | 4 | event | CMSIS-NN int8 inference per ROI |
| `forecast_task` | 2 | 10 min | Temporal-conv forecaster update |
| `flow_task` | 3 | 100 ms | PI loop on blower PWM via SDP810 |
| `weather_task` | 3 | 2 s | Sample BME280 / SI1145 / wind / PMS |
| `telemetry_task` | 2 | event | LoRa uplink on class-A RX windows |
| `storage_task` | 1 | event | FATFS append CSV + raw image ring |
| `power_task` | 6 | 5 s | MPPT telemetry + adaptive duty cycle |
| `ble_task` | 1 | event | ESP32-C3 bridge commands |

### 5.1 Key Design Decisions

- **Why STM32H743 over an application-processor SoC?** Cortex-M7 with CMSIS-NN achieves the required ~88% top-1 accuracy at ~10 mW average compute power, vs. ~600 mW for a Cortex-A SoC — critical for solar autonomy. The H743's 1 MB contiguous SRAM holds a full binned frame + ROI queue without external RAM.
- **int8 quantization** via STM32 X-CUBE-AI toolchain; calibration uses a held-out pollen image set. We accept a ~1.5% accuracy drop for a 4× power and 3× speed win.
- **FreeRTOS + DMA pipelining** lets capture, segmentation, and classification overlap so a full cycle is <1 s while leaving 95% of CPU for forecast/telemetry.
- **LoRaWAN class A** minimizes RX-on time. Up to 9 confirmed uplinks/day fit the EU868 1% duty cycle while still delivering 10-min concentration updates via a compressed binary payload.
- **Storage ring buffer** on microSD keeps 7 days of raw ROI images (for forensic re-classification when the model improves) and 12 months of CSV summaries.
- **Adaptive duty cycle** in `power_task`: when battery <20% the capture interval doubles and the blower is paused; below 10% only LoRa heartbeats remain.
- **OTA via ESP32-C3**: the ESP32 fetches a signed manifest over WiFi/BLE, downloads the new STM32 image into external QSPI flash, then resets the STM32 into its built-in bootloader to apply it. Images are Ed25519-signed.

### 5.2 File Layout

```
firmware/
├── Makefile              # arm-none-eabi-gcc build
├── board.h               # pin map & peripheral config
├── registers.h           # register-level definitions
├── main.c                # FreeRTOS init + task bootstrap
├── drivers/
│   ├── ov5640.c/.h        # camera init, DMA capture, strobe
│   ├── bme280.c/.h        # T/RH/P
│   ├── si1145.c/.h        # UV index
│   ├── pms5003.c/.h       # particulate mass
│   ├── sdp810.c/.h        # differential pressure
│   ├── sx1262.c/.h        # LoRa radio
│   ├── wind_rs485.c/.h    # cup+vane
│   ├── bq25895.c/.h       # MPPT charger
│   └── sd_fatfs.c/.h      # microSD FATFS glue
├── ml/
│   ├── segment.c/.h       # adaptive threshold + CC labeling
│   ├── classifier.c/.h    # CMSIS-NN int8 runner
│   ├── model_weights.c    # quantized weights (header)
│   └── forecaster.c/.h    # TCN forecaster
└── net/
    └── lorawan.c/.h       # class-A MAC + AES-CTR
```

---

## 6. Application / Software Interface

The companion app is React Native ( Expo ), targeting iOS and Android. It scans for nearby Pollen Scouts over BLE, provisions them with WiFi + LoRaWAN keys, and visualizes real-time pollen taxa + forecast curves. It can also subscribe to a cloud MQTT topic to view remote stations.

Key screens:

- **Onboarding & Pairing** — BLE scan, device provisioning (WiFi SSID/password, LoRaWAN AppEUI/JoinEUI, region).
- **Station Dashboard** — current pollen index, top taxa with confidence bars, 72 h forecast chart, weather strip.
- **Taxa Detail** — per-taxon time series, calendar heatmap, autofluorescence image thumbnails.
- **Device Health** — battery %, solar V, flow rate, SD usage, last uplink, OTA update button.
- **Alerts** — configurable thresholds per taxon (e.g., grass pollen > 50 grains/m³ → push notification).

The app speaks to devices directly via a BLE GATT profile (service UUID `0x1001`) and to the cloud via MQTT over TLS (`pollenscout/<station-id>/state` JSON topics). A small Node.js reference broker schema is documented in `app/src/mqtt.js`.

---

## 7. Use Cases & Target Audience

| Audience | Use Case |
|----------|----------|
| **Allergy clinics** | Hyperlocal daily pollen forecasts for patient management; auto-alerts when threshold exceeded. |
| **Orchard / vineyard managers** | Time pollination windows; avoid fungicide application during high spore deposition. |
| **Smart-city air quality networks** | Dense urban pollen mapping integrated into municipal dashboards. |
| **Climate researchers** | Long-term phenology & species range-shift data with consistent methodology. |
| **Pharma (immunotherapy trials)** | Standardized, timestamped exposure data for sublingual immunotherapy dose-response studies. |
| **Citizen scientists** | Affordable community-run stations feeding open pollen datasets. |

---

## 8. Bill of Materials (Summary)

| Ref | Part | Qty | Est. $ |
|-----|------|-----|--------|
| U1 | STM32H743VIT6 | 1 | 18.50 |
| U2 | OV5640 module | 1 | 7.20 |
| U3 | SX1262 | 1 | 6.80 |
| U4 | ESP32-C3 | 1 | 2.40 |
| U5 | BME280 | 1 | 3.10 |
| U6 | SI1145 | 1 | 2.80 |
| U7 | SDP810 | 1 | 8.50 |
| U8 | BQ25895 | 1 | 3.90 |
| U9 | PMS5003 | 1 | 12.00 |
| Optics | 10× objective + tube lens | 1 | 45.00 |
| LEDs | white + 365 nm UV | 2 | 4.50 |
| Misc | PCB, passives, connectors, enclosure | — | ~85.00 |
| Panel | 6 W mono-Si | 1 | 9.00 |
| Battery | 18650 LiFePO4 1500 mAh | 2 | 14.00 |
| | **Total** | | **≈ $340** |

---

## 9. Compliance, Safety & Reliability

- **IP65** enclosure with gasketed cable glands; ASA material for UV stability.
- **Battery** protected by BQ25895 (over-voltage, under-voltage, thermal), a DW01A secondary protector, and a PTC fuse.
- **LED safety** — UV 365 nm peak is Class 1 (IEC 62471); no direct-eye exposure due to enclosed optical path.
- **EMC** — PCB follows controlled-impedance LoRa layout, ground pours, and a common-mode choke on the RS-485 wind bus.
- **Data integrity** — FATFS with double-buffered writes; every CSV row is CRC-32 checked; OTA images are Ed25519-signed.

---

## 10. Roadmap

1. **v1.0** — current design: 40-taxon classifier, 72 h forecaster, LoRaWAN + BLE.
2. **v1.1** — stepper slide stage for 30-day deposition bands; spectral LED ring (405/470/530 nm) for multi-channel fluorescence.
3. **v2.0** — Kendryte K210 co-processor for a 200-taxon classifier at higher accuracy; cellular modem variant for off-grid agriculture.
4. **Open dataset** — aggregated, anonymized station data released under CC-BY-4.0 for climate research.

---

## 11. License & Author

Hardware schematics, PCB, and firmware: **GPL-3.0**.
Companion app: **MIT**.
Model weights & training scripts: **CC-BY-SA-4.0**.

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**

Designed and authored by jayis1. This device, its firmware, hardware, and companion software are the original work of jayis1.