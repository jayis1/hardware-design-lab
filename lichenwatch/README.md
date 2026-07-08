# Lichenwatch — In-Situ Lichen Colony Biomonitor

**Author:** jayis1
**License:** MIT
**Version:** 1.0.0

> A long-duration, solar-powered, multi-modal biomonitor that watches living
> lichen colonies in the wild and turns their slow biological responses into
> high-resolution environmental intelligence.

---

## 1. Purpose & Overview

Lichens are symbiotic organisms (fungus + photobiont) that have no roots, no
waxy cuticle, and no way to regulate what they absorb. They take in nutrients
and pollutants directly from air and precipitation, which makes them some of
the most sensitive bioindicators on Earth. A healthy lichen community means
clean air; a stressed, bleached, or species-poor community means trouble long
before any electronic sensor would flag it.

The problem: nobody actually *watches* lichens in the wild at high temporal
resolution. Traditional lichenology is a once-a-year field visit where a
human scrapes a thallus into a tube and sends it to a lab. By the time the
results come back, the pollution event that caused the damage is months gone.

**Lichenwatch** changes this. It is a small, rugged, solar-powered device that
clamps onto a rock or tree surface next to a lichen colony and continuously
measures:

| Modality | What it captures | Biological meaning |
|---|---|---|
| Chlorophyll fluorescence (pulse-amplitude modulation) | Photosynthetic quantum yield of the photobiont | Stress, bleaching, recovery |
| Spectral reflectance (6 bands, 400–900 nm) | Thallus color / NDVI-like indices | Bleaching, heavy-metal chlorosis, species shift |
| Gas exchange (CO₂ / H₂O NDIR + RH) | Net photosynthesis & respiration flux | Metabolic activity, wetness response |
| Microclimate (T, RH, PAR, UV index) | Local surface climate | Drying cycles, light stress, freezing |
| Acoustic micro-vibration (MEMS contact mic) | Thallus crack/pop events | Desiccation fragmentation events |
| Conductance / surface wetness | Conductive grid across thallus | Hydration state driving all metabolism |

An on-device ARM Cortex-M4F runs an edge-ML model that classifies colony state
into **Healthy / Stressed / Bleaching / Recovering / Dormant** every 15 minutes,
emits a compact event record over LoRaWAN, and buffers months of raw data on
local flash for opportunistic BLE download when a surveyor walks past.

Lichenwatch is *not* a generic weather station with a lichen stuck to it. The
whole sensor stack — the fluorescence excitation LEDs, the spectral ring light,
the wetness grid, the contact microphone — is designed around the physics of a
~3 cm lichen thallus and the biology of its photobiont layer, which lives in
the top ~200 µm of the colony.

---

## 2. Why lichens, why now

Lichens are canaries for:

- **Air quality** — SO₂, NOx, ozone, fluoride, and heavy metals accumulate in
  the thallus. Species composition and chlorophyll integrity degrade in
  measurable, well-characterized ways.
- **Climate micro-refugia** — In mountains and tundra, lichen colonies mark
  the microclimates where relict populations survive. Tracking their vigor
  over years reveals refugia that are worth protecting.
- **Radioecology** — Lichens are the primary vector for radionuclide
  accumulation in arctic food webs (cf. the lichen→reindeer→human pathway).
- **Early wildfire recovery** — Lichens are pioneer colonizers of burned rock;
  their return rate is a recovery metric.
- **Planetary protection analogues** — Lichen tolerance of extreme UV,
  vacuum, and cold makes them astrobiology models; field instruments help
  calibrate lab-to-field extrapolation.

Until now, all of these relied on periodic human sampling. Lichenwatch makes
the colony itself a continuously-instrumented sensor.

---

## 3. Hardware Specifications

### 3.1 MCU & Processing

| Block | Part | Role |
|---|---|---|
| Application MCU | STM32L476RG (Cortex-M4F, 128 KB RAM, 1 MB flash, low-power) | Sensor scheduling, edge ML inference, BLE/LoRa stack |
| Radio co-MCU | nRF52840 module (BLE 5, Thread) | BLE + opportunistic mesh relay between nodes |
| LoRa transceiver | Semtech SX1262 (sub-GHz, +22 dBm) | Long-range uplink to a gateway |
| ML accelerator | Tensilica Vision C5 (on-die, Q-mode int8) — emulated in firmware via CMSIS-NN | State classifier |
| Sensor scheduler | RTC + TPL5111 ultra-low-power timer | Sub-µA sleep gating |

### 3.2 Sensor stack (the "thallus head")

