# ThermoGlyph — Wearable Thermal-Tactile Communication & Navigation Display

![ThermoGlyph](https://img.shields.io/badge/PCB-60×45mm-blue) ![MCU](https://img.shields.io/badge/MCU-nRF5340-orange) ![Actuators](https://img.shields.io/badge/Thermal-12×8%20Peltier%20Array-red) ![Sensors](https://img.shields.io/badge/Sensors-IMU%20%2B%20Skin%20Temp-green) ![Comms](https://img.shields.io/badge/Comms-LoRa%20%2B%20BLE%205.4-teal) ![Power](https://img.shields.io/badge/Power-Solar%20%2B%20LiPo-yellow) ![Author](https://img.shields.io/badge/Author-jayis1-orange)

**Author: jayis1**  
**Copyright (c) 2026 jayis1**  
**License: CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)**

---

## 1. Overview

**ThermoGlyph** is a wrist-worn (or dorsal-hand-worn) haptic display that
communicates information to its wearer entirely through localized temperature
changes. Instead of vibrating motors or electro-tactile pins — both of which
are noisy, fatiguing, and attention-grabbing — ThermoGlyph uses a dense array
of 96 micro-Peltier thermoelectric cells (12×8 grid, 2.5 mm pitch) pressed
gently against the skin to render *thermal glyphs*: warm/cool patterns that the
wearer perceives as directional arrows, alphanumeric characters, waveforms, or
abstract alert shapes.

The human skin can distinguish warm and cool spots with sub-degree resolution
when the stimulus is well-localized, and thermal sensation has a unique
psychophysical quality — it is *ambient*, *silent*, and *non-alarming* compared
to vibration. This makes ThermoGlyph ideal for:

| Domain | Use Case |
|--------|----------|
| Accessibility | Blind/low-vision navigation: turn-by-turn thermal arrows on the hand |
| Emergency services | Firefighters in zero-visibility smoke: buddy location & exit direction |
| Diving / underwater | Divers wearing thick gloves who can't read screens: depth & ascent cues |
| Tactical / military | Silent, zero-RF-signature comms: pre-arranged thermal codes |
| Industrial | Workers in noise/vibration-heavy environments receiving process alerts |
| Medical | Patients with neuropathy/numbness receiving gentle medication reminders |
| VR/AR | Thermal feedback for virtual environments (temperature of touched objects) |

The device pairs with a smartphone over BLE 5.4 for glyph streaming, and also
has a LoRa 868/915 MHz radio for long-range (up to 5 km) buddy-to-buddy or
base-station-to-wearer messaging in environments without cellular coverage
(firegrounds, caves, dive boats, wilderness).

An onboard 6-axis IMU allows the wearer to *acknowledge* or *dismiss* glyphs
with simple wrist gestures (tap, double-tap, flip), making the interaction
fully hands-free.

---

## 2. Hardware Specifications

### 2.1 MCU & Processing

| Parameter | Value |
|-----------|-------|
| MCU | nRF5340 dual-core (Cortex-M33 @ 128 MHz network core, M33 @ 128 MHz app core) |
| Flash | 1 MB (app core) + 256 KB (net core) |
| RAM | 512 KB |
| Security | Arm CryptoCell-312, secure boot, OTA-DFU |
| Role | App core runs glyph engine, thermal PID loop, IMU gesture recognition; net core runs BLE + LoRa stacks |

### 2.2 Thermal Actuator Array

| Parameter | Value |
|-----------|-------|
| Grid | 12 columns × 8 rows = 96 cells |
| Cell type | Custom micro-Peltier TEC (Bi₂Te₃, 2.0 × 2.0 × 0.8 mm) |
| Pitch | 2.5 mm center-to-center |
| Active area | 30 mm × 20 mm |
| Temperature range | Skin-side: 18 °C – 42 °C (safety-clamped) |
| Heating/cooling rate | Up to 3 °C/s per cell |
| Driver | 12-channel cross-point PWM H-bridge matrix (4 TLC59711 LED drivers reconfigured for bidirectional current, 8 row-select analog switches) |
| Safety | Hardware over-temp cutoff at 44 °C, skin-temperature watchdog ADC, fail-open thermal fuse |

