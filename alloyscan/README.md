# AlloyScan — Handheld Multi-Frequency Eddy Current Metal Alloy Identifier

**Author:** jayis1  
**Copyright © 2026 jayis1. All rights reserved.**  
**License:** CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)

![AlloyScan](https://img.shields.io/badge/Form%20Factor-Pen%20probe%20%2B%20grip-blue)
![MCU](https://img.shields.io/badge/MCU-STM32G474%20Cortex--M4-orange)
![Method](https://img.shields.io/badge/Method-4%2Dfreq%20eddy%20current-green)
![Wireless](https://img.shields.io/badge/Wireless-BLE%205.0-lightgrey)
![Display](https://img.shields.io/badge/Display-1.3%22%20OLED-yellow)
![Power](https://img.shields.io/badge/Power-18650%20LiPo-red)
![Author](https://img.shields.io/badge/Author-jayis1-orange)

---

## 1. Purpose and Overview

**AlloyScan** is a handheld, battery-powered instrument that identifies metal
alloys by their electromagnetic fingerprint — non-destructively, instantly, and
at a fraction of the cost of laboratory equipment. You press the probe tip
against a metal surface, pull the trigger, and within 200 milliseconds the
device tells you what alloy you are looking at: "304 Stainless Steel",
"6061-T6 Aluminum", "Grade 5 Titanium (Ti-6Al-4V)", "C36000 Free-Cutting
Brass", or one of 60+ reference alloys stored in its on-device database.

### The Problem

Metal alloy identification is a critical need across scrap recycling, machine
shops, foundries, metallurgy labs, construction inspection, and customs /
quality control. The stakes are high:

- **Scrap yards** sort incoming metal by alloy grade — mixing a load of 304 SS
  with 316 SS can cost thousands of dollars, and misidentifying a critical
  aerospace alloy can cause catastrophic failure downstream.
- **Machine shops** need to verify bar stock before machining an expensive part.
  A wrong-alloy part in an aircraft or medical device can be fatal.
- **Foundries** need to verify incoming scrap chemistry before charging a melt.
  The wrong alloy in a heat means a scrapped batch and tens of thousands of
  dollars lost.
- **Construction inspectors** need to verify rebar grade and structural steel
  certification on-site.
- **Customs and QC** need to verify material certificates on incoming goods.

Current solutions fall into two extremes:

| Solution | Cost | Capability |
|---|---|---|
| **Visual / magnet / spark test / file test** | $0–$20 | Crude; distinguishes only broad categories (ferrous vs non-ferrous, austenitic vs martensitic SS); operator skill dependent; no grade-level resolution |
| **Portable XRF (X-ray fluorescence)** | $15,000–$50,000 | Lab-grade chemistry; identifies elemental composition; requires licensing, radiation safety, expensive X-ray tube; fragile |
| **Portable OES (optical emission spectrometry)** | $20,000–$40,000 | Lab-grade; requires argon, surface prep, sparks; not safe around flammables |
| **Laboratory ICP-OES / combustion analysis** | $100+/sample, days of turnaround | Gold standard chemistry; not field-portable |

**AlloyScan fills the gap** between crude manual tests and expensive XRF/OES. It
costs ~$100 in parts, requires no radiation safety training, works on any
electrically conductive metal, and identifies the alloy *grade* (not just the
elemental composition) using the same physics that sorting coils in recycling
plants have used for decades — eddy current testing — miniaturized into a
handheld pen.

### How Eddy Current Identification Works

When an alternating current flows through a coil near a conductive surface,
the oscillating magnetic field induces circulating **eddy currents** in the
metal. These eddy currents generate their own opposing magnetic field, which
modifies the impedance of the excitation coil. The magnitude and phase of this
impedance change depends on three material properties:

1. **Electrical conductivity (σ):** Ranges from ~0.6% IACS (manganese bronze) to
   ~100% IACS (silver). Even small alloying additions change conductivity
   measurably.
2. **Magnetic permeability (μ):** Ferromagnetic metals (carbon steel, ferritic
   SS) have high permeability (μᵣ >> 1); austenitic SS and non-ferrous metals
   have μᵣ ≈ 1. This is a strong discriminant between steel families.
3. **Skin depth penetration:** The depth to which eddy currents penetrate
   varies inversely with √(frequency). By probing at **multiple frequencies**,
   AlloyScan gathers depth-profiled information — a fingerprint that encodes
   not just the surface composition but the bulk electromagnetic character of
   the alloy.

At each frequency, the complex impedance change (ΔR + jΔX) yields a point in
the impedance plane. A sweep across four frequencies (1 kHz, 10 kHz, 100 kHz,
500 kHz) produces an 8-dimensional feature vector (4 freq × I/Q). This vector is
characteristic of the alloy — two different alloys may have similar
conductivity but will differ in permeability, frequency dispersion, or
phase response. AlloyScan's on-device k-NN classifier compares the measured
vector against a pre-loaded reference database of 60+ alloys and reports the
nearest match with a confidence score.

### Lift-Off Compensation

The distance between the coil and the metal surface ("lift-off") affects the
signal amplitude. AlloyScan compensates for lift-off using a normalized
feature representation: at each frequency, the impedance vector is rotated so
that the lift-off direction lies along one axis, then the signal is
projected onto the orthogonal (material) axis. This makes the classification
robust to the user pressing harder or softer, or to thin paint/paint coatings
up to ~0.5 mm.

### Edge Effect Suppression

Measurements near edges, corners, or holes produce distorted eddy current
patterns. AlloyScan monitors the phase coherence across frequencies: if the
four-frequency signatures are inconsistent (high cross-frequency residual),
the device warns "EDGE — reposition" rather than guessing.

---

## 2. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32G474CEU6 (Cortex-M4 @ 170 MHz, FPU, CORDIC, FMAC) |
| **Excitation** | 4 simultaneous sinusoidal tones: 1, 10, 100, 500 kHz |
| **Excitation Driver** | THS3091 high-speed current-feedback op-amp, ±5 V supply, up to ±250 mA coil drive |
| **Probe Coils** | Custom wound ferrite-cored coaxial pair: outer excitation (Ø12 mm, 80 turns), inner sense (Ø5 mm, 200 turns) |
| **Receive Front-End** | AD8429 low-noise instrumentation amplifier (1 nV/√Hz), gain 100×, bandwidth 2 MHz |
| **ADC** | ADS8866 16-bit 1 MSPS SAR ADC (SPI) — dual-channel option for I/Q or simultaneous sampling |
| **Anti-alias Filter** | 700 kHz 4th-order Bessel low-pass (MAX7403 switched-cap + passive LC) |
| **Lift-Off Sensor** | VL53L0X time-of-flight laser distance sensor (±3% accuracy to 30 mm) |
| **Display** | 1.3 inch OLED, SH1106 controller, 128×64 monochrome, SPI |
| **Wireless** | BLE 5.0 via BMD-300 module (Nordic nRF52832, UART bridge) |
| **Audio** | Piezo buzzer + MAX98357A I²S amp for spoken result via BLE phone |
| **Storage** | W25Q128 16 MB SPI flash — reference database + scan log (up to 100,000 scans) |
| **Power** | Single 18650 Li-ion cell (3.7 V, 2600 mAh), TP4056 USB-C charger, ~12 hours continuous |
| **Power Management** | TPS63020 buck-boost (3.3 V system), LM7660 inverter (−5 V for op-amp), AP2112 LDO (3.3 V for BLE/display) |
| **Input** | Momentary trigger switch (SPST), side button for menu navigation |
| **Enclosure** | 3D-printed PETG pen-grip body, 35 × 35 × 160 mm, probe tip replaceable |
| **Board Size** | 34 × 28 mm, 4-layer FR-4 |
| **Weight** | ~140 g with battery |
| **BOM Cost** | ~$97 USD (1K volume) |
| **Operating Temp** | −10 °C to +50 °C |
| **Measurement Time** | 200 ms per scan (typical) |
| **Alloy Database** | 64 reference alloys (ferrous, aluminum, copper, titanium, nickel, zinc, magnesium, specialty) |

### Alloy Database Coverage

| Family | Example Grades | Count |
|---|---|---|
| Carbon / Low-Alloy Steel | 1018, 1045, 12L14, 4140, 4340, A36, 8620 | 10 |
| Stainless Steel (Austenitic) | 304, 316, 316L, 904L, 321, 304L | 8 |
| Stainless Steel (Ferritic/Martensitic) | 410, 420, 430, 440C, 17-4 PH | 7 |
| Stainless Steel (Duplex) | 2205, 2507 | 2 |
| Aluminum | 1100, 3003, 5052, 6061, 6063, 7075, 2024, 356 casting | 10 |
| Copper / Brass / Bronze | C11000 Cu, C26000 Cartridge Brass, C36000 Free-Cutting Brass, C46400 Naval Brass, C95400 Al Bronze | 9 |
| Titanium | CP Grade 1, Grade 2, Grade 5 (Ti-6Al-4V), Grade 9 | 4 |
| Nickel Alloys | Inconel 625, Inconel 718, Monel 400, Hastelloy C-276 | 6 |
| Zinc / Magnesium / Others | Zn die-cast, AZ31B Mg, AZ91D Mg, Pb, Sn solder | 8 |
| **Total** | | **64** |

---

## 3. Architecture and Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         STM32G474CEU6                                │
│                    Cortex-M4 @ 170 MHz                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  DAC1 Ch0/Ch1 │  │  CORDIC +    │  │  FMAC Filter │              │
│  │  DMA multi-   │  │  DSP (lock-  │  │  Engine      │              │
│  │  tone DDS     │  │  in I/Q)     │  │              │              │
│  └──────┬───────┘  └──────┬───────┘  └──────────────┘              │
│         │                  ▲                                        │
│         ▼                  │                                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  SPI1: ADC   │  │  SPI2: OLED  │  │  SPI3: Flash │              │
│  │  (ADS8866)   │  │  (SH1106)    │  │  (W25Q128)   │              │
│  └──────┬───────┘  └──────────────┘  └──────────────┘              │
│         │                  ▲                                        │
│         ▼                  │                                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │  USART2 BLE  │  │  GPIO trigger │  │  I²C VL53L0X │              │
│  │  (BMD-300)   │  │  + nav button │  │  (liftoff)   │              │
│  └──────────────┘  └──────────────┘  └──────────────┘              │
└─────────┬───────────────────────────────────────────────────────────┘
          │ DAC output (multi-tone)
          ▼
┌──────────────────────────────────────────────────────┐
│                ANALOG FRONT-END                        │
│  DAC → 2nd-order reconstruction LPF (500 kHz BW)       │
│       → THS3091 power amp (±5 V, ±250 mA)             │
│       → Excitation coil (outer, ferrite core)          │
│                                                        │
│  Sense coil (inner) → AD8429 in-amp (G=100)            │
│       → MAX7403 + LC anti-alias LPF (700 kHz)          │
│       → ADS8866 16-bit ADC → SPI1 → MCU                │
└──────────────────────────────────────────────────────┘
          │
          ▼
┌──────────────────────────────────────────────────────┐
│                   PROBE HEAD                           │
│  ┌─────────────────────────────────┐                  │
│  │  ╔═════════════════════════════╗│  ← Excitation     │
│  │  ║  Excitation coil (Ø12mm)    ║│    coil (outer)   │
│  │  ║  ┌───────────────────────┐  ║│                  │
│  │  ║  │  Sense coil (Ø5mm)    │  ║│  ← Sense coil    │
│  │  ║  │  ┌─────────────────┐  │  ║│    (inner)       │
│  │  ║  │  │  VL53L0X ToF    │  │  ║│                  │
│  │  ║  │  │  (lift-off)     │  │  ║│                  │
│  │  ║  │  └─────────────────┘  │  ║│                  │
│  │  ║  └───────────────────────┘  ║│                  │
│  │  ╚═════════════════════════════╝│                  │
│  └─────────────────────────────────┘                  │
│  Ferrite cup core, PTFE tip, replaceable              │
└──────────────────────────────────────────────────────┘
```

### Signal Flow

1. **Excitation:** The MCU's DAC generates a composite multi-tone waveform
   (sum of 4 sinusoids at 1, 10, 100, 500 kHz) via a DDS table driven by DMA
   at 2 MHz sample rate. This analog signal passes through a reconstruction
   low-pass filter and into the THS3091 power amplifier, which drives up to
   ±250 mA through the excitation coil.

2. **Sensing:** The eddy-current-modified field is picked up by the inner sense
   coil. The AD8429 instrumentation amplifier amplifies this by 100× (40 dB),
   rejecting common-mode noise. The MAX7403 switched-capacitor filter plus a
   passive LC stage provides a 700 kHz anti-alias low-pass before the ADS8866
   16-bit ADC samples at 1 MSPS.

3. **Digital Lock-In:** In firmware, the digitized signal is multiplied by
   reference cosine and sine tables at each of the 4 excitation frequencies
   (in-phase/quadrature demodulation). A CORDIC-assisted CIC decimation filter
   extracts the I and Q components at each frequency, yielding 8 real values
   (4 frequencies × 2 quadratures).

4. **Lift-Off Compensation:** The VL53L0X measures probe-to-surface distance.
   The I/Q vectors are normalized and rotated using the known lift-off
   direction (determined during factory calibration) to suppress the
   lift-off component.

5. **Classification:** The 8-dimensional compensated feature vector is fed to
   a k-NN classifier (k=5, Euclidean distance, weighted by 1/d) that compares
   against the on-flash reference database. The top match, its confidence, and
   the 3 nearest alternatives are displayed.

6. **UI:** The SH1106 OLED shows the result. The piezo buzzer gives audible
   feedback (different tones for high/medium/low confidence). Results are
   logged to flash and streamed via BLE to the companion app.

---

## 4. Firmware Design

### Design Philosophy

The firmware is written in portable C99 targeting the STM32G474. It uses:

- **Bare-metal, no RTOS** — the measurement loop is deterministic and short
  (200 ms), so a cooperative state machine is simpler and more predictable
  than a scheduler.
- **CORDIC hardware accelerator** for sin/cos generation and complex
  multiplication in the lock-in stage — 5–10× faster than software float math.
- **FMAC (Filter Mathematics Accelerator)** for the CIC decimation filters —
  offloads the heaviest DSP loop from the CPU.
- **DMA for all data movement** — DAC output, ADC input, SPI flash, OLED
  refresh — so the CPU is free for DSP and classification.

### File Structure

```
firmware/
├── Makefile
├── board.h           — Pin assignments, clock config, peripheral mapping
├── registers.h       — STM32G474 register definitions (subset)
├── main.c            — Main loop, state machine, measurement orchestration
├── drivers/
│   ├── dac_drv.h/c   — Multi-tone DDS DAC driver (DMA-based)
│   ├── adc_drv.h/c   — ADS8866 SPI ADC driver
│   ├── lockin.h/c    — Digital lock-in amplifier (I/Q demod, CORDIC)
│   ├── oled_drv.h/c  — SH1106 OLED display driver (SPI, framebuffer)
│   ├── flash_drv.h/c — W25Q128 SPI flash driver (JEDEC ID, page read/write)
│   ├── ble_drv.h/c   — BMD-300 BLE UART driver (Nordic UART Service)
│   ├── tof_drv.h/c   — VL53L0X time-of-flight driver (I²C)
│   ├── button.h/c    — Debounced GPIO button driver
│   └── battery.h/c   — ADC battery monitor
├── alloy_db.h/c      — Alloy reference database (64 alloys, stored in flash)
├── classifier.h/c    — k-NN classifier with lift-off compensation
├── ui.h/c            — OLED UI state machine (scan, result, menu, cal)
└── calibration.h/c   — Factory calibration (coil constants, lift-off vector)
```

### State Machine

```
  ┌─────────┐  trigger  ┌──────────┐  200ms  ┌──────────┐  timeout
  │  IDLE   │─────────→│  SCANNING │────────→│  RESULT  │──────────┐
  │         │←─────────│           │         │          │          │
  └────┬────┘  timeout  └──────────┘         └────┬─────┘          │
       │                                            │                ▼
       │ side btn  ┌──────────┐    ┌─────────┐  ┌──────────┐
       └─────────→│  MENU    │───→│ CALIB.  │←─│  LOG     │
                   │ browse   │    │ (open/  │  │ (review) │
                   │ alloys   │    │  short/ │  └──────────┘
                   └──────────┘    │  load)  │
                                   └─────────┘
```

### Key Algorithms

#### Multi-Tone DDS Excitation

A 2048-sample lookup table contains one cycle of a composite waveform: the sum
of unit-amplitude sinusoids at 1, 10, 100, and 500 kHz, normalized to the DAC
range. The DAC DMA reads this table cyclically at 2 MHz, producing a continuous
multi-frequency excitation. The relative amplitudes are weighted (×1.0 at
1 kHz, ×0.8 at 10 kHz, ×0.6 at 100 kHz, ×0.4 at 500 kHz) to compensate for
the coil's frequency-dependent impedance.

#### Digital Lock-In Amplifier

For each excitation frequency fₖ, the firmware maintains a numerically
controlled oscillator (NCO) phase accumulator. On each ADC sample (1 MSPS),
the sample is multiplied by cos(φₖ) and sin(φₖ) where φₖ is the current NCO
phase. The products are accumulated over a measurement window of N=200 samples
(200 µs), then the accumulator is read and reset. This yields:

- Iₖ = Σ s[n] · cos(φₖ[n])  (in-phase component)
- Qₖ = Σ s[n] · sin(φₖ[n])  (quadrature component)

The magnitude |Zₖ| = √(Iₖ² + Qₖ²) and angle ∠Zₖ = atan2(Qₖ, Iₖ) characterize the
impedance change at frequency fₖ. The CORDIC unit performs the atan2 and
sqrt operations in hardware.

#### Lift-Off Compensation

During factory calibration, the user scans a reference sample (provided:
a known 6061 aluminum block) at three known lift-off distances (contact, 0.5 mm,
1.0 mm). The resulting I/Q vectors define the lift-off direction in the
impedance plane at each frequency. At runtime, the measured vector is
projected onto the axis perpendicular to the lift-off direction, yielding a
lift-off-compensated feature component.

#### k-NN Classifier

The classifier stores 64 alloy reference vectors in flash (8 floats × 4 bytes =
32 bytes per alloy, plus a 16-byte name = 48 bytes/alloy, total ~3 KB). On
each scan, it computes the Euclidean distance from the measured vector to all
64 references, selects the 5 nearest, and computes a confidence score:

  confidence = Σ(1/dᵢ) / Σ(1/dᵢ) for the winning class, over all 5 neighbors.

If confidence > 0.7: green LED + high beep. If 0.4–0.7: yellow + medium beep.
If < 0.4: red + low beep + "UNCERTAIN" display.

---

## 5. Application / Software Interface

### Companion App (React Native)

The AlloyScan companion app runs on iOS and Android via React Native + BLE
library (`react-native-ble-plx`). It provides:

1. **Live Scan View** — Receives scan results over BLE in real time. Shows the
   identified alloy, confidence, the impedance-plane plot (polar chart of the
   4-frequency I/Q vectors), and the 5 nearest matches.

2. **Reference Database Browser** — Browse all 64 alloys with their properties
   (conductivity %IACS, permeability, typical applications, density, common
   misidentifications). Searchable and filterable by family.

3. **Scan History / Log** — All scans are timestamped and geotagged (phone
   GPS). Export to CSV / JSON. Build heat-maps of scrap pile composition.

4. **Calibration Assistant** — Guides the user through factory calibration
   (open-circuit, short-circuit, reference block scans). Verifies coil
   integrity and drift.

5. **Firmware Update** — OTA firmware update via BLE DFU (Nordic Secure
   Bootloader on BMD-300).

6. **Custom Alloy Training** — The user can create a custom alloy entry by
   scanning a known sample 10 times. The app averages the vectors and uploads
   the new reference to the device flash. Useful for proprietary alloys.

### BLE Protocol

The device advertises as "AlloyScan-XXXX" (last 2 bytes of MAC). The Nordic
UART Service (NUS) provides a simple serial-over-BLE channel:

- TX characteristic (device → phone): scan results, log entries, status
- RX characteristic (phone → device): commands (trigger, mode, calibrate, train)

Message format: JSON lines, one JSON object per line, terminated by `\n`.

```json
{"type":"scan","ts":1721812800,"alloy":"304 SS","confidence":0.89,"alternatives":["316 SS","304L"],"iq":[[0.12,0.34],[0.56,0.78],[0.91,0.23],[0.45,0.67]],"liftoff_mm":0.3}
```

---

## 6. Use Cases and Target Audience

### Scrap Metal Recycling

The single largest application. Recycling facilities process tons of mixed
metal daily. Current handheld sorters use permanent magnets (only separates
ferrous from non-ferrous) or expensive XRF guns ($20K+). AlloyScan lets every
sorter on the floor carry a grade-level identifier. A single mis-sorted
300-series stainless batch can cost $500+; AlloyScan pays for itself in one
shift.

### Machine Shops and Manufacturing

Verifying incoming bar stock and billets before machining. "Is this 6061 or
7075?" is a daily question — they look identical but machine completely
differently and have vastly different mechanical properties. Mixing them in a
batch means scrapped parts. A 5-second scan on the receiving dock prevents
expensive mistakes.

### Foundries and Die Casters

Verifying incoming scrap chemistry before charging a melt. A load of "clean
aluminum" may contain zinc-contaminated alloy that will ruin an entire heat.
AlloyScan provides instant incoming inspection.

### Construction and Structural Inspection

Verifying rebar grade (Grade 40 vs Grade 60 vs Grade 80), structural steel
certification, bolt grade (A325 vs A490), and stainless steel type on installed
hardware. Code inspectors can verify material certifications on-site without
destroying samples.

### Welding and Fabrication

Verifying base metal before welding — critical for selecting the correct
filler metal and procedure. Welding 410 SS with 308 filler because you
misidentified the base metal leads to cracking and failure.

### Quality Control and Customs

Incoming inspection of metal parts and assemblies. Verify that supplier
certifications match the actual material. Detect substitution of cheaper
alloys (e.g., 304 sold as 316).

### Education and Hobby

Metallurgy education, maker spaces, metal detector enthusiasts, knife makers
(verify steel grade before heat treating), jewelry makers (verify precious
metal alloys).

### Target Audience

| Audience | Pain Point | AlloyScan Value |
|---|---|---|
| Scrap yard operators | Mis-sorted loads cost $500–$5,000 each | $100 device, grade-level sorting, no licensing |
| Machine shop QC | Wrong-alloy parts scrapped ($100–$10,000 each) | 5-second receiving dock verification |
| Foundry metallurgists | Bad scrap chemistry ruins melts | Instant incoming scrap check |
| Construction inspectors | Can't verify installed rebar/steel grade | Non-destructive on-site check |
| Welding engineers | Wrong filler/base metal combo = cracks | Pre-weld base metal verification |
| Customs / import QC | Certificate fraud, cheap alloy substitution | Verify material at the dock |

---

## 7. Calibration and Maintenance

### Factory Calibration (one-time, ~5 minutes)

1. **Open-circuit:** Hold probe in air (no metal nearby). The firmware records
   the baseline coil impedance at each frequency. This is the "zero" reference.
2. **Reference block:** Scan the provided 6061-T6 aluminum calibration block
   at contact, 0.5 mm, and 1.0 mm lift-off (using provided spacer shims). The
   firmware computes the lift-off direction vectors.
3. **Short-circuit:** Press probe against a solid copper block. Records the
   maximum-conductivity reference point.

All calibration data is stored in a dedicated flash sector (4 KB, wear-leveled).

### Periodic Recalibration

The firmware monitors calibration drift by comparing the open-circuit
baseline. If drift exceeds 5%, the OLED displays "CAL NEEDED" and refuses to
scan until recalibration is performed. Typical recalibration interval: 6
months or 5,000 scans.

### Probe Tip Replacement

The PTFE probe tip wears over time (especially on rough castings). The tip is
threaded and replaceable (M6 × 0.5). A worn tip changes the lift-off offset;
the VL53L0X compensates automatically, but tips should be replaced every
~2,000 scans on rough surfaces.

---

## 8. Bill of Materials (Summary)

| Component | Part | Qty | Unit Cost | Total |
|---|---|---|---|---|
| MCU | STM32G474CEU6 | 1 | $6.50 | $6.50 |
| Op-amp (excitation) | THS3091D | 1 | $4.20 | $4.20 |
| Instrumentation amp | AD8429ARZ | 1 | $7.80 | $7.80 |
| ADC | ADS8866IDRCR | 1 | $5.50 | $5.50 |
| Anti-alias filter | MAX7403CPA+ | 1 | $3.10 | $3.10 |
| BLE module | BMD-300 | 1 | $8.00 | $8.00 |
| OLED display | SH1106 1.3" | 1 | $3.50 | $3.50 |
| SPI flash | W25Q128JVSIQ | 1 | $1.20 | $1.20 |
| ToF sensor | VL53L0X | 1 | $2.80 | $2.80 |
| Charger | TP4056 | 1 | $0.40 | $0.40 |
| Buck-boost | TPS63020DSJR | 1 | $3.80 | $3.80 |
| Inverter | LM7660IN | 1 | $0.90 | $0.90 |
| LDO | AP2112K-3.3 | 1 | $0.30 | $0.30 |
| Ferrite core | Amidon FT-50-43 | 1 | $1.50 | $1.50 |
| Coil wire | 30 AWG enamelled | — | $0.50 | $0.50 |
| Connectors, passives, PCB | Misc | — | $46.00 | $46.00 |
| **Total** | | | | **$96.50** |

---

## 9. Limitations and Future Work

### Current Limitations

- **Surface condition:** Heavy rust, scale, or paint >1 mm reduces accuracy.
  The lift-off sensor compensates up to ~0.5 mm coating, but thick paint
  requires surface preparation.
- **Geometry:** Small parts (<10 mm) or thin sheet (<0.5 mm) produce edge
  effects that reduce classification confidence. The edge-detection algorithm
  warns the user.
- **Alloy database:** 64 alloys cover the vast majority of industrial metals,
  but proprietary or exotic alloys may not be in the database. The custom
  training feature addresses this.
- **No elemental chemistry:** AlloyScan identifies the *alloy grade* by
  electromagnetic fingerprint, not the exact elemental composition. It cannot
  replace XRF for forensic chemistry. For most sorting and verification tasks,
  grade-level identification is sufficient.

### Planned Improvements

- **Expanded database:** 200+ alloys via cloud-synced reference library
- **Multi-probe heads:** Interchangeable probes optimized for different
  frequency ranges and geometries (flat, curved, internal bore)
- **Temperature compensation:** RTD sensor to compensate conductivity
  temperature coefficient (currently assumes ambient ~20 °C)
- **Pulsed eddy current mode:** Time-domain analysis for thick-wall / through-
  wall measurements
- **On-device neural network:** Replace k-NN with a small quantized NN for
  improved discrimination of closely-spaced alloys (e.g., 304 vs 316)

---

## 10. License and Credits

- **Hardware:** CERN-OHL-S v2 — anyone may manufacture, modify, and distribute
  the PCB and mechanical designs.
- **Firmware:** GPL-2.0 — source available, modifications must be shared.
- **Companion App:** MIT — permissive, suitable for commercial integration.
- **Author:** jayis1 — all designs, firmware, code, and documentation.

---

*AlloyScan: Know your metal. In your pocket. For $100.*