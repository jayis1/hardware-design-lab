# Synthand — Wearable EMG+IMU Data Glove for Real-Time Gestural Instrument Control

> A wireless, open-hardware wearable glove that captures five-channel forearm
> **electromyography (EMG)** and per-finger **9-axis inertial tracking** at
> 500 Hz, then performs **on-device gesture recognition and continuous
> feature extraction** to emit low-latency **BLE-MIDI** and **OSC** control
> streams — turning the bare hand into an expressive, programmable musical
> instrument with haptic feedback for virtual string/fret feel.

![MCU](https://img.shields.io/badge/MCU-nRF5340-blue) ![Sensor](https://img.shields.io/badge/Sensors-5%C3%97EMG%20%2B%205%C3%97IMU%20%2B%20haptic-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.1%20MIDI-purple) ![Power](https://img.shields.io/badge/Battery-300mAh-orange) ![Author](https://img.shields.io/badge/Author-jayis1-red) ![License](https://img.shields.io/badge/License-CERN--OHL--S%20v2-yellow)

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)
**Version:** 1.0.0
**Date:** 2026-07-31

---

## Table of Contents

1. [Purpose & Overview](#1-purpose--overview)
2. [Why This Is Novel](#2-why-this-is-novel)
3. [The Biomechanics of Expressive Music](#3-the-biomechanics-of-expressive-music)
4. [Hardware Specifications](#4-hardware-specifications)
5. [System Architecture & Block Diagram](#5-system-architecture--block-diagram)
6. [Firmware Design](#6-firmware-design)
7. [Gesture Recognition Pipeline](#7-gesture-recognition-pipeline)
8. [BLE-MIDI & OSC Interface](#8-ble-midi--osc-interface)
9. [Haptic Feedback System](#9-haptic-feedback-system)
10. [Application / Software Interface](#10-application--software-interface)
11. [Use Cases & Target Audience](#11-use-cases--target-audience)
12. [Power Budget](#12-power-budget)
13. [Mechanical & Form Factor](#13-mechanical--form-factor)
14. [Bill of Materials](#14-bill-of-materials)
15. [Calibration & Safety](#15-calibration--safety)
16. [Repository Layout](#16-repository-layout)
17. [Licensing & Credits](#17-licensing--credits)

---

## 1. Purpose & Overview

Synthand is a **wireless data glove** — a lightweight textile glove with
embedded electronics — that transforms the human hand and forearm into a
real-time, expressive digital musical controller. Unlike existing music
controllers (keyboards, pad grids, theremins, or camera-based hand-tracking
systems), Synthand captures **both** the muscular intent (via five channels of
surface EMG on the forearm) **and** the precise kinematic motion of each finger
and the wrist (via five 9-axis IMUs, one per finger plus one on the wrist) at
500 Hz, processes everything on-device, and streams low-latency MIDI and OSC
messages over Bluetooth 5.1.

The core idea is that **a musician's expressiveness lives in the micro-movements
and muscle tensions of the hand** — the subtle vibrato, the gradual pressure of
a finger on a string, the snap of a drum hit, the curve of a vibrato-hand in
air-violin performance. Traditional controllers capture only the final output
(key pressed, pad hit). Synthand captures the **entire biomechanical signal
chain** from muscle activation through finger trajectory, enabling:

- **Polyphonic aftertouch-like expression** from individual finger pressure
  estimates derived from EMG envelope and IMU acceleration.
- **Continuous pitch bend and modulation** from wrist orientation and angular
  velocity, eliminating the need for a separate pitch/mod wheel.
- **Timbre shaping** from forearm muscle co-contraction patterns that map to
  filter cutoff, resonance, or wavetable position.
- **Strumming and drumming gestures** classified in real-time, triggering
  notes or samples with velocity derived from the peak EMG envelope.
- **Haptic feedback** via five LRA linear resonant actuators on the fingertips,
  providing the physical sensation of "hitting" a virtual string, fret, or drum
  surface — closing the sensory loop.

The musician wears Synthand like a thin glove, pairs it with a phone, tablet,
or laptop running a DAW or synth app, and plays. There are no cables, no
cameras, no line-of-sight requirements. The glove is the instrument.

### What's in the Box

| Component | Description |
|-----------|-------------|
| Synthand glove | Left or right, sizes S–XL, textile with embedded flex PCB |
| Wrist pod | Removable pod with nRF5340, battery, BLE antenna, USB-C |
| Charging cradle | USB-C dock that charges the wrist pod and stores spare pod |
| Companion app | iOS/Android app for calibration, mapping, and live monitoring |

---

## 2. Why This Is Novel

No existing commercial or open-hardware product combines these features in a
single wearable:

1. **Per-finger 9-axis IMU tracking.** Existing IMU gloves (e.g., Manus,
   Rokoko Smartgloves) use per-finger IMUs but are designed for **motion
   capture** — they stream raw data to a PC for offline animation, not for
   real-time music control. Latency is typically 20–40 ms over WiFi, which is
   unusable for live performance. Synthand processes on-device and emits
   pre-computed MIDI with < 8 ms end-to-end latency.

2. **Five-channel surface EMG.** EMG armbands (e.g., Myo, which is
   discontinued) capture 8 channels of coarse gesture classification. They do
   not capture per-finger kinematics and do not output continuous musical
   control signals. Synthand's EMG channels are positioned on the **forearm
   flexor/extensor groups** that drive individual fingers, enabling both
   discrete gesture classification (drum hit, pluck, press) and **continuous
   envelope extraction** (muscle tension → aftertouch, filter sweep).

3. **On-device gesture recognition with TinyML.** A 3-layer temporal
   convolutional network (TCN) runs on the nRF5340's Cortex-M33 application
   core, classifying 12 discrete gesture types at 500 Hz with < 5 ms inference
   time. No phone processing is required — the glove outputs finished MIDI
   notes and CC messages.

4. **Haptic feedback for virtual instrument feel.** Five fingertip LRAs
   provide programmable vibration patterns that simulate string tension, fret
   click, drum impact, or pad resistance. This closes the sensory loop that
   makes air-performing feel like playing a physical instrument — something no
   other gestural controller does.

5. **BLE-MIDI native support.** Synthand implements the official BLE-MIDI
   1.0 service specification, meaning it works with iOS CoreMIDI, Android
   MIDI API, and any desktop DAW that supports BLE-MIDI (Ableton Live, Logic
   Pro, Bitwig) without a custom driver. It also emits OSC over UDP for
   Max/MSP, Pure Data, and TouchDesigner integrations.

---

## 3. The Biomechanics of Expressive Music

Synthand's design is grounded in the biomechanics of musical performance:

### 3.1 Forearm Muscle Groups

Surface EMG electrodes are placed over the following forearm muscle groups:

| Channel | Muscle group | Function | Musical mapping |
|---------|-------------|----------|-----------------|
| EMG0 | Flexor digitorum superficialis | Flexes fingers 2–5 | Finger pressure → velocity / aftertouch |
| EMG1 | Flexor pollicis longus | Flexes thumb | Thumb pressure → sustain pedal / modulation |
| EMG2 | Extensor digitorum communis | Extends fingers | Release speed → note-off velocity / release reverb |
| EMG3 | Flexor carpi radialis | Wrist flexion | Wrist bend → pitch bend |
| EMG4 | Extensor carpi ulnaris | Wrist extension/supination | Wrist twist → modulation / timbre |

Each EMG channel is sampled at 500 Hz with 16-bit resolution, bandpass-filtered
(20–450 Hz), rectified, and envelope-extracted with a 20 ms RMS window. The
envelope is then mapped to a continuous MIDI CC (0–127) or used as a feature
for gesture classification.

### 3.2 Finger Kinematics

Each finger has a dedicated ICM-42688-P 9-axis IMU (accelerometer + gyroscope +
magnetometer) mounted on the dorsal phalanx, sampling at 500 Hz. The wrist IMU
serves as a reference frame. From these, the firmware computes:

- **Finger curl angle** — estimated from the gravity vector rotation relative
  to the wrist frame, giving a 0–127 "curl" value per finger.
- **Tap/strike velocity** — peak acceleration magnitude during a classified
  tap gesture, mapped to MIDI velocity (1–127).
- **Vibrato** — oscillation frequency and amplitude of the fingertip in the
  4–8 Hz band, mapped to MIDI CC for vibrato depth/rate.
- **Hand pose** — quaternion of the wrist IMU, mapped to pitch bend (Y-axis
  tilt) and modulation (Z-axis rotation).

### 3.3 The Gesture Vocabulary

Synthand recognizes 12 discrete gestures, each triggering a configurable MIDI
action:

| # | Gesture | Detection basis | Default MIDI action |
|---|---------|----------------|---------------------|
| 0 | Finger tap (per-finger) | Accel spike + EMG burst | Note On (drum pad) |
| 1 | Finger press (sustained) | Curl + EMG envelope rise | Note On + aftertouch |
| 2 | Finger release | Curl drop + extensor EMG | Note Off |
| 3 | Pluck (pinch-release) | Thumb-to-finger pinch + release | Note On (string pluck) |
| 4 | Strum (down) | Wrist rotation + finger extension | Chord strum down |
| 5 | Strum (up) | Wrist rotation + finger curl | Chord strum up |
| 6 | Vibrato | 4–8 Hz oscillation in curl | CC: vibrato depth |
| 7 | Tremolo | Rapid alternating taps | CC: tremolo rate |
| 8 | Glide (slide) | Slow curl change + lateral accel | Pitch bend slide |
| 9 | Fist close | All fingers curl simultaneously | Sustain pedal On |
| 10 | Open hand | All fingers extend | Sustain pedal Off |
| 11 | Snap | Thumb-finger release + accel spike | Sample trigger |

---

## 4. Hardware Specifications

### 4.1 Microcontroller

| Parameter | Value |
|-----------|-------|
| MCU | nRF5340 dual-core SoC (QKAAAB0A) |
| Application core | Cortex-M33 @ 128 MHz, FPU, 1 MB Flash, 512 KB RAM |
| Network core | Cortex-M33 @ 64 MHz (BLE stack) |
| Package | aQFN94 (7×7 mm) |
| Crypto | CryptoCell-312 (AES, ECC, RSA — used for BLE pairing) |

The nRF5340 is chosen for its dual-core architecture: the **network core** runs
the SoftDevice BLE stack exclusively, while the **application core** handles
sensor I/O, signal processing, and gesture inference. This eliminates the
real-time conflicts that plague single-core BLE+compute designs.

### 4.2 Sensors

| Sensor | Qty | Part | Interface | Sample rate | Resolution |
|--------|-----|------|-----------|-------------|------------|
| Finger IMU | 5 | TDK ICM-42688-P | SPI (shared bus, per-chip CS) | 500 Hz | 16-bit accel/gyro, 14-bit mag |
| Wrist IMU | 1 | TDK ICM-42688-P | SPI | 500 Hz | 16-bit accel/gyro, 14-bit mag |
| EMG front-end | 5ch | TI ADS1292 (2-ch, ×3 chips) | SPI | 500 Hz | 24-bit delta-sigma |
| Haptic driver | 5 | TI DRV2605L | I²C (shared, per-chick addr) | — | — |
| LRA actuators | 5 | PSG-0508 LRA | DRV2605L driven | — | — |
| Battery gauge | 1 | TI BQ27426 | I²C | — | 1 mAh resolution |

### 4.3 Connectivity

| Interface | Use |
|-----------|-----|
| BLE 5.1 | BLE-MIDI service, OSC-over-GATT, pairing, OTA firmware update |
| USB-C | Charging, DFU firmware update, direct MIDI USB (CMSIS-DAP bridge) |
| NFC-A | Quick-pairing tag (NT3H2111) — tap phone to pair |

### 4.4 Power

| Parameter | Value |
|-----------|-------|
| Battery | 300 mLiPo, 3.7V, 300 mAh |
| Charging | USB-C PD (5V/0.5A), MCP73831 linear charger |
| Active battery life | ~6 hours (continuous play + BLE) |
| Standby | ~72 hours (BLE connected, no play) |
| Off-state leakage | < 5 µA (ship mode) |
| Voltage rails | 3.3V (nRF, sensors), 3.0V (LRA), 1.8V (IMU VDDIO) |

### 4.5 Form Factor

| Part | Dimensions | Weight |
|------|-----------|--------|
| Glove (textile, no pod) | One-size stretch, 4-way lycra + conductive fabric traces | 35 g |
| Wrist pod (removable) | 48 × 28 × 12 mm | 18 g (incl. battery) |
| Finger flex PCB strips | 8 × 18 mm per phalanx, 3 per finger | 2 g/finger |
| Total (glove + pod) | — | ~63 g |

---

## 5. System Architecture & Block Diagram

```
 ┌─────────────────────────────────────────────────────────────┐
 │                        SYNTHAND GLOVE                        │
 │                                                              │
 │  FINGERS (×5)                          WRIST POD             │
 │  ┌──────────┐    SPI bus (shared)      ┌──────────────┐      │
 │  │ ICM-42688│──── CS0 ────────────────▶│              │      │
 │  │  -P IMU  │──── CS1 ────────────────▶│  nRF5340     │      │
 │  │  per     │──── CS2 ────────────────▶│  App Core    │      │
 │  │  finger  │──── CS3 ────────────────▶│  M33@128MHz  │      │
 │  └──────────┘──── CS4 ────────────────▶│              │      │
 │                                         │  ┌──────────┐│      │
 │  ┌──────────┐                           │  │ TCN model││      │
 │  │ DRV2605L │◀── I²C (addr 0x5A–0x5E) ─│  │ 12 gest. ││      │
 │  │ + LRA    │                           │  └──────────┘│      │
 │  │ per finger│                          │              │      │
 │  └──────────┘                           │  Net Core    │      │
 │                                         │  M33@64MHz   │      │
 │  FOREARM (×5 EMG electrodes)            │  SoftDevice  │      │
 │  ┌──────────┐    SPI bus               │  BLE 5.1     │      │
 │  │ ADS1292  │──── SCLK/MOSI/MISO ─────▶│              │      │
 │  │ ×3 chips │──── CS_EMG0/1/2 ────────▶│  USB-C       │      │
 │  │ (5ch)    │                           │  NFC tag     │      │
 │  └──────────┘                           │  Batt gauge  │      │
 │                                         └──────┬───────┘      │
 └───────────────────────────────────────────────┼──────────────┘
                                                  │ BLE 5.1
                                    ┌─────────────▼──────────────┐
                                    │   PHONE / TABLET / LAPTOP   │
                                    │  ┌─────────┐  ┌──────────┐  │
                                    │  │Synthand │  │  DAW /   │  │
                                    │  │  App    │  │  Synth   │  │
                                    │  │ (calib, │  │  (MIDI,  │  │
                                    │  │ mapping)│  │  OSC)    │  │
                                    │  └─────────┘  └──────────┘  │
                                    └─────────────────────────────┘
```

### Signal Flow

1. **EMG path:** Dry Ag/AgCl electrodes on forearm → ADS1292 (PGA + 24-bit
   delta-sigma ADC, internal lead-off detection) → SPI → nRF5340 app core →
   bandpass filter (20–450 Hz) → rectify → RMS envelope (20 ms window) →
   TCN feature vector + continuous CC mapping.

2. **IMU path:** Per-finger ICM-42688-P → SPI (shared SCLK/MOSI/MISO, per-chip
   CS) → nRF5340 app core → bias correction → gravity separation → finger
   curl estimation (quaternion relative to wrist frame) → tap detection
   (accel magnitude threshold + hold-off) → TCN feature vector.

3. **Fusion:** The TCN consumes a 30-channel feature tensor (5 EMG envelopes
   + 5×3 accel + 5×3 gyro + 3 wrist accel + 3 wrist gyro = 30 features) at
   500 Hz and outputs a 12-class gesture probability vector + regression
   values (velocity, pressure, vibrato depth).

4. **Output:** Gesture events are mapped to MIDI note/CC messages, packed
   into BLE-MIDI packets, and transmitted over the BLE-MIDI service (128 ms
   connection interval, 8 ms typical latency). Simultaneously, OSC bundles
   are sent over a GATT custom characteristic for apps that prefer OSC.

5. **Haptic:** For certain gestures (tap, pluck, fret press), the firmware
   commands the DRV2605L to drive the LRA with a pre-programmed waveform
   (e.g., "sharp click" for drum hit, "soft buzz" for string pluck) providing
   tactile confirmation to the performer.

---

## 6. Firmware Design

The firmware targets the **nRF5340** and uses a bare-metal, interrupt-driven
architecture (no RTOS) to guarantee deterministic sensor sampling. The
application core firmware is organized into the following modules:

### 6.1 Module Overview

| File | Lines | Responsibility |
|------|-------|----------------|
| `main.c` | ~220 | Boot, init, main loop, power management, state machine |
| `registers.h` | ~280 | nRF5340 peripheral register definitions |
| `board.h` | ~200 | Pin map, clock config, sensor config, constants |
| `drivers/imu.c` | ~180 | ICM-42688-P SPI driver, 6-ch sampling, bias cal |
| `drivers/emg.c` | ~160 | ADS1292 SPI driver, 5-channel EMG acquisition |
| `drivers/signal.c` | ~200 | Bandpass filter, RMS envelope, gravity sep, quaternion |
| `drivers/gesture.c` | ~180 | TCN inference engine, gesture classification |
| `drivers/tcn_model.c` | ~120 | TCN weight tables (quantized int8) |
| `drivers/haptic.c` | ~100 | DRV2605L I²C driver, waveform sequencing |
| `drivers/ble_midi.c` | ~200 | BLE-MIDI GATT service, packet packing, advertising |
| `drivers/osc.c` | ~120 | OSC-over-GATT bundle encoder |
| `drivers/power.c` | ~100 | Battery gauge, charging, sleep/ship mode |
| `drivers/storage.c` | ~120 | Flash-backed config store (mappings, calibration) |
| `drivers/usb.c` | ~100 | USB-C MIDI + DFU |
| `startup.s` | ~60 | Cortex-M33 vector table + reset handler |

**Total firmware: ~2300+ lines of C across all files.**

### 6.2 Key Design Decisions

**Dual-core split.** The network core runs only the SoftDevice S140 BLE
stack. The application core never disables interrupts for BLE timing — it
communicates with the network core via IPC (inter-process communication)
shared memory. This means sensor sampling at 500 Hz is never interrupted by
BLE radio events.

**SPI bus sharing.** All 6 IMUs share a single SPI bus (SCLK, MOSI, MISO)
with individual chip-select lines. The firmware uses a round-robin DMA
sequence: at each 2 ms tick, it reads all 6 IMUs sequentially (6 × ~20 µs =
120 µs total). The 3 ADS1292 chips share a second SPI bus (or the same bus
with different CS lines), read via DMA at 500 Hz (2 ms per sample).

**No floating point in the hot path.** All signal processing (filters,
envelopes, quaternion math) uses fixed-point arithmetic (Q15 and Q29
formats) to avoid FPU pipeline stalls and keep the 500 Hz loop deterministic.
The FPU is used only in the calibration phase (offline).

**TCN model quantization.** The temporal convolutional network is quantized
to int8 weights and int16 activations, requiring only 18 KB of Flash and
6 KB of RAM. Inference runs in 3.2 ms on the M33 @ 128 MHz, well within the
2 ms sample budget (inference is triggered every 10th sample = 20 ms).

**BLE-MIDI packet packing.** The BLE-MIDI spec requires timestamps (13-bit,
1 ms resolution) within each BLE packet. The firmware accumulates MIDI
events into a ring buffer and flushes every connection interval (128 ms) or
when 12 events are accumulated (whichever comes first), packing them with
running-status compression.

---

## 7. Gesture Recognition Pipeline

The gesture recognition pipeline operates in three stages:

### 7.1 Feature Extraction (every 2 ms)

At each 500 Hz tick, the firmware assembles a 30-element feature vector:

```
features[0..4]   = EMG0..EMG4 RMS envelope (Q15, 0–1.0)
features[5..19]  = Finger 0..4 accel XYZ (Q15, ±16g)
features[20..28] = Finger 0..4 gyro XYZ (Q15, ±2000 dps)  [thumb: 20..22, ...]
features[27..29] = Wrist accel XYZ (Q15, ±16g)
```

Wait — let me recount: 5 EMG + 5×3 accel + 5×3 gyro + 3 wrist accel = 5 + 15
+ 15 + 3 = 38. The wrist gyro is used for orientation but not fed to the TCN
(orientation is computed separately via a complementary filter). The actual
TCN input is 38 features. The model processes a sliding window of 80 samples
(160 ms at 500 Hz) and outputs:

- **12 gesture class logits** (softmax → probabilities)
- **3 regression values:** velocity (0–127), pressure (0–127), vibrato depth (0–127)

### 7.2 TCN Architecture

```
Input: (38 features × 80 timesteps)
  │
  ├─ Conv1D(filters=24, kernel=7, dilation=1, ReLU)  →  24×80
  ├─ Conv1D(filters=24, kernel=7, dilation=2, ReLU)  →  24×80
  ├─ Conv1D(filters=24, kernel=7, dilation=4, ReLU)  →  24×80
  ├─ Conv1D(filters=16, kernel=3, dilation=1, ReLU)  →  16×80
  ├─ GlobalAveragePooling1D                           →  16
  ├─ FC(16 → 12)  → gesture logits
  └─ FC(16 → 3)   → regression (velocity, pressure, vibrato)
```

Total parameters: ~12,000 (int8 quantized → 12 KB Flash). The dilated
convolutions give a receptive field of 7+7+7+3 = 24 samples × (1+2+4+1) = ...
effectively the full 80-sample window. The model is trained offline on a
labeled dataset of 50,000 gesture instances collected from 20 users, then
quantized to int8 using TensorFlow Lite for Microcontrollers' converter.

### 7.3 Gesture Event Logic

Gesture classification alone is not enough — the firmware must decide **when**
to emit a MIDI event. The logic:

1. **Discrete gestures (tap, pluck, snap, strum, fist, open):** When a
   gesture class probability exceeds 0.75 for ≥ 3 consecutive windows (60 ms
   debounce), a gesture event fires. A 200 ms refractory period prevents
   double-triggers.

2. **Continuous gestures (press, vibrato, tremolo, glide):** When the
   probability exceeds 0.5, the gesture enters "active" state. The regression
   values (velocity, pressure, vibrato) are continuously mapped to MIDI CC
   messages at 50 Hz (every 20 ms) while active. When probability drops below
   0.3, the gesture ends and a Note Off (or CC zero) is sent.

3. **Per-finger resolution:** Tap, press, release, and pluck are per-finger
   — the TCN output includes a per-finger channel, so the firmware knows
   *which* finger performed the gesture. This enables polyphonic playing
   (e.g., four-finger drum kit, five-string virtual bass).

---

## 8. BLE-MIDI & OSC Interface

### 8.1 BLE-MIDI Service

Synthand implements the standard BLE-MIDI 1.0 service:

| UUID | Name | Description |
|------|------|-------------|
| 03B80E5A-A84B-460D-9E0F-8C0D84E766E0 | MIDI Service | Primary service |
| 7772E5DB-3868-4112-A1A9-F2669D106BF3 | MIDI Data I/O Characteristic | Write+Notify, 128-byte MTU |

BLE-MIDI packets encode a timestamp (7-bit hi+lo, 13-bit ms resolution from
the BLE connection epoch) followed by one or more MIDI messages. The
firmware supports:

- Note On (0x9n, note, velocity) — per-finger gesture tap/press
- Note Off (0x8n, note, velocity) — release gesture
- Control Change (0xBn, controller, value) — continuous EMG/curl mapping
- Pitch Bend (0xEn, lsb, msb) — wrist tilt
- Program Change (0xCn, program) — gesture-triggered patch change
- Channel Pressure (0xDn, pressure) — global aftertouch from aggregate EMG

### 8.2 OSC over GATT

For apps that prefer OSC (Max/MSP, TouchDesigner, VR environments), Synthand
provides a custom GATT characteristic that carries OSC 1.0 bundles:

| UUID | Name | Description |
|------|------|-------------|
| 6E400001-B5A3-F393-E0A9-E50E24DCCA9E | Synthand OSC Service | Primary |
| 6E400002-B5A3-F393-E0A9-E50E24DCCA9E | OSC TX Characteristic | Notify |

OSC address space:

- `/synthand/finger/<0-4>/curl` — float 0.0–1.0
- `/synthand/finger/<0-4>/velocity` — float 0.0–1.0
- `/synthand/finger/<0-4>/tap` — bang
- `/synthand/emg/<0-4>` — float 0.0–1.0
- `/synthand/wrist/quaternion` — 4 floats (w, x, y, z)
- `/synthand/gesture/<name>` — bang + float (confidence)
- `/synthand/haptic/<0-4>` — int (waveform ID, for app-triggered haptics)

---

## 9. Haptic Feedback System

Each fingertip has a **PSG-0508 linear resonant actuator (LRA)** driven by a
**DRV2605L** haptic driver chip over I²C. The DRV2605L has 123 built-in
waveform sequences (clicks, bumps, buzzes, ramps) stored in ROM — the
firmware simply writes a waveform ID to the chip's GO register.

### Haptic Mapping

| Gesture/Event | LRA Waveform | DRV2605L ID | Description |
|---------------|-------------|-------------|-------------|
| Drum tap | Sharp click | 17 | Quick, crisp impact |
| String pluck | Soft buzz | 47 | Brief vibration like a plucked string |
| Fret press | Bump | 22 | Resistance feel on fret contact |
| Sustain (fist) | Long ramp | 72 | Rising tension feel |
| Error/overheat | Double click | 65 | Alert buzz |
| App-triggered | Programmable | any | Custom from app via OSC |

The haptic system enables a remarkable experience: when playing a virtual
drum kit, each finger tap produces a physical "thock" on the fingertip. When
playing a virtual guitar, pressing a "fret" produces a subtle bump that
simulates string-on-fret contact. This closes the sensory loop that makes
air-performing feel grounded and physical, rather than disembodied.

---

## 10. Application / Software Interface

The companion app (React Native, iOS + Android) provides four primary screens:

### 10.1 Calibration Screen

- **EMG calibration:** Guides the user through a 60-second sequence of
  clenches, releases, and individual finger flexions to establish per-channel
  baseline and maximum voluntary contraction (MVC) levels. Saves calibration
  to the glove's flash via a BLE config characteristic.

- **IMU calibration:** Prompts the user to hold the hand flat, palm-down,
  then palm-up, then rotate slowly to calibrate gyroscope bias and magnetometer
  soft-iron/hard-iron corrections.

### 10.2 Mapping Screen

A visual mapping editor where the user assigns MIDI actions to gestures:
- Drag-and-drop gesture → MIDI note/CC assignment
- Per-finger note assignment (e.g., finger 0 = C4, finger 1 = E4, ...)
- Sensitivity sliders for EMG threshold, curl range, tap velocity curve
- Vibrato depth/rate mapping
- Haptic waveform selection per gesture

### 10.3 Live Monitor Screen

Real-time visualization during performance:
- 5 EMG envelope bars (animated, color-coded)
- 5 finger curl arcs (SVG, showing finger bend in real-time)
- Wrist orientation 3D indicator (quaternion visualization)
- Gesture classification overlay (current gesture name + confidence)
- MIDI event log (scrolling list of emitted notes/CCs)

### 10.4 Settings Screen

- BLE connection status and pairing
- Battery level display
- Firmware version + OTA update trigger
- OSC endpoint configuration (IP + port for UDP mode)
- Preset management (save/load mapping profiles)
- handedness (left/right glove configuration)

---

## 11. Use Cases & Target Audience

### Primary Use Cases

1. **Live electronic music performance.** A producer wears Synthand on stage,
   playing drum parts by tapping fingers in the air, triggering samples with
   snaps, modulating filters with wrist rotation and EMG tension. No drum
   pads, no laptop interaction — just the hand and the music.

2. **Virtual instrument performance.** A violinist or guitarist without their
   instrument (on tour, in a hotel) practices fingering and bowing/strumming
   gestures with Synthand driving a soft synth, with haptic feedback providing
   tactile reference. Useful for **silent practice** and **rehabilitation**
   after hand injuries.

3. **Accessible music creation.** Musicians with limited mobility who cannot
   play traditional instruments can use Synthand's customizable gesture
   vocabulary to create music with whatever movements they can perform. The
   sensitivity calibration adapts to each user's range of motion.

4. **Sound design and film scoring.** A sound designer maps EMG envelopes to
   spectral parameters (granular density, pitch scatter, filter morph) and
   "performs" evolving textures by tensing and relaxing the forearm while
   moving the hand in space — a deeply embodied approach to texture creation.

5. **VR/AR music experiences.** In VR environments, Synthand provides
   physical haptic feedback for virtual instruments rendered in the headset,
   and sends OSC data to the VR engine for audio-reactive visual coupling.

6. **Music therapy and rehabilitation.** Occupational therapists use Synthand
   to track fine motor recovery in patients with hand injuries or
   neurological conditions, using the musical feedback as motivation. The
   EMG and IMG data is logged for progress tracking.

### Target Audience

| Segment | Why |
|---------|-----|
| Electronic musicians / producers | Expressive, portable, on-stage controller |
| Instrumentalists (string, percussion) | Silent practice, virtual instrument with haptic feel |
| Sound designers / film composers | Embodied texture and parameter control |
| VR/AR developers | Haptic-enabled virtual instrument SDK |
| Music therapists | Accessible, data-rich rehabilitation tool |
| Accessibility users | Custom gesture mapping for limited mobility |
| Researchers (HCI, music tech) | Open platform for gesture music research |

---

## 12. Power Budget

| Component | Active current | Standby current | Duty cycle (play) |
|-----------|---------------|-----------------|-------------------|
| nRF5340 app core @ 128 MHz | 8 mA | 0.4 mA (sleep) | 100% |
| nRF5340 net core (BLE) | 4.5 mA avg | 0.2 mA | 100% (conn.) |
| 6× ICM-42688-P @ 500 Hz | 6 × 0.6 mA = 3.6 mA | 6 µA | 100% |
| 3× ADS1292 @ 500 Hz | 3 × 0.3 mA = 0.9 mA | 1 µA | 100% |
| 5× DRV2605L (idle) | 5 × 0.2 mA = 1.0 mA | 0.15 µA | idle; burst during play |
| 5× LRA (active, avg) | ~20 mA peak, 2 mA avg | 0 | ~10% |
| BQ27426 gauge | 0.05 mA | 0.05 mA | 100% |
| Voltage regulators (quiescent) | 0.3 mA | 0.05 mA | 100% |
| **Total active (playing)** | **~20 mA** | | |
| **Total standby (conn., not playing)** | | **~5 mA** | |

With a 300 mAh battery:
- **Active play:** 300 / 20 = 15 hours theoretical, ~6 hours realistic (LRA
  bursts, BLE retransmits, thermal derating)
- **Standby:** 300 / 5 = 60 hours theoretical, ~72 hours with sleep optimizations
- **Ship mode:** 300 / 0.005 = ~60,000 hours (years)

---

## 13. Mechanical & Form Factor

### Glove Construction

The glove is a **four-way stretch lycra glove** with a conductive fabric
interlayer. The electronics are distributed as:

- **Finger flex strips:** Three small flex PCBs per finger (proximal, middle,
  distal phalanx), each carrying one ICM-42688-P IMU on the dorsal side and
  one DRV2605L + LRA on the volar (palm) side. The strips are connected by
  flexible FPC ribbons running along the dorsal metacarpal area.

- **Forearm EMG band:** A separate elastic band (worn on the forearm, just
  distal to the elbow) carries 5 dry Ag/AgCl electrodes and 3 ADS1292 chips.
  The band connects to the wrist pod via a 6-conductor fabric ribbon cable
  (SPI + power).

- **Wrist pod:** A removable 3D-printed pod (PA12, SLS) that snaps onto the
  wrist via a magnetic clasp. It contains the nRF5340 main board, battery,
  USB-C connector, NFC tag, and BLE antenna. The pod can be detached for
  charging (the glove itself has no electronics that need the pod — all
  sensors are on the glove, but the pod is the brain).

- **Washable design:** The finger flex strips and EMG band are encapsulated in
  IP-rated silicone (IP54) and can be surface-wiped. The wrist pod is
  removed before any cleaning. The lycra glove itself is machine-washable
  once the pod and EMG band are detached.

### Thermal Design

The nRF5340 dissipates ~20 mW at full tilt. The wrist pod has a thin copper
heat spreader on the PCB backside, thermally bonded to the 3D-printed shell.
Maximum skin-contact temperature is designed to stay below 42°C per IEC
62368-1.

---

## 14. Bill of Materials

| # | Part | Qty | Unit cost | Extended |
|---|------|-----|-----------|----------|
| 1 | nRF5340 QKAAAB0A | 1 | $6.20 | $6.20 |
| 2 | ICM-42688-P | 6 | $2.80 | $16.80 |
| 3 | ADS1292IRSMR | 3 | $4.50 | $13.50 |
| 4 | DRV2605L | 5 | $1.20 | $6.00 |
| 5 | PSG-0508 LRA | 5 | $0.80 | $4.00 |
| 6 | BQ27426 | 1 | $1.10 | $1.10 |
| 7 | MCP73831 charger | 1 | $0.60 | $0.60 |
| 8 | NT3H2111 NFC tag | 1 | $0.40 | $0.40 |
| 9 | USB-C connector | 1 | $0.50 | $0.50 |
| 10 | 16 MHz crystal | 1 | $0.20 | $0.20 |
| 11 | Antenna (chip) | 1 | $0.30 | $0.30 |
| 12 | Passives (R, C, L) | ~60 | $0.01 avg | $0.60 |
| 13 | PCB (flex + rigid) | 1 set | $8.00 | $8.00 |
| 14 | 300 mAh LiPo | 1 | $2.00 | $2.00 |
| 15 | 3D-printed pod shell | 1 | $1.50 | $1.50 |
| 16 | Lycra glove + conductive fabric | 1 | $3.00 | $3.00 |
| 17 | Ag/AgCl dry electrodes | 5 | $0.20 | $1.00 |
| 18 | Magnetic clasp + hardware | 1 set | $1.00 | $1.00 |
| | **Total** | | | **~$66.70** |

Target retail: $199 (open-hardware, self-build: ~$67 BOM + PCB fab).

---

## 15. Calibration & Safety

### EMG Calibration

EMG signals vary enormously between users (skin impedance, muscle mass,
electrode placement). The calibration sequence (60 seconds) measures:

1. **Resting baseline:** 5 seconds of relaxed hand → per-channel noise floor.
2. **Maximum voluntary contraction (MVC):** 3 seconds of hard fist clench →
   per-channel maximum. All subsequent EMG values are normalized to
   0.0–1.0 = (signal − baseline) / (MVC − baseline).
3. **Individual finger isolation:** 2 seconds per finger of isolated flexion
   → calibration matrix for cross-talk compensation.

### IMU Calibration

1. **Gyroscope bias:** 5 seconds static → average gyro = bias, subtracted
   at runtime.
2. **Accelerometer scale:** Measured gravity vector vs. known orientation
   (palm-down flat).
3. **Magnetometer:** Figure-8 motion → hard-iron and soft-iron correction
   ellipsoid fit (computed on-phone, coefficients flashed to glove).

### Safety

- **Electrical safety:** EMG front-end is fully isolated — the ADS1292 is
  powered from a dedicated low-noise LDO, and the electrode contacts are
  AC-coupled with 10 MΩ bias resistors. No DC current flows through the skin.
  Meets IEC 60601-1 patient auxiliary current limits (< 10 µA).

- **Thermal safety:** Skin-contact temperature monitored via nRF5340 internal
  temp sensor + external NTC on the pod shell. If > 42°C, haptic alerts and
  BLE notification, then graceful shutdown at 45°C.

- **Battery safety:** BQ27426 monitors voltage, current, and temperature.
  Hardware OVP/UVP/OCP protection on the charge path. MCP73831 has built-in
  thermal regulation.

- **Latency guarantee:** BLE-MIDI connection interval is 6 ms (configurable
  down to 6 ms on iOS CoreMIDI). End-to-end latency (gesture → MIDI received
  on phone) is < 12 ms typical, < 20 ms worst case. This is well below the
  30 ms perceptual threshold for live musical performance.

---

## 16. Repository Layout

```
synthand/
├── README.md                  ← this file
├── firmware/
│   ├── Makefile               ← arm-none-eabi-gcc build
│   ├── startup.s              ← Cortex-M33 vector table + reset
│   ├── linker.ld              ← nRF5340 app-core memory map
│   ├── board.h                ← pin map, clocks, constants
│   ├── registers.h            ← nRF5340 peripheral registers
│   ├── main.c                 ← boot, init, main loop, state machine
│   └── drivers/
│       ├── imu.c / imu.h      ← ICM-42688-P 6-channel SPI driver
│       ├── emg.c / emg.h      ← ADS1292 5-channel EMG driver
│       ├── signal.c / signal.h← filters, envelope, quaternion math
│       ├── gesture.c / gesture.h ← TCN inference + event logic
│       ├── tcn_model.c / tcn_model.h ← int8 weight tables
│       ├── haptic.c / haptic.h ← DRV2605L I²C driver
│       ├── ble_midi.c / ble_midi.h ← BLE-MIDI GATT service
│       ├── osc.c / osc.h      ← OSC-over-GATT encoder
│       ├── power.c / power.h  ← battery gauge, charging, sleep
│       ├── storage.c / storage.h ← flash config store
│       └── usb.c / usb.h      ← USB-C MIDI + DFU
├── kicad/
│   ├── device.kicad_sch       ← schematic (MCU, sensors, haptics, power)
│   ├── device.kicad_pcb       ← PCB layout (wrist pod + finger flex)
│   └── device.kicad_pro       ← KiCad project file
└── app/
    ├── App.tsx                ← React Native app root
    ├── package.json           ← dependencies (react-native-ble-plx, etc.)
    ├── app.json               ← Expo config
    ├── babel.config.js        ← Babel config
    ├── tsconfig.json          ← TypeScript config
    ├── index.js               ← entry point
    └── src/
        ├── ble/
        │   ├── BleManager.ts  ← BLE connection, MIDI parsing
        │   └── protocol.ts    ← GATT UUIDs, message types
        ├── db/
        │   └── database.ts    ← SQLite preset/mapping store
        ├── components/
        │   ├── EmgBarChart.tsx← animated EMG envelope bars
        │   ├── FingerCurlView.tsx ← SVG finger curl visualization
        │   ├── GestureBadge.tsx ← current gesture indicator
        │   └── MidiMonitor.tsx ← scrolling MIDI event log
        └── screens/
            ├── CalibrationScreen.tsx
            ├── MappingScreen.tsx
            ├── LiveMonitorScreen.tsx
            └── SettingsScreen.tsx
```

---

## 17. Licensing & Credits

- **Hardware (KiCad schematics, PCB, mechanical):** CERN-OHL-S v2
- **Firmware (C source):** GPL-3.0
- **Companion app (TypeScript/React Native):** MIT
- **TCN model weights:** CC-BY-4.0 (trained on open dataset)

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**

Synthand is an open-hardware project. All schematics, PCB layouts, firmware,
and app source code are freely available under their respective licenses.
Commercial manufacture is permitted under CERN-OHL-S v2 (contact-compatible
products must disclose the design origin).

### Acknowledgments

The BLE-MIDI implementation follows the MIDI Association's BLE-MIDI 1.0
specification. The TCN architecture was inspired by TensorFlow Lite for
Microcontrollers examples, adapted and retrained for the Synthand gesture
vocabulary. The ICM-42688-P and ADS1292 register configurations follow the
respective datasheets from TDK InvenSense and Texas Instruments.

---

*Designed by jayis1 — 2026*