The sensor head is a 4 cm × 4 cm PCB that hovers ~3 mm above the lichen
surface on a titanium spring clip. All optics look downward at the colony.

| Sensor | Part | Spec |
|---|---|---|
| PAM fluorometer | Custom: 2 × 470 nm LED (actinic), 1 × 620 nm LED (measuring pulse), Si photodiode + 700 nm long-pass | Modulated at 1–10 kHz; measures F₀, Fm, Fv/Fm yield |
| Spectral reflectance | AS7341 11-channel spectral sensor (415–670 nm + NIR) | 6 bands used for thallus indices |
| CO₂ / H₂O | Senseair S8 (NDIR CO₂) + SHT45 (T/RH) | 0–10000 ppm, ±40 ppm |
| PAR + UV | Si1145 UV/IR/visible + VEML6075 UV-A/UV-B | UV index 0–11+ |
| Surface wetness | Interdigitated gold-plated grid on flex tail, pressed onto thallus | 0–1 normalized conductance |
| Contact acoustic | SPH0641LU421 I²S MEMS, bonded to clip | Detects desiccation crack events |
| Ambient light reject | Shroud + dark foam ring around sensor head | Suppresses stray daylight in fluorescence reads |

### 3.3 Connectivity

- **LoRaWAN 1.0.4, EU868 / US915** — 15-minute state reports + daily raw
  summary; ACK'd downlink for config changes.
- **BLE 5 (nRF52840)** — opportunistic "walk-by" bulk download; also advertises
  a beacon so a surveyor's phone can locate the node in the field.
- **Thread / BLE Mesh** — optional peer relay for nodes in dense lichen mats
  (e.g., tundra polygon transects) where LoRa alone is shadowed.
- **No cellular** — by design. Most lichen sites are remote and we want
  months of autonomous operation.

### 3.4 Power

| Source | Details |
|---|---|
| Solar | 2 × IXYS SLMD121H12L monocrystalline cells, 0.66 W each, mounted on the lid |
| Storage | 1 × Saft LS14500 Li-SOCl₂ (3.6 V, 2.6 Ah) — chosen for −40 °C operation and 10-year shelf life |
| Harvester | TI bq25570 boost charger + maximum-power-point tracking |
| Average budget | ~180 µA average (sensor burst every 15 min, radio every 1 h) → >12 months on battery alone, infinite with solar |
| Peak | ~120 mA during LoRa TX; ~35 mA during fluorescence pulse train |

### 3.5 Form factor

- **Enclosure:** anodized aluminum base (acts as heat sink and EMI shield),
  PTFE-impregnated glass-PA top dome, IP68. Rated −40 to +60 °C.
- **Dimensions:** 96 mm × 78 mm × 38 mm (base), sensor head on 40 mm arm.
- **Mounting:** two M4 titanium spring clips grip rock edges or tree bark;
  optional lashing loops for branch mounting.
- **Weight:** 240 g including battery.
- **Field service:** battery replaceable with a coin-screw back; sensor head
  swappable without re-deploying the radio stack.

### 3.6 Storage

- 64 Mbit (8 MB) Winbond W25Q80 SPI flash for raw waveform + spectral
  snapshots — ~6 months of 15-minute bursts at full resolution.

---

## 4. Architecture & Block Diagram

```
                ┌──────────────────────────────────────────────────┐
                │                  STM32L476RG                     │
                │  Cortex-M4F · FreeRTOS · CMSIS-NN edge model     │
                │  ┌─────────┐ ┌──────────┐ ┌───────────────────┐  │
                │  │Scheduler│ │ Inference│ │  LoRa / BLE Stack  │  │
                │  │ (RTC)   │ │  Engine  │ │  (SX1262 / nRF)    │  │
                │  └────┬────┘ └────┬─────┘ └─────────┬─────────┘  │
                │       │           │                 │             │
                └───────┼───────────┼─────────────────┼────────────┘
                        │           │                 │
        ┌───────────────┼───────────┼─────────────────┼───────────┐
        │               │           │                 │           │
   ┌────▼─────┐  ┌──────▼──────┐  ┌─▼──────────┐  ┌──▼──────┐  ┌──▼──────┐
   │ Thallus  │  │ Env sensors │  │ SPI flash   │  │ SX1262  │  │ nRF52840│
   │ head:    │  │ SHT45, S8,  │  │ W25Q80      │  │ LoRa    │  │ BLE 5   │
   │ PAM LEDs,│  │ Si1145,     │  │ (8 MB)      │  │ +22 dBm │  │ Thread  │
   │ AS7341,  │  │ VEML6075,   │  │             │  │         │  │         │
   │ wet grid │  │ MEMS mic    │  │             │  │         │  │         │
   └────┬─────┘  └──────┬──────┘  └─────────────┘  └─────────┘  └─────────┘
        │               │
        └───────┬───────┘
                │
         ┌──────▼──────┐
         │  bq25570    │  ← 2× SLMD121H12L solar cells
         │  harvester  │  ← LS14500 Li-SOCl₂
         └─────────────┘
```

