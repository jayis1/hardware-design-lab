# Soilchord — Geotechnical Acoustic Soil Profiler

**Author:** jayis1
**License:** MIT
**Version:** 1.0.0
**Status:** Reference design (hardware + firmware + companion app)

> *A buried string orchestra that listens to the earth.*

---

## 1. What Is Soilchord?

Soilchord is a novel geotechnical instrument that turns a vertical column of soil into
a musical instrument — and then listens to it. Inside a slim, waterproof probe are
several taut steel wires ("chords"), each anchored at a different depth. A small
solenoid actuator *plucks* each chord on a schedule, and a piezoelectric pickup
captures the resulting resonance. The fundamental frequency, harmonic series, and
ring-down decay (Q-factor) of each chord are exquisitely sensitive to the mechanical
coupling between the wire and the surrounding soil matrix. As soil moisture rises, the
acoustic impedance and viscous damping around the wire change; as density or
compaction shifts, the effective mass loading on the wire changes; as a void or crack
propagates past a chord, the coupling suddenly drops. By repeatedly exciting and
measuring the response, Soilchord produces a *continuous, depth-resolved profile* of
subsurface soil conditions without ever needing to extract a sample or move a single
gram of dirt.

This is a fundamentally new sensing modality. Existing soil moisture sensors
(TDR/FDR, capacitance probes, neutron probes) measure a small sample volume around a
single point and require either expensive time-domain reflectometry electronics or
radioactive sources. Existing geotechnical monitoring (inclinometers, piezometers,
settlement plates) measures displacement or pore-water pressure but not the *bulk
mechanical state* of the soil itself. Soilchord fills a gap: it is a passive, low-power,
solar-powered, wireless instrument that reports the *acoustic mechanical signature* of
a vertical soil column in situ, continuously, for years. It is the geotechnical
equivalent of a doctor tapping a patient's knee and listening to the sound — except
it does it six times an hour, at six depths, and uploads the result over LoRa.

The core physics is well established but has never, to our knowledge, been packaged as
a practical field instrument. A buried tensioned wire is a damped harmonic oscillator
whose boundary conditions are set by the surrounding medium. Dry, loose soil couples
weakly: the wire rings long and bright, high Q, near its free-air frequency. Wet, dense
soil couples strongly: the wire is damped heavily, the frequency shifts down (mass
loading), and the Q drops. A sudden loss of coupling (void, pipe, crack) makes a chord
"come back to life" — its Q jumps upward. These signatures map directly onto the
failure modes that matter to civil engineers: internal erosion ("piping") in levees,
moisture buildup before a landslide, frost-thaw cycles in permafrost, and over/under
compaction at construction sites.

Soilchord is designed to be:
- **Install-and-forget.** Drive the probe into a borehole, backfill, walk away.
- **Self-powered.** Solar + LiFePO4, multi-year autonomy.
- **Wireless.** LoRaWAN-class telemetry, kilometers of range, no cell plan needed.
- **Open.** Full schematics, firmware, and app are released under MIT.

---

## 2. Why It Should Exist

Soil-related failures kill people and cost billions every year. Levee and tailings-dam
failures (e.g., Brumadinho, 2019) are often preceded by days of subtle internal erosion
that surface instrumentation misses. Landslides are frequently triggered by pore-pressure
buildup deep in a slope that surface moisture sensors cannot see. Agricultural
irrigation wastes enormous amounts of water because root-zone moisture is invisible from
above. Permafrost thaw — a climate-scale hazard — is monitored by a handful of
expensive borehole stations.

All of these problems share a common shape: *something important is happening a meter
or two underground, and no one is listening.* Soilchord listens.

Conventional solutions are either:
1. Too local (capacitance probes sense ~10 cm around themselves and only moisture),
2. Too expensive (TDR arrays, neutron probes),
3. Too indirect (inclinometers measure displacement *after* movement has occurred), or
4. Too invasive (sampling destroys the in-situ state).

Soilchord's acoustic approach is non-invasive after installation, senses a *volume* of
soil around each chord (tens of cm radially), captures *mechanical* state (not just
water content), and costs less than a hundred dollars in bill of materials. It is the
first instrument designed to put a stethoscope on the ground and leave it there.

---

## 3. Hardware Specifications

### 3.1 System overview

