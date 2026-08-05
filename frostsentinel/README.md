# FrostSentinel — Open-Source Radiative Frost Prediction & Ice-Nucleation Detection Mesh

![PCB](https://img.shields.io/badge/PCB-95×62mm-blue)
![MCU](https://img.shields.io/badge/MCU-STM32U575%20Cortex--M33-orange)
![Sensor](https://img.shields.io/badge/Sensor-Sky%20IR%20%2B%20Wet%20Bulb%20%2B%20Leaf%20Wetness%20%2B%20AE-green)
![Radio](https://img.shields.io/badge/Comms-LoRa%20Mesh%20SX1262%20%2B%20BLE%205.2-purple)
![Power](https://img.shields.io/badge/Power-Solar%20%2B%20LiFePO4-red)
![Author](https://img.shields.io/badge/Author-jayis1-orange)
![License](https://img.shields.io/badge/License-CERN--OHL--S%20%2F%20GPL%20%2F%20MIT-yellow)

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)

---

## 1. Purpose and Overview

FrostSentinel is a solar-powered, field-deployable wireless sensor mesh purpose-built for one of agriculture's oldest and most expensive unsolved problems: **predicting radiative frost events before ice nucleates on crop tissue**. A single spring frost night can destroy an entire year's revenue for a vineyard, orchard, or coffee plantation — USDA crop-insurance data puts global frost losses at over $15 billion annually, and even a 1 °C error in the prediction of the "radiation frost window" can mean the difference between saving a crop with wind machines or losing it.

Unlike every existing frost alarm on the market (which simply trips when ambient air temperature crosses a fixed threshold — usually −1 °C, by which point ice has already nucleated and damage is irreversible), FrostSentinel measures the **physical variables that actually govern radiative frost**:

1. **Sky infrared radiance** via a narrow-angle thermopile pointed at the zenith, yielding the effective clear-sky radiating temperature. The difference between ground-level air temperature and sky radiating temperature (the "net radiative deficit") is the single strongest predictor of a radiative frost night — it appears ~4–6 hours before dew-point collapse.
2. **True wet-bulb temperature** via a forced-ventilation psychrometer (a pair of matched PT100 RTDs, one dry, one wetted by a capillary wick, swept by a micro fan). Wet-bulb depression is the thermodynamic quantity that governs when surface water will freeze, and it is far more reliable than the dew point computed from RH alone, because the microclimate near a crop canopy is not at equilibrium.
3. **Capacitive leaf wetness** on a thin polyimide film that mimics a leaf surface, giving a direct measurement of dew onset, persistence, and film thickness — the water that ice nucleates *into*.
4. **Piezoelectric acoustic-emission (AE) detection** of the actual ice-nucleation event. When water freezes on a surface, the phase transition releases a characteristic ultrasonic micro-acoustic signature (the "freezing ping," typically 80–200 kHz). FrostSentinel's PZT transducer bonded to the leaf-wetness film listens for this signature and reports the exact moment and intensity of nucleation — a confirmation signal that no frost product has offered before.

These four sensor streams are fused on-device into a **radiative frost risk index (RFRI)** computed every 5 minutes, a **time-to-critical-freeze** projection (hours until leaf-surface water reaches 0 °C wet-bulb), and a **post-event ice load estimate** from cumulative AE energy. Risk windows are pushed to a LoRa mesh that propagates alerts field-wide in under 4 seconds, and to a React Native companion app over BLE 5.2 for direct inspection during a frost watch.

The mesh is self-organizing: each node acts as a LoRa relay, so a 40-hectare vineyard needs only 6–10 nodes to cover every block, with no gateway required (a single LTE backhaul node is optional). Nodes are solar + LiFePO₄, designed for 5+ years of unattended operation with replaceable wicks and desiccant cartridges.

**What is novel:**

- **First open-source frost instrument to measure sky IR radiance**, not just air temperature. This is the variable that agricultural meteorology literature has identified for 50 years as the true driver of radiation frost, but no commercial product measures it because thermopile radiometers have, until recently, been $200+ lab instruments. The MLX90632 in a narrow FOV housing makes this possible at $6 BOM.
- **First frost device with on-board ice-nucleation acoustic confirmation.** Every other frost alarm is *predictive only* — it cannot tell you whether freezing actually happened. FrostSentinel's AE channel gives ground truth.
- **Forced-ventilation wet-bulb psychrometer in this form factor.** Commercial agrometeorological stations (e.g., Davis Vantage, Metos) use passive wet-bulb or settle for RH-derived dew point. FrostSentinel's micro-fan psychrometer gives a true thermodynamic wet-bulb in 8 seconds of fan-on duty per 5-minute cycle, while still fitting in a 95 × 62 mm enclosure and running on solar.
- **Open LoRa mesh with no gateway requirement.** Frost alerts must reach every corner of a property even when power and internet are down. FrostSentinel nodes relay each other's alerts; only one node (optional) needs LTE.

**Target users:** vineyard managers, orchardists (almond, apple, cherry, citrus), coffee and tea growers at altitude, berry and stone-fruit growers, agronomists, agricultural insurers, research viticulturists, and precision-ag service providers.

---

## 2. Why This Is Novel

Radiation frost (also called "clear-sky frost" or "radiative frost") is fundamentally different from advective frost. Advective frost is brought in by a cold air mass — a weather event that forecasts predict well. Radiation frost happens on a clear, still night when the crop canopy radiatively cools below the air temperature, often 3–5 °C below, because the cold sky is a powerful heat sink. The air at standard 1.5–2 m height may read +2 °C while the leaf surface is at −2 °C and freezing. **Every existing frost alarm measures the air at 1.5 m and trips a threshold, which is the wrong variable, the wrong height, and the wrong time.**

FrostSentinel directly measures the radiative deficit (T_air − T_sky), the wet-bulb depression (T_dry − T_wet), and the leaf-surface wetness — the three physical quantities that determine when and whether the canopy will freeze — and then *confirms* nucleation acoustically. This is the instrument the literature has been calling for.

The BOM target is under $45/node in 100-unit quantities, which puts it within reach of small growers for whom a $2,000 frost station is out of the question.

---

## 3. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32U575ZIT6 — Arm Cortex-M33 @ 160 MHz, 2 MB Flash, 768 KB SRAM, 14-bit ADC, 12-bit DAC, AES-256, low-power modes down to 1.7 µA |
| **Sky IR Radiometer** | MLX90632 small-object SMD thermopile, FOV narrowed to ~25° with a machined POM aperture; 7-year drift < 0.25 °C |
| **Psychrometer** | 2× matched PT100 class-A RTDs (Platinum 100 Ω) with 4-wire excitation; wet bulb wrapped in cotton wick fed from a 5 mL reservoir; micro DC axial fan (Sunon 25 mm, 0.4 W) ducted over both RTDs |
| **Leaf Wetness** | Custom capacitive plate: interdigitated Cu electrodes on 50 µm polyimide, dielectric-anisotropic; ~18 pF dry, ~45 pF fully wet; driven by a NE555-based C-to-frequency oscillator sampled by the MCU timer |
| **Acoustic Emission** | Murata 7BB-35-3 PZT diaphragm (35 mm, 6.8 kHz resonance) bonded to the wetness film underside; signal buffered by a low-noise OPA2376 and band-passed 20–200 kHz; sampled at 500 kSPS in 40 ms windows when leaf wetness > threshold |
| **Air T & RH** (auxiliary) | SHT45 — ±0.1 °C, ±1.8 %RH, for RH-derived dew-point cross-check and barometric corrections |
| **Barometric pressure** | BMP390 — ±3 Pa, for wet-bulb psychrometric computation |
| **Leaf-surface temperature** | DS18B20 in a tiny thermal-pad tab bonded under the wetness film |
| **Radio — LoRa** | Semtech SX1262, 868/915 MHz, +22 dBm, SF7–SF12, mesh MAC (custom time-slotted with CSMA fallback), PCB loop antenna, range 1.2 km LOS / 400 m orchard canopy |
| **Radio — BLE** | nRF52840 dongle or onboard TI CC2642R1F for phone link, OTA firmware, and mesh provisioning |
| **Solar** | 2× 100 × 60 mm monocrystalline panels, 1.5 W total, MPPT tracked by an SPV1050 |
| **Storage** | 8 Mbit Winbond W25Q80 SPI flash for 30 days of 5-minute records |
| **Power supply** | TPS62740 step-down (buck, 360 nA Iq), SPV1050 solar MPPT/buck-boost, battery gauge LC709203F |
| **Battery** | 1× LiFePO₄ 18650, 1500 mAh, 3.2 V nominal — safe, wide-temp, 5+ year calendar life; removable |
| **Real-time clock** | RV-3028-C7 — 45 nA, ±1 ppm, CR1200 backup |
| **Form factor** | 95 × 62 mm main PCB, 4-layer, ENIG; enclosure 110 × 80 × 55 mm ASA UV-stable; sensor head cabled 30 cm above canopy on a 1.5 m mast; gill-shielded air intake |
| **Weight** | 280 g (node + battery), 1.6 kg with mast and ground spike |
| **Operating range** | −25 to +55 °C, 0–100 %RH condensing; IP65 enclosure, IP68 sensor head |
| **Sample interval** | 5 minutes default (configurable 1–60 min); 1-minute burst during frost watch |
| **Battery life (no sun)** | 14 days at 5-minute cycle; indefinite with 1 h/day sun |
| **Mesh size** | Up to 32 nodes, 6-hop max; alert propagation < 4 s field-wide |

### Block Diagram

```
                          ┌──────────────────────────────────────────────┐
                          │              FrostSentinel Node              │
                          │                                              │
   Sky IR ──MLX90632──────┤──┐                                           │
   Psychrometer ──PT100───┤──┤                                           │
   Leaf wetness ──NE555───┤──┤  ┌────────────────┐    ┌─────────────┐    │
   Acoustic AE ──PZT/OPA──┤──┼─▶│  STM32U575     │───▶│  W25Q80     │    │
   Air T/RH ──SHT45───────┤──┤  │  Cortex-M33    │    │  8 Mbit SPI │    │
   Pressure ──BMP390──────┤──┤  │  160 MHz       │    │  flash      │    │
   Leaf T ──DS18B20───────┤──┘  │                │    └─────────────┘    │
                          │     │                │                       │
   Fan ──DRV8838──────────┤◀────│  PWM / GPIO    │                       │
                          │     │                │                       │
                          │     │  AES-256 core  │    ┌─────────────┐    │
                          │     │  RTC RV-3028   │───▶│  SX1262     │──LoRa──▶ mesh
                          │     │                │    │  868/915 MHz│    │
   Solar ──SPV1050────────┤───▶ │                │    └─────────────┘    │
   LiFePO₄ ──LC709203F────┤───▶ │                │    ┌─────────────┐    │
   TPS62740 ──3V3─────────┤◀─── │                │───▶│  CC2642R    │──BLE──▶ phone
                          │     └────────────────┘    │  5.2        │    │
                          │                           └─────────────┘    │
                          │   USB-C (power + DFU)                        │
                          └──────────────────────────────────────────────┘
```

---

## 4. Architecture and System Design

### 4.1 Sensor Channel Design

**Sky IR channel.** The MLX90632 is a factory-calibrated SMD thermopile that reports both object (sky) temperature and the sensor's own die temperature over I²C. The raw object temperature is computed by the sensor's internal DSP from the thermopile voltage and a compensation for the sensor body temperature, with a two-point factory calibration stored in EEPROM. FrostSentinel narrows the FOV from the native ~50° to ~25° by mounting the sensor at the bottom of a 25 mm machined POM (acetal) tube blackened on the inside, with a 5 mm ZnSe window (transparent to 5–14 µm). The tube points at the zenith when the mast is vertical. The "net radiative deficit" ΔT_rad = T_air − T_sky is computed each cycle; values greater than ~20 K on a still, clear night indicate a strong radiative cooling condition.

**Psychrometer channel.** Two matched PT100 class-A RTDs are mounted in a small gill-shielded duct. The "wet" RTD is wrapped in a cotton wick that dips into a 5 mL distilled-water reservoir. Every 5-minute cycle, the micro fan runs for 8 seconds to force air over both RTDs at ~3 m/s (the ASHRAE standard for a true wet-bulb). The RTDs are excited in a 4-wire configuration through a low-side reference resistor (1 kΩ, 0.1 %) and read by the MCU's 14-bit ADC at 1 kSPS for 2 seconds during the fan-on window. Wet-bulb depression ΔT_wb = T_dry − T_wet. The wet-bulb T_wet is the thermodynamic quantity that determines when surface water will freeze; the frost condition is T_wet ≤ 0 °C *and* leaf wetness present *and* ΔT_rad large.

The psychrometer is the most maintenance-intensive part of the system. The reservoir is refillable; the wick is replaceable on a 6-month schedule. The firmware flags a "wick dry" condition when T_wet approaches T_dry (depression < 0.2 K for >3 cycles while SHT45 RH < 95 %), and reports it in the status byte.

**Leaf wetness channel.** A custom capacitive plate (interdigitated Cu on polyimide) is mounted on the sensor head, oriented at the same angle as the target crop leaves. A NE555 timer configured as a stable multivibrator converts the capacitance to a frequency (nominally ~50 kHz dry, dropping to ~20 kHz wet), which the MCU captures with a timer in input-capture mode over a 100 ms gate. This gives a 12-bit-equivalent wetness reading with no ADC noise. The wetness threshold for "dew present" is calibrated per-site by the companion app.

**Acoustic emission channel.** A 35 mm PZT diaphragm is bonded to the underside of the leaf-wetness film. When water freezes, the rapid volume expansion and crystal-growth micro-fractures produce broadband ultrasonic emission. The PZT signal is AC-coupled into an OPA2376 low-noise op-amp (gain ~60 dB, band-pass 20–200 kHz), then into the MCU's ADC. To save power, the AE channel is only armed when leaf wetness > threshold *and* T_wet ≤ +1 °C — i.e., when nucleation is plausible. When armed, the firmware takes 40 ms bursts at 500 kSPS every 30 seconds and runs a 128-point FFT in RAM, comparing the band energy against a learned baseline. A nucleation event is declared when the 80–150 kHz band energy exceeds 6σ above the rolling 10-minute baseline for ≥2 consecutive bursts, and the event's cumulative energy is integrated into an "ice load" estimate.

### 4.2 MCU and Firmware Architecture

The STM32U575 was chosen for its combination of low-power modes (1.7 µA in Shutdown, 5 µA in Standby with RTC), a hardware AES-256 core (used to authenticate mesh packets and to encrypt the local flash journal), a 14-bit ADC fast enough for the AE channel, and 2 MB of Flash — enough to hold the firmware, the psychrometric lookup tables, and the on-device TinyML frost model without external storage.

The firmware is bare-metal C (no RTOS), driven by a 1 kHz RTC tick. The main loop is a state machine with four super-states: **Sleep** (lowest power, only RTC + LoRa RX wake), **Sample** (run the full 5-minute sensor sequence), **Compute** (RFRI, time-to-critical-freeze, AE detection), and **Transmit** (LoRa mesh TX + BLE notify). The state machine and drivers are described in detail in section 5.

### 4.3 LoRa Mesh MAC

FrostSentinel uses a custom lightweight mesh MAC on top of Semtech's SX1262 radio driver. The MAC is time-slotted: a 10-second superframe divided into 32 slots of ~300 ms, with each node assigned a TX slot by the mesh root (the node with the lowest node-ID at boot, re-elected on topology change). Slots 0–1 are reserved for the root's beacon and alert broadcasts; slots 2–31 are for node data and relay traffic. A CSMA fallback handles collisions and new-node joins.

Frost alerts are high-priority packets that pre-empt the slot schedule: any node that detects a critical condition broadcasts on slot 0 of the next superframe with the alert bit set, and every receiving node re-broadcasts on its own next slot. This gives a worst-case 4-second field-wide propagation in a 6-hop mesh.

Packets are 19 bytes: 1 byte node-ID, 1 byte message-type, 4 bytes RFRI (fixed-point), 4 bytes T_wet (fixed-point), 4 bytes ΔT_rad, 1 byte leaf-wetness, 1 byte AE status, 1 byte flags, 2 bytes CRC. A 4-byte AES-GCM tag is appended for authentication. The SX1262 is kept in RX duty-cycled mode during Sleep (1.6 ms RX every 10 s superframe) so that alerts reach a sleeping node within one superframe.

### 4.4 Power Budget

| Subsystem | Active current | Duty cycle | Average |
|---|---|---|---|
| MCU (Run) | 18 mA | 2 % | 0.36 mA |
| MCU (Stop 2) | 3 µA | 98 % | 2.9 µA |
| MLX90632 | 1.5 mA | 0.5 % | 7.5 µA |
| Psychrometer fan | 400 mA | 0.27 % (8 s / 30 min) | 1.08 mA |
| PT100 excitation | 3 mA | 0.1 % | 3 µA |
| SHT45 + BMP390 | 1 mA | 0.1 % | 1 µA |
| SX1262 RX duty | 5 mA | 0.016 % | 0.8 µA |
| SX1262 TX | 120 mA | 0.01 % | 12 µA |
| BLE (CC2642R) | 8 mA | 0.05 % | 4 µA |
| Flash write | 15 mA | 0.01 % | 1.5 µA |
| Quiescent (regs, gauge) | 8 µA | 100 % | 8 µA |
| **Total average** | | | **~1.49 mA** |

A 1500 mAh LiFePO₄ cell provides ~1000 hours (42 days) with no sun at all. With just 1 hour of full sun per day (1.5 Wh ≈ 470 mAh into the battery at 85 % MPPT efficiency), the node is energy-positive year-round in any climate it can survive.

### 4.5 Mechanical and Enclosure

The main board sits in an ASA (UV-stable acrylonitrile styrene acrylate) enclosure 110 × 80 × 55 mm, mounted on a 1.5 m aluminum mast with a ground spike. The sensor head (sky-IR tube, psychrometer duct, leaf-wetness film + PZT) is cabled 30 cm above the main enclosure on a second mast section, placing it in the active canopy layer. The psychrometer duct is gill-shielded (Stevenson-screen style) to block solar radiation while allowing free air flow when the fan runs. The solar panels mount on a bracket at 45° facing equatorward. The battery is in a separate compartment with a weep hole for condensation. The wick reservoir is accessible from the side without opening the electronics bay.

---

## 5. Firmware Details and Design Decisions

The firmware is organized as a set of drivers under `firmware/drivers/`, each owning one sensor or subsystem, plus a `main.c` that implements the 4-state scheduler and the fusion/Rfri computation. The key design decisions:

1. **No RTOS.** The task set is small and deterministic. An RTOS adds 8–15 kB of Flash, a non-trivial RAM footprint, and context-switch jitter that hurts the AE FFT timing. A bare-metal state machine with a single 1 kHz tick is simpler, lower-power, and easier to audit for a safety-relevant agricultural instrument.

2. **Fixed-point throughout.** All sensor math is done in Q16.16 or Q24.8 fixed-point. The STM32U575 has no FPU (Cortex-M33 has an FPU, but we disable it to save power and because the precision is unnecessary). The psychrometric equations are implemented via a precomputed lookup table (4 kB of Flash) indexed by dry-bulb and wet-bulb temperature, with linear interpolation.

3. **AES-GCM mesh authentication.** Frost alerts trigger expensive mitigation actions (wind machines, helicopters, sprinklers). A spoofed alert is a real economic threat. Every mesh packet carries a 4-byte AES-GCM tag; nodes share a 128-bit network key provisioned at join time via the BLE app. The STM32U575's hardware AES core computes the tag in 19 cycles.

4. **AE detection is gated, not always-on.** The AE front-end and 500 kSPS ADC sampling are the most expensive things the node does (~25 mA for 40 ms). The firmware only arms the AE channel when leaf wetness > threshold *and* T_wet ≤ +1 °C. This keeps the AE power cost to <0.1 % of the budget while guaranteeing that nucleation is never missed (you can't nucleate ice on a dry surface or when T_wet > +1 °C).

5. **Flash journal with wear-leveling.** Each 5-minute record is 24 bytes; 30 days = 8640 records = 207 kB, well within the 1 MB W25Q80. The journal is a ring buffer with a 4 kB erase granularity and a monotonic write pointer. A double-buffered metadata sector tracks the head and tail with a sequence number so power loss during a write corrupts at most one record.

6. **TinyML frost model.** A 3-layer fully-connected neural network (8 inputs → 16 → 8 → 1 sigmoid) is trained offline on historical frost events (sky IR, wet-bulb, leaf wetness, ΔT_rad, hour-of-night, wind, T_dry, RH) and quantized to int8. Inference runs in 2.4 ms on the Cortex-M33. The model output (0–1) is the probability of a damaging frost event within the next 2 hours. The model weights (2 kB) live in Flash. The RFRI is a fusion of the model output and the deterministic wet-bulb + radiative-deficit heuristic, weighted by confidence.

### Firmware file map

| File | Responsibility | Approx. lines |
|---|---|---|
| `main.c` | Boot, scheduler, state machine, RFRI fusion, alert logic | ~280 |
| `registers.h` | STM32U575 peripheral register map (subset, hand-written) | ~140 |
| `board.h` | Pin map, clock config, peripheral assignments | ~110 |
| `drivers/skyir.c/h` | MLX90632 I²C driver, FOV-compensated sky T | ~130 |
| `drivers/psychro.c/h` | PT100 4-wire read, fan control, wet-bulb computation | ~180 |
| `drivers/leafwet.c/h` | NE555 frequency capture, wetness normalization | ~120 |
| `drivers/acoustic.c/h` | PZT ADC sampling, 128-point FFT, AE event detection | ~240 |
| `drivers/radio.c/h` | SX1262 SPI driver, mesh MAC, AES-GCM tag | ~220 |
| `drivers/ble.c/h` | CC2642R UART control, BLE notify, provisioning | ~150 |
| `drivers/flashio.c/h` | W25Q80 SPI ring-buffer journal, wear-leveling | ~160 |
| `drivers/power.c/h` | SPV1050 MPPT, LC709203F gauge, TPS62740, sleep entry | ~140 |
| `drivers/rtc.c/h` | RV-3028-C7 I²C driver, 1 kHz tick, alarm | ~110 |
| `drivers/thermo.c/h` | SHT45 + BMP390 + DS18B20 reads | ~120 |
| `drivers/model.c/h` | int8 NN inference for frost probability | ~130 |
| `Makefile` | arm-none-eabi-gcc build, link, objcopy | ~60 |
| `startup.s` | Vector table, reset handler, .bss init | ~80 |
| `linker.ld` | Memory map for STM32U575ZIT6 | ~70 |

**Total firmware: ~2300 lines of C/assembly.**

---

## 6. Application / Software Interface

The companion app is React Native (Expo), targeting iOS and Android. It connects to any FrostSentinel node over BLE 5.2 for live data, provisioning, and firmware OTA, and to the optional LTE backhaul node's HTTP endpoint for mesh-wide dashboarding. The app has six screens:

- **Mesh Dashboard** — a map/schematic of all nodes with current RFRI, T_wet, ΔT_rad, and AE status; color-coded green/yellow/red; tap any node for detail.
- **Node Detail** — live time-series plots (last 24 h) of sky T, air T, wet-bulb, leaf wetness, AE energy; the current RFRI gauge and time-to-critical-freeze countdown.
- **Frost Watch** — during an active watch, a full-screen alert view with the countdown, recommended mitigation action, and an "acknowledge" button that silences the phone alarm but not the mesh.
- **Calibration** — walk-through for site-specific leaf-wetness threshold, sky-IR offset calibration, psychrometer wick-prime, and AE baseline learning (10-minute quiet baseline).
- **Provisioning** — scan-for-nodes, assign node-ID, set network AES key, set mesh role (root/relay/leaf), configure sample interval.
- **Settings** — units, notification thresholds, data export (CSV / JSON), firmware OTA (file picker → BLE upload → reboot).

The BLE protocol uses a custom GATT service (UUID `f5b00001-...`) with characteristics for live data (notify), command (write), and log dump (notify, chunked). The protocol file (`src/ble/protocol.ts`) defines the binary framing.

The optional LTE backhaul is a Raspberry Pi Zero 2 W (or any Linux SBC) attached to one FrostSentinel node's USB-C port, running a small Node.js/Express server that aggregates the mesh and exposes a REST + WebSocket API. The app's Mesh Dashboard switches to this endpoint when BLE is not in range.

---

## 7. Use Cases and Target Audience

### Primary use cases

1. **Vineyard frost protection.** The canonical use case. A vineyard manager deploys 6–10 nodes across the property. At 18:00, the app shows the RFRI forecast. If it climbs above 0.6 at any node, the manager starts a "frost watch." When RFRI > 0.85 or T_wet ≤ +0.5 °C, the mesh pushes an alert and the manager starts wind machines or helicopters. The AE channel confirms whether ice actually nucleated, telling the manager whether to continue mitigation or stand down. After the event, the cumulative AE energy gives an "ice load" map showing which blocks were hit hardest, guiding the morning's damage assessment.

2. **Orchard bud-break protection (almond, cherry, apple).** Bud-break is the most frost-vulnerable stage. FrostSentinel nodes placed at canopy height in each block give block-specific frost risk, because cold air pools differently in each block. The AE channel detects nucleation on the surrogate leaf-wetness film even when the buds themselves are not yet wet, giving the earliest possible warning.

3. **Coffee and tea at altitude.** Tropical highland frosts are radiative and can wipe out a year's harvest. FrostSentinel's solar + LiFePO₄ design works in off-grid plantations; the mesh relay means a single LTE node at the processing shed covers the whole estate.

4. **Agricultural insurance verification.** Insurers need ground-truth evidence of frost events to process claims. FrostSentinel's AE-confirmed nucleation log, with timestamped sky-IR and wet-bulb data, is far more rigorous than a weather-station temperature reading and is tamper-evident via the AES-GCM-signed mesh packets.

5. **Research viticulture and micrometeorology.** The open data format and the sky-IR radiometer make FrostSentinel a low-cost research instrument for canopy energy-balance studies, radiation-frost modeling, and phenology-frost interaction.

### Target audience

- **Commercial vineyard and orchard managers** (10–500 ha) — the core market.
- **Specialty crop growers** (berries, stone fruit, citrus, coffee, tea) in frost-prone microclimates.
- **Agricultural insurers and risk assessors** wanting ground-truth event data.
- **Agronomists and precision-ag service providers** offering frost-management contracts.
- **Agricultural research stations and university viticulture programs.**
- **Serious hobbyists and smallholders** for whom a $2,000 frost station is out of reach but a $45 BOM open build is not.

---

## 8. Bill of Materials (Top-Level, 100-unit pricing)

| Part | Qty | Unit $ | Extended $ |
|---|---|---|---|
| STM32U575ZIT6 | 1 | 6.20 | 6.20 |
| MLX90632 | 1 | 5.80 | 5.80 |
| PT100 class-A 4-wire | 2 | 2.10 | 4.20 |
| Sunon 25 mm fan | 1 | 3.40 | 3.40 |
| PZT 7BB-35-3 | 1 | 0.90 | 0.90 |
| OPA2376 | 1 | 1.80 | 1.80 |
| SHT45 | 1 | 2.40 | 2.40 |
| BMP390 | 1 | 2.20 | 2.20 |
| DS18B20 | 1 | 1.10 | 1.10 |
| SX1262 module | 1 | 5.50 | 5.50 |
| CC2642R1F | 1 | 3.20 | 3.20 |
| W25Q80 | 1 | 0.60 | 0.60 |
| RV-3028-C7 | 1 | 1.40 | 1.40 |
| SPV1050 | 1 | 1.80 | 1.80 |
| TPS62740 | 1 | 1.60 | 1.60 |
| LC709203F | 1 | 1.20 | 1.20 |
| NE555 + passives | 1 | 0.80 | 0.80 |
| Solar panels 1.5 W | 2 | 1.50 | 3.00 |
| LiFePO₄ 18650 1500 mAh | 1 | 3.50 | 3.50 |
| PCB (4-layer, ENIG) | 1 | 2.20 | 2.20 |
| Enclosure + mast + hardware | 1 | 4.50 | 4.50 |
| **Total** | | | **~$56.80** |

---

## 9. Calibration and Maintenance

- **Sky IR:** factory-calibrated MLX90632; field offset check against a known blackbody (boiling-water-steam reference) annually.
- **Psychrometer:** wick replaced every 6 months; reservoir refilled with distilled water monthly; RTD ice-point check annually.
- **Leaf wetness:** site-specific threshold set via the app's Calibration screen at install; no recurring calibration.
- **AE baseline:** learned automatically over the first 10 minutes of quiet operation after install; re-learned on demand.
- **Battery:** LiFePO₄ 18650 replaced every 5 years (calendar life); hot-swappable.

---

## 10. Open-Source Licensing

- **Hardware (KiCad schematics, PCB, mechanical):** CERN-OHL-S v2 — anyone may manufacture, modify, and sell derivatives, provided the modified designs remain open under the same license.
- **Firmware (C source):** GPL-3.0 — derivative firmware must remain GPL.
- **Companion app (TypeScript/React Native):** MIT — permissive, to encourage integration into proprietary farm-management platforms.

All files authored by **jayis1**. Copyright © 2026 jayis1.