### 4.1 Data flow

1. The TPL5111 ultra-low-power timer wakes the STM32 every 15 min (typical).
2. The scheduler runs a **measurement burst**: dark-adapt the thallus 30 s
   (shroud), run a PAM saturation-pulse sequence, snap AS7341 reflectance,
   sample SHT45/S8/Si1145/VEML6075/wetness, capture 2 s of contact audio.
3. Features are extracted (Fv/Fm, NDVI-like index, wetness, ΔCO₂ flux, UV
   dose, acoustic event count) and fed to the CMSIS-NN state classifier.
4. The state vector + raw burst are written to SPI flash; the state is queued
   for the next LoRa uplink window.
5. Every 1 h (configurable), the SX1262 transmits the latest state and any
   alerts; downlink can adjust the schedule.
6. On walk-by BLE connect, the nRF52840 streams buffered raw data and the
   companion app renders a "lichen vital signs" dashboard.

---

## 5. Firmware Details & Design Decisions

### 5.1 RTOS & power architecture

FreeRTOS with tickless idle. The whole application is a single measurement
task that sleeps for 15 min, wakes, does a burst, sleeps again. All
peripherals are clock-gated and the STM32L4 drops to Stop 2 mode (≈1.7 µA)
between bursts. The TPL5111 gates the regulator entirely during the long
sleep, bringing system current to ~300 nA.

### 5.2 PAM fluorometry implementation

Pulse-Amplitude Modulation fluorometry is the gold standard for measuring
photosynthesis without contacting the leaf (or lichen). The firmware
implements a simplified saturating-pulse protocol:

1. **Dark adapt** — close optical shroud (a PWM-driven micro-shutter or, in
   the simplest build, just wait in darkness with LEDs off) for 30 s so the
   reaction centers open.
2. **F₀** — low-frequency (1 kHz) 620 nm measuring pulses, record baseline
   fluorescence.
3. **Fm** — fire a 0.8 s saturating pulse of 470 nm actinic light; record
   peak fluorescence.
4. **Yield** — `Fv/Fm = (Fm − F₀) / Fm`, the maximum quantum yield of PSII.
   Healthy lichens: 0.65–0.82. Stressed: <0.55.

The actinic LEDs are current-limited to 1500 µmol photons m⁻² s⁻¹ to avoid
photodamage on repeated reads. The photodiode signal is AC-coupled and
demodulated in firmware with a synchronous (lock-in) detector — this rejects
ambient daylight leaking through the shroud, which is the main reason
field PAM is hard.

### 5.3 Spectral indices for lichens

Lichens don't have a canonical NDVI, but thallus reflectance in the visible
bands tracks chlorophyll degradation and melanin content (a UV-protective
pigment many lichens upregulate). The firmware computes:

- **LNDVI** = `(NIR − Red) / (NIR + Red)` using AS7341's 670 nm and 865 nm
  bands — a lichen analog of NDVI.
- **ChlIndex** = `R(560) / R(670)` — drops as chlorophyll degrades.
- **Melanin proxy** = `R(415) / R(560)` — rises as UV-screening pigments
  accumulate.
- **Bleach index** = `R(415) / R(560)` combined with LNDVI trend.

### 5.4 Edge ML state classifier

A small CMSIS-NN int8 multilayer perceptron (6 inputs → 24 → 16 → 5 outputs)
classifies colony state every 15 min:

| Class | Meaning |
|---|---|
| `HEALTHY` | Fv/Fm > 0.65, LNDVI stable, no acoustic events |
| `STRESSED` | Fv/Fm 0.45–0.65, melanin proxy rising |
| `BLEACHING` | Fv/Fm < 0.45, LNDVI falling, ChlIndex low |
| `RECOVERING` | Fv/Fm rising after a stress event, wetness high |
| `DORMANT` | Frozen or desiccated; no metabolic activity, expected |

The model is trained offline on a labeled lichen-stress dataset and the int8
weights are baked into firmware as a C array. Inference takes ~3 ms on the
M4F.

### 5.5 Acoustic desiccation events

When a lichen thallus dries rapidly, the cortex can micro-fracture and
produce faint audible clicks. The MEMS contact mic samples 2 s at 16 kHz per
burst; the firmware runs a simple energy-threshold click detector and counts
events. A burst of clicks during a rapid dry-down is an early indicator of
thallus fragmentation — a physically irreversible stress event.

