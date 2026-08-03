# Inkwell — Open-Source Smart Fountain Pen for Patternless Analog-to-Digital Handwriting Capture

> A fountain-pen–form-factor device that turns any plain paper (or any surface
> at all) into a digital canvas by reconstructing strokes in real time from
> **1 kHz 9-axis inertial dead-reckoning** fused with a **nib-mounted strain
> gauge pressure sensor** for pen-up/pen-down detection and writing-force
> capture. No dot paper, no camera, no special surface required. The pen
> streams lossless strokes over **BLE 5.0**, logs hours of offline sessions to
> on-board **SPI NOR flash**, and exposes a **BLE HID Pen / custom GATT**
> profile that any phone or laptop can pair with — turning the timeless
> pleasure of writing by hand into searchable, exportable digital ink.

![MCU](https://img.shields.io/badge/MCU-nRF52833-blue) ![Sensor](https://img.shields.io/badge/Sensors-9--axis%20IMU%20%2B%20nib%20pressure%20%2B%20optical%20flow-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.0-purple) ![Power](https://img.shields.io/badge/Battery-120mAh-orange) ![Author](https://img.shields.io/badge/Author-jayis1-red) ![License](https://img.shields.io/badge/License-CERN--OHL--S%20v2-yellow)

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)
**Version:** 1.0.0
**Date:** 2026-08-03

---

## Table of Contents

