# TensilGuard — Magnetoelastic Cable Tension & Structural Health Monitor

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)
**SPDX-License-Identifier:** MIT

![MCU](https://img.shields.io/badge/MCU-STM32G431RBT6-orange) ![Sensor](https://img.shields.io/badge/Magnetoelastic-Villari%20effect-blue) ![Radio](https://img.shields.io/badge/Radio-SX1262%20LoRa%20Mesh-purple) ![Power](https://img.shields.io/badge/Power-Solar%20%2B%20LiFePO4-yellow) ![Author](https://img.shields.io/badge/Author-jayis1-green)

> *A collar that listens to the magnetism, the music, and the cracking of a steel cable under load.*

---

## 1. Device Name, Purpose, and Overview

### 1.1 What TensilGuard Is

TensilGuard is a clamp-on, solar-powered, wireless structural-health node that
wraps around a structural steel cable — a cable-stayed bridge stay, a suspension
bridge hanger, a guyed-mast guy rope, a ski-lift haul cable, a rock-bolt / pre-
stressed tendon, or a roadside cable barrier — and continuously measures its
**tension** and **internal integrity** without removing a single wire from
service. It fuses three independent physics modalities, each sensitive to a
different failure mode, into a single self-contained instrument that runs for
years on harvested solar energy and reports over a LoRa mesh:

1. **Magnetoelastic (Villari-effect) tension sensing.** A ferromagnetic steel
   cable's magnetic permeability is a function of the mechanical stress inside
   it (the inverse magnetostriction, or *Villari effect*). TensilGuard wraps an
   excitation/sense coil around the cable, drives it with a swept-frequency AC
   current, and measures the coil's complex impedance. The inductance is
   directly modulated by the effective permeability of the cable cross-section,
   which shifts monotonically with applied tensile stress after a one-time
   calibration. This is the *only* non-destructive method that measures the
   internal load of a cable from outside its jacket, and it works through HDPE
   sheathing and corrosion grease because the magnetic field penetrates both.

2. **Accelerometer-based cable-frequency tension estimation.** A taut cable's
   fundamental transverse vibration frequency is `f₁ = (1 / 2L)·√(T / μ)`, where
   `L` is the free length, `T` the tension, and `μ` the linear mass density. A
   low-noise MEMS accelerometer samples the cable's ambient vibration for tens of
   seconds; the firmware performs a windowed FFT and harmonic product spectrum
   to pick the fundamental, then inverts the string equation to obtain an
   *independent* tension estimate. This is the classical "vibration method" used
   by bridge engineers with hand-held accelerometers; TensilGuard automates it
   continuously and uses it to cross-check the magnetoelastic reading. The two
   methods disagree when something is wrong (a broken wire changes `μ` and the
   magnetic signature but not the vibration frequency in the same way), so the
   disagreement is itself a diagnostic.

3. **Acoustic-emission (AE) wire-break monitoring.** When an individual wire
   inside a multi-wire cable snaps, it releases a sharp burst of elastic energy
   that propagates along the cable as a stress wave at ~5000 m/s. A piezoelectric
   contact transducer clamped to the cable, feeding a charge amplifier and a
   continuously-running threshold detector, captures these bursts. The firmware
   classifies each burst (amplitude, rise-time, duration, ring-down counts) to
   distinguish a true wire break from fretting, rain-wind vibration, impacts, and
   traffic noise. Each confirmed break is time-stamped, its waveform is captured
   to SPI flash, and an urgent LoRa uplink is sent. This is how bridge owners
   discover that a parallel-wire stay is failing *before* it loses enough section
   to sag.

A precision temperature sensor (TMP117, ±0.1 °C) compensates all three
modalities — permeability, cable frequency, and AE propagation velocity are all
temperature-dependent.

### 1.2 Why It Should Exist

Cable-supported and cable-restrained infrastructure is everywhere, and it is
aging. Cable-stayed bridges (there are thousands worldwide), suspension bridges,
guyed TV/broadcast masts (some > 600 m tall), ski lifts, rock anchors in tunnels,
and roadside cable barriers all rely on steel cables whose tension and integrity
are the difference between a structure standing and falling. The state of the
art for monitoring these cables is, in most of the world, **periodic manual
inspection**: a technician drives out, clamps a hand-held accelerometer to the
cable, records a few seconds of vibration, computes a tension from a laptop,
writes a number in a spreadsheet, and leaves. Between visits — typically yearly
to every few years — nothing is monitored. The catastrophic failures tell the
story:

- The **Kolkata Majerhat bridge** (2018) and the **Genoa Polcevera viaduct**
  ("Morandi bridge", 2018, 43 deaths) both involved corroded, understensioned
  or over-stressed cable/element failure that progressed invisibly between
  inspections.
- **Cable-stay corrosion** is a known killer: several major bridges
  (Angleur/Meuse 1938 replacement, Pasco-Kennewick, the Luling Bridge) have
  required full cable replacement after corrosion was found late, at huge cost.
- **Guyed masts** collapse regularly when a single guy loses tension
  (corrosion, anchor failure, wind fatigue); the mast has no second chance.
- **Ski-lift haul cables** must stay within a narrow tension window for the
  grips to function; deropement is a direct safety hazard.

Manual vibration testing is also *single-modality*: it gives a tension number
but says nothing about the internal wire count, and it is dominated by the
lowest-frequency mode which averages over the whole cable. Magnetoelastic
sensing sees the *local* stress state of the steel right under the coil and
responds to section loss. Acoustic emission sees the *individual wire breaks*
that precede section loss. No single existing technique gives all three; no
existing commercial product fuses them in a solar-powered wireless node; and no
open-source reference design for cable-load monitoring exists at all.

TensilGuard fills that gap. It is:

- **Install-and-forget.** Hinge the collar closed around the cable, tighten two
  captive screws, pair it with the app, walk away. No cable deropement, no
  lane closure, no permanent wiring.
- **Multi-modal and redundant.** Two independent tension estimates + a
  break detector, with automatic cross-check and disagreement alarming.
- **Self-powered.** A small solar panel and a LiFePO4 cell give multi-year
  autonomy even in overcast climates; the node sleeps in STOP2 between
  measurements and wakes on an RTC alarm or on an AE event.
- **Mesh-networked.** LoRa mesh firmware relays uplinks from node to node so a
  whole cable fan on one bridge tower reports through a single gateway. No
  per-node SIM card, no cellular plan.
- **Open.** Full schematics, KiCad PCB, firmware, and companion app under
  permissive licenses, authored by jayis1.

---

## 2. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32G431RBT6 — ARM Cortex-M4F @ 170 MHz, 256 KB flash, 32 KB SRAM, on-chip CORDIC + FMAC math accelerators, 12-bit ADC5 @ 4 Msps, OPAMP, comparators |
| **Magnetoelastic front-end** | Excitation coil (32 turns around cable, AWG26) driven by an H-bridge current driver (DRV8872) from a 24 V boost rail; sense via the same coil in a half-bridge with a precision series reference resistor (10 Ω, 0.1%). ADC5 samples coil voltage and shunt voltage simultaneously at 500 kcps; an on-chip OPAMP (PGA gain ×8) conditions the shunt signal. FMAC does the quadrature (I/Q) demodulation at the excitation frequency. |
| **Sweep range** | 500 Hz – 10 kHz, 24 logarithmic steps, configurable. Resonance-tracking mode locks onto the coil's self-resonance for maximum sensitivity. |
| **Accelerometer** | TDK ICM-42688-P, 6-axis, SPI at 24 MHz, low-noise mode (70 µg/√Hz), sampled at 100 Hz for 32 s windows (3200 samples/axis) for cable-frequency extraction down to ~0.2 Hz. |
| **Acoustic emission** | Murata 7BB-27-4 piezo contact transducer clamped to the cable surface, feeding an AD8542 charge amplifier (gain 100, HPF at 20 kHz) → ADC5 at 200 kcps continuously in a low-power "listen" mode; the on-chip comparator wakes the MCU from STOP2 on a threshold crossing. Captured waveform: 8 ms pre-trigger ring buffer + 24 ms post-trigger at 200 kcps = 6400 samples/event. |
| **Temperature** | TMP117 (I2C, ±0.1 °C, 16-bit) bonded inside the collar; also the MCU internal sensor for board compensation. |
| **Radio** | Semtech SX1262IMLTRT (LoRa, 868/915 MHz, +22 dBm, sub-GHz), SPI, mesh firmware with store-and-forward relaying. |
| **Power** | 6 W 6-cell monocrystalline solar panel (custom 90×60 mm) → BQ25570 energy-harvesting charger with MPPT → 1-cell 18650 LiFePO4 (1500 mAh, 3.2 V). MAX17055 fuel gauge reports state-of-charge, voltage, current. A 24 V boost converter (TPS61175) powers the coil driver only during a sweep (duty-cycled). |
| **Storage** | Winbond W25Q128JVSIQ (16 MB SPI flash) — circular log of up to ~2000 AE events with waveforms, plus a 6-month rolling tension/time-series ring buffer. |
| **Connector / antenna** | External RP-SMA for a dipole; magnetic or adhesive mount for the solar panel. |
| **Form factor** | Hinged two-piece aluminum collar, ID 40–90 mm (4 cable sizes), IP68 (potted coil cavity, vented battery compartment). 130 × 95 × 55 mm closed, 320 g. |
| **Operating range** | −40 °C to +85 °C; cable tension 5–250 kN (model-dependent after calibration); cable diameter 20–80 mm ferromagnetic steel (magnetoelastic) or any material (vibration + AE only modes). |
| **Telemetry interval** | Default 30 min (configurable 5 min–24 h); AE events trigger immediate uplink; adaptive: 5 min during storms. |

### 2.1 Bill of Materials (summary)

| Ref | Part | Function |
|---|---|---|
| U1 | STM32G431RBT6 | MCU |
| U2 | SX1262IMLTRT | LoRa radio |
| U3 | DRV8872 | H-bridge coil driver |
| U4 | ICM-42688-P | Accelerometer |
| U5 | TMP117 | Temperature |
| U6 | AD8542 | AE charge amplifier |
| U7 | BQ25570 | Solar harvesting charger |
| U8 | MAX17055 | Fuel gauge |
| U9 | TPS61175 | 24 V boost (coil rail) |
| U10 | W25Q128JVSIQ | SPI flash |
| L1 | Excitation coil (32 t) | Magnetoelastic sense |
| PIEZO | 7BB-27-4 | AE transducer |
| SOLAR | 6 W mono panel | Power |
| BAT | 18650 LiFePO4 1500 mAh | Storage |

---

## 3. Architecture and Block Diagram

```
                       ┌──────────── TensilGuard collar ────────────┐
                       │                                            │
   6 W solar ─────────▶│ BQ25570 ──▶ LiFePO4 ──▶ 3.3 V LDO ──▶ VDD   │
                       │   (MPPT)    18650          │                │
                       │                  │        └─▶ TPS61175 ─▶ 24 V (coil only)
                       │                  │                          │
                       │            MAX17055 ◀ fgauge                 │
                       │                                            │
   Excitation coil  ◀══│══ DRV8872 ◀── PWM/DMA  ─┐                  │
   (32 t, around cable)│                          │   STM32G431RBT6  │
        │ sense ──────▶│ OPAMP(PGA) ─ ADC5 ─▶ FMAC ─ I/Q demod ─▶ L │
                       │                          │   (mag tension) │
   ICM-42688-P ◀──SPI──│                          │                 │
   (3-axis accel)     │                          ├─ FFT ─▶ f1 ─▶ T_vib
                       │                          │                 │
   7BB-27-4 piezo ──▶  │ AD8542 ─ ADC5 ─▶ comp ──▶│ (wake on AE)    │
   (clamped to cable) │              └▶ flash  ◀──┘                 │
                       │                          │                 │
   TMP117 ◀── I2C ────▶│ (temperature comp)       │                 │
                       │                          │                 │
                       │ SX1262 ◀── SPI ─────────▶│ LoRa mesh uplink │
                       │   RP-SMA ─ dipole antenna│                 │
                       └────────────────────────────────────────────┘
                                       │ LoRa mesh
                                       ▼
                            ┌────────────────────┐
                            │  Bridge gateway     │
                            │  (one per tower)     │
                            └─────────┬──────────┘
                                      │ cellular / ethernet
                                      ▼
                            ┌────────────────────┐
                            │ TensilGuard Field   │  ◀──── operators, engineers
                            │ companion app (RN)  │
                            └────────────────────┘
```

### 3.1 Measurement flow

Each measurement cycle (default 30 min) the firmware, woken from STOP2 by the
RTC:

1. Enables the 24 V boost and waits 50 ms for it to settle.
2. Runs the **magnetoelastic sweep**: for each of 24 frequencies it drives the
   coil for 20 ms, simultaneously ADC-samples coil voltage and shunt current,
   FMAC-demodulates I/Q at the drive frequency, and records |Z| and ∠Z. After
   the sweep it fits the impedance-vs-frequency curve, extracts the effective
   inductance, applies the temperature and one-time calibration coefficients,
   and produces `T_mag` (kN).
3. Disables the 24 V boost. Reads the **accelerometer** FIFO (it has been
   buffering at 100 Hz while the MCU slept), runs a Hann-windowed 4096-point
   real FFT, harmonic-product-spectrum peaks the fundamental `f1`, and computes
   `T_vib = μ·(2·L·f1)²`. Temperature-corrects `f1` for thermal expansion of L.
4. Reads TMP117 and stores `T_mag`, `T_vib`, `ΔT = |T_mag − T_vib|`, temperature,
   battery state.
5. If `ΔT` exceeds a learned threshold, or a new AE event was logged since last
   cycle, flags the message `urgent`.
6. Compresses the message, transmits over LoRa (mesh-relay if needed), logs to
   flash, and goes back to STOP2.

The **AE detector** runs *independently* of the cycle: a low-power comparator on
the piezo signal wakes the MCU from STOP2 at any time. The firmware then captures
the waveform, classifies it, logs it to flash, and either queues an urgent uplink
or returns to sleep — typically in under 200 ms.

---

## 4. Firmware Details and Design Decisions

### 4.1 Bare-metal, no RTOS

TensilGuard runs bare-metal C on the STM32G431. The main loop is a state machine
driven by RTC alarms and the AE wake comparator. An RTOS was considered and
rejected: the device spends >99 % of its life asleep, the working set is a small
fixed set of static buffers, and an RTOS would add ~6 KB of RAM and an
unpredictable scheduler for no benefit. The static-buffer design also makes the
firmware trivially analysable for stack depth and worst-case latency.

### 4.2 Why the STM32G431

The G431 was chosen specifically for its analog and math accelerators:

- **CORDIC** computes sin/cos/arctan in hardware in ~8 cycles, which is how the
  firmware generates the swept excitation waveform and how it demodulates phase.
- **FMAC** (Filter Math Accelerator) does the I/Q quadrature demodulation
  (multiply-by-NCO + accumulate) as a streaming DMA operation without CPU
  intervention — this is the inner loop of the magnetoelastic measurement and
  running it in the FMAC saves ~40 % of the cycle's CPU time.
- **ADC5** at 4 Msps with dual simultaneous sampling lets us capture coil voltage
  and shunt current at the same instant, so the phase between them (the
  permeability signal) is not corrupted by inter-channel skew.
- **OPAMP** as a PGA for the shunt current avoids an external signal-conditioning
  IC for the magnetoelastic front end.
- **Comparator** on the piezo channel is the zero-cost always-on AE wake source.

A Cortex-M0+/M3 could not have done the on-device FFT or the streaming
demodulation within the energy budget; an M7 would be overkill. The M4F with
CORDIC+FMAC is the sweet spot.

### 4.3 Magnetoelastic physics & calibration

The Villari effect: for ferromagnetic steel under tension, the differential
permeability `µ_diff = dB/dH` decreases monotonically (for most bridge steels)
as stress rises, because the tensile stress rotates domains against the
magnetization axis. TensilGuard does **not** need to know the absolute
permeability — it measures the *change* in coil inductance `L = k·µ_eff` as
stress changes. Because the relationship is mildly non-linear and depends on the
specific steel and the coil geometry, TensilGuard uses a one-time field
calibration: the app walks an engineer through loading the cable to 3 known
tensions (e.g. via a hydraulic jack) and records the impedance at each. The
firmware stores a 3-point piecewise-linear `T(µ_eff)` curve per node in flash. For
un-instrumented retrofits where a jack isn't available, the accelerometer-based
`T_vib` is used as the reference and the magnetoelastic channel self-calibrates
against it over the first week.

The measurement is differential in another sense: temperature shifts `µ_eff` and
`L` independently of stress (Curie-Weiss behaviour). The TMP117 reading and a
per-node temperature coefficient (measured at calibration) correct this. The
calibration coefficients, the coil geometry ID, and the cable parameters (L, μ,
diameter, material) are all stored in a flash "cal page" with a CRC.

### 4.4 Accelerometer-based tension

The string formula `T = μ·(2·L·f₁)²` assumes a pinned-pinned cable with no sag
and no bending stiffness. For bridge stays, sag and flexural rigidity introduce
small corrections; TensilGuard implements the Irvine sag correction
`f₁² = f₁₀²·(1 + ...)` as a second-order refinement when the user enters the
cable's sag. For guy wires and most stays the correction is < 1 %. The firmware
picks `f₁` with a harmonic product spectrum (HPS) to avoid the common error of
selecting a higher harmonic: HPS multiplies the spectrum by its
frequency-shifted copies and the true fundamental appears as the tallest peak.
A 32-second window at 100 Hz gives 0.03 Hz resolution, i.e. sub-0.1 % tension
resolution for a 50 m stay.

### 4.5 Acoustic emission classification

A wire-break burst is a short (50–200 µs rise, 0.5–3 ms total), high-amplitude
(> 60 dB re 1 µV at the transducer), broadband event whose energy is concentrated
above 80 kHz. Fretting and rain-wind vibration produce longer, lower, narrower
events below 60 kHz; traffic/impact noise is low-frequency. The firmware uses a
small hand-tuned feature classifier (amplitude, rise-time, duration, ring-down
counts, centroid frequency, partial-power ratio 80–200 kHz vs 20–80 kHz) and a
decision-tree scoring scheme. The threshold and scoring are configurable from
the app; the node ships with conservative defaults that bias toward low false
positives (a missed break is bad, but a flood of false alarms gets ignored).

### 4.6 Energy budget

A typical 30-minute cycle consumes ~12 J (sweep 6 J + accel FFT 1 J + LoRa TX
0.5 J + sleep leakage 4.5 J). A 6 W panel yields ~25 Wh/day in temperate
latitudes (≈ 90 kJ/day), enough for ~7000 cycles/day — far more than the 48
default cycles — so there is comfortable margin for overcast weeks. The 1500 mAh
LiFePO4 (≈ 5.7 Wh) buffers ~1700 cycles with no sun. The adaptive scheduler
reduces the cycle rate when the battery drops below 30 %.

### 4.7 File map

| File | Role | Lines (approx) |
|---|---|---|
| `main.c` | Scheduler, state machine, telemetry builder | 320 |
| `mag.c` | Magnetoelastic sweep + I/Q demod + tension fit | 280 |
| `accel.c` | ICM-42688 driver + FFT + HPS + string-tension | 260 |
| `ae.c` | AE capture, feature extraction, classifier | 240 |
| `lora.c` | SX1262 driver + mesh relay | 230 |
| `power.c` | BQ25570/MAX17055 + MPPT + adaptive scheduling | 150 |
| `flash_log.c` | W25Q128 ring buffer + cal page + AE waveform store | 200 |
| `temp.c` | TMP117 + compensation coefficients | 110 |
| `board.h` | Pin map, constants, geometry | 140 |
| `registers.h` | Register base addresses and bit helpers | 120 |
| `proto.h` | Uplink protocol structs | 90 |
| `tensilguard.h` | Common types | 110 |
| **Total** | | **~2060** |

---

## 5. Application / Software Interface

**TensilGuard Field** is a React Native (Expo) companion app for structural
engineers and maintenance crews. Screens:

- **Dashboard** — map/list of all nodes on a structure, each showing
  `T_mag`, `T_vib`, disagreement `ΔT`, last AE event, battery, signal. Colour
  bands (green/amber/red) flag nodes whose `ΔT` or AE count exceeds thresholds.
- **Node Detail** — time-series plots of tension (both modalities),
  temperature, and battery over selectable windows (6 h / 24 h / 7 d / 30 d);
  a "wire breaks" timeline with tap-to-view captured waveforms.
- **Calibration** — guided 3-point or vibration-reference calibration wizard;
  writes the cal page back to the node over a BLE provisioning link (the LoRa
  gateway bridges BLE → LoRa).
- **Alerts** — push-notification feed of urgent uplinks (wire breaks,
  `ΔT` threshold crossings, low battery, node silent).
- **Mesh Health** — link-quality, hop count, and per-node RSSI/SNR for the LoRa
  mesh; lets the installer place relays.
- **Export** — CSV/JSON export of the full time series and AE log for a
  selected date range, suitable for a bridge inspection report.

The app talks to a bridge gateway (one per structure, e.g. a Raspberry Pi with a
LoRa HAT) over a simple REST/WebSocket API; the gateway holds the LoRa packets,
decodes them, and serves history. The app also works fully offline against a
locally cached SQLite copy of the gateway database.

---

## 6. Use Cases and Target Audience

### 6.1 Cable-stayed and suspension bridges

The primary use case. A medium cable-stayed bridge has 20–80 stays; TensilGuard
nodes on each stay report tension drift, broken-wire AE, and temperature to one
tower-mounted gateway. Owners get continuous tension logs (regulatory trend
toward "smart bridges"), early warning of corrosion-induced section loss (the AE
channel), and a defensible audit trail. Target: department of transportation
bridge engineers, specialist inspection contractors.

### 6.2 Guyed masts (broadcast / telecom)

A 300 m guyed mast has 9–18 guy levels, each with 3 guys. A single guy losing
tension is catastrophic and undetectable from the ground until the mast leans.
TensilGuard on each guy reports tension continuously; an anchor-failure or
turnbuckle slippage shows up as a sudden `T` drop. Target: tower operators,
broadcasters, telecom carriers.

### 6.3 Ski lifts and ropeways

Haul-cable tension must stay in a tight window for grips and brakes; too slack
risks deropement, too tight overloads towers. TensilGuard on the haul cable
near the tensioning carriage reports tension in real time; the AE channel
detects strand breaks in the moving cable. Target: ropeway operators,
regulators (e.g. ANSI B77, EN 12929 compliance evidence).

### 6.4 Pre-stressed concrete tendons and rock anchors

Post-tensioned bridge girders, anchored tunnel linings, and dam rock anchors
lose tension to creep, corrosion, and anchor relaxation. A clamp-on
TensilGuard (the coil fits around the bare tendon between anchorages) tracks
retention over the design life. Target: geotechnical contractors, dam owners.

### 6.5 Roadside cable barriers and fencing

Highway cable barriers save lives but lose effectiveness when slack; a stolen
or vandalised anchor drops a barrier's working tension silently. TensilGuard
on key posts gives highway agencies a maintenance-alert system. Target: highway
authorities.

### 6.6 Research and academia

Civil-engineering departments use TensilGuard as an open, affordable platform
for cable-dynamics research, magnetoelastic NDT, and structural-health-monitoring
teaching — the full design is open, so students can modify the DSP, add
modalities, or instrument test rigs for a fraction of the cost of commercial
vibration dataloggers.

---

## 7. Mechanical and Environmental

The collar is a hinged two-piece aluminum shell (anodised) with an elastomer
cable-grip liner sized to the cable OD; four sizes cover 20–30, 30–50, 50–70, and
70–90 mm. The excitation coil is wound on a 3D-printed PETG former that snaps
into the lower shell and is potted in polyurethane for IP68. The piezo AE
transducer is spring-loaded against the cable through a thin grease couplant
window in the liner. The PCB stacks above the coil cavity; the solar panel clips
to the top shell and can be re-aimed ±30°. The battery sits in a vented,
replaceable compartment. Total mass 320 g (collar + battery, excluding panel
mount). Operating temperature −40 °C to +85 °C; the LiFePO4 chemistry sustains
charge to −20 °C and the firmware derates the coil current above +70 °C.

---

## 8. Performance and Validation (target)

| Metric | Target |
|---|---|
| Magnetoelastic tension resolution | 0.5 % of full scale (after 3-pt cal) |
| Vibration tension resolution | 0.1 % (f₁ to 0.03 Hz) |
| Disagreement alarm threshold | 3 % (auto-learned over first week) |
| AE wire-break detection | ≥ 95 % single-wire breaks at < 5 % false alarms/day |
| AE localisation (with 2 nodes) | ± 2 m along a 100 m cable (TDOA) |
| Telemetry range | 2 km urban / 8 km line-of-sight (one hop) |
| Mesh depth | up to 8 hops |
| Battery autonomy (no sun) | 25 days at 30-min cycle |
| Solar autonomy (full sun) | indefinite |

---

## 9. License and Credit

Hardware (KiCad schematics, PCB, mechanical): **CERN-OHL-S v2**.
Firmware (C sources, Makefile): **GPL-2.0**.
Companion app (TypeScript/React Native): **MIT**.

**Author of all designs, firmware, code, and documentation: jayis1.**
All copyright notices, file headers, and metadata credit jayis1. No other
entity is credited as author.