| Parameter | Value |
|---|---|
| MCU | STMicroelectronics STM32L432KCU6 (ARM Cortex-M4F, 80 MHz, 256 KB flash, 64 KB SRAM) |
| Radio | Semtech SX1262 sub-GHz LoRa (868/915 MHz), +22 dBm, ~10 km LoS range |
| Actuator | 4× mini push-pull solenoids (6 V, ~250 mA pulse, 30 ms pluck) — one per active chord |
| Sensor | 6× 27 mm piezoelectric disc pickups (one per chord) + 1 MEMS mic for ambient ref |
| ADC | Internal 12-bit @ 1 MSPS for sampling; external TI ADS8866 16-bit optional ref design |
| Signal chain | Charge amp (OPA2333) → 2nd-order Sallen-Key BP (300 Hz–8 kHz) → PGA (MCP6S22) → ADC |
| Temperature | 6× Maxim DS18B20 1-Wire digital sensors (one per chord depth) ±0.5 °C |
| Power | 6 W solar panel + 3.2 V 10 Ah LiFePO4 cell + MPPT-ish buck charger (BQ24650) |
| RTC | Internal LSE + backup register; external PCF85263 for tamper-proof timestamping |
| Storage | 64 Mb (8 MB) SPI NOR flash (W25Q64) for offline logging buffer |
| Form factor | Probe: 1500 mm × Ø48 mm, IP68 (machined 6061-T6 body + acetal tip); surface unit: 180×120×60 mm, IP66 |
| Weight | Probe ~1.1 kg; surface unit ~0.6 kg |
| Operating temp | -30 °C to +70 °C (probe); -20 °C to +60 °C (surface electronics) |
| Measurement interval | Configurable, default 6× per hour (every 10 min); adaptive in high-risk mode |
| Battery life | > 1 year with no sun; indefinite with ≥ 2 h equivalent full sun/day |

### 3.2 Chord array

The probe contains **6 chords** spaced at depths of 0.0, 0.3, 0.6, 0.9, 1.2 and 1.5 m
(default spacing; field-configurable). Each chord is a 0.30 mm diameter stainless steel
music wire ("piano wire") tensioned against a sprung anchor at the bottom and a tuning
peg at the top of its segment. Free-air fundamental frequencies are staggered across
chords (≈ 220, 294, 392, 523, 698, 880 Hz — i.e. A3–A5) so that spectra never overlap
and the DSP can deconvolve multi-chord captures even if all strings ring at once.

Each chord has:
- Its own piezo disc bonded to the anchor block (mechanical pickup, not acoustic air),
- Its own DS18B20 fastened to the anchor for thermal compensation,
- A shared solenoid that strikes it via a small lever (4 solenoids drive 6 chords in a
  multiplexed arrangement; chords 1+5 share, 2+6 share — see firmware for ordering).

### 3.3 Block diagram

```
        ┌────────────────────── SOILCHORD SURFACE UNIT ──────────────────────┐
        │                                                                     │
   ┌────┴─────┐   ┌──────────┐    ┌──────────────┐      ┌──────────┐         │
   │ 6 W PV   │──▶│ BQ24650  │───▶│ LiFePO4 10Ah │──┬──▶│ 3.3 V    │─┬─────▶│ all logic
   │ panel    │   │ charger  │    │   battery     │  │   │ LDO x2  │ │      │
   └──────────┘   └──────────┘    └───────────────┘  │   └──────────┘ │      │
                                                     │                │      │
                                                     │   ┌────────────┴──┐    │
                                                     └──▶│ Fuel gauge    │    │
                                                         │ MAX17048      │    │
                                                         └───────────────┘    │
        ┌────────────────────────────────────────────────────────────────────┘
        │
        │   ┌────────────────────────────────────────────────────────┐
        │   │  STM32L432KC (Cortex-M4F, 80 MHz)                        │
        │   │  ┌──────────┐  ┌──────────┐  ┌────────┐  ┌──────────┐ │
        │   │  │ Scheduler│  │ DSP core  │  │ LoRa   │  │ 1-Wire   │ │
        │   │  │ (RTC)    │  │ (CMSIS-DSP│  │ stack  │  │ temp bus │ │
        │   │  └────┬─────┘  └─────┬────┘  └───┬────┘  └────┬─────┘ │
        │   └───────┼──────────────┼────────────┼────────────┼──────┘
        │           │              │            │            │
        │   ┌───────▼──────┐  ┌────▼─────┐  ┌───▼─────┐  ┌────▼─────┐
        │   │ Solenoid drv  │  │ ADC +    │  │ SX1262  │  │ DS18B20 │
        │   │ (ULN2003 +    │  │ PGA +    │  │ SPI +   │  │ ×6      │
        │   │  MOSFET)      │  │ charge   │  │ DIO IRQ │  │         │
        │   └───────┬──────┘  │ amp      │  └─────────┘  └─────────┘
        │           │         └────┬─────┘
        │   ┌───────▼──────┐   ┌────▼─────┐
        │   │ 4× solenoids │   │ 6× piezo │
        │   │ (pluckers)   │   │ pickups │
        │   └───────┬──────┘   └────┬─────┘
        │           │               │
        └───────────┼───────────────┼──────────────────────────────────
                    │               │
        ┌───────────▼───────────────▼──── PROBE (IP68, Ø48×1500 mm) ─────┐
        │  Chord 6 (1.5 m) ──┐                                              │
        │  Chord 5 (1.2 m) ──┤  each chord: piano wire + piezo + DS18B20   │
        │  Chord 4 (0.9 m) ──┤  struck by shared solenoid lever            │
        │  Chord 3 (0.6 m) ──┤                                              │
        │  Chord 2 (0.3 m) ──┤                                              │
        │  Chord 1 (0.0 m) ──┘                                              │
        └──────────────────────────────────────────────────────────────────┘
```

