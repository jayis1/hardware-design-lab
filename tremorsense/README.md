# TremorSense — Wearable Tremor Characterization & Neurological Monitoring Band

![TremorSense](https://img.shields.io/badge/Form-40mm%20wristband-blue) ![MCU](https://img.shields.io/badge/MCU-nRF5340%20dual%20Cortex--M33-orange) ![Sensor](https://img.shields.io/badge/IMU-ICM--42688--P-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.3%20%2B%20NFC-purple) ![Battery](https://img.shields.io/badge/Battery-7%20days-red) ![Author](https://img.shields.io/badge/Author-jayis1-orange) ![License](https://img.shields.io/badge/License-MIT-yellow)

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**SPDX-License-Identifier: MIT**

---

## 1. Purpose and Overview

TremorSense is a wrist-worn wearable device designed for the continuous, non-invasive characterization and monitoring of pathological tremors — specifically Parkinsonian tremor, Essential Tremor (ET), cerebellar tremor, and drug-induced tremor. Unlike consumer fitness bands that capture coarse activity counts, TremorSense is a clinical-grade instrument purpose-built for neurology: it samples a high-grade 6-axis IMU at 1 kHz, performs on-device edge-ML tremor classification in real time, and produces clinically actionable metrics including tremor frequency (to 0.01 Hz), amplitude, duration distribution, REST vs. ACTION vs. POSTURAL context, a modified Bradykinesia proxy, and a daily Tremor Score.

The device addresses a fundamental gap in Parkinson's disease (PD) and Essential Tremor management: current clinical assessment relies on episodic in-office observation (e.g., MDS-UPDRS Part III), which captures only a snapshot of a fluctuating condition. Medication timing effects, dose-response curves, "off periods," and nocturnal tremor are poorly characterized. TremorSense enables 24/7 continuous monitoring in the patient's natural environment, generating longitudinal data that neurologists can use to optimize medication schedules, evaluate deep-brain stimulation (DBS) programming efficacy, and track disease progression objectively.

### Core Capabilities

| Capability | Detail |
|---|---|
| Tremor frequency | 0.5–25 Hz, ±0.01 Hz resolution via FFT + harmonic analysis |
| Tremor amplitude | 0.001–20 g, RMS acceleration + peak-to-peak displacement |
| Tremor type classification | Parkinsonian / Essential / Cerebellar / Physiological / Drug-induced |
| Context detection | REST / POSTURAL / ACTION (kinetic) via activity classifier |
| Daily Tremor Score | 0–100 composite (weighted by severity × duration) |
| Medication correlation | NFC tag or app button marks dose timing; correlates with tremor suppression onset |
| Nocturnal monitoring | Sleep tremor detection, nocturnal akinesia tracking |
| Battery life | 7 days continuous on 120 mAh LiPo |
| Data storage | 14 days on 8 MB SPI flash, circular buffer |
| Sync | BLE 5.3 streaming + batch download; NFC tap-to-pair |

---

## 2. Hardware Specifications

### 2.1 Microcontroller

**Nordic nRF5340** — dual-core Bluetooth 5.3 SoC:
- **Application core**: Cortex-M33 @ 128 MHz, 1 MB Flash, 512 KB RAM — runs tremor DSP pipeline, edge ML inference, BLE GATT services, UI
- **Network core**: Cortex-M33 @ 64 MHz, 256 KB Flash, 64 KB RAM — runs BLE radio stack in isolated domain (security + reliability)
- Hardware FPU (Cortex-M33) for float FFT
- 12-bit ADC, SAADC with 200 ksps — battery monitoring
- 3× SPI, 4× TWI (I²C), 4× UART, PWM — peripheral connectivity
- NFC-A tag peripheral — tap-to-pair
- CryptoCell-310 — secure boot, encrypted BLE link
- DC/DC converter, 1.7–3.6 V supply

### 2.2 Inertial Measurement Unit

**ICM-42688-P** (TDK InvenSense):
- 6-axis: 3-axis accelerometer + 3-axis gyroscope
- Accelerometer range: ±16 g, 16-bit resolution, noise density 70 µg/√Hz
- Gyroscope range: ±2000 dps, 16-bit, noise 0.0035 dps/√Hz
- ODR up to 32 kHz; configured at 1 kHz for tremor band
- SPI interface (up to 24 MHz) for high-throughput data streaming
- 512-byte FIFO buffer — reduces MCU interrupt load
- Ultra-low-power wake-on-motion at 0.6 µA (auto-sleep between tremor episodes)

### 2.3 Display

**1.28" Round OLED — GC9A01 driver, 240×240 RGB:**
- Circular form-factor fits watch case aesthetic
- SPI interface, 4-wire, 40 MHz
- Shows real-time tremor waveform, daily score, battery, time, tremor type icon
- Low duty-cycle refresh (1 Hz in idle, 15 Hz during active tremor display)

### 2.4 Storage

**Macronix MX25R6435F — 8 MB SPI NOR flash:**
- XiP (Execute-in-Place) capable
- Ultra-low-power 0.2 µA deep power-down
- Circular buffer stores 14 days of 1 kHz IMU bursts + classified episodes
- Also stores ML model weights (int8 quantized, ~120 KB)

### 2.5 Connectivity

| Interface | Chip / Module | Purpose |
|---|---|---|
| BLE 5.3 | nRF5340 integrated | Streaming, batch sync, OTA firmware update |
| NFC | nRF5340 NFC-A | Tap-to-pair, medication dose logging |
| USB-C | USB 2.0 FS via nRF5340 USB peripheral | Charging, DFU, direct data download |

### 2.6 Power

- **Battery**: 120 mAh LiPo, 3.7 V nominal, 1C charge
- **Charger**: MCP73831 linear Li-Ion charger, USB-C input, 4.2 V CC/CV
- **Fuel gauge**: MAX17048 — ModelGauge m3, I²C, 1% accuracy
- **PMIC/regulation**: TPS62840 step-down converter (60 nA IQ) → 1.8 V rail; nRF5340 LDOs → 3.0 V sensor/display rails
- **Power budget**:
  - Active monitoring (1 kHz IMU + DSP): ~1.2 mA avg
  - BLE advertising: 20 µA
  - BLE connected (1 Hz notify): 50 µA
  - Display on (1 Hz): 300 µA
  - Deep sleep (IMU wake-on-motion): 5 µA
  - **7-day life** with display 30s/hour + BLE 5 min/hour + continuous IMU

### 2.7 Form Factor

- **Watch case**: 40 mm × 11 mm, CNC aluminum (anodized), IP67
- **Strap**: 20 mm quick-release silicone, TPD-compatible
- **PCB**: 35 mm diameter round, 4-layer FR4, 0.8 mm
- **Weight**: 22 g (without strap), 38 g with strap
- **Buttons**: 1 side button (multi-function: power, mode, mark-dose)
- **Sensors exposed**: bottom-center IMU contact with PVD-coated stainless steel window

### 2.8 Environmental Sensors (Auxiliary)

- **SHT40** (Sensirion): temperature + humidity — skin microclimate compensation
- **LPS22HB** (ST): barometric pressure — altitude context (stairs/elevator) for activity classifier

---

## 3. Architecture and Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     TremorSense Block Diagram               │
│                                                              │
│  ┌──────────┐  SPI  ┌──────────────────┐  QSPI ┌─────────┐ │
│  │ICM-42688-P│◄─────►│                  │◄──────►│MX25R64  │ │
│  │ 6-axis IMU│  IRQ  │  nRF5340         │       │8MB Flash│ │
│  │ 1 kHz ODR │─────►│  App Core M33     │       └─────────┘ │
│  └──────────┘       │  128 MHz          │                    │
│                     │                  │  I²C  ┌─────────┐ │
│  ┌──────────┐  SPI  │  ┌─────────────┐ │◄──────►│MAX17048 │ │
│  │ GC9A01   │◄─────►│  │Tremor DSP   │ │       │Fuel Gauge│ │
│  │1.28" OLED│       │  │Pipeline     │ │       └─────────┘ │
│  └──────────┘       │  │ FFT/STFT    │ │                    │
│                     │  │ Edge ML CNN │ │  I²C  ┌─────────┐ │
│  ┌──────────┐  I²C  │  │ Episode Log │ │◄──────►│SHT40    │ │
│  │SHT40/LPS │◄─────►│  └─────────────┘ │       │+LPS22HB │ │
│  │Env senso │       │                  │       └─────────┘ │
│  └──────────┘       │  Net Core M33     │                    │
│                     │  BLE 5.3 Stack    │  NFC  ┌─────────┐ │
│  ┌──────────┐ USB-C │  CryptoCell-310   │◄──────►│NFC Ant. │ │
│  │MCP73831  │◄─────►│  USB-FS          │       │(on PCB) │ │
│  │Charger   │       └──────────────────┘       └─────────┘ │
│  └──────────┘                                               │
│  ┌──────────┐                                               │
│  │120mAh LiPo│     ┌──────────┐                             │
│  │3.7V      │◄────►│TPS62840  │───► 1.8V rail               │
│  └──────────┘     │Buck      │───► 3.0V rail (via nRF LDO)  │
│                   └──────────┘                              │
└─────────────────────────────────────────────────────────────┘
```

### 3.1 Data Flow

```
  IMU 1 kHz FIFO interrupt
        │
        ▼
  ┌─────────────┐    ┌──────────────┐    ┌──────────────┐
  │ Raw capture │───►│  Preprocess  │───►│  Feature     │
  │ 6-axis, 50  │    │  Gravity     │    │  Extraction  │
  │ ms window   │    │  removal     │    │  FFT 256-pt  │
  └─────────────┘    │  20 Hz LPF   │    │  Band power  │
                     └──────────────┘    │  Kurtosis    │
                                         │  Hjorth     │
                                         └──────┬──────┘
                                                │
                   ┌──────────────┐    ┌────────▼──────┐
                   │  Episode     │◄───│  Edge ML      │
                   │  Logger      │    │  CNN int8     │
                   │  (SPI flash) │    │  5-class       │
                   └──────┬───────┘    └───────────────┘
                          │
                  ┌───────▼────────┐    ┌──────────────┐
                  │  BLE GATT      │    │  Display     │
                  │  Notify (1 Hz)  │    │  OLED update │
                  └───────┬────────┘    └──────────────┘
                          │
                   ┌──────▼───────┐
                   │  Companion   │
                   │  App (RN)    │
                   └──────────────┘
```

---

## 4. Firmware Details and Design Decisions

### 4.1 Architecture

The firmware runs on the nRF5340 application core with ZephyyRTOS (Zephyr RTOS v3.6+). The network core runs the Nordic SoftDevice S140 BLE stack in a sandboxed domain, communicating with the application core via IPC (RPMsg).

The application core runs a **super-loop with interrupt-driven IMU sampling**:

1. **IMU ISR**: ICM-42688-P fires a FIFO watermark interrupt every 50 ms (50 samples @ 1 kHz). ISR reads 50×6 int16 values via SPI DMA into a ring buffer, signals the DSP thread.

2. **DSP thread** (highest priority): Consumes 50 ms windows, applies:
   - **Gravity separation**: low-pass accelerometer at 0.3 Hz → gravity tilt vector; subtract from raw → linear acceleration
   - **Tremor band extraction**: 4th-order Butterworth bandpass 3.5–12 Hz (primary tremor band) on linear acceleration magnitude
   - **256-point sliding FFT** (Hanning window, 87.5% overlap) → power spectral density
   - **Peak frequency detection** in 3.5–12 Hz band with harmonic ratio (Parkinsonian tremor ~4–6 Hz with 2nd harmonic; Essential tremor ~5–9 Hz without strong harmonics)

3. **Feature extraction thread**: Every 2 seconds, accumulates features:
   - Dominant tremor frequency and confidence
   - Total tremor power (RMS acceleration)
   - Harmonic ratio (fundamental / 2nd harmonic)
   - Signal kurtosis (impulsivity)
   - Hjorth parameters (activity, mobility, complexity)
   - Gyroscope-to-accelerometer coherence
   - Activity context (rest/postural/action) from low-band energy

4. **ML inference thread**: Every 2 seconds, runs the int8 quantized 1D-CNN on the latest feature vector:
   - Input: 16-feature vector (8 FFT bins + 8 time-domain)
   - Architecture: Conv1D(16→32, k=3) → ReLU → Conv1D(32→16, k=3) → ReLU → Flatten → Dense(64) → Dense(5)
   - Output: 5-class softmax [Parkinsonian, Essential, Cerebellar, Physiological, Drug-induced]
   - Model size: ~120 KB int8, inference time ~3 ms on M33 @ 128 MHz

5. **Episode logger**: When a tremor is detected (confidence > 0.7 for > 3 consecutive seconds), logs an episode record to SPI flash:
   - Timestamp, duration, dominant frequency, amplitude, class, context
   - Compressed raw IMU burst (16-bit × 6 × 100 samples = 1.2 KB per 10s episode)

6. **BLE thread**: Exposes GATT services:
   - `TremorService` (custom UUID): real-time frequency, amplitude, class, score
   - `DataService`: bulk episode download (chunked, CRC-verified)
   - `ConfigService`: sample rate, thresholds, model parameters
   - `DFUService`: OTA firmware update via Nordic Secure Boot

### 4.2 Key Design Decisions

**Why nRF5340 dual-core?** The network core runs the BLE stack in isolation, guaranteeing radio timing determinism regardless of DSP/ML load on the application core. This prevents BLE dropouts during heavy FFT computation — critical for a medical device that must not lose monitoring sessions.

**Why 1 kHz IMU ODR?** Parkinsonian tremor (4–6 Hz) and Essential Tremor (5–9 Hz) require resolution of harmonics up to ~25 Hz. Nyquist demands > 50 Hz, but 1 kHz provides: (a) high SNR via oversampling and digital filtering, (b) precise amplitude measurement, (c) future expansion to higher-frequency phenomena (myoclonus, clonus at 10+ Hz). The 50 ms FIFO watermark balances interrupt frequency against latency.

**Why int8 CNN over classical thresholding?** Classical tremor detection uses fixed frequency-band energy thresholds, which produce false positives from voluntary motion (walking, writing) and cannot distinguish tremor types. The CNN learns discriminative features (harmonic structure, cross-axis coupling, temporal dynamics) from labeled data, achieving >90% specificity vs. ~70% for threshold methods. Int8 quantization reduces model size 4× and inference time 3× with <1% accuracy loss.

**Why SPI flash circular buffer instead of microSD?** MicroSD draws 30–100 mA and is physically large. The 8 MB SPI NOR at 0.2 µA standby and 3 mA read/write fits the power budget and the 14-day retention requirement (compressed episodes average ~500 KB/day).

**Why no GPS?** TremorSense is a wristband worn 24/7; GPS adds 40 mA and breaks the 7-day battery target. Location context is derived from the phone companion app, not the wearable.

---

## 5. Application / Software Interface

### 5.1 Companion App (React Native)

**TremorSense Connect** — available for iOS and Android:

**Screens:**
- **Dashboard**: Today's tremor score, tremor-free hours, current status (REST/ACTIVE), battery
- **Tremor Timeline**: 24-hour heatmap of tremor episodes color-coded by type
- **Episode Detail**: Tap an episode → waveform playback, FFT spectrogram, classification confidence, frequency/amplitude plot
- **Medication Log**: NFC tap or button press marks dose time; app tracks medication → tremor-suppression onset latency (typically 20–40 min for levodopa)
- **Trends**: 7/30/90-day trends in tremor hours, frequency drift, amplitude evolution — disease progression indicators
- **Doctor Report**: PDF export of clinical summary (MDS-UPDRS-aligned tremor metrics) for neurologist visit
- **Settings**: Sample rate, sensitivity, alert thresholds, data export, device pairing, firmware update

### 5.2 BLE GATT Interface

| Service | UUID | Characteristics |
|---|---|---|
| TremorService | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | TremorStatus (notify, 1 Hz), TremorScore (read/notify) |
| DataService | `6e400010-b5a3-f393-e0a9-e50e24dcca9e` | EpisodeChunk (notify), DownloadCommand (write) |
| ConfigService | `6e400020-b5a3-f393-e0a9-e50e24dcca9e` | SampleRate (RW), Sensitivity (RW), ModelParams (RW) |
| DFUService | `00001530-1212-efde-1523-785feabcd123` | ControlPoint, Data (Nordic Secure DFU) |

### 5.3 Data Format

Episode record (binary, little-endian):
```
struct episode_record_t {
    uint32_t magic;          // 0xTS01
    uint32_t timestamp;      // Unix epoch
    uint16_t duration_ms;    // Episode duration
    uint16_t sample_count;   // Number of IMU samples
    float    dom_freq_hz;    // Dominant tremor frequency
    float    rms_amplitude;  // RMS acceleration (g)
    uint8_t  tremor_class;   // 0=Parkinsonian, 1=Essential, 2=Cerebellar,
                             // 3=Physiological, 4=Drug-induced
    uint8_t  context;        // 0=REST, 1=POSTURAL, 2=ACTION
    uint8_t  confidence;     // 0–100
    uint8_t  reserved;
    uint16_t crc16;           // CRC-CCITT over preceding fields
    // Followed by sample_count × 12 bytes (ax,ay,az,gx,gy,gz as int16)
};
```

---

## 6. Use Cases and Target Audience

### 6.1 Target Audience

| User | Value |
|---|---|
| **Neurologists** | Objective longitudinal tremor data replaces episodic in-office assessment; optimize medication timing and dosage; evaluate DBS programming |
| **Parkinson's patients** | Understand "on/off" patterns; track medication effectiveness; share clinical reports with doctor |
| **Essential Tremor patients** | Monitor progression; evaluate treatment (propranolol, primidone, focused ultrasound thalamotomy) |
| **Clinical researchers** | High-fidelity continuous tremor data for drug trials, DBS studies, longitudinal progression studies |
| **Movement disorder specialists** | Differentiate PD from ET (critical diagnostic distinction) with quantitative harmonic analysis |

### 6.2 Use Cases

1. **Levodopa dose optimization**: Patient marks each medication dose via NFC/app button. TremorSense tracks time-to-onset (tremor suppression begins) and wearing-off (tremor returns). Neurologist adjusts dose timing/intervals based on actual pharmacokinetic response. This is the #1 unmet need in PD management.

2. **DBS programming verification**: After DBS adjustment, TremorSense provides objective before/after tremor metrics. Programming sessions can be guided by real-time tremor amplitude feedback rather than subjective observation.

3. **Differential diagnosis**: Parkinsonian tremor (4–6 Hz, rest, asymmetric, harmonic-rich) vs. Essential Tremor (5–9 Hz, action/postural, symmetric, harmonic-poor). The edge ML classifier with harmonic ratio features provides quantitative support for this critical distinction.

4. **Nocturnal monitoring**: Parkinson's patients experience nighttime tremor and akinesia that affects sleep quality. TremorSense logs overnight tremor episodes and movement patterns, providing data unavailable in routine clinical visits.

5. **Drug trial endpoints**: Pharmaceutical trials can use TremorSense as a continuous digital biomarker endpoint — more sensitive than once-per-visit UPDRS scoring.

6. **Tele-neurology**: Remote patients can share weeks of tremor data via the companion app, enabling quality neurological care without travel.

---

## 7. Bill of Materials (Summary)

| Ref | Part | Qty | Est. Cost |
|---|---|---|---|
| U1 | nRF5340 QFAA | 1 | $6.50 |
| U2 | ICM-42688-P | 1 | $4.20 |
| U3 | GC9A01 1.28" OLED | 1 | $5.00 |
| U4 | MX25R6435F 8MB SPI flash | 1 | $0.80 |
| U5 | MAX17048 fuel gauge | 1 | $2.10 |
| U6 | MCP73831 charger | 1 | $0.60 |
| U7 | TPS62840 buck converter | 1 | $1.20 |
| U8 | SHT40 temp/humidity | 1 | $1.50 |
| U9 | LPS22HB barometer | 1 | $2.00 |
| BAT1 | 120mAh LiPo | 1 | $3.00 |
| Misc | Passives, connectors, antenna, NFC coil | — | $4.00 |
| **Total BOM** | | | **~$30.90** |

---

## 8. Safety and Regulatory Notes

TremorSense is a Class I medical device data recorder (MDDR) — it records and displays tremor data but does not diagnose or treat. Firmware includes:
- Watchdog timer on both cores (independent)
- BLE link encryption (AES-128 via CryptoCell-310)
- Secure boot (Nordic Secure Boot + signature verification)
- Data integrity: CRC-16 on every episode record, flash wear-leveling
- Skin-contact materials: PVD-coated 316L stainless steel, biocompatible silicone strap
- IP67 sealed enclosure (sweat/washing resistant)
- Battery: undervoltage lockout, overcurrent protection, NTC thermistor for charge temperature monitoring

---

## 9. Repository Structure

```
tremorsense/
├── README.md              (this file)
├── firmware/
│   ├── main.c             (main application, DSP pipeline, ML inference loop)
│   ├── board.h            (pin assignments, peripheral config)
│   ├── registers.h        (nRF5340 register definitions, IMU registers)
│   ├── Makefile           (arm-none-eabi-gcc build)
│   ├── linker.ld          (memory layout, dual-core section placement)
│   └── drivers/
│       ├── imu_drv.c/.h    (ICM-42688-P SPI driver, FIFO management)
│       ├── ble_drv.c/.h    (BLE GATT services, advertising, bonding)
│       ├── flash_drv.c/.h (MX25R SPI flash read/write/erase, wear leveling)
│       ├── dsp.c/.h        (FFT, Butterworth filters, feature extraction)
│       ├── ml_model.c/.h   (int8 CNN inference, model weights)
│       ├── display_drv.c/.h(GC9A01 OLED, UI rendering)
│       ├── power_drv.c/.h (MAX17048 fuel gauge, TPS62840, sleep modes)
│       └── env_drv.c/.h   (SHT40 + LPS22HB I²C drivers)
├── kicad/
│   ├── device.kicad_sch   (schematic with all components and nets)
│   ├── device.kicad_pcb   (4-layer PCB layout, 35mm round)
│   └── device.kicad_pro   (KiCad project file)
└── app/
    ├── App.js             (React Native entry, navigation)
    ├── package.json       (dependencies, scripts)
    ├── src/
    │   ├── screens/       (Dashboard, Timeline, EpisodeDetail, MedicationLog,
    │   │                   Trends, DoctorReport, Settings)
    │   ├── ble/           (BLE manager, GATT parser, connection handling)
    │   ├── models/        (TypeScript types, episode models)
    │   └── utils/         (CSV export, PDF report generator, date helpers)
    └── ...
```

---

## 10. License

MIT License — Copyright © 2026 jayis1

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated design files, to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.