### 2.3 Sensors

| Sensor | Part | Purpose |
|--------|------|---------|
| IMU | ICM-42688-P (6-axis accel + gyro) | Gesture recognition, tap detection, orientation |
| Skin temperature | 12 × TMP117 digital temp sensors (one per row) | Closed-loop thermal PID, safety monitoring |
| Ambient temperature | Si7051 | Compensation, ambient cancellation |
| Ambient light | LTR-390UV | Display auto-dim, "under bright sun" detection |
| Battery gauge | MAX17048 | LiPo fuel gauging |

### 2.4 Connectivity

| Radio | Part | Range | Role |
|-------|------|-------|------|
| BLE 5.4 | nRF5340 integrated | ~30 m | Phone pairing, glyph streaming, OTA updates |
| LoRa | SX1262 (868/915 MHz) | up to 5 km | Long-range buddy/base comms, mesh relay |
| USB-C | — | — | Charging, debug, firmware download |

### 2.5 Power

| Source | Details |
|--------|---------|
| Battery | 500 mAh LiPo (3.7 V, 1.85 Wh) |
| Solar | 2 × KXOB25-05X12F amorphous-Si cells (top of wrist strap) |
| Harvester | BQ25570 (solar MPPT + battery charge management) |
| Runtime | 14 hours continuous thermal, 5 days standby |
| Charge time | 2.5 h via USB-C, ~6 h via solar (full sun) |
| Power budget | ~35 mA active glyph rendering, ~0.3 mA sleep |

### 2.6 Form Factor

| Parameter | Value |
|-----------|-------|
| PCB | 60 × 45 mm, 4-layer, 0.6 mm thick |
| Wrist strap | Silicone band, adjustable 140–210 mm |
| Weight | ~38 g (incl. battery) |
| Enclosure | FR4 PCB + urethane overmold on skin side |
| IP rating | IP67 (with conformal coating + potted TEC array) |
| Skin interface | Gold-plated thermal contact pads, 0.3 mm protrusion |

---

## 3. Architecture & Block Diagram

```
                         ┌──────────────────────────────────────────┐
                         │              nRF5340 (App Core)           │
                         │  ┌─────────────┐   ┌──────────────────┐  │
   USB-C ──► BQ25570 ──► │  │ Glyph Engine│   │  Thermal PID      │  │
   Solar  ──►  (charge)   │  │  (renderer) │◄─►│  Loop (12-row)    │  │
                         │  └──────┬──────┘   └────────┬─────────┘  │
   500mAh LiPo ◄─────────┤         │                   │            │
   MAX17048 ─────────────┤         │                   │            │
                         │  ┌──────▼──────┐   ┌────────▼─────────┐  │
                         │  │ IMU Gesture │   │  PWM Matrix Ctrl  │  │
                         │  │ Recognizer  │   │  (96-cell drive)  │  │
                         │  └──────┬──────┘   └────────┬─────────┘  │
                         │         │                   │            │
                         │  ┌──────▼──────────────────▼─────────┐  │
                         │  │       nRF5340 (Net Core)          │  │
                         │  │  BLE 5.4 stack ◄──► LoRa SX1262   │  │
                         │  └──────────────────────────────────┘  │
                         └──────────────────────────────────────────┘
                                    │           │           │
                    ┌───────────────┘           │           └──────────────┐
                    ▼                           ▼                          ▼
            ICM-42688-P (IMU)          12× TMP117 (skin temp)       TLC59711 ×4
            I2C @ 1 MHz                I2C bus (shared)             + ADG711
                    │                           │                   row select
                    │                           │                         │
                    └───────────┬───────────────┘                         │
                                ▼                                         ▼
                        Si7051 / LTR-390                          96 × micro-TEC
                        (ambient sense)                           (12×8 thermal array
                                                                    on skin surface)
```

### 3.1 Thermal Drive Matrix

The 96 Peltier cells are driven in a time-multiplexed matrix:

- **Rows (8):** Selected one at a time by ADG711 analog switches; each row
  selects the direction of current (heating vs. cooling) for that row's 12
  cells.