### 3.4 Mechanical design notes

- The probe body is a 6061-T6 aluminium tube (1.5 mm wall) providing EMI shielding and
  heat conduction; the leading tip is acetal (POM) to ease driving and resist corrosion.
- Each chord segment is isolated by an internal acetal baffle so that striking one chord
  does not mechanically cross-talk excessively to another (measured < -25 dB).
- The top cap carries a vented Gore membrane for pressure equalisation and a radial
  O-ring seal rated to 10 m submersion.
- The surface unit is mounted on a 1 m pole with the solar panel angled at site latitude.

### 3.5 Power budget

| State | Current | Duration / cycle | Energy / day (default 144 cycles) |
|---|---|---|---|
| Sleep (RTC + RAM retention) | 8 µA | ~597 s of every 600 s | 0.115 Wh |
| Measurement (pluck + sample + DSP) | 45 mA | ~3 s | 0.024 Wh |
| TX (LoRa, +20 dBm, SF9) | 90 mA | ~0.12 s × 144 | 0.0043 Wh |
| Avg standby overhead (RTC, supervisor) | 40 µA | continuous | 0.001 Wh |
| **Total** | | | **~0.15 Wh/day** |

A 10 Ah LiFePO4 cell (3.2 V → 32 Wh) provides **>200 days** of darkness autonomy; the
6 W panel (≈ 25 Wh/day in temperate winter) covers the load ~150× over. MPPT not strictly
required; a simple buck charger suffices, but the BQ24650 gives headroom for cloudy weeks.

---

## 4. Firmware Design

### 4.1 Overview

The firmware is bare-metal C targeting the STM32L432. It uses the CMSIS-DSP library for
all signal processing (no RTOS — a cooperative scheduler driven by the RTC alarm is
sufficient and keeps the codebase auditable). The full source lives in
`soilchord/firmware/` and is buildable with `arm-none-eabi-gcc` and a provided Makefile.

The main loop is event-driven and spends >99% of its time in STOP2 mode with the RTC
running on the 32.768 kHz LSE. Every measurement interval (default 600 s) the RTC alarm
fires, the MCU wakes, performs a measurement cycle on all six chords, processes the
captured waveforms, packages the results, transmits over LoRa, logs to flash, and goes
back to sleep.

### 4.2 Measurement cycle

