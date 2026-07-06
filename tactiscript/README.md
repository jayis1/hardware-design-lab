# TactiScript — Wearable Fingertip Haptic Display Ring

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

![Device](https://img.shields.io/badge/Form%20Factor-22mm%20ring-blue)
![MCU](https://img.shields.io/badge/MCU-nRF5340%20dual%20Cortex--M33-orange)
![Actuator](https://img.shields.io/badge/Actuator-24--element%20PZT%20bimorph%20array-green)
![Wireless](https://img.shields.io/badge/Wireless-BLE%205.0%20%2F%20Thread-lightgrey)
![Power](https://img.shields.io/badge/Power-Qi%20wireless%20charging-yellow)

---

## 1. Purpose and Overview

**TactiScript** is a novel wearable device — a ring worn on the index finger —
that renders digital information as **real-time tactile sensations on the
fingertip**. It uses a dense 4×6 array of piezoelectric bimorph micro-actuators
that physically protrude and retract against the skin of the fingertip, creating
patterns the user can *feel*: scrolling Braille text, tactile graphics, texture
maps, and directional navigation cues.

### Why this device is needed

Refreshable Braille displays have existed for decades, but they remain bulky,
expensive ($2,000–$15,000), and limited to a single line of 14–40 cells sitting on
a desk. A blind professional reading email on a phone, a student following a
lecture, or a pedestrian navigating a city cannot conveniently use a desk-bound
Braille display. TactiScript reimagines refreshable Braille as a **wearable
ring** — always on the finger, always available, rendering one cell at a time in
continuous scroll. Beyond accessibility, the same hardware serves sighted users
as a **universal haptic channel**: turn-by-turn navigation without looking at a
screen, music rhythm feedback for the hearing-impaired, and rich haptic
notifications for smart-device interaction.

### What makes it novel

| Existing Braille displays | TactiScript |
|---|---|
| Desk-bound, 200–800 g | Wearable ring, ~12 g |
| Electromagnetic pin actuators (loud, slow, power-hungry) | Piezoelectric bimorph micro-actuators (silent, fast, low-power) |
| Single line of 14–40 cells, static | 4×6 array with continuous time-multiplexed scroll |
| $2,000–$15,000 | Target BOM ~$60, open-hardware |
| USB-only, needs a PC | BLE 5.0 to phone, works anywhere |
| Accessibility-only | Accessibility + general haptic I/O for all users |

The key innovation is the **piezoelectric bimorph micro-actuator array**. Each
element is a tiny PZT cantilever (1.5 × 0.6 mm) bonded to a flexible substrate.
Applying a drive voltage (up to 120 V from a boost converter) causes the
cantilever tip to deflect ~120 µm — enough to be clearly felt by fingertip
mechanoreceptors (Pacinian and Meissner corpuscles have thresholds of ~10 µm).
Because PZT actuators are essentially capacitive loads, they draw almost no DC
current — the ring can run for 18+ hours on a 120 mAh battery. The array is
time-multiplexed at 200 Hz refresh to render continuous scrolling text and
texture waveforms.

---

## 2. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | Nordic nRF5340 (dual Cortex-M33 @ 128 MHz, 1 MB Flash, 512 KB RAM) |
| **Actuator Array** | 24-element (4×6) PZT bimorph micro-cantilevers, 120 µm deflection @ 120 V |
| **Actuator Driver** | TI DRV2700 dual-channel HV piezo driver (110 V boost + 2× push-pull) × 12 channels via demux |
| **High-Voltage Supply** | On-board boost converter, 3.7 V → 120 V, 4 W peak, gated per-refresh |
| **IMU** | TDK ICM-42688-P (6-axis accel + gyro, 32 kHz ODR) — gesture: tap, swipe, double-tap |
| **Status Display** | 0.49" OLED, 64×32 monochrome (SSD1316) — battery, mode, BLE state |
| **Haptic Feedback** | Additional LRA (linear resonant actuator) for whole-ring vibration alerts |
| **Wireless** | BLE 5.0 + Thread (nRF5340 built-in radio), NFC-A passive pair |
| **Power** | 120 mAh LiPo (3.7 V), Qi 1.3 wireless charging receiver coil, USB-C fallback |
| **Battery Life** | ~18 h active reading, ~7 days standby |
| **Charging** | Qi wireless charging (5 W) + USB-C PD (5 V/500 mA trickle) |
| **Sensors** | Skin-contact detect (capacitive), temperature (NTC), ambient light (for auto-dim) |
| **Form Factor** | Ring, 22 mm inner diameter, 10 mm wide, 8 mm thick |
| **Weight** | ~12 g |
| **Operating Temp** | -10 °C to +45 °C |
| **Water Resistance** | IP54 (splash-proof, not submersible) |
| **Buttons** | 1 capacitive touch zone on ring exterior (mode select) |
| **PCB** | 6-layer rigid-flex, 0.4 mm total, wrapped around ring circumference |

### Block Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                         TactiScript Ring                      │
│                                                               │
│   ┌─────────┐    SPI     ┌──────────────┐   I2C   ┌────────┐│
│   │ ICM-    │◄──────────►│              │◄───────►│ OLED   ││
│   │ 42688-P │            │  nRF5340     │         │ 64×32  ││
│   └─────────┘            │  (app core)  │         └────────┘│
│                          │              │                    │
│   ┌─────────┐   GPIO/IRQ │  ┌────────┐  │   I2C   ┌────────┐│
│   │ Skin    │◄──────────►│  │ net    │  │◄───────►│ NTC +  ││
│   │ detect  │            │  │ core   │  │         │ ALS    ││
│   └─────────┘            │  └────────┘  │         └────────┘│
│                          └──────┬───────┘                    │
│                                 │ SPI + HV-gate               │
│                          ┌──────▼───────┐                    │
│                          │  DRV2700 ×6  │  120 V drive        │
│                          │  (12 ch)     │────────────────────┼──┐
│                          └──────┬───────┘                    │  │
│                                 │                            │  │
│                    ┌────────────▼─────────────┐              │  │
│                    │  4 × 6 PZT Bimorph Array  │◄─────────────┘
│                    │  (24 micro-cantilevers)   │
│                    │  contacts fingertip skin  │
│                    └──────────────────────────┘
│                                                               │
│   ┌────────┐         ┌──────────┐         ┌──────────────┐   │
│   │ Qi RX  │────────►│ PMIC     │────────►│ 120 mAh LiPo │   │
│   │ coil   │  5 W    │ (nPM1300)│  3.7 V  │              │   │
│   └────────┘         └──────────┘         └──────────────┘   │
│                                                               │
│   ┌────────┐  GPIO    ┌──────────┐                             │
│   │ LRA    │◄─────────│ PWM out  │  (whole-ring vibration)     │
│   └────────┘          └──────────┘                             │
│                                                               │
│   ┌────────┐  NFC-A   ┌──────────┐                             │
│   │ NFC    │◄────────►│  radio   │  BLE 5.0 + Thread           │
│   │ antenna│          └──────────┘                             │
│   └────────┘                                                   │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. Architecture and Design Decisions

### 3.1 MCU Selection: nRF5340

The nRF5340 was chosen for three reasons:

1. **Dual-core architecture** — the *application core* (128 MHz Cortex-M33)
   runs the haptic rendering engine, Braille encoder, and gesture recognizer;
   the *network core* (64 MHz Cortex-M33) runs the BLE/Thread stack
   independently. This separation guarantees jitter-free actuator timing
   (critical for tactile rendering) regardless of radio activity.

2. **Ultra-low power** — 1.7 V operation, 2.6 mA in BLE-connected sleep, which
   enables the 18-hour active battery life target.

3. **Integrated radio + security** — BLE 5.0, Thread, and ARM CryptoCell-310
   for secure pairing. No external radio chip needed.

### 3.2 Actuator Technology: PZT Bimorph Micro-Cantilevers

The core hardware innovation. Each of the 24 tactile "dots" is a PZT-5H
bimorph cantilever: two thin PZT layers bonded with opposite polarity, so
applying voltage causes the tip to deflect upward (toward the skin). Key
parameters:

- **Dimensions**: 1.5 mm × 0.6 mm × 0.3 mm (per cantilever)
- **Tip deflection**: 120 µm at 120 V drive (well above the ~10 µm fingertip
  perception threshold)
- **Resonant frequency**: ~180 Hz (matches optimal Pacinian corpuscle
  sensitivity range)
- **Capacitance**: 2.2 nF per element → 24 × 2.2 nF = 52.8 nF total
- **Energy per refresh**: E = ½CV² = ½ × 52.8 nF × (120 V)² = 380 µJ
  → at 200 Hz refresh = 76 mW actuator power (manageable)

A **time-multiplexing scheme** drives the 24 elements in 4 groups of 6 via
analog demux, refreshing at 200 Hz — fast enough for smooth tactile
perception (fingertip temporal resolution is ~10 ms).

### 3.3 High-Voltage Generation

A boost converter (3.7 V → 120 V, 4 W) generates the drive voltage. It is
**gated per-refresh**: the converter runs only during the ~500 µs drive pulse
window, then shuts off, minimizing EMI and average power. The DRV2700
piezo-driver IC integrates the boost converter and push-pull output stages.

### 3.4 Gesture Input via IMU

The ICM-42688-P 6-axis IMU detects finger gestures without buttons:
- **Single tap** → advance to next Braille character (or next line)
- **Double tap** → toggle play/pause of text stream
- **Swipe left/right** → previous/next paragraph
- **Tilt** → adjust scroll speed
- **Long press** → enter settings mode

### 3.5 Power and Charging

The 120 mAh LiPo is recharged via a **Qi 1.3 wireless charging coil** embedded
in the ring's outer surface. The Nordic nPM1300 PMIC manages charging, fuel
gauge, and power-path. A USB-C port provides a fallback charging path and
debug access. The device is designed for all-day wear: 18 h active, 7 days
standby.

### 3.6 Form Factor: Rigid-Flex Wraparound PCB

The PCB is a 6-layer rigid-flex design that wraps around the ring
circumference. The inner surface holds the actuator array (facing the
fingertip), the outer surface holds the OLED, Qi coil, and capacitive
touch zone. The flex hinge allows the ring to open and close for finger
insertion.

---

## 4. Firmware Details

The firmware is written in C and built with the Nordic nRF Connect SDK
(zephyr-based). Key modules:

| Module | File | Responsibility |
|---|---|---|
| Board config | `board.h` | Pin assignments, GPIO definitions |
| Register map | `registers.h` | nRF5340 register definitions, DRV2700, ICM-42688-P |
| Main loop | `main.c` | Init, scheduler, mode state machine |
| Actuator driver | `drivers/actuator_drv.c` | HV boost control, DRV2700 SPI, demux, refresh ISR |
| Braille engine | `drivers/braille.c` | Unicode→Braille translation, scroll buffer |
| IMU driver | `drivers/imu_drv.c` | ICM-42688-P SPI, tap/swipe detection |
| BLE service | `drivers/ble_svc.c` | Custom GATT service, text streaming, command pipe |
| OLED driver | `drivers/oled_drv.c` | SSD1316 I2C, status rendering |
| Power manager | `drivers/power.c` | Fuel gauge, Qi state, sleep modes |
| Haptic renderer | `drivers/haptic_render.c` | Texture/waveform synthesis, navigation arrows |

### Firmware Design Decisions

1. **Dual-core split**: Network core runs BLE; app core never blocked by
   radio. Haptic rendering ISR has highest priority (preempt-config).

2. **Lock-free actuator refresh**: A double-buffered frame (current frame +
   next frame) is updated atomically via pointer swap. The 200 Hz refresh ISR
   reads the active frame with zero jitter.

3. **Braille translation table**: Supports UEB (Unified English Braille) with
   grade-1 (uncontracted) and grade-2 (contracted) modes, stored in flash as
   a lookup table (~4 KB).

4. **Adaptive drive voltage**: The skin-contact sensor detects when the ring
   is worn; the NTC temperature sensor compensates PZT deflection for
   temperature drift (PZT sensitivity varies ~0.05 %/°C).

5. **Low-latency text pipe**: BLE GATT notifications with a 20-byte MTU, ring
   buffer in RAM, zero-copy handoff to the rendering engine.

---

## 5. Application / Software Interface

The companion app (React Native) provides:

- **Reader mode**: Streams clipboard, notifications, or screen content as
  Braille to the ring. Supports iOS VoiceOver and Android TalkBack text
  interception.
- **Navigation mode**: Connects to phone GPS and streams directional haptic
  arrows (left/right/forward/stop) to the ring.
- **Braille Tutor**: Interactive learning mode for new Braille readers — shows
  a character on screen, renders it on the ring, quizzes the user.
- **Music Haptics**: Translates song rhythm and melody contour into tactile
  patterns (for hearing-impaired music enjoyment).
- **Texture Lab**: Let users design and stream custom tactile textures
  (grids, waves, dots) to the ring.
- **Settings**: Adjust drive intensity, scroll speed, gesture sensitivity,
  Braille grade, battery/charging status.

### BLE GATT Service

| UUID | Name | Type | Description |
|---|---|---|---|
| `0xT001` | Text Pipe | Write | Stream UTF-8 text for Braille rendering |
| `0xT002` | Command | Write | Control bytes (mode switch, speed, intensity) |
| `0xT003` | Status | Notify | Battery %, mode, connection state, errors |
| `0xT004` | Texture | Write | Raw 4×6 frame data (custom haptic patterns) |
| `0xT005` | Gesture | Notify | Detected gesture events for app feedback |
| `0xT006` | Nav Vector | Write | Navigation direction byte for haptic arrows |

---

## 6. Use Cases and Target Audience

### Primary: Blind and Visually Impaired Users
- **Reading on the go**: Emails, messages, articles streamed from a phone
  rendered as continuously-scrolling Braille on the fingertip — no desk, no
  bulky display.
- **Navigation**: Haptic directional cues from a GPS app, felt on the finger,
  no audio needed in noisy environments.
- **Education**: Braille tutor mode for students learning Braille literacy.

### Secondary: Hearing-Impaired Users
- **Music haptics**: Feel rhythm and melody through the fingertip.
- **Alerts**: Doorbell, timer, and alarm notifications as distinct tactile
  patterns.

### Tertiary: General / Sighted Users
- **Eyes-free interaction**: Navigation while driving or cycling, haptic
  notifications during meetings.
- **VR/AR haptics**: Tactile feedback layer for immersive experiences.
- **Accessibility development**: Platform for haptic UX research.

### Target Markets
- Assistive technology (primary commercial path)
- Wearable haptics for VR/AR
- Accessibility education and research labs
- General smart-device haptic interface

---

## 7. File Structure

```
tactiscript/
├── README.md                  ← this file
├── firmware/
│   ├── Makefile               ← build system
│   ├── board.h                 ← pin map, GPIO defs
│   ├── registers.h            ← register definitions
│   ├── main.c                 ← init, scheduler, state machine
│   └── drivers/
│       ├── actuator_drv.c     ← HV + DRV2700 + demux + refresh ISR
│       ├── actuator_drv.h
│       ├── braille.c           ← Unicode→Braille translation
│       ├── braille.h
│       ├── imu_drv.c           ← ICM-42688-P + gesture detection
│       ├── imu_drv.h
│       ├── ble_svc.c           ← custom GATT service
│       ├── ble_svc.h
│       ├── oled_drv.c          ← SSD1316 status display
│       ├── oled_drv.h
│       ├── power.c             ← battery/charging/sleep
│       ├── power.h
│       ├── haptic_render.c     ← texture/nav waveform synthesis
│       └── haptic_render.h
├── kicad/
│   ├── device.kicad_sch        ← schematic
│   ├── device.kicad_pcb        ← PCB layout
│   └── device.kicad_pro        ← project file
└── app/
    ├── App.js                  ← React Native entry
    ├── package.json
    ├── screens/
    │   ├── ReaderScreen.js
    │   ├── NavigationScreen.js
    │   ├── TutorScreen.js
    │   ├── MusicScreen.js
    │   ├── TextureLabScreen.js
    │   └── SettingsScreen.js
    ├── components/
    │   ├── ConnectionStatus.js
    │   └── BraillePreview.js
    └── utils/
        ├── ble.js              ← BLE manager
        └── braille.js          ← client-side Braille helpers
```

---

## 8. Bill of Materials (Summary)

| Ref | Part | Qty | Est. Cost |
|---|---|---|---|
| U1 | nRF5340 QFAA | 1 | $7.50 |
| U2-U7 | DRV2700 piezo driver | 6 | $9.00 |
| U8 | ICM-42688-P | 1 | $3.20 |
| U9 | SSD1316 OLED 0.49" | 1 | $2.80 |
| U10 | nPM1300 PMIC | 1 | $2.40 |
| Q1 | PZT bimorph cantilever | 24 | $12.00 |
| L1 | Qi RX coil | 1 | $1.50 |
| BAT1 | 120 mAh LiPo | 1 | $3.00 |
| Misc | passives, flex PCB, housing | — | $18.60 |
| **Total** | | | **~$60.00** |

---

## 9. Licensing

- **Hardware** (KiCad files, mechanical): CERN-OHL-S v2
- **Firmware** (C code): GPL-2.0
- **Companion app** (React Native): MIT

All authored by **jayis1**. Copyright © 2026 jayis1. All rights reserved.

---

*TactiScript — put the page on your finger.*