- **Columns (12):** Each column has a TLC59711 PWM current sink. During a
  row's active time slot, the 12 column channels PWM the current to set the
  per-cell intensity (0–255 duty steps).
- **Refresh:** 200 Hz row scan (each row gets 1/8th = 625 µs per frame).
  Within each row slot, 12 channels are PWM-driven at 20 kHz sub-cycle.
- **Safety:** The TMP117 sensors (one per row, mounted at the row's thermal
  contact pad) feed the PID loop. If any row's skin temperature exceeds
  43 °C, that row's driver is immediately disabled (hardware comparator + GPIO
  interrupt).

### 3.2 Glyph Engine

The glyph engine is a frame-buffer renderer that accepts abstract commands and
produces a 12×8 thermal-pixel frame buffer with per-cell target temperatures.

Supported primitives:

1. **Dots / pixels** — set individual cells to warm or cool targets.
2. **Arrows** — 8-directional thermal arrows for navigation.
3. **Characters** — a 5×3 thermal font (A–Z, 0–9) scrolled across the array.
4. **Shapes** — circles, waves, crosshairs, expanding rings (alerts).
5. **Animations** — keyframe-based transitions (e.g., warm wave sweeping left
   to right to indicate "proceed forward").
6. **Continuous bars** — a proportional thermal bar (e.g., depth meter).

The engine runs at 50 Hz, producing a new thermal frame every 20 ms, which the
PID loop tracks at 200 Hz (10 PID updates per frame).

---

## 4. Firmware Details & Design Decisions

### 4.1 Architecture

The firmware uses nRF Connect SDK's dual-core split:

- **App core:** FreeRTOS with 4 tasks:
  - `glyph_task` (priority 4): runs the glyph renderer, processes the command
    queue.
  - `thermal_task` (priority 5, highest): runs the 200 Hz PID loop, reads
    TMP117 sensors, drives the PWM matrix.
  - `imu_task` (priority 3): 200 Hz IMU polling, gesture detection.
  - `comm_task` (priority 2): IPC to net core for BLE/LoRa message RX/TX.

- **Net core:** Zephyr BLE stack + custom LoRa driver (SX1262 via SPI).
  Communicates with app core via nRF53 IPC channels.

### 4.2 Thermal PID Control

Each of the 8 rows has an independent PID controller:

```
T_error = T_target[row] - T_skin[row]
PWM_duty[row][col] = clamp(Kp * T_error + Ki * integral + Kd * derivative, 0, 255)
```

Gains are tuned per-row (Kp=120, Ki=8, Kd=15, in counts/°C) with anti-windup.
The PID runs at 200 Hz; the TMP117 sensors sample at 10 Hz (they settle slowly,
which is fine because skin thermal time constant is ~1–2 seconds).

### 4.3 Safety System

Safety is defense-in-depth:

1. **Software:** PID clamps target to [18, 42] °C. Hardcoded.
2. **Firmware watchdog:** A hardware timer (RTC2) must be kicked by the
   thermal_task every 50 ms. If it expires, all row-select GPIOs are driven
   low (disconnecting all TECs).
3. **Hardware over-temp:** A TLV3691 comparator on each row's TMP117 output
   drives a shared `THERM_FAULT` line. If any row exceeds 44 °C, the line
   asserts and physically cuts power to the TLC59711 drivers via a P-FET.
4. **Thermal fuse:** A 48 °C one-shot thermal fuse in series with the TEC
   power rail.

### 4.4 Gesture Recognition

The IMU gesture recognizer detects:

- **Tap:** sharp acceleration spike (>2 g) followed by damped oscillation,
  duration < 80 ms.
- **Double-tap:** two taps within 400 ms.
- **Flip:** pitch rotation > 90° within 500 ms (palm-up to palm-down).
- **Shake:** sustained lateral acceleration > 1.5 g for > 300 ms.

Gestures map to: tap = acknowledge/clear glyph, double-tap = repeat last
glyph, flip = snooze, shake = emergency SOS.

### 4.5 Communication Protocol

