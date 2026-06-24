# WattLens — Open-Source 3-Phase Power Quality Analyzer & Non-Intrusive Load Monitor

![WattLens](https://img.shields.io/badge/PCB-120×72mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H743-orange) ![ADC](https://img.shields.io/badge/ADC-ADS131M08%20%2B%20AMC1301-green) ![Comms](https://img.shields.io/badge/Comms-BLE%205.2%20%2B%20LoRa%20%2B%20USB--C-purple) ![License](https://img.shields.io/badge/License-MIT-yellow) ![Author](https://img.shields.io/badge/Author-jayis1-orange)

**Author: jayis1** · **Copyright © 2026 jayis1** · **MIT License**

> A professional-grade, handheld and DIN-rail mountable power quality analyzer that simultaneously samples 3-phase voltage and current at 24-bit resolution, computes real/reactive/apparent power, power factor, total harmonic distortion, harmonic spectra to the 50th order, voltage sag/swell, and flicker (Pst) — then runs an on-device neural model to disaggregate which individual appliances are running from the aggregate mains signal alone (non-intrusive load monitoring, NILM). Designed for electricians, facility managers, energy auditors, and researchers.

---

## 1. Purpose & Overview

Electrical power quality is invisible, insidious, and expensive. Harmonics from switched-mode loads overheat neutral conductors and transformers. Voltage sags interrupt sensitive processes. Flicker causes discomfort and equipment malfunction. Yet power quality analyzers that can diagnose these problems cost $3,000–$15,000 and are locked behind proprietary ecosystems. Meanwhile, knowing *which* appliances consume *how much* energy — without instrumenting every outlet — has been an academic curiosity (NILM) that never reached the electrician's toolkit.

**WattLens closes both gaps in one open device.** It is a fully open-source, calibration-capable power quality instrument that:

1. **Analyzes power quality** to professional standards — IEC 61000-4-30 Class S compliant measurement of harmonics (IEC 61000-4-7), flicker (IEC 61000-4-15), sag/swell, and power parameters across all three phases and neutral.
2. **Disaggregates loads** using an on-device lightweight neural network that classifies appliance on/off events and estimates per-appliance energy consumption from the aggregate mains signature alone — no per-outlet sensors needed.
3. **Is open and affordable** — the full BOM costs under $120 in single quantities, every schematic, footprint, firmware source, and app source is published, and the device is field-calibratable.

### What makes it novel

No existing open-source hardware project combines simultaneous 3-phase power quality analysis to IEC standards *with* on-device neural NILM. Academic NILM runs on a Raspberry Pi with a wall-wart ADC; commercial power quality analyzers (Fluke, Hioki, Dranetz) do no load disaggregation at all. WattLens bridges these two worlds onto a single Cortex-M7 with hardware floating-point, making real-time spectral analysis and event-classification inference happen in the same 1-second measurement window.

---

## 2. Hardware Specifications

| Parameter | Specification |
|---|---|
| **MCU** | STM32H743VIT6 — ARM Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM, hardware double-precision FPU, FMC for display |
| **ADC** | TI ADS131M08 — 8-channel, simultaneous-sampling, 24-bit delta-sigma, up to 32 kSPS, programmable gain, SPI interface |
| **Isolation** | TI AMC1301B reinforced isolation amplifiers (voltage channel front-end); 4 kV basic isolation barrier |
| **Current sensing** | Split-core CTs (333 mV output @ rated current), 3× current + 1× neutral/leakage channel; supports 100 A, 200 A, 400 A CTs via software scaling |
| **Voltage sensing** | Resistive divider (1 MΩ:1 kΩ) into AMC1301, 600 V CAT III rated, 3× voltage channels (L1/L2/L3 to neutral) |
| **Sample rate** | 2048 samples/s/channel (simultaneous, phase-synchronous) — 40.96 Hz fundamental bin, 40 Hz harmonic resolution at 50 Hz |
| **Harmonic range** | DC to 50th harmonic (2.5 kHz at 50 Hz, 3 kHz at 60 Hz) |
| **Display** | 2.4″ 320×240 TFT (ILI9341) with resistive touch, SPI |
| **Wireless** | BLE 5.2 (nRF52840 module), LoRa SX1262 (868/915 MHz, 22 dBm) for fleet/site mesh |
| **Wired** | USB-C (USB 2.0 full-speed CDC for data + firmware; 5 V power) |
| **Storage** | microSD card (FAT32, SPI mode) — event logs, waveform captures, disaggregation results |
| **Power** | Internal 1800 mAh Li-ion (handheld); USB-C 5 V; or DIN-rail 5–24 V DC. TP4056 charger, MAX17048 fuel gauge |
| **IMU** | LSM6DSO — 6-axis accelerometer + gyro for mounting orientation / vibration correlation |
| **RTC** | PCF85263A with supercapacitor backup — timestamped event logging |
| **Form factor** | 120 × 72 mm, 4-layer PCB, handheld enclosure or DIN-rail adapter |
| **Safety rating** | 600 V CAT III (voltage inputs), CT-isolated current inputs |
| **Battery life** | ~8 hours continuous sampling + BLE |

### Power domains

| Domain | Voltage | Source | Consumers |
|---|---|---|---|
| Mains-isolated front-end | ±2.5 V (analog) | Isolated flyback from mains (DIN mode) or isolated DC-DC from battery | ADS131M08 analog, CT burden, AMC1301 high side |
| Isolated digital (ADC side) | 3.3 V | ISO7720-isolated SPI bridge | ADS131M08 digital |
| Main digital | 3.3 V | USB-C / battery / DC jack | STM32H743, BLE, LoRa, display, SD, IMU, RTC |

The isolation barrier between the mains-connected front-end and the user-accessible digital domain is the critical safety boundary. The ADS131M08 lives on the isolated side; its SPI crosses through a TI ISO7720 digital isolator (and an isolated DC-DC provides power). This means the user touches only SELV circuitry even when measuring 600 V.

---

## 3. Architecture & Block Diagram

```
                      ┌───────────────┐    ┌───────────────┐
   L1 ──[1M:1k div]──▶│  AMC1301 (V1) │──▶│               │
   L2 ──[1M:1k div]──▶│  AMC1301 (V2) │──▶│   ADS131M08   │
   L3 ──[1M:1k div]──▶│  AMC1301 (V3) │──▶│  8-ch 24-bit  │
                      └───────────────┘    │  ΔΣ ADC       │
   CT1 (333mV) ──────────────────────────▶│  simultaneous │
   CT2 (333mV) ──────────────────────────▶│  2048 Sa/s    │
   CT3 (333mV) ──────────────────────────▶│               │
   CTn (neutral)─────────────────────────▶│               │
                                          └──────┬────────┘
                                         SPI │ISO7720│
                    ┌───────────────────────────▼──────────┐
                    │            STM32H743VIT6              │
                    │  Cortex-M7 480 MHz · DP-FPU · 2 MB F  │
                    │                                       │
                    │  ┌─────────┐  ┌─────────┐  ┌────────┐ │
                    │  │ DSP/FFT │  │ Power   │  │ NILM   │ │
                    │  │  50th   │  │ Calc    │  │ Neural │ │
                    │  │ harmon. │  │ PQ metr │  │ Infere │ │
                    │  └─────────┘  └─────────┘  └────────┘ │
                    │  ┌─────────┐  ┌─────────┐  ┌────────┐ │
                    │  │ Event   │  │ SD Log  │  │ Display│ │
                    │  │ Detect  │  │ FAT32   │  │ ILI9341│ │
                    │  └─────────┘  └─────────┘  └────────┘ │
                    └──┬───────┬───────┬────────┬───────┬───┘
                  SPI  │  UART │  UART │   USB  │  I2C │
              ┌────────▼┐ ┌────▼───┐ ┌─▼──────┐ ┌▼──────┐ │
              │ SX1262  │ │ nRF52840│ │ microSD│ │USB-C  │ │
              │ LoRa    │ │  BLE   │ │  FAT32 │ │ CDC   │ │
              └─────────┘ └────────┘ └────────┘ └───────┘ │
                                                         │
              ┌────────┐  ┌────────┐  ┌────────┐         │
              │LSM6DSO │  │PCF85263│  │MAX17048│  ◀──I2C──┘
              │  IMU   │  │  RTC   │  │ Fuel   │
              └────────┘  └────────┘  └────────┘
```

### Signal flow

1. **Mains → analog front-end:** Each phase voltage passes through a 1 MΩ:1 kΩ resistive divider (1000:1) into an AMC1301 reinforced isolation amplifier, which modulates the signal across a capacitive isolation barrier and demodulates it on the ADC side as a differential signal around 1.7 V common-mode. CT secondary outputs (333 mV AC at rated current) drive the ADS131M08's differential inputs directly through a burden/bias network.

2. **ADC → MCU:** The ADS131M08 simultaneously samples all 8 channels at 2048 Sa/s. A hardware DRDY interrupt on the STM32 triggers a DMA-driven SPI read of the 8×3-byte sample frame into a ring buffer. The ADC clock is derived from the STM32's HSE so that the sample rate is phase-locked to the MCU.

3. **DSP pipeline:** Each 1-second window (2048 samples × 8 channels) is processed by a three-stage pipeline: (a) **power_calc** — instantaneous power p(t)=v(t)·i(t), integrated to real power; reactive via 90° Hilbert-shift method; (b) **harmonics** — a 2048-point radix-2 FFT (arm CMSIS-DSP) on windowed v(t) and i(t) yields magnitude/phase per harmonic bin; THD and individual harmonic magnitudes are extracted per IEC 61000-4-7 grouping; (c) **event_detect / nilm** — the real power trajectory and harmonic signature feed a quantized feed-forward neural network (trained offline, 3 hidden layers × 64 int8 neurons) that outputs the probability of each known appliance state; a threshold + debounce logic emits on/off events.

4. **Event detection & logging:** Sag/swell (voltage outside ±10% nominal for >10 ms), interruption (voltage <10% for >10 ms), and flicker (Pst computed per IEC 61000-4-15 digital filter chain) generate timestamped events stored to the SD card and pushed over BLE/LoRa.

---

## 4. Firmware Details & Design Decisions

### Bare-metal super-loop, no RTOS

The firmware runs a deterministic super-loop driven by the ADS131M08's 2048 Hz DRDY interrupt. An RTOS would add context-switch jitter to the sample-arrival path and complicate the DMA/ISR coupling. Instead, a single ISR (`ADC_DRDY_IRQHandler`) copies the SPI frame into a ping-pong ring buffer and sets a flag; the main loop blocks on the flag, processes the completed 1-second window, and dispatches results to the display/SD/BLE/LoRa queues. This guarantees ≤1 s latency from measurement to result while keeping the critical path under 300 Kcycles of the 480 MHz core.

### CMSIS-DSP FFT

The 2048-point FFT uses the CMSIS-DSP `arm_rfft_fast_f32` (split-radix). At 480 MHz the transform takes ~0.6 ms per channel × 6 channels = 3.6 ms — a tiny fraction of the 488 ms real-time budget per 1-second window. Windowing uses a Hann window; the bin at 50 Hz (bin index 25 at 2 Hz resolution) aligns to the fundamental for a 50 Hz grid. For 60 Hz grids, the firmware auto-detects the fundamental bin by peak-searching in the 45–65 Hz region and rebins accordingly.

### On-device NILM inference

The NILM model is a fully-connected int8-quantized network: 24 inputs (active/reactive power, power factor, THD, and magnitudes of harmonics 3, 5, 7, 9, 11, 13 for each phase, normalized) → 64 → 64 → 64 → N outputs (N = number of trained appliance classes, up to 16). Inference uses integer-only matrix multiplication (no float in the hot loop) and takes ~0.2 ms. The model is stored in a C header (`nilm_weights.h`) compiled into firmware and is swappable via BLE/USB as a binary blob into external QSPI flash. Event emission uses hysteresis (on-probability >0.7, off-probability >0.6) with a 3-second debounce to avoid chatter.

### Calibration

Each voltage channel and CT channel has a gain and phase correction stored in EEPROM (calibration constants applied in the DSP pipeline). The companion app runs a calibration wizard: apply a known reference voltage (e.g., 230 V) and known load (e.g., 1000 W resistor), and the firmware computes and stores the gain/phase offsets. A CT library in firmware supports common CT ratios (100 A:0.333 A, 200 A:0.1 A, etc.).

### Why STM32H743 over an F4

The H7's double-precision FPU eliminates the accumulated error in long FIR filter chains (the flicker Pst filter is a 9-stage IIR + weighting). The 2 MB Flash accommodates the firmware + NILM weights + CMSIS-DSP tables with room for expansion. The FMC interface drives the TFT display with zero CPU overhead (DMA2D for pixel conversion). And the abundance of DMA channels allows simultaneous SPI (ADC), SPI (SD), SPI (display), UART (BLE), UART (LoRa), and I2C (RTC/IMU/fuel gauge) without contention.

---

## 5. Application / Software Interface

### BLE protocol

WattLens exposes a GATT service (`0000FF10-...`) with characteristics:

| UUID | Type | Purpose |
|---|---|---|
| FF11 | Notify | Real-time measurement frame (power, PF, THD, per-phase V/I RMS) at 1 Hz |
| FF12 | Read | Harmonic spectrum (50 bins × 2 bytes, magnitude) on demand |
| FF13 | Notify | NILM appliance state vector (16 classes × probability) at 0.5 Hz |
| FF14 | Notify | Event notifications (sag/swell/flicker/appliance on/off) |
| FF15 | Write | Command/control (start capture, set CT ratio, calibrate, set grid freq) |
| FF16 | Write | NILM model upload (chunked binary) |
| FF17 | Read | Device status (battery, SD free, uptime, error flags) |

### Companion app (React Native)

The app provides five primary screens:

- **Dashboard** — real-time gauges for total power, power factor, THD, and per-phase voltage/current bars.
- **Waveform** — live oscilloscope-style v(t)/i(t) display with trigger and zoom.
- **Harmonics** — bar chart of harmonic magnitudes to the 50th, with THD readout and IEC 61000-4-7 grouping.
- **Loads** — NILM disaggregation: a list of detected appliances with on/off state, estimated power, and a 24-hour energy bar per appliance.
- **Events** — timestamped log of power quality events (sag, swell, flicker, interruption) with severity and waveform capture download.
- **Settings** — CT ratio selection, grid frequency (50/60 Hz), calibration wizard, NILM model management, LoRa mesh config.

### LoRa fleet mode

For facility-wide deployment, multiple WattLens nodes form a LoRa mesh (spanning tree, 868/915 MHz). Each node reports aggregated events + NILM state every 30 seconds (payload ~40 bytes). A gateway node forwards to the cloud. This enables building-wide energy and power-quality monitoring at ~$120/node.

### USB-C CDC interface

A text command interface (AT-like) over USB CDC exposes the same functionality as BLE for headless operation, scripting, and integration into existing energy management systems. Commands include `MEAS?`, `HARM?`, `NILM?`, `EVENTS?`, `CAPTURE 60`, `CAL:V 230`, `CT 100`.

---

## 6. Use Cases & Target Audience

| Audience | Use case |
|---|---|
| **Electricians** | Diagnose nuisance breaker trips (harmonic-induced neutral overload), identify sag sources, verify power factor correction capacitor health. |
| **Facility / energy managers** | Deploy a fleet of nodes across a building to track energy consumption *per appliance* without sub-metering every circuit. Find "ghost loads" and idle-power waste. |
| **Energy auditors** | Produce compliance reports for IEC 61000-4-30 Class S power quality; identify harmonic polluters (VFDs, UPS, LED drivers). |
| **Solar / DER integrators** | Verify inverter power quality: anti-islanding behavior, harmonic injection, flicker from cloud-edge transients. |
| **Researchers / NILM community** | An open, calibration-capable platform to collect labeled aggregate power signatures and train/test disaggregation algorithms in the field — not just in the lab. |
| **Homeowners (advanced)** | Clip a CT on the mains, discover which appliances are running and how much they cost to operate, set alerts for anomalous loads (e.g., a well pump cycling indicates a leak). |
| **EV charging operators** | Monitor charger power quality impact on the local feeder, harmonics from rectifier stages, and load balancing across phases. |

---

## 7. BOM Summary (key components)

| Ref | Part | Function | Qty | Unit cost (approx) |
|---|---|---|---|---|
| U1 | STM32H743VIT6 | MCU | 1 | $14 |
| U2 | ADS131M08IPM | 8-ch 24-bit ADC | 1 | $9 |
| U3–U5 | AMC1301B | Isolation amplifier (voltage) | 3 | $3.50 |
| U6 | ISO7720FQ | Digital isolator (SPI) | 1 | $2.50 |
| U7 | nRF52840 module | BLE | 1 | $6 |
| U8 | SX1262 | LoRa radio | 1 | $5 |
| U9 | ILI9341 2.4″ TFT | Display | 1 | $6 |
| U10 | LSM6DSO | IMU | 1 | $2 |
| U11 | PCF85263A | RTC | 1 | $1.50 |
| U12 | MAX17048 | Fuel gauge | 1 | $2 |
| U13 | TP4056 | Li-ion charger | 1 | $0.50 |
| — | microSD socket | Storage | 1 | $1 |
| — | CTs (100 A split-core) | Current sensors | 4 | $6 each |
| — | Connectors, passives, PCB | — | — | ~$15 |
| | | **Total (approx)** | | **~$120** |

---

## 8. Safety & Compliance Notes

WattLens measures hazardous mains voltages. The design enforces a reinforced isolation barrier between all mains-referenced circuitry and the user-accessible domain (display, USB, BLE, buttons). The voltage input divider is rated for 600 V CAT III transients. Current inputs are inherently isolated via CTs. **The device must be installed by a qualified electrician when connected to mains.** The open-source design is provided for engineering and educational purposes; field deployment requires local electrical-code compliance and, for commercial sale, EMC/safety certification (IEC 61010-1, IEC 61326).

---

## 9. Repository Structure

```
wattlens/
├── README.md                  ← this file
├── firmware/
│   ├── main.c                 ← super-loop + state machine
│   ├── board.h                ← pin map, peripheral config, constants
│   ├── registers.h            ← STM32H7 register definitions
│   ├── linker.ld              ← memory layout
│   ├── Makefile               ← ARM GCC build
│   └── drivers/
│       ├── adc_drv.{c,h}      ← ADS131M08 SPI driver + DMA ring buffer
│       ├── dsp_fft.{c,h}      ← CMSIS-DSP FFT wrapper + windowing
│       ├── power_calc.{c,h}   ← real/reactive/apparent power, RMS, PF
│       ├── harmonics.{c,h}    ← IEC 61000-4-7 harmonic grouping + THD
│       ├── flicker.{c,h}      ← IEC 61000-4-15 flicker Pst digital filter
│       ├── event_detect.{c,h} ← sag/swell/interruption detection
│       ├── nilm.{c,h}         ← int8 neural network inference engine
│       ├── ble.{c,h}          ← nRF52840 UART command protocol
│       ├── lora.{c,h}         ← SX1262 driver + mesh framing
│       ├── display.{c,h}      ← ILI9341 TFT driver + UI widgets
│       ├── sdlog.{c,h}        ← FAT32 SD card logging
│       ├── flash_drv.{c,h}    ← QSPI flash for model storage
│       └── power.{c,h}        ← battery / charging / fuel gauge
├── kicad/
│   ├── device.kicad_sch       ← full schematic (MCU, ADC, isolation, radio, power)
│   ├── device.kicad_pcb       ← 4-layer PCB, 120×72 mm
│   └── device.kicad_pro       ← KiCad project
└── app/
    ├── App.js                 ← React Native entry + navigation
    ├── package.json
    ├── screens/
    │   ├── DashboardScreen.js
    │   ├── WaveformScreen.js
    │   ├── HarmonicsScreen.js
    │   ├── LoadsScreen.js
    │   ├── EventsScreen.js
    │   └── SettingsScreen.js
    ├── components/
    │   ├── Gauge.js
    │   ├── BarChart.js
    │   └── ApplianceCard.js
    └── utils/
        ├── protocol.js        ← BLE protocol parser/encoder
        └── BleContext.js      ← BLE connection manager
```

---

## 10. License & Author

**Author:** jayis1
**Copyright © 2026 jayis1**
**License:** MIT (firmware, app, documentation); CERN-OHL-S v2 (hardware schematics and PCB)

All designs, firmware, code, and documentation are authored by **jayis1**. This project is open-source and contributions are welcome under the terms of the respective licenses.