### 5.6 LoRa uplink payload

A 19-byte uplink:

```
| 1B class | 2B Fv/Fm*1000 | 2B LNDVI*1000 | 2B ChlIndex*100 |
| 2B wetness*100 | 2B CO2 ppm | 1B T °C | 1B RH % | 1B UV index*10 |
| 1B acoustic events | 2B battery mV | 1B flags | 1B seq |
```

A daily raw summary (64 bytes) carries per-channel statistics for the day.

### 5.7 File layout

```
firmware/
  Makefile            — ARM GCC toolchain, -Os, Cortex-M4F
  board.h             — pin map, peripheral assignments
  registers.h         — STM32L4 register definitions
  main.c              — FreeRTOS tasks, scheduler, measurement loop
  drivers/
    pam_drv.{c,h}     — PAM fluorometer: LED PWM, lock-in demod, Fv/Fm
    spectral_drv.{c,h}— AS7341 I²C driver, lichen indices
    env_drv.{c,h}     — SHT45, S8, Si1145, VEML6075 aggregation
    wetness_drv.{c,h} — ADC + grid conductance
    audio_drv.{c,h}   — I²S MEMS capture, click detector
    flash_drv.{c,h}   — W25Q80 SPI flash ring buffer
    lora_drv.{c,h}    — SX1262 driver, LoRaWAN uplink/downlink
    ble_svc.{c,h}     — BLE GATT service for walk-by download
    power.{c,h}       — bq25570 status, battery gauge, sleep
    inference.{c,h}   — CMSIS-NN state classifier
    model.{c,h}       — int8 weights + quantization scales
```

### 5.8 Key design decisions

- **No cellular, no Wi-Fi.** Lichen sites are remote; LoRa + opportunistic
  BLE is the right trade-off.
- **Li-SOCl₂ not Li-ion.** Lichen monitoring is multi-year; Li-ion would
  degrade and the cold-chain is brutal. Li-SOCl₂ gives 10-year calendar life
  and −40 °C operation.
- **Shroud-coupled PAM.** The single hardest measurement is field
  fluorometry under ambient light. The dark shroud + synchronous detection
  is what makes it work.
- **Sensor head on a spring clip.** Lichen surfaces are irregular; a rigid
  mount would crush the thallus. The spring keeps ~3 mm standoff without
  crushing.
- **On-device inference, not raw streaming.** LoRa bandwidth is tiny; we
  must summarize biology at the edge and only stream state + alerts.

---

## 6. Application / Software Interface

A React Native companion app (see `app/`) provides:

- **Colony Dashboard** — current Fv/Fm, spectral indices, wetness, microclimate,
  and the classified state with a confidence bar. Trend charts over 7/30/365
  days.
- **Walk-by Sync** — BLE scan + connect; downloads buffered raw bursts from
  the node's SPI flash; shows sync progress and frees flash on confirmation.
- **Alerts** — push notifications when a node transitions to BLEACHING or
  logs a desiccation event storm.
- **Site Map** — map of all deployed nodes with their current state as a
  color-coded pin (green/amber/red/blue-dormant).
- **Field Notes** — geo-tagged photo + note attached to a node; useful for
  ground-truthing the classifier ("I see the thallus is visibly bleached here").
- **Config** — adjust measurement cadence, LoRa duty cycle, classifier
  sensitivity; push a new config over BLE downlink.

### BLE GATT service

```
Service UUID: 0000 lic1 0000 1000 8000 00805f9b34fb
  Characteristic lic2  ...  Status  (read, notify)   — latest state vector
  Characteristic lic3  ...  BulkData (write-without-response, notify) — flash stream
  Characteristic lic4  ...  Config   (read, write)    — schedule params
  Characteristic lic5  ...  Info     (read)           — firmware, battery, node ID
```

### LoRaWAN downlink commands

| Port | Payload | Action |
|---|---|---|
| 1 | state report (default uplink) | — |
| 2 | daily raw summary | — |
| 10 | `[interval_min][0xFF]` | set measurement interval |
| 11 | `[lora_hours][0xFF]` | set uplink cadence |
| 12 | `[0xA5]` | force immediate burst + uplink |
| 20 | `[class_thresholds...]` | retune classifier sensitivity |

---

## 7. Use Cases & Target Audience

### 7.1 Environmental protection agencies
Deploy a transect of Lichenwatch nodes downwind of an industrial site. A
sustained drop in Fv/Fm with rising melanin proxy is an early-warning that
emissions are exceeding what the local lichen community can tolerate — weeks
before a stack monitor would catch a diffuse fugitive source.