**BLE (phone ↔ device):** Custom GATT service `0x TG 01`:
- `0x TG 01` (write): glyph command (binary packet, see protocol below).
- `0x TG 02` (notify): glyph acknowledgment / gesture event.
- `0x TG 03` (notify): telemetry (battery, temps, IMU).
- `0x TG 04` (write): configuration (PID gains, safety limits, brightness).
- `0x TG 05` (write): OTA DFU control.

**LoRa (buddy/base ↔ device):** Simple binary packets, AES-128 encrypted:

```
[0xTG] [cmd] [len] [payload...] [CRC16]
 cmd=0x01: glyph render
 cmd=0x02: broadcast position
 cmd=0x03: SOS alert
 cmd=0x04: heartbeat
```

### 4.6 Power Management

- **Active:** All subsystems on, thermal rendering at full rate (~35 mA).
- **Idle:** Thermal array off, BLE connected, IMU on standby (~3 mA).
- **Sleep:** Only LoRa CAD (channel activity detection) every 10 s, RTC + IMU
  tap-detect (~0.3 mA). Wake on LoRa preamble, BLE connection event, or IMU
  tap.
- **Solar harvesting:** BQ25570 autonomously tracks MPPT and trickle-charges
  the LiPo. The firmware reads the BQ25570's `VBAT_OK` threshold to decide
  whether to enter a low-power "solar-sustain" mode.

---

## 5. Application / Software Interface

The companion app (React Native + Expo) provides:

1. **Dashboard:** Live thermal array preview (mirrors what the wearer feels),
   battery, solar input, current glyph queue.
2. **Navigation Screen:** Map with route; user taps a direction and the app
   streams thermal arrows to the device. Integration with Mapbox for turn-by-
   turn navigation.
3. **Glyph Composer:** Drag-and-drop thermal pattern editor. Users can draw
   custom glyphs, assign them to contacts or events, and save them as
   templates.
4. **Contacts & Buddies:** Manage LoRa buddy list; send pre-arranged thermal
   codes to other ThermoGlyph devices.
5. **Gesture Settings:** Remap tap/double-tap/flip/shake to custom actions.
6. **Telemetry & Logs:** Historical temperature data, glyph delivery logs,
   battery/solar charts.
7. **OTA Firmware Update:** Push firmware updates to the device over BLE.

### 5.1 Developer API

The app exposes a TypeScript SDK (`ThermoGlyphSDK`) that wraps BLE GATT
operations:

```typescript
import { ThermoGlyphSDK } from './src/services/thermoglyph';

const tg = await ThermoGlyphSDK.connect();

// Render an arrow pointing north
await tg.renderGlyph({ type: 'arrow', direction: 'north', intensity: 0.7 });

// Scroll text
await tg.renderGlyph({ type: 'text', text: 'HELLO', scroll: true });

// Send to a buddy over LoRa
await tg.sendToBuddy('buddy-id-123', { type: 'arrow', direction: 'east' });

// Subscribe to gestures
tg.onGesture((g) => console.log('Wearer gesture:', g));
```

---

## 6. Use Cases & Target Audience

### 6.1 Blind / Low-Vision Navigation

The primary accessibility use case: a blind user wears ThermoGlyph on the
dorsal hand. Their phone's GPS provides turn-by-turn directions, which the app
translates into thermal arrows. Warm = "turn this way," cool = "straight/keep
going." The thermal channel is silent and doesn't interfere with hearing (the
primary sense for blind navigation). Unlike vibration, thermal cues don't
cause sensory adaptation or numbness over hours of use.

### 6.2 Firefighter Situational Awareness

In thick smoke, firefighters can't see their HUDs. ThermoGlyph worn on the
wrist under the glove (with the contact pads on the inner forearm) receives
thermal commands from an incident commander: "exit left," "evacuate now"
(repeated expanding warm ring), "buddy 10 m ahead." LoRa ensures comms work
inside buildings without cellular.

### 6.3 Diver Communication

Scuba divers have limited communication (hand signals only). ThermoGlyph,
worn under a wetsuit sleeve, can receive LoRa acoustic-ultrasound-relayed
messages from a dive boat: "ascend," "stop," "boat is above." The thermal
channel works through neoprene (which is a poor thermal conductor at the
thickness used, but the pads are designed to press firmly).