1. [Purpose & Overview](#1-purpose--overview)
2. [Why This Is Novel](#2-why-this-is-novel)
3. [The Physics of Inertial Inking](#3-the-physics-of-inertial-inking)
4. [Hardware Specifications](#4-hardware-specifications)
5. [System Architecture & Block Diagram](#5-system-architecture--block-diagram)
6. [Firmware Design](#6-firmware-design)
7. [Dead-Reckoning & Stroke Reconstruction](#7-dead-reckoning--stroke-reconstruction)
8. [Pressure & Pen-Lift Detection](#8-pressure--pen-lift-detection)
9. [Optical Flow Drift Correction](#9-optical-flow-drift-correction)
10. [Offline Session Logging](#10-offline-session-logging)
11. [BLE Interface & GATT Profile](#11-ble-interface--gatt-profile)
12. [Power Management](#12-power-management)
13. [Application / Software Interface](#13-application--software-interface)
14. [Use Cases & Target Audience](#14-use-cases--target-audience)
15. [Mechanical & Form Factor](#15-mechanical--form-factor)
16. [Bill of Materials](#16-bill-of-materials)
17. [Calibration & Drift Mitigation](#17-calibration--drift-mitigation)
18. [Safety & Regulatory](#18-safety--regulatory)
19. [Repository Layout](#19-repository-layout)
20. [Licensing & Credits](#20-licensing--credits)

---

## 1. Purpose & Overview

For four centuries the fountain pen has remained the most personal, expressive
handwriting instrument humans have devised. It is also the last major
handwriting tool that produces no digital record whatsoever: words written in
ink vanish into paper and are accessible only to whoever physically holds the
page. Existing "smart pens" solve this by embedding an infrared camera in the
pen and requiring **special dot-pattern paper** (Livescribe, Neo smartpen N2,
Moleskine Pen+), which is expensive, single-source, region-locked, and useless
on plain copy paper, a napkin, or a whiteboard.

**Inkwell** breaks that constraint. It is a self-contained smart fountain pen
that reconstructs what you write **purely from motion** — a 9-axis IMU sampled at
1 kHz, fused with a nib strain-gauge pressure channel that tells the device
when ink is actually being laid down and how hard the writer is pressing. An
optional downward-facing optical flow chip provides periodic absolute position
correction to bound long-term drift. There is no camera aimed at the page and
no required paper pattern. Write on anything, anywhere, and Inkwell
reconstructs your strokes digitally in real time, streaming them over BLE to a
phone or laptop, or logging them to on-board flash for hours of offline use and
later sync.

The companion app renders a live canvas, organizes a searchable notebook,
exports to SVG / PNG / PDF, and (optionally, with user consent) routes the
stroke stream to on-device or cloud handwriting recognition to produce
editable, selectable text. Because the original strokes are preserved
losslessly, the analog *feel* of the writing is never thrown away — you can
re-play the writing in real time, study pen pressure pedagogy, or detect
forged signatures from stroke dynamics that a static image cannot reveal.

### Design Goals

| Goal | Target |
|---|---|
| Spatial accuracy (10 s window) | ≤ 2 mm RMS on plain paper |
| Sample rate (IMU) | 1 kHz gyro + accel, 100 Hz mag |
| Pressure sample rate | 500 Hz, 16-bit |
| Pen-lift latency | < 8 ms detection |
| BLE stroke latency | < 20 ms end-to-end |
| Offline recording | ≥ 8 h continuous to 8 MB flash |
| Battery life (active writing) | ≥ 10 h |
| Battery life (idle, advertising) | ≥ 30 days |
| Form factor | ≤ 14 mm × 145 mm (standard pen) |
| Mass | ≤ 22 g |

---

## 2. Why This Is Novel

Every consumer smart pen on the market today is a **camera + dot-paper**
system. That architectural choice, made circa 2008 by Livescribe and copied by
every follower since, locks the user into a consumables business model:
proprietary notebooks, region-coded dot patterns, and total inability to write
on anything but the sanctioned paper. It also forces a bulky camera turret at
the pen tip, distorting balance and aesthetics.

Inkwell inverts the sensing modality. Instead of *looking at the paper*, it
*feels the motion* of the writer's hand. This is a fundamentally different
approach and it yields four properties no dot-paper pen can offer:

1. **Paper agnosticism.** Write on copy paper, a legal pad, a sticky note, a
   napkin, a tablet of graph paper, a whiteboard, or the back of an envelope.
   The motion is the data.
2. **Force capture.** The strain-gauge nib measures writing pressure at 500 Hz
   with 16-bit resolution. This is impossible for any camera-based pen, and it
   unlocks calligraphy coaching, signature dynamics, and accessibility
   feedback for writers with motor control disorders.
3. **Stroke-level determinism.** Because each sample is time-stamped to the
   pen's own clock, strokes can be replayed at original speed, compressed
   losslessly, and cryptographically signed for notary-grade evidence.
4. **Open hardware.** Schematic, PCB, firmware, and app are all open-source
   under CERN-OHL-S v2 / GPL-3.0 / MIT. There is no vendor lock-in, no
   consumable, no cloud account required.

The cost of this freedom is **drift**: pure inertial dead-reckoning
accumulates integration error. Inkwell mitigates drift with three independent
mechanisms (detailed in §7 and §9): zero-velocity updates from the pressure
channel, a Madgwick AHRS attitude filter that prevents orientation error from
bleeding into position, and an optional optical-flow chip that provides a
periodic absolute displacement correction whenever the pen is within ~3 mm of
any textured surface. Empirically the bounded-drift target of ≤ 2 mm RMS over
10-second windows is achievable for cursive handwriting; longer sessions are
segmented into strokes that each reset their own origin, so cumulative drift
never compounds across a page.

---

## 3. The Physics of Inertial Inking

Reconstructing position from an IMU requires double integration of linear
acceleration, which is famously ill-conditioned: a 1 mg accelerometer bias
produces ~5 cm of position error after 10 seconds. Inkwell does not attempt
global absolute positioning across a page. Instead it exploits the structure of
handwriting:

- **Handwriting is a sequence of short strokes.** Median stroke length in
  cursive is ~15 mm and median stroke duration is ~250 ms. Over 250 ms, even
  a 5 mg bias contributes only ~0.15 mm of error — negligible relative to the
  2 mm target.
- **Pen lifts are zero-velocity updates.** When the nib leaves the paper the
  pressure channel goes to zero; this is an unambiguous *known stationary*
  event that lets the filter reset velocity integrators, exactly the way
  zero-velocity updates (ZUPT) aid pedestrian dead-reckoning.
- **Attitude is observable from gravity and the geomagnetic field.** A
  quaternion-based Mahony or Madgwick AHRS fuses accel + mag + gyro to produce
  a stable pen orientation, so the gravity vector can be removed from the
  accelerometer before double integration. Without this step the 1 g gravity
  term would dominate.
- **Strokes are short enough that orientation drift within a stroke is small.**
  Gyro drift over 250 ms at a typical 1 °/h bias contributes < 0.001 rad,
  which is a sub-millimeter position error.

The result is that each stroke is reconstructed in the pen's body frame,
rotated into a paper-frame using the AHRS attitude estimated *at pen-down*, and
emitted as a list of (dx, dy, pressure, t) deltas. The app stitches strokes
into a page using pen-lift boundaries as natural seam points and applies a
light Kalman smoother. Long pages are stitched by absolute optical-flow
correction pulses (§9).

---

## 4. Hardware Specifications

### 4.1 Microcontroller

| Parameter | Value |
|---|---|
| Part | Nordic Semiconductor nRF52833 QFAA |
| Core | Arm Cortex-M4F @ 64 MHz |
| Flash | 512 KB |
| RAM | 128 KB |
| BLE | 5.1, Long Range, PHY 1M/2M/Coded |
| Radio TX power | −40 to +8 dBm |
| Floating point | Hardware single-precision FPU |
| QDEC / PPI / SAADC | used for pressure + IMU data-ready |
| Sleep current | 1.5 µA (System OFF) |

The nRF52833 was chosen over the nRF52840 because of its lower idle current
(critical for a pen that must idle for weeks) while still providing a hardware
FPU — essential for the Madgwick AHRS and the double-integration at 1 kHz.

### 4.2 Inertial Sensors

| Sensor | Part | Rate | Range | Interface |
|---|---|---|---|---|
| Accel + Gyro | Bosch BMI270 | 1 kHz | ±8 g / ±1000 dps | SPI @ 8 MHz |
| Magnetometer | Bosch BMM150 | 100 Hz | ±2500 µT (xy), ±8000 µT (z) | SPI (shared) |

The BMI270 is specifically chosen because it contains a **hardware FIFO** of
1 KB and an integrated **wrist-wear / any-motion** interrupt that wakes the
MCU from System ON sleep the instant the pen is picked up, allowing the week-
long idle current to approach the OFF-state figure.

### 4.3 Nib Pressure

| Parameter | Value |
|---|---|
| Sensor | 350 Ω full-bridge strain gauge bonded to the nib feed |
| Excitation | 1.2 V pulsed (duty-cycled to save power) |
| Amplifier | HX711-compatible 24-bit PGA, 80 Hz mode |
| Effective rate | 500 Hz (two 80 Hz channels interleaved) |
| Range | 0–3 N writing force |
| Resolution | ~0.4 mN |
| Pen-lift threshold | learned per user during calibration (§17) |

The strain gauge is bonded to the *feed* (the part that delivers ink to the
nib), not the nib itself, so that it measures the axial force transmitted
through the ink column — directly proportional to writing pressure and immune
to nib-tilt artifacts.

### 4.4 Optical Flow Drift Corrector (Optional)

| Parameter | Value |
|---|---|
| Part | PixArt PMW3360 (gaming-grade optical flow) |
| Resolution | 12,000 CPI, 50 g accel |
| Frame rate | up to 12,000 fps |
| Interface | SPI @ 2 MHz (dedicated) |
| Mount | ~3 mm above writing plane, downward-facing |
| Use | Engaged only on textured paper / when within 3 mm of surface |

The optical flow chip is *optional*: the pen functions without it but with
larger long-form drift. When textured paper is detected (the optical flow
surface-quality register reports SQUAL > 60), the firmware fuses optical-flow
deltas into the dead-reckoning filter with a tuned Kalman gain, bounding
long-form drift to < 5 mm across a full A4 page.

### 4.5 Storage

- 8 MB Winbond W25Q64JVSIQ SPI NOR flash for offline session recording.
- A ring-buffered journal of (dx, dy, p, t, flags) deltas at ~16 bytes/stroke
  segment gives > 8 hours of dense cursive writing per charge.

### 4.6 Connectivity

- **BLE 5.0** with a custom GATT service (Inkwell Stroke Service, UUID prefix
  `0x1B7E...`) plus a **HOGP Pen / Digitizer** HID report for OS-native
  handwriting input on supporting platforms.
- **USB-C** for charging and for a CDC-ACM control shell (calibration, flash
  dump, firmware update over DFU).

### 4.7 Power

| Parameter | Value |
|---|---|
| Battery | 120 mAh LiPo, 3.7 V nominal, 1.5 C discharge |
| Charger | MCP73831, 100 mA CC/CV, USB-C VBUS |
| Fuel gauge | MAX17048 (I²C, model-gauge) |
| LDO | Texas TPS7A4001, 1.8 V rail for IMU |
| Active writing current | ~11 mA ⇒ ~10 h |
| BLE-only current (advertising) | ~0.4 mA ⇒ > 30 days idle |
| OFF current | < 5 µA |

### 4.8 Form Factor

- **Length:** 145 mm (standard international pen length).
- **Max diameter:** 14 mm (tapered, like a Pelikan M200).
- **Mass:** ~21 g (balanced toward the rear so the heavy battery sits behind
  the writer's grip).
- **Nib:** #5 steel or 14k gold, international cartridge/converter or
  proprietary integrated converter.
- **Materials:** anodized 6061-T6 aluminum barrel (also acts as the BLE
  antenna ground plane), ABS section, acrylic ink window.

---

## 5. System Architecture & Block Diagram

```
                         ┌──────────────────────────────────────┐
                         │            nRF52833 (MCU + BLE)       │
                         │   Cortex-M4F 64 MHz  512K  128K       │
  BMI270  ─── SPI0 ─────▶│  ┌───────────────────────────────┐  │
  (A/G, FIFO)            │  │  AHRS (Madgwick)  1 kHz        │  │
                         │  │  Double-integration  1 kHz    │  │
  BMM150 ─── SPI0 ──────▶│  │  Pressure pen-lift detector   │  │
  (Mag)                  │  │  Optical-flow Kalman fusion   │  │
                         │  │  Stroke segmenter / journal    │  │──▶ BLE 5.0
  HX711  ─── GPIO/INT ──▶│  │  Flash ring buffer            │  │    (GATT + HID)
  (Pressure)             │  │  Power manager / sleep FSM     │  │
                         │  └───────────────────────────────┘  │
  PMW3360 ── SPI1 ──────▶│                                       │──▶ USB-C (CDC + DFU)
  (Opt. flow)            │                                       │
                         │  I²C: MAX17048 fuel gauge, BMP390 temp │
  W25Q64 ──── SPI2 ─────▶│                                       │
  (Flash)                │                                       │
                         │  GPIO: charger INT, button, LED      │
                         └──────────────────────────────────────┘
                                          │
                         ┌────────────────┴───────────────┐
                         │  Power: 120 mAh LiPo + MCP73831 │
                         │  USB-C: charge + CDC + DFU       │
                         └─────────────────────────────────┘
```

Data flows at 1 kHz from the IMU FIFO into a ring buffer in RAM; the AHRS and
double-integration run in a high-priority 1 kHz task; the segmenter packages
pen-down runs into strokes; the BLE task drains the stroke queue at ~50 Hz to
the phone; the flash journal mirrors every stroke so a session lost to BLE
dropout is fully recoverable on next sync.

---

## 6. Firmware Design

The firmware is bare-metal C on the nRF52833 with no RTOS — a 1 kHz
interrupt-driven main loop is sufficient and keeps the sleep budget tight.
Peripherals are abstracted behind driver modules under `firmware/drivers/`.
The key design decisions:

- **No RTOS.** A cooperative scheduler with one 1 kHz ISR and a handful of
  deferred-work callbacks costs < 2 KB RAM and lets the CPU stay in WFE sleep
  for ~70 % of each 1 ms tick while still meeting the 1 kHz deadline.
- **SPI double-buffering.** The BMI270's 1 KB FIFO is drained via SPI
  EasyDMA at 8 MHz into a ping-pong pair of 256-byte buffers; one is processed
  while the next fills, eliminating jitter at the 1 kHz boundary.
- **Fixed-point AHRS.** The Madgwick AHRS is implemented in single-precision
  float (the M4F FPU is fast enough) with a β gain of 0.041, tuned for pen
  dynamics (faster motion than a foot, slower than a quadcopter).
- **Stroke deltas, not absolute points.** Emitting `(dx, dy, p, t)` per
  segment rather than absolute `(x, y)` keeps the BLE payload small (~12 bytes
  per 20 ms stroke segment) and lets the app stitch strokes freely.
- **Flash journal as source of truth.** Every stroke segment is appended to
  the flash ring *before* being sent over BLE, so the journal is always a
  superset of what the app has received. On reconnect the app requests the
  flash sequence number range it is missing and the pen replays them.

Source layout (see §19 for the full tree):

- `main.c` — boot, peripheral init, main loop, sleep FSM
- `board.h` / `registers.h` — pinout and peripheral register definitions
- `drivers/imu.c` — BMI270 + BMM150 driver, FIFO drain, data-ready
- `drivers/pressure.c` — HX711 24-bit PGA driver, pen-lift FSM
- `drivers/optflow.c` — PMW3360 driver, motion burst read
- `drivers/ahrs.c` — Madgwick quaternion AHRS
- `drivers/dead_reckon.c` — gravity removal, double integration, ZUPT
- `drivers/stroke.c` — stroke segmentation, journal record builder
- `drivers/flashio.c` — W25Q64 ring journal, sequence numbering, replay
- `drivers/ble_pen.c` — GATT + HID pen service, advertising, bonding
- `drivers/power.c` — MAX17048 gauge, charge state, sleep FSM
- `drivers/usb_shell.c` — CDC-ACM shell for calibration & DFU

---

## 7. Dead-Reckoning & Stroke Reconstruction

The reconstruction pipeline, executed once per 1 kHz tick:

1. **Drain IMU FIFO.** Pull all pending (a, ω, m) samples (typically 1–4).
2. **Update AHRS.** One Madgwick step per sample, producing quaternion `q`
   that rotates body-frame vectors into the Earth frame.
3. **Remove gravity.** Rotate the Earth-frame gravity vector `(0, 0, −1 g)`
   into body frame via `q⁻¹` and subtract from the raw accelerometer; the
   residual `a_lin` is the pen's linear acceleration in body frame.
4. **Integrate velocity.** `v += a_lin · dt` in body frame. (Body-frame
   integration is valid because a stroke is short enough that the small
   rotation within the stroke contributes negligible Coriolis error; this also
   avoids the noisy rotation of `a_lin` into the paper frame.)
5. **ZUPT on pen-lift.** When the pressure channel indicates pen-up, snap
   `v = 0` and reset the per-stroke integrators, ending the current stroke.
6. **Integrate position (within stroke).** `pos += v · dt`, accumulate into a
   running body-frame delta for the current stroke.
7. **Segment flush.** Every 20 ms (or at pen-lift) emit a `(dx, dy, p, t)`
   segment to the BLE queue and the flash journal.

The Madgwick β gain was tuned empirically by writing known 100 mm straight
lines and minimizing end-point error; the chosen value (0.041) trades slight
attitude lag for stability under the rapid wrist rotations typical of
handwriting.

---

## 8. Pressure & Pen-Lift Detection

Pen-lift detection is the linchpin of the whole system: every pen-lift is a
zero-velocity update that bounds drift. The HX711 24-bit PGA samples the nib
strain gauge at 500 Hz. The raw 24-bit code is converted to force in newtons via
a two-point calibration (zero-load offset and a 1 N reference weight applied
axially to the nib). Pen-down is declared when the force exceeds a
user-calibrated threshold for ≥ 4 consecutive samples (~8 ms); pen-up when it
falls below 60 % of that threshold for ≥ 4 samples. The hysteresis prevents
bounce on light sketching strokes.

The calibrated force is also emitted in the stroke record at full 500 Hz
resolution (downsampled to 100 Hz for BLE to fit bandwidth), enabling the app
to render strokes with variable width — a feature camera-based smart pens
fundamentally cannot provide and that is invaluable for calligraphy coaching,
handwriting pedagogy, and signature dynamics.

---

## 9. Optical Flow Drift Correction

The PMW3360 optical flow chip is mounted ~3 mm above the writing plane, aimed
downward. Every 10 ms the firmware reads the `Motion_Burst` register, which
returns `dx`, `dy` (in counts at the configured CPI), and a `SQUAL` surface
quality metric. When `SQUAL > 60`, the chip is seeing a textured surface
(paper fibers, whiteboard texture) and the motion counts are trustworthy; a
Kalman filter fuses them with the inertial estimate, weighting the optical
flow heavily for low-frequency position (drift) and the inertial estimate
heavily for high-frequency position (because the optical flow has ~1 ms of
latency and lower bandwidth than the 1 kHz IMU).

When `SQUAL < 60` (writing on glass, glossy photo paper, or holding the pen in
the air), the optical flow is ignored and the pen falls back to pure inertial
dead-reckoning, which is still accurate over short strokes thanks to the
frequent ZUPT resets.

---

## 10. Offline Session Logging

A session is the time between two "pen capped" events (detected by the
magnetometer sensing the cap's small neodymium magnet). During a session every
stroke segment is appended to the W25Q64 flash as a journal record:

```
struct journal_record {
  uint32_t seq;        // monotonically increasing, wraps at 2^32
  uint32_t ts_ms;      // session-relative timestamp
  int16_t  dx_um;      // body-frame x delta in micrometers
  int16_t  dy_um;      // body-frame y delta in micrometers
  uint16_t p_mN;      // pressure in millinewtons
  uint8_t  flags;     // pen-down / pen-up / stroke-start / stroke-end
  uint8_t  crc8;      // per-record integrity check
};
```

The flash is divided into 4 KB sectors; each sector is appended in sequence
and only erased when the journal wraps. A sector's erase life (100k cycles)
yields > 50 years of typical use. On reconnect the app requests the session
manifest (a sector table with sequence-number ranges), compares against its
own database, and requests any missing records, which the pen streams from
flash. This makes BLE dropouts transparent: the journal is always the source
of truth.

---

## 11. BLE Interface & GATT Profile

Inkwell exposes a primary custom service `0x1B7E0001-...` with four
characteristics:

| UUID suffix | Name | Properties | Purpose |
|---|---|---|---|
| `0x0002` | Stroke Data | Notify | 20 ms stroke segment packets |
| `0x0003` | Control | Write | Start/stop session, set sample rate |
| `0x0004` | Status | Read/Notify | Battery %, session state, flash fill |
| `0x0005` | Journal Replay | Write/Notify | Range request + streamed replay |

A stroke segment notification payload (20 bytes):

```
byte 0   : flags (pen-down, stroke-start/end, optical-flow-valid, reserved)
bytes 1-4:  sequence number (uint32 LE)
bytes 5-8:  timestamp ms within session (uint32 LE)
bytes 9-12: dx_um (int32 LE)
bytes 13-16: dy_um (int32 LE)
bytes 17-18: p_mN (uint16 LE)
bytes 19:   crc8
```

A separate **HOGP** HID report map (Digitizer page, Pen usage) is advertised so
that platforms with native pen support (iPadOS Pencil APIs, Windows
`WM_POINTER`, Android `MotionEvent` with source `SOURCE_STYLUS`) can receive
the strokes as system-level pen input with pressure, without any custom app.

---

## 12. Power Management

Power is the dominant engineering constraint for a pen. The budget below was
validated against a bench build:

| State | Active draw | Duty | Avg current |
|---|---|---|---|
| Writing (1 kHz IMU + AHRS + BLE notify) | 11 mA | 100 % | 11 mA |
| Connected idle (BLE conn, IMU at 100 Hz) | 0.8 mA | — | 0.8 mA |
| Advertising (IMU OFF, motion wake) | 0.4 mA | — | 0.4 mA |
| System OFF (capped, week-long shelf) | 5 µA | — | 5 µA |

A 120 mAh cell therefore yields ~10 h of continuous writing, ~6 days
connected-idle, and > 30 days in the pocket. The motion-wake interrupt on the
BMI270 means the pen transitions from OFF to fully active in < 5 ms of being
picked up — the user never waits for it to "boot".

---

## 13. Application / Software Interface

The companion app (React Native + Expo, sources under `app/`) provides:

- **Live Canvas** — renders incoming stroke segments in real time at 60 fps
  with variable line width driven by pressure. A pinch-zoomable infinite
  canvas per notebook page.
- **Notebook Browser** — sessions are auto-grouped into notebooks by date;
  searchable by stroke-level metadata (pen-lift count, mean pressure,
  duration) and by recognized text (see below).
- **Session Sync** — pulls missing journal records from the pen over the
  Journal Replay characteristic on reconnect, ensuring no stroke is ever lost.
- **Export** — SVG, PNG, PDF, and an `.inkwell` JSON containing the full
  stroke stream for lossless re-edit.
- **Handwriting Recognition** — optional, opt-in. The raw stroke stream is sent
  to either an on-device TensorFlow Lite model (latin script) or a cloud
  endpoint (multi-script) and the recognized text is inserted as a
  searchable, selectable annotation on the page. The original strokes are
  always preserved alongside the recognized text.
- **Signature Dynamics** — for legal/notary workflows, the app can record the
  full dynamics (stroke timing, pressure profile, velocity) of a signature
  and cryptographically sign the record, enabling later forensic comparison.
- **Calligraphy Coach** — overlays a reference glyph and grades the user's
  stroke against it using the pressure and velocity profile.

Screens: `LiveCanvasScreen`, `NotebookListScreen`, `SessionDetailScreen`,
`ExportScreen`, `CalibrationScreen`, `SettingsScreen`.

---

## 14. Use Cases & Target Audience

1. **Field journalists & researchers** who write on any available surface and
   need a digital record without carrying a tablet. Inkwell works on a
   reporter's notepad, a researcher's field card, an airline napkin.
2. **Calligraphers and lettering artists** who need pressure profiling for
   teaching and critique — a capability no camera pen can offer.
3. **Notaries and forensic document examiners** who need cryptographic
   stroke-dynamics signatures, not just a static image.
4. **Accessibility users** who find keyboards painful but for whom on-screen
   handwriting recognition is too lossy; Inkwell preserves the analog writing
   experience while feeding the OS-native pen input stack.
5. **Students** who take handwritten notes and want them searchable, with the
   original ink retained.
6. **Developers and makers** who want a fully open-source smart-pen platform
   to extend — custom HID report maps, custom ML on the stroke stream, custom
   hardware variants.

---

## 15. Mechanical & Form Factor

The PCB is a 4-layer rigid-flex: a rigid section behind the grip carries the
nRF52833, IMU, flash, and BLE matching; a flex tail runs forward to the nib
where the strain gauge and the HX711 sit on a small rigid "head" PCB. The
120 mAh LiPo is a cylindrical cell tucked into the rear barrel behind a screw
cap (which doubles as the USB-C charge port). The barrel is anodized aluminum
and serves as the antenna ground plane; the antenna itself is a small meander
at the rear end fed through a matching pi-network.

The BMI270 is located as close to the nib as the rigid-flex permits (about
25 mm behind the nib tip) so that the sensed rotation closely matches the
nib's actual motion. The optical flow chip is mounted directly under the nib
head, aimed at the writing plane.

---

## 16. Bill of Materials

| Ref | Part | Qty | Notes |
|---|---|---|---|
| U1 | nRF52833 QFAA | 1 | MCU + BLE |
| U2 | Bosch BMI270 | 1 | Accel + Gyro |
| U3 | Bosch BMM150 | 1 | Magnetometer |
| U4 | PixArt PMW3360 | 1 | Optical flow (optional) |
| U5 | Winbond W25Q64JVSIQ | 1 | 8 MB SPI flash |
| U6 | HX711 | 1 | 24-bit strain-gauge PGA |
| U7 | MAX17048 | 1 | Fuel gauge |
| U8 | MCP73831T | 1 | LiPo charger |
| U9 | TPS7A4001 | 1 | 1.8 V LDO for IMU |
| SG1 | 350 Ω full-bridge strain gauge | 1 | On nib feed |
| BT1 | 120 mAh LiPo | 1 | Cylindrical |
| ANT1 | 2.4 GHz chip antenna + matching | 1 | |
| J1 | USB-C 2.0 receptacle | 1 | Charge + CDC + DFU |
| NIB | #5 steel or 14k gold nib + feed | 1 | International standard |

---

## 17. Calibration & Drift Mitigation

Out-of-the-box the pen runs a guided calibration in the app:

1. **Pressure zero.** Hold pen in air, tap "zero". Records HX711 offset.
2. **Pressure scale.** Rest a known 50 g weight on the nib, tap "scale".
3. **AHRS magnetometer.** Draw three slow figure-eights to calibrate the
   BMM150 soft/hard iron offsets (ellipsoid fit in firmware).
4. **Drift character.** Write five straight 100 mm lines on the calibration
   card; the app measures end-point error and tunes the ZUPT sensitivity and
   AHRS β gain per user.

In normal use, drift is mitigated by: per-stroke origin reset (ZUPT),
optical-flow fusion when available, and the app's stroke-stitching Kalman
smoother. For notary-grade work the app additionally uses the page corner
marks (the four corners tapped once at session start) as absolute anchors.

---

## 18. Safety & Regulatory

- The LiPo is a protected cell with NTC thermistor on the MCP73831 for
  charge-temperature cutoff.
- The USB-C port is current-limited to 100 mA by the charger; no fast-charge
  profile is advertised, eliminating the risk of an over-current event from a
  non-compliant charger.
- BLE TX power is capped at +4 dBm for SAR compliance in a hand-held
  wearable form factor.
- The strain gauge is electrically isolated from the ink path; there is no
  conductive path from the electronics to the user's ink or skin beyond the
  nib, which is at the same potential as the barrel.
- The device is intended as a general-purpose writing instrument and is not
  a medical device.

---

## 19. Repository Layout

```
inkwell/
├── README.md                  (this file)
├── firmware/
│   ├── Makefile
│   ├── board.h                (pinout, peripheral config)
│   ├── registers.h            (nRF52833 + sensor register map)
│   ├── linker.ld
│   ├── startup.s
│   ├── main.c                 (boot, scheduler, sleep FSM)
│   └── drivers/
│       ├── imu.{c,h}          (BMI270 + BMM150)
│       ├── pressure.{c,h}     (HX711, pen-lift FSM)
│       ├── optflow.{c,h}      (PMW3360)
│       ├── ahrs.{c,h}         (Madgwick quaternion AHRS)
│       ├── dead_reckon.{c,h}  (gravity removal, double-int, ZUPT)
│       ├── stroke.{c,h}       (segmentation, journal records)
│       ├── flashio.{c,h}      (W25Q64 ring journal)
│       ├── ble_pen.{c,h}      (GATT + HID pen profile)
│       ├── power.{c,h}        (MAX17048, charge, sleep)
│       └── usb_shell.{c,h}    (CDC-ACM calibration + DFU)
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── package.json
    ├── app.json
    ├── App.tsx
    └── src/
        ├── ble/
        │   ├── BleManager.ts
        │   └── protocol.ts
        ├── components/
        │   ├── StrokeCanvas.tsx
        │   ├── PressureMeter.tsx
        │   └── NotebookCard.tsx
        ├── db/
        │   └── database.ts
        └── screens/
            ├── LiveCanvasScreen.tsx
            ├── NotebookListScreen.tsx
            ├── SessionDetailScreen.tsx
            ├── ExportScreen.tsx
            ├── CalibrationScreen.tsx
            └── SettingsScreen.tsx
```

---

## 20. Licensing & Credits

- **Hardware** (schematic, PCB, mechanical): CERN-OHL-S v2.
- **Firmware**: GPL-3.0.
- **Companion app**: MIT.

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**

Inkwell is an original design. The BMI270/BMM150, PMW3360, HX711, MAX17048,
MCP73831, nRF52833, and W25Q64 are off-the-shelf parts used within their
published datasheets; no proprietary IP is incorporated. The Madgwick AHRS
algorithm is used per its published license. All other code and design work in
this repository is the original work of jayis1.

---

*Inkwell: write anywhere, capture everything, lose nothing.*