### 7.2 Climate ecologists
Mountain-top and tundra lichen colonies are climate micro-refugia. A
multi-year Lichenwatch record shows whether a refugium is stable, expanding,
or collapsing — the kind of evidence that drives protection decisions.

### 7.3 Forest & wildfire managers
Post-fire rock surfaces are recolonized by lichens over years. Lichenwatch
nodes on burned outcrops give a quantitative recovery curve, useful for
post-fire restoration reporting.

### 7.4 Radioecology & nuclear safety
Lichens accumulate radionuclides. Pairing Lichenwatch with periodic gamma
spectrometry gives a continuous biological-uptake proxy between lab samples.

### 7.5 Astrobiology field analogues
Lichen extremophile sites (high-UV alpine, Antarctic dry valleys) are used
to calibrate life-detection instrument concepts. Lichenwatch gives
ground-truth biological state alongside the analogue instrument suite.

### 7.6 Citizen science / lichen enthusiast community
The walk-by BLE sync and friendly app make a single node usable by a
naturalist who wants to track the lichen on their favorite boulder. The
LoRa uplink is optional; the app works fully offline over BLE.

---

## 8. Bill of Materials (summary)

| Ref | Part | Qty | Note |
|---|---|---|---|
| U1 | STM32L476RGT6 | 1 | MCU |
| U2 | nRF52840 module | 1 | BLE/Thread |
| U3 | SX1262 | 1 | LoRa |
| U4 | bq25570 | 1 | solar harvester |
| U5 | AS7341 | 1 | spectral sensor |
| U6 | SHT45 | 1 | T/RH |
| U7 | Senseair S8 | 1 | CO₂ NDIR |
| U8 | Si1145 | 1 | UV/visible |
| U9 | VEML6075 | 1 | UV-A/B |
| U10 | SPH0641LU421 | 1 | I²S MEMS mic |
| U11 | W25Q80 | 1 | 8 MB SPI flash |
| U12 | TPL5111 | 1 | nano-power timer |
| D1–D3 | 470 nm / 620 nm LEDs | 3 | PAM excitation |
| Q1 | TEMD5080 Si photodiode | 1 | fluorescence detect |
| BAT1 | Saft LS14500 | 1 | Li-SOCl₂ |
| SOL1–2 | SLMD121H12L | 2 | solar cells |

---

## 9. Calibration & field protocol

1. **Dark adaptation** is mandatory for Fv/Fm — the shroud must close. The
   firmware checks the photodiode dark level and aborts the burst if the
   shroud failed.
2. **Wetness baseline** is site-specific; the first 24 h after install are
   used to establish a dry-state and wet-state conductance for that colony.
3. **Spectral baseline** is captured at install and used as the reference
   for trend indices — absolute reflectance varies hugely between lichen
   species, but trends are robust.
4. **Classifier retraining** is done offline from walk-by downloads; a new
   int8 model is pushed over BLE in ~2 s.

---

## 10. Limitations & honest notes

- PAM fluorometry on lichens is less validated than on higher plants; the
  cortex and pigments scatter excitation light. Lichenwatch is a *trend*
  instrument, not an absolute photosynthesis meter.
- The acoustic click detector is a novel signal; its interpretation will
  mature with field data.
- CO₂ flux from a 3 cm thallus is tiny; the S8 reading is dominated by
  ambient CO₂. The firmware reports the local CO₂, not a true per-thallus
  flux. (Future rev: a small chamber.)
- Li-SOCl₂ has a steep voltage curve; the gauge is approximate.

These are real constraints, not marketing. The device is a useful,
honest field instrument within them.

---

## 11. Repository layout

```
lichenwatch/
  README.md          ← you are here
  firmware/          ← C firmware, FreeRTOS, CMSIS-NN
    Makefile
    board.h
    registers.h
    main.c
    drivers/
      pam_drv.{c,h}
      spectral_drv.{c,h}
      env_drv.{c,h}
      wetness_drv.{c,h}
      audio_drv.{c,h}
      flash_drv.{c,h}
      lora_drv.{c,h}
      ble_svc.{c,h}
      power.{c,h}
      inference.{c,h}
      model.{c,h}
  kicad/             ← KiCad schematic + PCB + project
    device.kicad_sch
    device.kicad_pcb
    device.kicad_pro
  app/               ← React Native companion app
    App.js
    package.json
    components/
    screens/
    utils/
```

---

## 12. License

MIT © jayis1. See LICENSE in repo root.

---

*Designed, written, and built by jayis1.*