### 6.4 Silent Tactical Communication

Military/law-enforcement scenarios where radio silence is required:
pre-arranged thermal codes can be sent from a base station via LoRa (very low
RF power, bursty transmission) and received as discrete thermal patterns that
only the wearer perceives. No audible alert, no visible screen glow.

### 6.5 VR / AR Thermal Feedback

In immersive VR, ThermoGlyph adds a thermal channel: touching a virtual ice
surface cools the hand; approaching a virtual fire warms it. The BLE
connection streams thermal events from the VR engine in real time.

### 6.6 Medical: Neuropathy Reminders

Patients with diabetic neuropathy often can't feel vibration reminders but can
feel gentle temperature changes. ThermoGlyph delivers medication reminders as
a slow warm wave — pleasant, non-startling, and noticeable.

---

## 7. Bill of Materials (Key Components)

| Ref | Part | Qty | Notes |
|-----|------|-----|-------|
| U1 | nRF5340-QKAA-AB0-R | 1 | Dual-core MCU |
| U2 | SX1262IMLTTR | 1 | LoRa transceiver |
| U3–U6 | TLC59711PWPR | 4 | 12-ch PWM current sink (column drivers) |
| U7 | ADG711BRUZ | 2 | 4:1 analog switch (row select) |
| U8 | ICM-42688-P | 1 | 6-axis IMU |
| U9–U20 | TMP117AIDRVR | 12 | Digital temp sensor (per-row) |
| U21 | Si7051-A20-GM | 1 | Ambient temp |
| U22 | LTR-390UV-01 | 1 | Ambient light / UV |
| U23 | MAX17048G+T10 | 1 | Fuel gauge |
| U24 | BQ25570RGRR | 1 | Solar harvester |
| TEC array | Custom micro-Peltier 2×2×0.8mm | 96 | Bi₂Te₃ TECs |
| Solar | KXOB25-05X12F | 2 | Amorphous Si cells |
| Battery | 502030 LiPo 500mAh | 1 | 3.7 V |
| Misc | TLV3691, P-FET, passives | — | Safety + power |

---

## 8. Schematic Overview

### Power Subsystem
- USB-C (5 V) → BQ25570 → LiPo (3.7 V) → 3.3 V LDO (TPS702) for MCU/analog
- BQ25570 also manages solar input (2 cells in series, ~1.2 V MPPT)
- TEC power rail: separate 3.6 V direct from LiPo via P-FET (safety cutoff)

### MCU Subsystem
- nRF5340 with 32 MHz crystal, 32.768 kHz RTC crystal
- NFC antenna pins (unused, NC)
- USB-C data lines to D+/D-
- SWD debug header (2×5, 1.27 mm pitch)
- IPC channels to net core (internal)

### Thermal Array Subsystem
- 4 × TLC59711 daisy-chained (48 channels total = 12 cols × 4 segments)
- 2 × ADG711 for 8-row selection (heating/cooling direction per row)
- 96 × micro-TEC cells in 12×8 matrix
- 12 × TMP117 on I2C (addresses 0x48–0x53, via 2 I2C buses for 12 sensors)
- TLV3691 comparators → THERM_FAULT line → P-FET gate

### RF Subsystem
- nRF5340 integrated BLE 5.4 (chip antenna, Johanson 2450AT18B100)
- SX1262 via SPI (SCLK, MOSI, MISO, NSS, BUSY, DIO1) + RF switch (SKY13317)
- LoRa antenna: 868/915 MHz PCB trace antenna

### Sensor Subsystem
- ICM-42688-P via SPI (dedicated, 1 MHz)
- Si7051, LTR-390UV via I2C
- MAX17048 via I2C

---

## 9. PCB Layout Notes

- **Layer stack:** 4-layer (Signal / GND / PWR / Signal), 0.6 mm total
  thickness for flexibility and weight.
- **Thermal array area:** Solid copper pour on all layers under the 96 TECs
  for heat spreading; the skin-side thermal pads are 1.5 mm diameter gold
  plating on 0.3 mm protruding epoxy bumps.