For each chord `i` in 1..6:
1. Power-gate the analog signal chain (LDO enable high).
2. Read temperature `T_i` from DS18B20 `i` (750 ms max-conversion; uses one-shot mode).
3. Energise solenoid mapped to chord `i` for 30 ms (the "pluck").
4. Wait 20 ms for transients to settle.
5. Sample piezo `i` at 16 kHz for 512 ms → 8192 samples into SRAM buffer.
6. Window (Hann), real-input FFT (4096-point), compute power spectrum.
7. Detect fundamental `f0_i` (peak search near the chord's nominal band), estimate Q
   from the −3 dB bandwidth and from the time-domain ring-down envelope.
8. Compute features: `{ f0, Q, RMS, crest factor, harmonic ratio (H2/H1, H3/H1) }`.
9. Temperature-compensate `f0` using the known thermal expansion coefficient of steel
   (α ≈ 17 ppm/K) and the chord's tension/length geometry.
10. Store the feature vector against chord id + timestamp.

After all six chords are sampled, the per-cycle aggregate is assembled into a LoRa
packet (see §5) and into a NOR-flash log record.

### 4.3 DSP pipeline

- **Window:** Hann, to reduce leakage given short captures.
- **Transform:** `arm_rfft_fast_f32` (CMSIS-DSP), 4096-point real FFT.
- **Peak detection:** parabolic-interpolated peak within a ±15% band of each chord's
  nominal free-air frequency, to reject harmonic/mirror ambiguity.
- **Q estimation (frequency domain):** `Q = f0 / Δf` where Δf is the −3 dB half-power
  bandwidth measured by descending both sides of the peak until the power drops to half.
- **Q estimation (time domain):** fit an exponential `A·exp(-t/τ)` to the RMS envelope
  (computed in 10 ms windows); `Q = π·f0·τ`. The two estimates are cross-checked; a
  disagreement > 25% flags the measurement as suspect (e.g., a double-pluck or a strike
  that didn't separate from the wire).
- **Anomaly scoring:** a lightweight Z-score test against a rolling baseline
  (exponentially-weighted, α=0.05) for each chord's `f0` and `Q`. If `|Z| > 3` on any
  chord for ≥3 consecutive cycles, an `ALERT` flag is set in the next packet.

### 4.4 Adaptive scheduling

In normal operation the interval is 600 s. If the anomaly score exceeds a threshold on
any chord (e.g., rapid Q drop suggesting moisture surge), the firmware shortens the
interval to 120 s for the next 24 cycles and sets the `URGENT` bit so the gateway can
raise alarms. This trades ~5× power for ~5× temporal resolution exactly when it matters.

### 4.5 Reliability

- Independent watchdog (IWDG) fed at the top of every wake cycle; a hang resets the
  device and a "reset cause" byte is reported in the next packet.
- Brown-out reset enabled; a low-battery condition suspends LoRa TX (logging only) below
  3.0 V and suspends measurements below 2.8 V to protect the cell.
- The LoRa TX is retried up to 3 times with random jitter; if all fail the record is
  queued in NOR flash for later backfill (the gateway can request missed sequence
  numbers).
- All flash writes go through a wear-levelled ring buffer; metadata is double-buffered
  to survive power loss mid-write.

### 4.6 Build

```
cd soilchord/firmware
make            # builds soilchord.elf, .hex, .bin
make flash      # openocd via ST-LINK
make size       # prints flash/RAM usage
```

Dependencies: `arm-none-eabi-gcc` ≥ 10, `arm-none-eabi-binutils`, `make`,
CMSIS headers (vendored in `firmware/CMSIS/`), STM32L4 HAL (vendored in
`firmware/stm32l4xx_hal/`).

---

## 5. Application / Software Interface

### 5.1 LoRa downlink/uplink

Soilchord speaks a compact binary protocol (documented in `firmware/proto.h`). Uplink
frames carry the latest per-chord feature vector; downlink frames adjust the interval,
calibrate the baseline, or request a backfill. A standard LoRaWAN-compatible gateway
(e.g., RAK7243, TTN) terminates the radio link; the companion app talks to a broker
(MQTT over TLS) that the gateway publishes to.

### 5.2 Companion app (`app/`)

A React Native (Expo) mobile + tablet app, **Soilchord Field**, provides:

- **Site dashboard.** Per-probe cards with last-contact, battery, and a depth profile
  sparkline (f0 and Q vs depth).
- **Probe detail.** Full time-series charts for each chord, with anomaly markers; the
  user can swipe between chords and toggle f0 / Q / temperature / moisture-estimate.
- **Risk score.** A per-site composite score (0–100) derived from the rate-of-change of
  Q across chords (levee piping signature) and the absolute moisture estimate, with
  colour-coded banners (green / amber / red).
- **Calibration.** A guided wizard that walks the user through a dry-baseline capture
  (free-air) and a site-specific soil-type selection that picks the moisture↔Q transfer
  function.
- **Alerts.** Push notifications on `URGENT`/`ALERT` packets, with acknowledge-and-snooze.
- **Export.** CSV / JSON export of any date range for import into GIS or slope-stability
  software.

The app is intentionally offline-friendly: all data is mirrored to local SQLite and
synced opportunistically, so field work without cell coverage still shows recent
history.

---

## 6. Use Cases & Target Audience

| Audience | Use case | What Soilchord gives them |
|---|---|---|
| Levee / dam operators | Internal-erosion (piping) early warning | Q-drop + f0-shift signature on the affected depth band, hours-to-days before visible seepage |
| Slope / landslide authorities | Pre-failure moisture buildup | Continuous root-zone-to-depth moisture proxy at the slide-prone layer |
| Precision irrigators | Scheduling, deficit irrigation | Depth-resolved moisture without a neutron probe or costly TDR array |
| Geotechnical labs (QA/QC) | Compaction verification | In-place density trend after lift placement; no nuclear density gauge licence |
| Permafrost researchers | Active-layer thaw tracking | f0 drift across the freeze-thaw transition per chord |
| Forestry / wildfire risk | Soil-water deficit for fire-load models | Long-term unattended moisture record under canopy |
| Mining (tailings) | Tailings-dam health | Acoustic signature of phreatic surface movement |
| Citizen science / farms | Affordable borehole logging | Sub-$100 BOM, installable by hand auger |

---

## 7. Novelty & Differentiation

Soilchord is, to the best of the author's knowledge, the first instrument to package
*tensioned-string resonance* as a general-purpose in-situ soil mechanical profiler.
Related prior art:

- **Vibrating-wire strain gauges** (Geokon, RST) measure tension change of a *single*
  embedded wire to read strain in concrete or rock — a one-shot scalar, not a
  frequency/Q profile, not a soil-volume probe, and not self-powered/wireless.
- **Acoustic emission** listening is passive and only catches active cracking.
- **TDR / FDR probes** measure water content electrically, not mechanical state.
- **Spectral-analysis-of-surface-waves (SASW)** surveys are non-invasive but require a
  field crew, a heavy source, and produce a snapshot, not a continuous record.

Soilchord's combination of (a) an *active* pluck-and-listen excitation, (b) *multiple
depths* in one probe, (c) *both* frequency and damping as observables, (d)
*solar/LoRa* autonomy, and (e) an *open, sub-$100* design is novel and addresses a real
gap in the geotechnical monitoring market.

---

## 8. Limitations & Future Work

- The moisture↔Q transfer function is soil-type-dependent; the app ships with
  calibrated curves for sand, silt, clay, and loam, but field calibration improves
  accuracy substantially.
- Chord tension drifts with temperature; the per-chord DS18B20 compensates this to
  first order, but extreme diurnal swings (desert) leave a residual that should be
  characterised per installation.
- A strike that fails to separate from the wire produces a "dead pluck"; the DSP
  detects this (low RMS + low harmonic ratio) and retries once.
- Future: a high-bandwidth variant with an external 24-bit ADC for sub-Hz resolution of
  f0, and an eddy-current version for use in conductive soils where piezo coupling is
  weak.

---

## 9. Repository Layout

```
soilchord/
├── README.md            # this file
├── firmware/
│   ├── Makefile
│   ├── board.h          # pin map + board-level constants
│   ├── registers.h      # peripheral register convenience macros
│   ├── soilchord.h      # public API of the firmware modules
│   ├── proto.h          # LoRa wire protocol definitions
│   ├── main.c           # scheduler, state machine, startup
│   ├── dsp.c            # FFT, peak detection, Q estimation, anomaly
│   ├── actuator.c       # solenoid pluck sequencing
│   ├── piezo.c         # ADC + PGA + signal chain init, sampling
│   ├── lora.c           # SX1262 driver + packet framing + retry
│   ├── temp.c           # DS18B20 1-Wire driver
│   ├── power.c          # battery/solar monitoring, sleep, BOR
│   └── flash_log.c      # NOR flash wear-levelled ring buffer
├── kicad/
│   ├── soilchord.kicad_sch
│   ├── soilchord.kicad_pcb
│   └── soilchord.kicad_pro
└── app/
    └── (React Native Expo project)
        ├── App.tsx
        ├── app.json
        ├── package.json
        ├── tsconfig.json
        └── src/
            ├── store.ts
            ├── proto.ts
            ├── api.ts
            └── screens/
                ├── DashboardScreen.tsx
                ├── ProbeDetailScreen.tsx
                ├── RiskScreen.tsx
                ├── CalibrationScreen.tsx
                ├── AlertsScreen.tsx
                └── ExportScreen.tsx
```

---

## 10. License & Author

All hardware schematics, PCB layouts, firmware, and application code in this directory
are © 2026 **jayis1** and released under the MIT licence. See `LICENSE` excerpt in each
source file header. No part of this design is claimed to be patented by the author;
implementers are responsible for their own freedom-to-operate analysis.

---

*Soilchord: when the earth hums, listen.*