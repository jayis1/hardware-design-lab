# MusselWatch — Open Bivalve Valvometric Biosensor Network

![MusselWatch](https://img.shields.io/badge/PCB-100×80mm-blue)
![MCU](https://img.shields.io/badge/MCU-STM32L432KC-orange)
![Sensor](https://img.shields.io/badge/Sensor-8×%20DRV5053%20Hall-green)
![Radio](https://img.shields.io/badge/Radio-SX1262%20LoRa%20868-purple)
![Power](https://img.shields.io/badge/Power-Solar%20%2B%20Li--ion-teal)
![Comms](https://img.shields.io/badge/Comms-LoRaWAN%20%2B%20UART-9cf)
![Author](https://img.shields.io/badge/Author-jayis1-orange)
![License](https://img.shields.io/badge/License-CERN--OHL--S%20%2F%20GPL%20%2F%20MIT-yellow)

**Author: jayis1** · **Copyright © 2026 jayis1** · **License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## 1. Purpose & Overview

**MusselWatch** is an open-source, solar-powered, LoRaWAN-capable biosensor node that uses live freshwater bivalves (mussels) as biological early-warning sentinels for water contamination. Each node continuously monitors the shell-gape (valve-opening) behaviour of up to eight individual mussels using a Hall-effect sensor array, and reports anomalies — sustained shell closure, gape freezing, or depressed valve activity — over LoRa to a gateway within minutes of onset. A companion mobile app lets environmental scientists, water-utility operators, and watershed managers visualise network status, drill into per-mussel gape traces, and triage alerts.

### Why mussels?

Freshwater bivalves (order Unionida) are among the most sensitive aquatic organisms known. They filter-feed by opening their shells and rhythmically pumping water across their gills. When they detect toxins, heavy metals, pesticide runoff, sudden pH shifts, or dissolved-oxygen crashes, they **clamp shut within minutes** — a reflexive avoidance response that is far faster than what downstream chemical sensor arrays can detect. A cluster of mussels acting in concert is a reliable, self-calibrating, biologically grounded early-warning system. The valvometry technique has been validated in peer-reviewed limnology literature for decades (e.g. the *Limnoscope* and *Mosselmonitor* systems), but existing commercial valvometers are expensive, closed-source, single-channel, and mains-powered. MusselWatch is the first open-source, multi-channel, wireless, solar-powered valvometric biosensor designed for deployment at scale across watersheds.

### What's new here

| Feature | Existing valvometers | MusselWatch |
|---|---|---|
| Channels per node | 1–2 | **8** |
| Sensing principle | Coil-inductance or optical | **Hall-effect + NdFeB magnet** (contactless, robust, cheap) |
| Communications | Wired RS-485 | **LoRaWAN 868/915 MHz, kilometres of range** |
| Power | Mains or large battery | **Solar + 18650 Li-ion, months of autonomy** |
| On-device analytics | Raw ADC stream only | **Rolling baseline, activity score, clamp/stall anomaly detection** |
| Cost per node | €1500–€4000 | **< €80 in parts** |
| Openness | Closed / proprietary | **Full open hardware + firmware + app** |
| Companion app | None / desktop only | **React Native mobile app** |

---

## 2. Hardware Specifications

### Microcontroller
- **MCU:** STMicroelectronics STM32L432KC (Ultra-low-power ARM Cortex-M4F, 80 MHz, 256 KB flash, 64 KB SRAM, UFQFPN32 / QFN-32 5×5 mm)
- Chosen for its excellent low-power modes (STOP2 at ~ 1 µA), rich analog peripheral set (12-bit ADC with hardware oversampling), and mature bare-metal toolchain support.

### Sensors
| Sensor | Part | Function | Interface |
|---|---|---|---|
| Hall-effect (×8) | Texas Instruments **DRV5053A1** (ratiometric, ±80 G, SOT-23) | Measure shell-gape displacement via NdFeB magnet bonded to one valve | Analog → TMUX1108 → MCU ADC |
| Analog multiplexer | TI **TMUX1108PWR** (8:1, low-leakage, TSSOP-16) | Route one of 8 Hall sensor outputs to the single ADC channel | 3 GPIO (A0/A1/A2) + /EN |
| Magnet | N42 NdFeB disc, Ø3 × 1 mm | Provides the field whose strength varies with gap distance | (mechanical bond) |
| Water temperature | Maxim **DS18B20** (TO-92, 12-bit, −55..+125 °C) | Water temperature for ecological context & heater control | 1-Wire on PB6 |
| Battery voltage | 1:3 resistive divider → ADC channel 9 (PB0) | Monitor Li-ion cell voltage | ADC |
| Solar voltage | 1:3 resistive divider → ADC channel 15 (PB1 alt) | Monitor solar-panel input | ADC |

### Connectivity
- **Radio:** Semtech **SX1262** sub-GHz LoRa transceiver (868 MHz EU / 915 MHz US, +22 dBm max, SF7–SF12, 125/250/500 kHz BW). Bit-banged SPI on PB3/PB4/PB5, NSS on PA8, DIO1 IRQ on PA11, reset on PA12.
- **Debug console:** USART1 on PA9/PA10 (115200 8N1) — used for the `C` (calibrate), `S` (send uplink now), `X` (exit cal) commands and boot diagnostics.
- **I²C bus:** PB8 (SCL) / PB9 (SDA) at 100 kHz — hosts the BQ25870 charger (0x6B) and FM24C64 FRAM event log (0x50).

### Power
- **Solar panel:** 1 W 6 V monocrystalline panel (100 × 60 mm), input via screw terminals to the BQ25870 VBUS pins.
- **Charger / PMIC:** Texas Instruments **BQ25870** (I²C-controlled 1-cell Li-ion buck-boost charger, VQFN-34). Configurable charge current (64 mA steps), power-good output on PB7, shipping-mode support.
- **Battery:** Single 18650 Li-ion cell (3.7 V nominal, 1200–3500 mAh). Battery voltage read through 1:3 divider (2 MΩ / 1 MΩ).
- **Anti-condensation heater:** Small PCB trace or SMD resistor (1–2 W) on PC15, enabled when water temperature < 0 °C and battery > 3.7 V to prevent ice formation on the Hall sensors.
- **Consumption budget:** ~ 250 µA average at 4 Hz sampling, ~ 3 mA during ~ 200 ms LoRa TX every 10 min → months of dark autonomy on a 1200 mAh cell.

### Form factor
- **PCB:** 100 × 80 mm, 2-layer, 1.6 mm FR-4. IP-65 rated enclosure with cable glands for the 8 Hall-sensor pigtail leads (each ~ 0.5–2 m of PVC-jacketed wire to the in-stream mussel cage).
- **Weight (node):** ~ 90 g PCB + battery + enclosure ≈ 250 g.
- **Mussel cage:** Standard 316-stainless mesh cage (off-the-shelf biomonitoring hardware), not part of the PCB design.

### Block diagram

```
                 ┌────────────────────────────────────────────────────────┐
                 │                      STM32L432KC                       │
                 │  (Cortex-M4F 80 MHz, 256 KB flash, 64 KB SRAM)          │
                 │                                                        │
   DS18B20 ─1-Wire─ PB6      PA0/ADC ─ HALL_SIGNAL                         │
                 │                │                                       │
                 │   ┌────────────┴──────────────┐                        │
                 │   │     TMUX1108 8:1 mux       │                        │
                 │   │  A0/A1/A2 = PA4/PA5/PA6    │                        │
                 │   │  /EN      = PA7            │                        │
                 │   └────────────┬──────────────┘                        │
                 │     S1 S2 S3 S4 S5 S6 S7 S8                              │
                 │     │  │  │  │  │  │  │  │                              │
                 │  ┌──┴──┴──┴──┴──┴──┴──┴──┴──┐                          │
                 │  │ DRV5053 Hall sensors x8   │  ← NdFeB magnets on     │
                 │  │ (one per mussel valve)     │    mussel shells         │
                 │  └───────────────────────────┘                          │
                 │                                                        │
                 │   PB0/ADC ─ VBAT divider (1:3)                          │
                 │   PB1/ADC ─ SOLAR divider (1:3)                         │
                 │   PB7     ─ BQ25870 PG (power-good)                      │
                 │   PB8/PB9 ─ I²C: BQ25870 charger + FM24C64 FRAM log     │
                 │                                                        │
                 │   PB3/PB4/PB5 ─ SPI (SCK/MISO/MOSI)                     │
                 │   PA8  ─ SX1262 NSS                                     │
                 │   PA11 ─ SX1262 DIO1 (radio IRQ)                        │
                 │   PA12─ SX1262 RESET                                    │
                 │   PA9/PA10 ─ USART1 debug console                        │
                 │   PC14 ─ status LED                                     │
                 │   PC15 ─ heater enable                                  │
                 └────────────────────────────────────────────────────────┘
                                          │
                          ┌───────────────┴────────────────┐
                          │     SX1262 LoRa 868 MHz         │
                          │     (telemetry & alert uplink)  │
                          └────────────────┬───────────────┘
                                           │  LoRaWAN
                                           ▼
                                   ┌───────────────┐
                                   │   Gateway     │  (TTN / ChirpStack)
                                   │   MQTT → HTTP  │
                                   └───────┬───────┘
                                           │
                                           ▼
                                  ┌─────────────────┐
                                  │  Mobile app      │  (React Native)
                                  │  NetworkDashboard │
                                  │  NodeDetail      │
                                  │  Alerts          │
                                  │  Settings        │
                                  └─────────────────┘
```

---

## 3. Firmware Design

The firmware is written in portable C11 for the GNU Arm Embedded toolchain (`arm-none-eabi-gcc`). It is fully bare-metal — no RTOS, no HAL library — to keep the code self-contained, easy to audit, and to minimise flash/RAM footprint. All peripheral registers are defined in `registers.h` with explicit addresses, making the code readable without vendor headers.

### File layout

```
firmware/
├── main.c              — clock, SysTick, state machine, uplink logic (~ 460 lines)
├── board.h             — pinout, constants, data model (~ 130 lines)
├── registers.h         — STM32L4 peripheral register map (~ 155 lines)
├── Makefile            — GNU Make + arm-none-eabi-gcc
└── drivers/
    ├── adc.h / adc.c           — 12-bit ADC, Vbat/solar dividers, low-power shutdown
    ├── mux.h / mux.c           — TMUX1108 8:1 analog multiplexer driver
    ├── sx1262.h / sx1262.c     — Semtech SX1262 LoRa radio (bit-banged SPI, TX/RX, sleep)
    ├── onewire.h / onewire.c   — DS18B20 1-Wire water temperature
    ├── i2c_pmic.h / i2c_pmic.c — BQ25870 charger + FM24C64 FRAM event log over I²C
    └── gape.h / gape.c         — shell-gape analysis engine (baseline, activity, anomaly)
```

### State machine

The firmware runs a low-duty-cycle super-loop:

1. **SLEEP** — The MCU executes `WFI` (Wait For Interrupt) and is woken every 250 ms by the SysTick interrupt. In a production build, this would be STOP2 with LPTIM1 as the wake source to reduce current to ~ 1 µA between samples.

2. **SAMPLE** (every 250 ms = 4 Hz) — The ADC is powered up, the TMUX1108 is addressed to each of the 8 channels in turn, and the DRV5053 Hall reading is oversampled 4× and averaged. The ADC is then powered down. Each sample takes ~ 20 µs per channel, so a full sweep is ~ 160 µs plus mux settle time.

3. **ANALYSE** — For each channel, `gape_update()`:
   - Converts the raw ADC count to inferred shell opening in micrometres (`gape_raw_to_um`, linear model ~ 0.4 counts/µm over 0–3 mm).
   - Pushes the raw sample into a 64-element ring buffer.
   - Computes an **activity score** (0–100) as the normalised short-term standard deviation of the ring — high activity means rhythmic valve pumping; low activity means stress.
   - Evaluates two anomaly flags:
     - **CLAMP** — gape < 5% of nominal for > 60 s → bit 0x01
     - **GAPE_STALL** — ring stdev < 1 ADC count for > 120 s → bit 0x02

4. **TELEMETRY** (every 15 s) — `update_telemetry()` reads battery voltage, solar voltage, water temperature (DS18B20, blocking 750 ms conversion), charger state (BQ25870 via I²C), active-channel mask, and the maximum anomaly score across all channels. It also controls the anti-condensation heater.

5. **UPLINK** — Two paths:
   - **Periodic** — every 600 s, a 24-byte `PKT_TYPE_TELEMETRY` packet is sent via LoRa (SX1262) with battery/solar/temp/max-anomaly and a CRC32. LoRa SF7/125 kHz at 14 dBm gives ~ 200 ms airtime and kilometres of range.
   - **Alert** — whenever any channel has a non-zero `anomaly_flag` and at least 30 s have passed since the last alert uplink, a `PKT_TYPE_ALERT` packet is sent immediately with the specific channel, anomaly type, gape, and activity score.

6. **CALIBRATION** — When the operator sends a `C` character over the UART console, the next full sample sweep is taken with all shells held closed, and the raw ADC reading for each channel is stored as `raw_baseline`. This baseline is the reference for all subsequent gape calculations.

### Key design decisions

- **Why Hall-effect, not optical or coil?** Optical gape sensors fog up underwater and drift with biofouling. Coil-inductance sensors are bulky and need an oscillator. The DRV5053 + NdFeB magnet pair is contactless (no wear), tiny (SOT-23 + Ø3 mm disc), immune to fouling (the magnet is inside the shell), and costs under €2 per channel. The trade-off is a non-linear B-field-vs-distance relationship, but over the 0–3 mm working range it is approximately linear and is calibrated per-channel at deployment.

- **Why multiplex 8 sensors into one ADC?** The STM32L432 has only 10 external ADC channels and we want to reserve several for Vbat/solar/debug. The TMUX1108 has < 1 pA off-leakage and < 10 Ω on-resistance, so it does not measurably degrade the DRV5053 output. The settle time (< 1 µs) is negligible compared to the ADC conversion time.

- **Why bare-metal C and not an RTOS?** The duty cycle is dominated by sleep (> 99.9% of the time). An RTOS adds flash footprint and scheduler jitter for no benefit. The super-loop + WFI pattern is simpler, more auditable, and lower-power.

- **Why LoRa and not BLE or cellular?** BLE range is ~ 10 m — useless for watershed deployment. Cellular modems draw 100s of mA and require a subscription. LoRaWAN gives kilometres of range at µA-average current and runs on the unlicensed ISM band. The SX1262 is the current state-of-the-art sub-GHz LoRa transceiver and is widely supported by TTN / ChirpStack gateways.

- **Why FRAM for the event log?** The FM24C64 (64 Kbit) provides 8192 bytes of non-volatile storage with unlimited endurance and no write delay, unlike EEPROM. It stores the last 256 alert events (32 bytes each) for post-mortem analysis even if the node loses power.

- **CRC32 on every packet** — The 24-byte uplink packet ends with a IEEE 802.3 CRC32 over the preceding 20 bytes, computed in firmware by `crc32_calc()`. The gateway rejects corrupted packets, preventing false alerts from radio interference.

### Uplink packet format (24 bytes, packed)

| Offset | Field | Type | Meaning |
|---|---|---|---|
| 0 | type | u8 | 1=telemetry, 2=alert, 3=baseline |
| 1 | node_id | u8 | Low byte of MCU UID64 XOR |
| 2 | seq | u8 | Sequence number (wraps at 256) |
| 3 | flags | u8 | bit0=heater, bit1=low_batt, bit2=alert |
| 4 | battery_mv | u16 | Battery voltage in mV |
| 6 | solar_mv | u16 | Solar panel voltage in mV |
| 8 | water_temp_c10 | i16 | Water temperature ×10 °C |
| 10 | active_channels | u8 | Bitmask of populated channels |
| 11 | max_anomaly | u8 | Highest anomaly score this epoch (0–100) |
| 12 | channel | u8 | For alert packets: which channel (0xFF otherwise) |
| 13 | anomaly_flag | u8 | bit0=clamp, bit1=stall |
| 14 | gape_um | u16 | Shell opening in µm (alert packets) |
| 16 | activity_score | u8 | 0–100 instantaneous variability |
| 17 | reserved | u8 | 0 |
| 18 | uptime_s | u32 | Seconds since boot |
| 22 | crc32 | u32 | IEEE 802.3 CRC32 over bytes 0–19 |

---

## 4. Application / Software Interface

The companion app is a **React Native (Expo)** mobile application targeting iOS and Android. It communicates with a LoRaWAN gateway (The Things Network or ChirpStack) via an HTTP integration that exposes node telemetry and alerts as JSON. When no gateway is reachable, the app falls back to a built-in mock dataset so it can be demoed without any hardware.

### Screens

1. **NetworkDashboard** — A FlatList of all deployed nodes, each showing label, river/reach, species, battery %, solar voltage, water temperature, charger state, channel count, max anomaly score, uptime, and last-seen age. A colour-coded badge (nominal/advisory/warning/critical) gives an at-a-glance triage view. Tapping a node navigates to NodeDetail.

2. **NodeDetail** — Full per-node view with:
   - Complete telemetry grid (battery, solar, water temp, charger, uptime, seq, max anomaly, last seen).
   - A grid of channel selector pills (1–8), coloured by anomaly severity. Tapping a channel shows its gape, activity score, anomaly label, and score.
   - A 4-hour gape trace (LineChart from `react-native-chart-kit`) for the selected channel, showing the diurnal rhythm of healthy mussel valve activity.
   - A **Calibrate Baseline** button that sends a downlink command to the node instructing it to capture the next sample sweep as the closed-shell baseline.

3. **Alerts** — A chronological list of anomaly events from all nodes, sorted newest-first. Each alert card shows level (advisory/warning/critical), age, node ID, channel, anomaly type (clamp/stall), a free-text note, gape, activity score, water temperature, and acknowledge state. Open alerts have an Acknowledge button that prompts for an optional operator note and POSTs to the gateway.

4. **Settings** — Gateway URL, poll interval, alert threshold, temperature unit (°C/°F), dark-mode toggle, and a Reset-to-Defaults button. The About section explains the ecological rationale and licensing.

### Data model

The app's TypeScript types (`types.ts`) mirror the firmware's `board.h` structures: `ChannelState`, `Telemetry`, `AlertEvent`, `Node`, `AppConfig`. Shared helpers (`anomalyLabel`, `alertLevelFromScore`, `chargerStateLabel`, `formatTemp`, `formatUptime`, `batteryPct`) ensure consistent formatting across screens.

### Gateway API

The `MusselWatchClient` class in `api.ts` implements:
- `GET /nodes` → `Node[]`
- `GET /alerts` → `AlertEvent[]`
- `POST /alerts/:id/ack` → acknowledge
- `GET /nodes/:id/channels/:c/history?hours=N` → `number[]` (gape µm per minute)
- `POST /nodes/:id/label` → set node label
- `POST /nodes/:id/calibrate` → trigger baseline calibration downlink

---

## 5. Use Cases & Target Audience

### Use case 1: Drinking-water intake protection
A water utility deploys MusselWatch nodes upstream of its river intake. Mussels filter the raw river water and pump rhythmically under normal conditions. If a tanker spill, agricultural runoff, or industrial discharge event introduces toxins upstream, the mussels clamp shut within minutes — before the plume reaches the intake. The LoRa alert triggers a diversion of the intake to a backup reservoir, preventing contaminated water from entering the treatment plant.

### Use case 2: Wastewater treatment plant compliance
Nodes deployed just upstream and just downstream of a treatment plant outfall provide a differential signal. If downstream mussels show anomalous behaviour while upstream mussels do not, the plant's effluent is the likely cause — supporting regulatory investigation and auto-sampling.

### Use case 3: Ecological research & biodiversity monitoring
Limnologists and freshwater ecologists use MusselWatch to study Unionid behaviour in the wild — diurnal gape rhythms, spawning cues, seasonal activity patterns, and responses to flow regime changes. The 4 Hz, 8-channel, months-long time series is a richer dataset than any single-channel commercial valvometer can provide, and the per-node cost is low enough to instrument an entire river system.

### Use case 4: Industrial discharge monitoring
Factories, mines, and chemical plants can deploy MusselWatch nodes at their outfalls as a continuous biomonitors. Many jurisdictions already mandate "fish biomonitoring" (e.g. the US EPA's continuous fish-lethality monitors); mussels are a cheaper, more sensitive, and less ethically fraught alternative.

### Use case 5: Citizen-science watershed watch
Local river trusts and community science groups can build and deploy MusselWatch nodes from the open designs for a few hundred euros. The mobile app lets volunteers see their network in real time and receive push alerts when water quality may be degraded.

### Target audience
- **Water-utility operators** needing early-warning intake protection
- **Environmental regulators** monitoring compliance and enforcing discharge limits
- **Academic researchers** in limnology, ecotoxicology, and freshwater ecology
- **Industrial environmental, health, and safety (EHS) teams** at plants with aqueous discharges
- **Citizen-science watershed organisations** and river trusts
- **Makers and hobbyists** interested in novel, ecologically meaningful open hardware

---

## 6. Deployment & Calibration Procedure

1. **Assemble the PCB** per the KiCad design files. Solder the STM32, SX1262, DRV5053 sensors, TMUX1108 mux, DS18B20, BQ25870, and FM24C64. Attach the status LED, heater resistor, and screw terminals for the solar panel and Hall-sensor pigtails.

2. **Flash the firmware** via SWD (ST-Link) or the UART bootloader. Connect a USB-TTL adapter to PA9/PA10 at 115200 baud. On boot you will see:
   ```
   === MusselWatch v1.0 (c) jayis1 ===
   Bivalve valvometric biosensor node
   Node ID: 0xA3
   INIT: LoRa OK
   RUN: entering main loop
   ```

3. **Mount the sensors.** Bond a Ø3 × 1 mm N42 NdFeB magnet to one valve of each mussel using underwater-curing epoxy. Fix the DRV5053 Hall sensor to the other valve (or to the cage adjacent to the mussel) so that the sensor-to-magnet gap is ~ 2 mm at full closure. Run the sensor pigtail wires back to the node enclosure.

4. **Calibrate.** Hold all mussel shells gently closed and send the `C` command over UART. The node captures the per-channel ADC reading as `raw_baseline` and begins computing gape relative to that reference. Confirm with `X`.

5. **Register with a LoRaWAN gateway.** Create a device in TTN/ChirpStack with the node's DevEUI (derived from the STM32 UID64), AppEUI/AppKey, and the frequency plan (EU868 or US915). Point the gateway's HTTP integration at your app backend. The mobile app's Settings screen lets you enter the gateway URL.

6. **Monitor.** Open the MusselWatch app. The NetworkDashboard shows your node(s) with live telemetry. Gape traces, alerts, and calibration are all accessible from the UI.

---

## 7. Bill of Materials ( indicative )

| Ref | Part | Qty | Unit cost (€) |
|---|---|---|---|
| U1 | STM32L432KCUx | 1 | 6.50 |
| U2 | Semtech SX1262 | 1 | 9.00 |
| U3 | DRV5053A1 (SOT-23) | 8 | 1.20 |
| U4 | TMUX1108PWR | 1 | 1.80 |
| U5 | DS18B20 (TO-92) | 1 | 1.50 |
| U6 | BQ25870 | 1 | 4.20 |
| U7 | FM24C64B (SOIC-8) | 1 | 1.80 |
| — | N42 NdFeB Ø3×1 mm magnet | 8 | 0.20 |
| — | 18650 Li-ion 1200 mAh | 1 | 4.00 |
| — | 1 W 6 V solar panel | 1 | 4.50 |
| — | Passives (R, C, L, crystal) | — | 2.00 |
| — | PCB (100×80 mm, 2-layer) | 1 | 3.00 |
| — | Enclosure + cable glands | 1 | 6.00 |
| **Total** | | | **~ 55** |

At modest volume (100 nodes) the per-node cost drops below €40.

---

## 8. Licensing & Attribution

- **Hardware** (KiCad schematic, PCB, project): **CERN-OHL-S v2** — open-source hardware license. You may manufacture, modify, and distribute the hardware, but derivative hardware must also be open-source under the same terms.
- **Firmware** (all C code under `firmware/`): **GPL-2.0** — you may use, modify, and distribute, but derivative firmware must also be GPL-2.0 and source must be provided.
- **Companion app** (all TypeScript/React under `app/`): **MIT** — the most permissive practical license, to encourage integration into third-party platforms.

**Author: jayis1** — all designs, firmware, code, and documentation are the original work of jayis1. Copyright © 2026 jayis1. All rights reserved within the terms of the above licenses.

---

## 9. Repository Layout

```
musselwatch/
├── README.md                 — this file
├── firmware/
│   ├── main.c                 — main loop, state machine, uplink
│   ├── board.h                — pinout & data model
│   ├── registers.h            — STM32L4 peripheral map
│   ├── Makefile               — GNU Make build
│   └── drivers/
│       ├── adc.h / adc.c
│       ├── mux.h / mux.c
│       ├── sx1262.h / sx1262.c
│       ├── onewire.h / onewire.c
│       ├── i2c_pmic.h / i2c_pmic.c
│       └── gape.h / gape.c
├── kicad/
│   ├── device.kicad_pro       — KiCad project file
│   ├── device.kicad_sch       — schematic (symbols + netlist)
│   └── device.kicad_pcb       — PCB layout (footprints + routing)
└── app/
    ├── package.json
    ├── app.json               — Expo config
    ├── tsconfig.json
    └── src/
        ├── App.tsx            — navigation entry
        ├── types.ts           — shared types & helpers
        ├── api.ts             — gateway client
        └── screens/
            ├── NetworkDashboard.tsx
            ├── NodeDetail.tsx
            ├── Alerts.tsx
            └── Settings.tsx
```

---

*MusselWatch — letting nature's own sentinels guard our water. Open hardware by jayis1.*