- **RF routing:** BLE antenna keep-out area (no copper, no components within
  5 mm of antenna feed). LoRa trace antenna at board edge.
- **Power isolation:** TEC power rail has its own ground return path to the
  battery negative; analog sensor ground is star-connected to MCU ground at
  a single point.
- **Safety components:** TLV3691 comparators placed adjacent to TMP117
  sensors; P-FET safety cutoff on the TEC rail near the battery connector.

---

## 10. Comparison to Existing Technology

| Feature | ThermoGlyph | Vibrator array | Electro-tactile pin array | Audio bone-conduction |
|---------|-------------|----------------|--------------------------|-----------------------|
| Silent | ✓ | ✗ (motors audible) | ✓ | ✗ |
| Non-fatiguing | ✓ | ✗ (adaptation) | ✗ (pain/pins) | ✓ |
| Through gloves | ✓ (inner forearm) | ✗ | ✗ | ✓ |
| Directional | ✓ (12×8 grid) | limited | ✓ | ✗ |
| Power-efficient | ✓ (~35 mA) | ~80 mA | ~20 mA | ~40 mA |
| Safety | Hardware + firmware | none | skin burn risk | hearing damage risk |

ThermoGlyph is the first wearable device to use a dense thermoelectric array
for information display. The concept is grounded in established
thermoreceptor physiology (warm receptors respond at 30–45 °C, cold receptors
at 20–40 °C, with spatial resolution of ~1 cm on the dorsal hand), but no
commercial product has packaged micro-Peltier cells at this density for
wearable use.

---

## 11. Open-Source Licensing

- **Hardware (KiCad schematics, PCB, BOM):** CERN-OHL-S v2
- **Firmware (C source):** GPL-3.0
- **Companion app (TypeScript/React Native):** MIT

All authored by **jayis1**. Attribution required for derivatives.

---

## 12. Repository Structure

```
thermoglyph/
├── README.md                 ← this file
├── firmware/
│   ├── Makefile              ← build system (GNU Arm Embedded Toolchain)
│   ├── board.h               ← pin assignments, peripheral config
│   ├── registers.h           ← register definitions for nRF5340, SX1262, etc.
│   ├── main.c                ← FreeRTOS init, task creation, startup
│   ├── glyph_engine.c        ← frame buffer renderer, glyph primitives
│   ├── glyph_engine.h        ← glyph engine API
│   ├── thermal_pid.c         ← 8-row PID control loop + safety
│   ├── thermal_pid.h         ← thermal PID API
│   ├── imu_gesture.c         ← IMU polling + gesture detection
│   ├── imu_gesture.h         ← gesture API
│   ├── ble_service.c         ← BLE GATT service implementation
│   ├── ble_service.h         ← BLE API
│   ├── lora_link.c           ← LoRa SX1262 driver + protocol
│   ├── lora_link.h           ← LoRa API
│   ├── power_mgmt.c          ← power states, sleep, solar management
│   ├── power_mgmt.h          ← power management API
│   └── startup_nrf5340.s     ← startup assembly
├── kicad/
│   ├── thermoglyph.kicad_pro  ← KiCad project file
│   ├── thermoglyph.kicad_sch  ← schematic
│   └── thermoglyph.kicad_pcb  ← PCB layout
└── app/
    ├── App.tsx               ← React Native entry point
    ├── package.json          ← dependencies
    ├── tsconfig.json         ← TypeScript config
    └── src/
        ├── types/index.ts    ← shared types
        ├── store/deviceStore.ts ← Zustand state
        ├── services/ThermoGlyphSDK.ts ← BLE SDK
        ├── components/
        │   ├── ThermalPreview.tsx ← live 12×8 thermal array preview
        │   ├── GlyphComposer.tsx  ← pattern editor
        │   └── BatteryWidget.tsx  ← battery + solar gauge
        └── screens/
            ├── DashboardScreen.tsx
            ├── NavigationScreen.tsx
            ├── ComposerScreen.tsx
            ├── BuddiesScreen.tsx
            ├── TelemetryScreen.tsx
            └── SettingsScreen.tsx
```

---

*Designed and authored by jayis1. Copyright (c) 2026 jayis1.*