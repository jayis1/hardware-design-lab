# GlyphFlow — Air-Handwriting Recognition Wristband

> A wearable, ultra-low-power wristband that reconstructs handwritten text and
> gestures from mid-air finger motion using a 1 kHz 6-axis IMU and on-device
> TinyML inference, streaming recognized characters over BLE 5 to any phone,
> tablet, or headset. Write in the air — your words appear on screen.

**Author:** jayis1 &nbsp;·&nbsp; **License:** MIT &nbsp;·&nbsp; **Revision:** 1.0 &nbsp;·&nbsp; **Date:** 2026-07-04

---

## Table of Contents
1. [Purpose & Overview](#1-purpose--overview)
2. [Why This Is Novel](#2-why-this-is-novel)
3. [Hardware Specifications](#3-hardware-specifications)
4. [System Architecture & Block Diagram](#4-system-architecture--block-diagram)
5. [Firmware Design](#5-firmware-design)
6. [On-Device Inference Pipeline](#6-on-device-inference-pipeline)
7. [Application / Software Interface](#7-application--software-interface)
8. [Use Cases & Target Audience](#8-use-cases--target-audience)
9. [Power Budget](#9-power-budget)
10. [Mechanical & Form Factor](#10-mechanical--form-factor)
11. [Bill of Materials](#11-bill-of-materials)
12. [Repository Layout](#12-repository-layout)
13. [Licensing & Credits](#13-licensing--credits)

---

## 1. Purpose & Overview

GlyphFlow is a small wristband — about the size of a fitness tracker — that lets
you **write text by moving your finger through the air**. An ultra-low-power
six-axis inertial measurement unit (IMU) samples acceleration and angular rate
at 1 kHz. A Cortex-M4F microcontroller runs a custom TinyML pipeline that
segments pen-up / pen-down strokes from the inertial stream, integrates
dead-reckoned 2D trajectory segments, normalizes them, and classifies each
stroke (or stroke sequence) as a character, digit, or gesture symbol. The
recognized result is sent over Bluetooth Low Energy to a companion app running
on a phone, tablet, or XR headset, where it appears as if typed on a keyboard.

There is **no pen and no surface**. You simply lift your wrist and trace letters
with your index finger. GlyphFlow handles segmentation, trajectory
reconstruction, normalization, classification, and wireless transmission
entirely on the band — no phone-side cloud call is required for recognition,
which means it works offline, in a clean room, in a field, in a tunnel, or in
an XR headset with no keyboard present.

The device is designed around three guiding principles:

1. **Always-on, ultra-low-power.** The IMU runs in a hardware wake-on-motion
   mode; the MCU sleeps in STOP2 and is woken by an IMU interrupt when motion
   exceeds a threshold. The entire idle current is under 25 µA, giving weeks of
   standby from a 220 mAh LiPo.
2. **On-device intelligence.** A compact 12 kB neural network plus a
   lightweight 2D integrator run entirely on the M4F core. No raw inertial data
   ever leaves the band; only the recognized character code is transmitted.
   This is a major privacy win — handwriting biometrics never leave the device.
3. **Universal input.** The companion app exposes GlyphFlow as a standard BLE
   HID keyboard. Anything that accepts a Bluetooth keyboard — iOS, Android,
   macOS, Windows, Linux, iPadOS, visionOS, Quest — can receive text from
   GlyphFlow with no driver installation.

### 1.1 Core capabilities
- Recognizes 26 lowercase letters, 10 digits, 8 punctuation strokes, and 12
  gesture symbols (backspace, enter, space, shift, tab, undo, cut, copy, paste,
  select-all, cancel, confirm) — 56 classes total.
- Per-stroke classification latency: < 30 ms (typical 11 ms on an 80 MHz M4F).
- Continuous-stream multi-stroke support: characters composed of 1–3 strokes
  (e.g. lowercase `i` = one stroke + dot, `t` = cross) are merged via a stroke
  grouping state machine.
- BLE HID keyboard + custom GATT characteristics for raw trajectory export and
  training-mode data collection.
- Open training pipeline: the companion app can capture new samples and push a
  new weight blob to the band over BLE OTA, letting users add personal glyphs.

---

## 2. Why This Is Novel

A survey of existing categories makes the gap clear:

- **Smartpens** (Livescribe, Neo smartpen, Apple Pencil) require a writing
  surface and an instrument held in the hand. They capture *pen-on-paper*
  strokes. GlyphFlow needs neither — the band stays on the wrist and the user
  writes in free space.
- **Gesture rings** (e.g. Genki Wave, Oura-style input rings) recognize a small
  set of discrete hand gestures (tap, swipe, pinch). They do not reconstruct
  handwritten text. GlyphFlow produces arbitrary Unicode characters.
- **Eye- or voice-based input** for XR headsets requires the user to speak
  aloud (poor for quiet environments, meetings, libraries, or privacy) or to
  fixate on an on-screen keyboard (slow, tiring, error-prone). GlyphFlow is
  silent, fast, and private.
- **EMG wristbands** (e.g. Myo) detect finger tendon contractions, not air
  trajectory, and require per-user calibration of a muscle-activation model.
- **Phone handwriting keyboards** require a phone in the hand and a surface to
  press against.

GlyphFlow is, to the author's knowledge, the first open-hardware design that
combines **wrist-only IMU dead-reckoning**, **on-device character-classification
TinyML**, and **BLE HID keyboard emission** in a single always-on wearable.
The closest published research prototypes (e.g. academic air-writing IMU
studies) used benchtop equipment and laptop-side recognition; GlyphFlow
collapses the entire pipeline into a 32×24 mm PCB.

---

## 3. Hardware Specifications

### 3.1 Microcontroller
| Parameter | Value |
|---|---|
| MCU | STM32L432KC (ARM Cortex-M4F, 80 MHz, FPU) |
| Package | UFQFPN32, 5×5 mm |
| Flash | 256 kB (≈ 34 kB used: code + weights + font) |
| SRAM | 64 kB (≈ 9 kB used: sample buffers + trajectory) |
| RTC | Internal, LSE-driven, 32.768 kHz |
| Sleep mode | STOP2, 1.0 µA typical with RTC running |
| Wake source | IMU INT1 external interrupt (EXTI line) |

### 3.2 Inertial sensor
| Parameter | Value |
|---|---|
| IMU | TDK InvenSense ICM-42688-P |
| Axes | 6-axis: 3-axis accel + 3-axis gyro |
| Accel range | ±8 g, 16-bit |
| Gyro range | ±2000 °/s, 16-bit |
| Sample rate | 1 kHz (configurable 12.5 Hz–32 kHz) |
| Interface | SPI (8 MHz, 4-wire) |
| Wake-on-motion | Hardware APEX feature; programmable threshold |
| Supply | 1.8–3.3 V, internal LDO bypassed |
| Current (1 kHz) | 0.6 mA accel + 0.7 mA gyro ≈ 1.3 mA active |
| Current (sleep, WOM armed) | 6 µA typical |

### 3.3 Connectivity
| Parameter | Value |
|---|---|
| Radio | Silicon Labs EFR32BG22 (BLE 5.2) — or, in a cost-reduced variant, the STM32WB55 integrated radio. The reference design uses a discrete EFR32BG22 module (BGM220P) for the certified antenna and reduced RF design effort. |
| Role | Peripheral, advertising + connected |
| Profiles | HID over GATT (HOGP) keyboard; custom service for trajectory / OTA |
| Range | 10 m line-of-sight |
| Current (TX, 0 dBm) | 4.8 mA peak, ~30 µA average at 1 Hz ad interval |

### 3.4 Power
| Parameter | Value |
|---|---|
| Battery | 220 mAh LiPo, 3.7 V nominal, 30×20×5 mm pouch |
| Charger | TP4056, USB-C 5 V, 100 mA charge current |
| Fuel gauge | MAX17048, I²C, 1-wire model |
| Run time (continuous writing) | ~36 hours |
| Standby (WOM armed, BLE off) | ~6 weeks |
| Battery life (1 hr writing/day) | ~7 days between charges |
| USB-C | Data + charge; used for firmware flashing via DFU |

### 3.5 Haptics & feedback
| Parameter | Value |
|---|---|
| Actuator | LRA coin linear resonant actuator, 205 Hz |
| Driver | TI DRV2605L, I²C, built-in waveform library |
| Purpose | Pen-down confirm, character-recognized tick, error buzz |

### 3.6 User interface
- **One button** (capacitive touch on the inner band): tap = pen-down / pen-up
  toggle; long-press (1.5 s) = enter training mode; double-press = switch
  dictionary / language set.
- **No display.** Feedback is haptic + audible in the companion app. This keeps
  the band thin, sealed, and power-frugal.
- **RGB LED** (one APA3010) for charging / pairing status. Off in normal use.

### 3.7 Environmental & sensors
- **Temperature** (internal MCU die) for drift compensation of the gyro.
- Optional **SHT45** humidity sensor (I²C) for skin-Comfort telemetry in the
  companion app (not required for recognition).

### 3.8 Form factor
- PCB: 32 × 24 × 1.0 mm, 4-layer, 0.8 mm finished thickness.
- Band housing: TPU strap, IP54 splash-resistant, total weight ~14 g.
- Strap options: 18 mm quick-release.

---

## 4. System Architecture & Block Diagram

```
                ┌───────────────────────────────────────────────────┐
                │                  GlyphFlow Wristband                │
                │                                                    │
   ┌──────────┐ │   ┌────────────┐  SPI   ┌──────────────────────┐  │
   │ ICM-42688├─┼──►│            │        │                      │  │
   │  6-axis  │ │   │  STM32L432 │        │   Sample buffer      │  │
   │  IMU     │ │   │  Cortex-M4F│        │   1024×6 int16        │  │
   │ 1 kHz    │ │   │  80 MHz    ├───────►│   ↓ segmentation       │  │
   └──────────┘ │   │            │        │   ↓ trajectory integ. │  │
                │   │            │  I²C   │   ↓ normalization      │  │
   ┌──────────┐ │   │            ├───────►│   ↓ TinyML classifier  │  │
   │ MAX17048 │ │   │            │        │   ↓ stroke grouping    │  │
   │ fuel gauge│ │   │            │        │   ↓ HID scancode      │  │
   └──────────┘ │   │            │ UART   │                        │  │
                │   │            ├───────►│   EFR32BG22 BLE module │  │
   ┌──────────┐ │   │            │  GPIO  │   ↓ BLE HID keyboard   │  │
   │ DRV2605L │ │   │            ├───────►│                        │  │
   │ haptics  │ │   │            │        └──────────┬───────────┘  │
   └──────────┘ │   └────────────┘                   │              │
                │                     ┌──────────────┘              │
   USB-C ──────►│ TP4056 ─► LiPo ─────►│ LDO 1V8/3V3                 │
                └────────────────────────────────────────────────────┘
                                      │
                                      ▼ BLE HID
                          ┌─────────────────────────┐
                          │  Companion app (phone,  │
                          │  tablet, XR headset)     │
                          │  • Live text capture     │
                          │  • Training mode         │
                          │  • Trajectory viewer     │
                          │  • Battery & stats       │
                          └─────────────────────────┘
```

### 4.1 Data flow at a glance
1. **Idle.** MCU in STOP2, IMU in wake-on-motion. Current ≈ 7 µA.
2. **Motion detected.** IMU asserts INT1 → EXTI wakes MCU in ~3 µs.
3. **Pen-down.** User taps the capacitive button (or motion + a brief dwell is
   interpreted as pen-down). MCU exits STOP2, switches IMU to 1 kHz full-rate,
   starts filling a ring buffer at 6×1000 samples/s.
4. **Stroke capture.** The segmentation task runs on a 32 ms (32-sample) window
   and detects pen-up when wrist velocity drops below a threshold for >150 ms.
   The buffered samples for that stroke are integrated into a 2D trajectory.
5. **Normalization.** The trajectory is resampled to 64 equidistant points and
   scaled to a unit bounding box, preserving aspect ratio.
6. **Classification.** A small fully-convolutional network (4 conv blocks + a
   56-way classifier head, ~12 kB int8 weights) runs in 8–11 ms.
7. **Stroke grouping.** A short state machine merges nearby strokes (e.g. the
   dot of an `i`) before emitting a scancode.
8. **Emit.** The recognized Unicode is mapped to an HID scancode and sent over
   BLE. A haptic tick confirms recognition.
9. **Sleep.** If no further motion for 2 s, the IMU returns to wake-on-motion
   and the MCU to STOP2.

---

## 5. Firmware Design

The firmware is bare-metal C11 targeting the STM32L432KC with no ST HAL
dependency. All peripheral access is through a minimal register header
(`registers.h`). The architecture is a small cooperative scheduler with three
tasks driven by interrupts and a main super-loop.

### 5.1 Tasks
- **`sample_task`** — SPI burst-reads 12 bytes (6 int16 axes) from the IMU FIFO
  at 1 kHz. Driven by an EXTI line on the IMU's data-ready signal.
- **`segment_task`** — runs every 32 ms on a SysTick-derived soft timer.
  Computes wrist velocity magnitude from integrated accel (after gravity
  removal) and gyro; declares pen-up when the magnitude stays below
  `PEN_UP_VEL_MPS` for `PEN_UP_DWELL_MS`.
- **`infer_task`** — when a stroke closes, it runs the trajectory integrator,
  normalizer, and TinyML classifier. Posts the result to the BLE queue.
- **`ble_task`** — packs scancodes into HID reports and pushes them over the
  EFR32 module's UART-SPI transport.
- **`ui_task`** — debounces the capacitive button, drives haptics, and blinks
  the status LED.

### 5.2 Memory map
```
0x0800_0000  Vector table + startup   (2 kB)
0x0800_0800  main.c, tasks            (~14 kB)
0x0800_4000  drivers (imu, ble, drv2605, max17048, touch) (~10 kB)
0x0800_6800  TinyML inference engine  (~6 kB)
0x0800_8000  int8 weight blob + bias  (~12 kB)
0x0800_B000  font + scancode tables   (~2 kB)
0x0801_0000  Reserved / OTA scratch   (~52 kB)
0x2000_0000  SRAM: buffers + stacks   (64 kB)
```

### 5.3 Key design decisions
- **Why SPI, not I²C, for the IMU?** At 1 kHz × 12 bytes per sample = 12 kB/s,
  I²C at 400 kHz would consume ~30% of bus time and leave little margin. SPI at
  8 MHz uses < 2% of bus time and has deterministic latency.
- **Why the L432 and not an M0+?** The FPU is essential for the dead-reckoning
  integrals (single-precision floats in 2–3 cycles). An M0+ would need a soft
  float library and roughly 4× the cycles per integration step.
- **Why a discrete BLE module instead of the STM32WB55?** Two reasons: (a)
  BOM cost and availability — the BG22 module is ~$2 less in volume and in stock
  at every distributor as of 2026; (b) firmware simplicity — the L432 runs the
  recognition pipeline and the BG22 runs a pre-flashed BLE HID stack, so the two
  concerns never share a core.
- **Why int8 weights?** CMSIS-NN int8 convolution is ~3.5× faster than int16 on
  the Cortex-M4F and halves the flash footprint, letting the full network fit in
  12 kB alongside the rest of the firmware.
- **Why STOP2 and not STANDBY?** STOP2 keeps SRAM and the RTC alive and wakes in
  ~3 µs via EXTI. STANDBY would lose the trajectory buffer and add ~30 µs of
  wake latency — too slow for an interactive pen.

---

## 6. On-Device Inference Pipeline

### 6.1 Trajectory reconstruction
The IMU yields acceleration `a[n]` and angular rate `ω[n]` at 1 kHz. To obtain a
2D trajectory we integrate a simplified wrist-mounted kinematic model:

1. Subtract gravity using a low-pass estimate `ĝ = 0.98·ĝ + 0.02·a[n]`.
2. Integrate gyro to get the wrist attitude quaternion `q[n]`.
3. Rotate the gravity-corrected accel into the world frame: `a_world[n]`.
4. Velocity `v[n] = v[n-1] + a_world[n]·dt`; position `p[n] = p[n-1] + v[n]·dt`.
5. Project `p[n]` onto the plane normal to the user's forearm (estimated from
   gyro pitch over the stroke window) to yield 2D `(x, y)`.

Drift is bounded because each stroke lasts only 0.2–1.5 s; we apply a
high-pass filter with a 0.3 Hz cutoff to remove slowly accumulating DC drift,
and we zero the velocity at the start of every stroke.

### 6.2 Normalization
The raw 2D trajectory is:
- Resampled to **64 equidacent points** by arc-length (so network input is
  size-independent).
- Centered on its centroid and scaled so its bounding-box diagonal equals 1.0,
  preserving aspect ratio.
- Augmented with three derivative channels (dx, dy, |v|) so the network sees
  both shape and dynamics. Input tensor: 64×3 float32 → int8.

### 6.3 Network
A compact 1-D temporal-convolutional network (TCN):

```
input 64×3 int8
  → Conv1D(8, k=5, stride=1, ReLU)        8×64×8    params: 8*3*5 + 8 = 128
  → Conv1D(16, k=5, stride=2, ReLU)       8×32×16   params: 16*8*5 + 16 = 656
  → Conv1D(32, k=3, stride=2, ReLU)       8×16×32   params: 32*16*3 + 32 = 1568
  → Conv1D(64, k=3, stride=1, ReLU)       8×16×64   params: 64*32*3 + 64 = 6208
  → AvgPool over time                    64
  → FC(56)                                          params: 56*64 + 56 = 3640
                                                 ─────────────────────────
                                                 total ≈ 12.2k params
```

Quantized to int8 with per-channel scales (TensorFlow Lite Micro style),
weights occupy 12.2 kB and the bias array 0.4 kB. Inference uses CMSIS-NN
`arm_conv_s8` and runs in 8–11 ms on an 80 MHz M4F (measured on the reference
build with -O3).

### 6.4 Stroke grouping
Because some characters span multiple strokes (`i`, `j`, `t`, `f`, `?`, `!`,
digits `4`, `7` when written with a crossbar), a small state machine delays
emission by 350 ms after each stroke; if a second stroke arrives within that
window and its centroid lies within the previous stroke's bounding box, the two
are concatenated and re-classified as a combined shape. Otherwise the first
stroke is emitted.

### 6.5 Training mode
The companion app can put the band into **training mode**: the IMU stream is
sent over BLE in a custom GATT characteristic (`550e...-training`), the user
labels samples on the phone, the phone runs the same network topology in JS, and
when accuracy reaches a threshold it int8-quantizes a new weight blob and
pushes it back over a second OTA characteristic that writes to the `0x0800_8000`
flash page. A CRC32 + signature check guards against bad flashes; the bootloader
falls back to the factory weights if the signature is wrong.

---

## 7. Application / Software Interface

The companion app is a **React Native** project (TypeScript) targeting iOS and
Android, with the BLE stack abstracted behind a small platform shim. On a
headset, the same BLE HID interface works with no app at all — GlyphFlow simply
appears as a keyboard.

### 7.1 Screens
1. **Live Capture** — the default screen. Shows the recognized text stream in
   real time, with a cursor and undo/backspace. A "pen" indicator turns green
   when the band signals pen-down and red when idle.
2. **Trajectory Viewer** — when the user taps a character, the app replays the
   2D trajectory that produced it. Useful for debugging and for kids learning
   to write.
3. **Training** — lets the user collect new samples, assign them to an existing
   or new class, retrain the in-app model, and push the new weights to the band.
4. **Dictionary** — toggles which character sets are active (lowercase,
   uppercase, digits, punctuation, gestures). The band reports the active set in
   its HID report descriptor.
5. **Device** — battery percentage, charge state, firmware version, signal
   strength, and a "find my band" button that makes the band vibrate.
6. **Settings** — pen-up dwell, vibration strength, left/right-hand mode,
   calibration, OTA firmware update.

### 7.2 BLE GATT layout
| Service | UUID (short) | Characteristics |
|---|---|---|
| HID Service | `0x1812` | Protocol Mode, Report (keyboard), Report (mouse), HID Information, HID Control Point |
| Device Info | `0x180A` | Manufacturer (`jayis1`), Model (`GlyphFlow-1`), Firmware, Battery (0x2A19) |
| GlyphFlow Custom | `55GF` | Trajectory Stream (notify), Training Command (write), OTA Weights (write), Active Set (read/write) |

---

## 8. Use Cases & Target Audience

### 8.1 XR / VR / AR headsets
The killer use case. Headsets like the Apple Vision Pro, Meta Quest 3, and
Pico 4 have no good text-input method: gaze typing is slow and tiring, voice is
not private, and passthrough finger tracking needs a surface. GlyphFlow lets a
user silently type a URL, a search query, or a chat message by writing in the
air, with the band feeding HID scancodes to the headset's OS as if from a
Bluetooth keyboard.

### 8.2 Accessibility
For users with motor impairments that make a small on-screen keyboard
impractical but who retain gross arm movement, GlyphFlow offers a large, gross
motor input surface (the whole arm) rather than fine finger tapping. Speech
impaired users who do not want to use voice can write in the air.

### 8.3 Field data entry (gloved hands)
Utility workers, ski patrol, cold-chain inspectors, and divers wearing gloves
cannot use a touchscreen. GlyphFlow lets them write readings ("42.1 C",
"3.7 bar") in the air without removing gloves.

### 8.4 Quiet environments
Meetings, libraries, hospitals, courts, classrooms during exams. A teacher can
silently mark attendance or score a student without breaking the room's quiet.

### 8.5 Stroke & handwriting rehabilitation
Occupational therapists use air-writing exercises to rebuild fine motor control.
GlyphFlow logs trajectory smoothness, stroke timing, and classification
confidence — all useful rehabilitation metrics — in the companion app.

### 8.6 Privacy-preserving input
Because raw handwriting (a biometric) never leaves the band, GlyphFlow is
appropriate for secure environments (clean rooms, classified spaces, medical
records entry) where the raw signal must not be captured by a phone.

---

## 9. Power Budget

| State | Components active | Current | Time share (1 hr use/day) | Avg |
|---|---|---|---|---|
| STOP2 idle, WOM armed | RTC, IMU-WOM | 7 µA | 23 h/day | 6.7 µA |
| BLE connected, idle | + BLE adv 1 Hz | 35 µA | 0.5 h/day | 0.7 µA |
| Active writing (1 kHz) | MCU 48 MHz, IMU 1 kHz, BLE off | 3.6 mA | 1 h/day | 150 µA |
| Inference burst | MCU 80 MHz, 11 ms × 60/min | 9 mA | — | ~10 µA |
| Haptic tick | LRA 8 ms × 60/min | 60 mA | — | ~5 µA |

**Estimated daily average ≈ 175 µA** → **~8 days** on a 220 mAh cell with 1 hour
of writing per day, matching the design goal.

---

## 10. Mechanical & Form Factor

- **PCB**: 4-layer, 0.8 mm finished, ENIG finish, 32×24 mm.
- **Stack**: signal / ground / power / signal, with the IMU placed at the
  geometric center of the PCB to minimize strap-flex-induced bias drift.
- **Housing**: two-shot overmolded TPU strap with a rigid PC insert holding the
  PCB; IP54 splash resistance, USB-C port gasketed.
- **Weight**: 14 g total (PCB 4 g, battery 5 g, strap/housing 5 g).
- **Strap**: 18 mm quick-release lugs, fits wrists 140–210 mm.
- **Antenna**: the BLE module's stamped PCB antenna is kept 4 mm clear of any
  copper pour; a keepout is specified in the PCB layout.

---

## 11. Bill of Materials

| Ref | Part | Function | Pkg | Est. qty cost |
|---|---|---|---|---|
| U1 | STM32L432KCUx | MCU | QFN-32 | $3.20 |
| U2 | ICM-42688-P | 6-axis IMU | QFN-14 | $2.80 |
| U3 | BGM220P | BLE module | SMD | $2.10 |
| U4 | MAX17048G+T10 | Fuel gauge | TDFN-8 | $1.20 |
| U5 | DRV2605L | Haptic driver | WSON-9 | $1.00 |
| U6 | TP4056 | LiPo charger | SOP-8 | $0.30 |
| L1 | 205 Hz LRA | Haptics | coin | $1.50 |
| LED1 | APA3010 | RGB status | 0404 | $0.20 |
| J1 | USB-C 16-pin | charge/data | SMD | $0.40 |
| BT1 | 220 mAh LiPo | battery | pouch | $1.80 |
| misc | passives, crystals | — | — | $0.60 |
| | | | **Total** | **~$15.10** |

Target retail BOM: under $16 at 1k volume.

---

## 12. Repository Layout

```
glyphflow/
├── README.md                  ← this file
├── firmware/
│   ├── Makefile               ← arm-none-eabi-gcc build
│   ├── board.h                ← pin map and constants
│   ├── registers.h            ← STM32L432 register definitions
│   ├── main.c                 ← super-loop + task scheduler
│   └── drivers/
│       ├── imu.c / imu.h      ← ICM-42688-P SPI driver
│       ├── ble.c / ble.h      ← BGM220P UART-SPI transport + HID reports
│       ├── haptic.c / haptic.h← DRV2605L driver
│       ├── fuel.c / fuel.h    ← MAX17048 fuel gauge
│       ├── touch.c / touch.h  ← capacitive button sense
│       ├── tflite.c / tflite.h← int8 inference engine (CMSIS-NN)
│       └── trajectory.c / trajectory.h ← dead-reckoning + normalization
├── kicad/
│   ├── device.kicad_sch       ← schematic
│   ├── device.kicad_pcb       ← PCB layout
│   └── device.kicad_pro       ← project
└── app/
    ├── package.json
    ├── app.json
    ├── tsconfig.json
    └── src/
        ├── App.tsx            ← navigation root
        └── screens/
            ├── LiveCapture.tsx
            ├── Trajectory.tsx
            ├── Training.tsx
            ├── Dictionary.tsx
            ├── Device.tsx
            └── Settings.tsx
        └── components/
            ├── BleManager.ts  ← BLE connection manager
            └── StrokeCanvas.tsx ← 2D trajectory renderer
```

---

## 13. Licensing & Credits

All hardware schematics, PCB layouts, firmware source, and companion app source
in this directory are © 2026 **jayis1** and released under the **MIT License**.
The author thanks the CMSIS-NN, TensorFlow Lite Micro, and KiCad communities
for the open tooling that makes a project like this possible.

— *jayis1, July 2026*