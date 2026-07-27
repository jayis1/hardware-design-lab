# LithoCore — Portable Lithium-Ion Cell Degradation & Impedance Spectrometer

> A pocket-sized, open-hardware diagnostic instrument that performs
> **Electrochemical Impedance Spectroscopy (EIS)** on individual lithium-ion
> cells (18650, 21700, 26650, or small packs) from 0.01 Hz to 100 kHz,
> simultaneously measuring DC internal resistance, true capacity via Coulomb
> counting, self-discharge rate, and open-circuit voltage relaxation curves —
> then classifies cell health and predicts remaining cycle life entirely on
> device using an embedded electrochemical model. Results stream over BLE 5.2
> or USB-C to a companion app that builds per-cell impedance Nyquist and Bode
> plots, trend history, and pack assembly reports.

![MCU](https://img.shields.io/badge/MCU-STM32G474-orange) ![EIS](https://img.shields.io/badge/EIS-0.01Hz–100kHz-blue) ![ADC](https://img.shields.io/badge/ADC-24--bit%20%2B%2016--bit%20simul-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.2%20%2B%20USB--C-purple) ![Author](https://img.shields.io/badge/Author-jayis1-red) ![License](https://img.shields.io/badge/License-CERN--OHL--S%20v2-yellow)

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)
**Version:** 1.0.0
**Date:** 2026-07-27

---

## Table of Contents

1. [Purpose & Overview](#1-purpose--overview)
2. [Why This Is Novel](#2-why-this-is-novel)
3. [How EIS Reveals Cell Health](#3-how-eis-reveals-cell-health)
4. [Hardware Specifications](#4-hardware-specifications)
5. [System Architecture & Block Diagram](#5-system-architecture--block-diagram)
6. [Firmware Design](#6-firmware-design)
7. [Impedance Measurement Pipeline](#7-impedance-measurement-pipeline)
8. [Cell Health Classification Model](#8-cell-health-classification-model)
9. [Application / Software Interface](#9-application--software-interface)
10. [Use Cases & Target Audience](#10-use-cases--target-audience)
11. [Power Budget](#11-power-budget)
12. [Mechanical & Form Factor](#12-mechanical--form-factor)
13. [Bill of Materials](#13-bill-of-materials)
14. [Calibration & Safety](#14-calibration--safety)
15. [Repository Layout](#15-repository-layout)
16. [Licensing & Credits](#16-licensing--credits)

---

## 1. Purpose & Overview

LithoCore is a handheld instrument — roughly the size of a thick marker pen —
that **characterizes the electrochemical health of individual lithium-ion
cells** in seconds. You clip it onto a cell (or touch the probes to a pack
terminal pair), press the single button, and LithoCore runs a complete
diagnostic sweep:

1. **Open-circuit voltage (OCV)** measurement with 0.1 mV resolution.
2. **Electrochemical Impedance Spectroscopy (EIS)** sweep from 0.01 Hz to
   100 kHz — injecting a small (< 20 mA) AC perturbation and measuring the
   complex voltage/current response at 48 logarithmically-spaced frequencies.
3. **DC internal resistance (DCIR)** via a 2 A, 100 ms discharge pulse followed
   by a relaxation capture.
4. **Self-discharge rate** estimation from the OCV relaxation slope over a
   configurable 30-second to 10-minute window.
5. **Quick capacity check** (optional) via Coulomb-counted partial discharge at
   a user-selected C-rate, extrapolated by the on-device model.

The result is a **Nyquist plot** (imaginary vs. real impedance), a **Bode
plot** (magnitude and phase vs. frequency), a **state-of-health (SoH) score**
from 0–100 %, and a **degradation mode classification** (healthy, SEI growth,
lithium plating, electrolyte dry-out, internal short / self-discharge fault).
All computation happens on the STM32G474 — no cloud, no phone required for the
core measurement. The companion app provides visualization, historical
trending, and pack-building workflows.

### Who needs this?

The lithium-ion second-life market is exploding. E-bike builders, e-skate
makers, DIY powerwall enthusiasts, and off-grid solar installers salvage
hundreds of cells from old laptop packs and EV modules. The current state of
the art for these users is a **multimeter and a hobby charger** — which can
measure voltage and (slowly) capacity, but **cannot detect incipient
degradation**. A cell may read 4.20 V and deliver 2800 mAh yet be on the verge
of lithium plating that will cause thermal runaway in the next 50 cycles. EIS
catches this; voltage and capacity alone do not.

Professional EIS instruments (Biologic VMP-300, Gamry Reference, Zahner) cost
$8,000–$40,000, are benchtop-only, and require a PC. LithoCore brings the key
diagnostic — the impedance fingerprint — to a $120 handheld device that runs
on a coin cell and fits in a pocket.

---

## 2. Why This Is Novel

**No portable, affordable, open-hardware EIS instrument for battery cells
exists.** The closest commercial product is the Midtronics EXP-1080
(conductance tester, ~$2,500), which measures only a single low-frequency
conductance number — not a full spectrum. Hobby-grade cell testers (LiitoKora,
Opus BT-C3100, ZB2L3) measure capacity and DCIR but have no AC impedance
capability at all. Research-grade EIS hardware is benchtop, expensive, and
closed-source.

LithoCore is novel in several specific ways:

| Feature | LithoCore | Existing Solutions |
|---|---|---|
| Full EIS spectrum (0.01 Hz–100 kHz) | ✅ 48-point sweep | Only $8k+ benchtop instruments |
| On-device Nyquist/Bode + SoH classification | ✅ No PC needed | PC software required |
| Portable, coin-cell powered | ✅ 85 g, pocketable | ❌ Benchtop, wall power |
| Open hardware + firmware | ✅ CERN-OHL-S + GPL | ❌ Closed proprietary |
| BLE + USB data export | ✅ Live streaming | RS-232 / USB-only |
| Price target | ~$120 BOM | $2,500–$40,000 |
| Degradation mode classification | ✅ 5-mode on-device | Manual Nyquist interpretation |

The firmware's **embedded equivalent-circuit fitting engine** (a
Randles-model + Warburg element fitter running in fixed-point on the Cortex-M4)
is an original contribution — we are not aware of any open-source firmware that
performs on-device complex-nonlinear-least-squares (CNLS) impedance fitting on
an MCU.

---

## 3. How EIS Reveals Cell Health

Electrochemical Impedance Spectroscopy works by injecting a small AC current
perturbation into the cell and measuring the resulting AC voltage response. The
ratio V_ac / I_ac is the **complex impedance Z(f)**, which has a real part
(resistive) and an imaginary part (reactive). Plotting -Im(Z) vs. Re(Z) gives
the **Nyquist plot** — the fingerprint of the cell's internal electrochemistry.

A healthy lithium-ion cell's Nyquist plot shows three distinct regions:

```
  -Im(Z) (Ω)
      │
      │      ╭───►  High-freq semicircle
      │     ╱        (SEI layer + contact resistance)
      │    ╱
      │   ╱  ╭──►  Mid-freq semicircle
      │  ╱  ╱       (charge-transfer resistance + double-layer capacitance)
      │ ╱  ╱
      │╱  ╱
      ├───┼──────────────────────────────►  Re(Z) (Ω)
      0   Rs    Rct           Warburg tail
                     ╲
                      ╲────►  Low-freq 45° line (diffusion / Warburg)
```

- **High-frequency intercept (Rs):** electrolyte + contact + current collector
  resistance. Increases with electrolyte dry-out.
- **First semicircle (Rsei, Csei):** solid-electrolyte-interphase layer. Grows
  as the SEI thickens with age.
- **Second semicircle (Rct, Cdl):** charge-transfer resistance at the
  electrode/electrolyte interface. Increases dramatically with lithium
  plating (Li metal blocks active sites).
- **Warburg tail:** solid-state diffusion in the electrode particles. Shortens
  and steepens as the active material degrades.

LithoCore fits the measured spectrum to an extended Randles equivalent circuit
(Rs + Rsei‖Csei + Rct‖Cdl + Zw) using on-device CNLS and reports each
parameter. The pattern of parameter changes vs. a healthy baseline determines
the degradation mode:

| Degradation Mode | Key Signature |
|---|---|
| Healthy | Baseline Rs, Rsei, Rct, Warburg |
| SEI growth | Rsei ↑, Csei ↓, Rs ~constant |
| Lithium plating | Rct ↑↑, Cdl ↓, high-freq shift |
| Electrolyte dry-out | Rs ↑↑, all semicircles broaden |
| Internal short / self-discharge | Rs ↓ (abnormal), OCV drift detected |

---

## 4. Hardware Specifications

### Microcontroller

- **MCU:** STM32G474RET6 — 170 MHz Cortex-M4F with CORDIC and FMAC hardware
  accelerators (critical for fast fixed-point complex arithmetic during CNLS
  fitting). 512 KB Flash, 128 KB SRAM.
- **Why:** The CORDIC unit computes magnitude/phase of complex IQ pairs in 6
  clock cycles — 20× faster than software float. The FMAC unit accelerates the
  FIR filtering used in the digital lock-in detection. This combination makes
  on-device EIS fitting feasible at the power budget of a coin cell.

### Analog Front End

- **Excitation DAC:** AD9833 programmable DDS sine-wave generator (0.01 Hz–
  100 kHz, 0.6 mHz resolution, 28-bit phase accumulator) → programmable
  gain amplifier (PGA) → current-sense resistor → cell.
- **Signal conditioning:** OPA2188 zero-drift op-amps for voltage-sense
  (differential, ±5 V range with 100× gain on the AC component) and
  current-sense (TIA on the 0.1 Ω sense resistor).
- **ADC:** Simultaneous-sampling 24-bit dual-channel delta-sigma —
  **ADS1256** (24-bit, 30 kSPS, SPI) for the low-frequency range (0.01–1 kHz)
  and the MCU's built-in 12-bit ADC at 4 MSPS with hardware oversampling for
  the high-frequency range (1 kHz–100 kHz). The dual-ADC strategy gives
  >110 dB dynamic range at low frequencies and >70 dB at high frequencies.
- **Synchronous detection:** Digital lock-in implemented in firmware — the
  DDS and ADC sampling are phase-locked via a common 16.384 MHz TCXO, so the
  reference sine/cosine for demodulation is known exactly. This eliminates the
  need for a second ADC channel to capture the reference.

### Power & Safety

- **Power:** CR2477 coin cell (3 V, 1000 mAh) for the logic + a small
  supercapacitor (2.5 F, 5.5 V) that buffers the excitation current pulses so
  the coin cell never sees > 5 mA continuous drain. Total idle current:
  < 8 µA (MCU in STOP2, DDS powered down, analog rails off via load switch).
  Active sweep current: ~45 mA for 8–40 seconds per sweep → >500 sweeps per
  coin cell.
- **Cell under test isolation:** The cell being measured is **never charged or
  deeply discharged** by LithoCore. The AC perturbation is ±20 mA max,
  superimposed on the cell's own OCV via a DC-blocking capacitor network. The
  DCIR pulse is 2 A for 100 ms (0.056 mAh — negligible). A hardware
  over-voltage comparator (TLV3201) hard-disables the excitation if the cell
  terminal voltage exceeds 4.5 V or drops below 1.5 V.
- **Reverse-polarity protection:** MOSFET ideal-diode bridge (LM74700-Q1)
  — connecting the cell backwards simply produces a "REVERSE" error, no damage.

### Connectivity & UI

- **BLE 5.2:** nRF21540 RF FEM + STM32G474's integrated radio (via SPI-bridge
  to a nRF52840 co-processor for the BLE stack, or a simpler BL654 module).
  Streams Nyquist/Bode data points and SoH results. Range: ~50 m line-of-sight.
- **USB-C:** USB 2.0 full-speed for data export (CSV/JSON), firmware updates
  (DFU), and optional external power during long capacity tests.
- **UI:** Single mechanical button (start/abort) + 4 white LEDs (status:
  idle, sweeping, done, fault) + 1 bi-color LED (red = bad cell, green = good).
  No display — all visualization is in the app. This keeps power and cost
  minimal.

### Physical

- **Form factor:** Cylindrical, 142 mm × 22 mm dia. (marker-pen sized).
- **Weight:** 85 g including coin cell.
- **Probes:** Spring-loaded pogo pins at the tip (positive) and a sliding ring
  contact (negative), sized for 18650/21700/26650 cylindrical cells. A
  break-out alligator-clip adapter is included for prismatic/pack testing.
- **Environmental:** IP54 (splash-resistant), 0–45 °C operating.
- **PCB:** 4-layer, 1.6 mm, 120 mm × 18 mm, ENIG finish.

---

## 5. System Architecture & Block Diagram

```
 ┌──────────────────────────────────────────────────────────────────┐
 │                        STM32G474 (MCU)                          │
│  ┌──────────┐  ┌──────────┐  ┌────────────┐  ┌──────────────┐  │
│  │ EIS      │  │ Lock-in  │  │ CNLS Fit   │  │ SoH / Class  │  │
│  │ Sweep    │  │ Detect   │  │ (Randles)  │  │ Engine       │  │
│  │ Manager  │  │ (CORDIC) │  │ (FMAC)     │  │              │  │
│  └────┬─────┘  └────┬─────┘  └─────┬──────┘  └──────┬───────┘  │
│       │             │              │                │           │
│  ┌────▼─────────────▼──────────────▼────────────────▼─────┐     │
│  │           Peripheral Driver Layer                      │     │
│  │  SPI (DDS/ADC) · TIM (PWM/ pulses) · UART · USB · BLE  │     │
│  └────┬──────────────────────────────────────────────┬────┘     │
└───────┼──────────────────────────────────────────────┼──────────┘
        │ SPI                                          │ UART
        │                                              │
  ┌─────▼──────┐                              ┌────────▼────────┐
  │  AD9833    │  sine @ f                    │  BL654 BLE      │
  │  DDS       │──────────┐                   │  5.2 Module     │
  └────────────┘          │                   └─────────────────┘
        │                  │                          │
  16.384 MHz TCXO          │                          │ BLE 5.2
  (phase ref)              ▼                          │
                   ┌───────────────┐                  │
                   │  PGA + TIA    │                  │
                   │  (OPA2188)    │                  │
                   └───────┬───────┘                  │
                           │  ±20 mA AC               │
                    ┌──────▼──────┐                  │
                    │  Cell Under  │                  │
                    │  Test        │                  │
                    │  (18650 etc) │                  │
                    └──────┬──────┘                  │
                           │  V_ac / V_dc             │
                    ┌──────▼──────┐                  │
                    │  Voltage    │                  │
                    │  Sense AFE  │                  │
                    │  (diff. +   │                  │
                    │   100× AC)  │                  │
                    └──────┬──────┘                  │
                           │                         │
                    ┌──────▼──────┐                  │
                    │  ADS1256    │◄── SPI ──────────┘
                    │  24-bit ADC │
                    │  + MCU ADC  │
                    └─────────────┘

  Safety:  TLV3201 comparator ──► hardware EN gate on excitation
           LM74700 ideal-diode bridge ──► reverse polarity
           Load switch (TPL7407) ──► analog rail power gating
```

**Data flow during a sweep:**

1. Sweep manager configures the DDS to frequency f₁, waits for settling.
2. ADS1256 samples V and I simultaneously at a rate ≥ 10× f₁ (low-freq range)
   or MCU ADC at ≥ 50× f₁ (high-freq range), for ≥ 5 cycles of f₁.
3. Lock-in detector multiplies the sample stream by the reference sin/cos
   (generated from the DDS phase register, so they are perfectly coherent),
   low-pass filters via FMAC FIR, and outputs Re(Z) and Im(Z) at f₁.
4. Repeat for all 48 frequencies (log-spaced).
5. CNLS fitter runs 20 Levenberg-Marquardt iterations on the Randles model
   in fixed-point, extracting Rs, Rsei, Csei, Rct, Cdl, and Warburg σ.
6. SoH classifier compares fitted parameters to an age/chemistry baseline
   table and outputs a 0–100 % score + degradation mode.
7. Result packet sent over BLE/USB; LEDs updated.

---

## 6. Firmware Design

### Design Principles

1. **Deterministic timing.** The lock-in detection depends on exact coherence
   between the DDS output and the ADC sampling. A 16.384 MHz TCXO clocks both
   the DDS and the MCU's HSE input; the MCU PLL generates the ADC trigger from
   the same clock domain via a timer-slave configuration. No software jitter.
2. **Fixed-point throughout.** The STM32G474 has a single-precision FPU, but
   the CORDIC and FMAC units operate on fixed-point Q1.31 / Q1.15. The entire
   impedance pipeline uses Q1.31 fixed-point (31 fractional bits, ±1 range,
   scaled to physical units only at the final reporting stage) to exploit the
   CORDIC/FMAC and to keep the CNLS iterations deterministic in cycle count.
3. **Low-power by default.** Between sweeps the MCU is in STOP2, the analog
   rails are off, and only the RTC + GPIO wake (button press) are alive. The
   BLE co-processor handles its own sleep and wakes the STM32 via a UART CTS
   edge on an incoming connection.
4. **Safety state machine.** A hardware comparator can cut excitation
   independently of the MCU. The firmware additionally monitors cell voltage
   on every sample and aborts if |V| > 4.5 V or the AC amplitude exceeds limits.

### File Layout

```
firmware/
├── main.c              — top-level state machine, button handling, power mgmt
├── board.h             — pin map, clock config, peripheral assignments
├── registers.h         — STM32G474 register definitions (lightweight, no HAL)
├── Makefile            — arm-none-eabi-gcc build, OpenOCD flash target
├── linker.ld           — memory layout for STM32G474RET6
├── startup.s            — vector table + reset handler
└── drivers/
    ├── ads1256.c/h     — 24-bit ADC driver (SPI, calibration, drdy handling)
    ├── dds.c/h         — AD9833 DDS driver (freq/phase/amplitude control)
    ├── lockin.c/h      — Digital lock-in detection (CORDIC + FMAC FIR)
    ├── eis_sweep.c/h   — Multi-frequency sweep orchestrator
    ├── randles.c/h     — Extended Randles equivalent-circuit model eval
    ├── cnls.c/h        — Levenberg-Marquardt CNLS fitter (fixed-point)
    ├── soh.c/h         — State-of-health scoring + degradation classification
    ├── dcir.c/h        — DC internal resistance pulse measurement
    ├── coulomb.c/h     — Coulomb counter for capacity check
    ├── safety.c/h      — Over-voltage/reverse-polarity/thermal protection
    ├── power.c/h       — Analog rail gating, STOP2 entry/exit, supercap mgmt
    ├── ble.c/h         — UART protocol to BL654 co-processor
    ├── usb.c/h         — USB-CDC serial + DFU jump
    └── storage.c/h     — Flash ring buffer for last 256 cell results
```

### Key Firmware Algorithms

**Lock-in detection (lockin.c):** For each frequency point, the ADC samples
V(t) and I(t). The DDS phase register is read at the start of the acquisition
window, giving the exact reference phase φ₀. The complex impedance is:

  Z = (Σ V[n]·cos(φ₀ + 2π·f·n·Δt)) + j·(Σ V[n]·sin(...))  ÷
      (Σ I[n]·cos(...) + j·(Σ I[n]·sin(...)))

The cos/sin products use the CORDIC's vector-rotate mode; the summation is a
boxcar FIR executed by the FMAC. The low-pass cutoff is set to ~0.1×f to
reject harmonics.

**CNLS fitter (cnls.c):** Implements Levenberg-Marquardt minimization of
Σ |Z_meas(fᵢ) - Z_model(fᵢ, θ)|² where θ = [Rs, Rsei, Csei, Rct, Cdl, σ] is
the Randles parameter vector. The Jacobian is computed analytically (each
model impedance component has a closed-form derivative w.r.t. each parameter).
Twenty iterations converge in < 300 ms on the 170 MHz M4F. The LM damping
factor λ starts at 0.01 and adapts (×10 on step rejection, ×0.1 on acceptance).

**SoH engine (soh.c):** The fitted parameters are compared against a baseline
table indexed by cell chemistry (NMC, LFP, NCA) and nominal capacity. The SoH
score is a weighted geometric mean of the ratios R_baseline/R_measured for
each resistance component (higher resistance = lower SoH). The degradation
mode is a rule-based classifier:

- Rsei > 2× baseline → "SEI growth"
- Rct > 3× baseline AND Cdl < 0.5× baseline → "Lithium plating"
- Rs > 2× baseline AND Rct > 1.5× baseline → "Electrolyte dry-out"
- Rs < 0.5× baseline AND OCV drift > 5 mV/min → "Internal short"
- All within 1.3× baseline → "Healthy"

---

## 7. Impedance Measurement Pipeline

### Frequency Plan

The 48-point sweep is log-spaced across four decades:

| Band | Frequencies | Sample Rate | ADC | Cycles Captured | Duration |
|---|---|---|---|---|---|
| Ultra-low | 0.01–0.1 Hz (6 pts) | 1 Hz | ADS1256 | 5 | ~7 min |
| Low | 0.1–10 Hz (12 pts) | 100 Hz | ADS1256 | 10 | ~4 min |
| Mid | 10–1000 Hz (14 pts) | 10 kHz | ADS1256 | 20 | ~15 s |
| High | 1–100 kHz (16 pts) | 500 kHz | MCU ADC | 50 | <1 s |

A **full sweep** takes ~12 minutes (dominated by the ultra-low-frequency
points). A **fast sweep** (10 Hz–100 kHz, 30 points, 20 s) is available for
production-line screening. The user selects the mode in the app or by
button-hold duration (short press = fast, long press = full).

### Dynamic Range

The cell's AC voltage response to a 20 mA perturbation is typically
50 µV – 50 mV depending on frequency and cell impedance. The ADS1256 provides
24-bit resolution (±2.5 V range → 0.3 µV LSB), and the analog front-end
applies a 100× gain to the AC component (extracted by a high-pass filter at
0.005 Hz), giving an effective 3 nV LSB at the cell — far below the Johnson
noise floor. Averaging 5 cycles gives > 110 dB SNR at low frequencies.

At high frequencies (> 1 kHz), the ADS1256's maximum 30 kSPS rate is
insufficient, so the MCU's 12-bit ADC is used at 500 kSPS with hardware
oversampling (16×) to achieve an effective 16-bit resolution. The signal
amplitude is larger at high frequencies (lower impedance), so the reduced
resolution is acceptable.

---

## 8. Cell Health Classification Model

The on-device model combines the **equivalent-circuit parameters** from the
CNLS fit with the **DCIR** and **OCV relaxation** measurements into a
6-dimensional feature vector. A lightweight k-NN classifier (k=5, Euclidean
distance, 80-cell reference database stored in flash) assigns the degradation
mode. The reference database was built from cycled cells with known degradation
modes (confirmed by post-mortem analysis in published literature).

The SoH score is:

  SoH = 100 × [ (Rs₀/Rs) · (Rsei₀/Rsei) · (Rct₀/Rct) · (Cdl/Cdl₀) ]^(1/4)

clamped to [0, 100], where subscript ₀ denotes the baseline for the cell's
chemistry and nominal capacity. This geometric mean ensures that any single
degraded component pulls the score down proportionally.

### Supported Chemistries

| Chemistry | Nominal V | Baseline Rs (mΩ) | Baseline Rsei (mΩ) | Baseline Rct (mΩ) | Baseline Cdl (F) |
|---|---|---|---|---|---|
| NMC 18650 (3500 mAh) | 3.70 | 35 | 8 | 25 | 0.8 |
| NMC 21700 (5000 mAh) | 3.70 | 18 | 5 | 15 | 1.2 |
| LFP 26650 (3300 mAh) | 3.30 | 12 | 4 | 10 | 1.5 |
| NCA 18650 (3500 mAh) | 3.60 | 30 | 7 | 22 | 0.9 |
| LCO (laptop pack) | 3.70 | 60 | 12 | 30 | 0.6 |

The chemistry is auto-detected from the OCV (LFP: 3.30 V plateau, NMC/NCA:
3.70 V) or manually selected in the app.

---

## 9. Application / Software Interface

The companion app is a React Native application targeting iOS and Android. It
connects to LithoCore over BLE 5.2 (or USB-C via OTG) and provides:

- **Live Sweep Screen:** Real-time Nyquist plot (animated as each frequency
  point completes) and Bode magnitude/phase plots. Current frequency, Re(Z),
  Im(Z) displayed numerically. Progress bar.
- **Cell Report Screen:** After a sweep completes: SoH score (large gauge),
  degradation mode (color-coded badge), all six Randles parameters in a table,
  DCIR, OCV, and self-discharge rate. A quality verdict (EXCELLENT / GOOD /
  FAIR / REPLACE) with explanation.
- **History Screen:** List of all tested cells, sortable by date, SoH, or
  chemistry. Each row shows cell ID (user-assigned label), date, SoH, and
  mode. Tap to see the full report and Nyquist/Bode plots.
- **Pack Builder Screen:** For assembling multi-cell packs. Scan cells one by
  one, assign to slot positions in a configurable series/parallel topology
  (e.g., 10S4P). The app flags mismatched cells (SoH spread > 10 %, Rct spread
  > 30 %) and recommends grouping cells by matched impedance for pack
  longevity. Generates a pack assembly report (PDF export).
- **Settings Screen:** Sweep mode (fast/full/custom frequency range), chemistry
  override, BLE device name, data export format (CSV/JSON), calibration, and
  firmware update (OTA via BLE or USB).

### BLE Protocol

LithoCore exposes a custom GATT service (UUID `6e400001-b5a3-f393-e0a9-e50e24dcca9e`):

| Characteristic | UUID suffix | Direction | Purpose |
|---|---|---|---|
| Command | ...0002 | Phone→Device | Start sweep, abort, set config |
| Sweep Data | ...0003 | Device→Phone | Notification: per-frequency Z(f) point |
| Result | ...0004 | Device→Phone | Notification: final SoH + parameters |
| Status | ...0005 | Device→Phone | Notification: state changes, errors |

Sweep data points are packed as 20-byte notifications: `[freq(4) | Re_Z(4) |
Im_Z(4) | mag(4) | phase(4) | flags(1) | rsvd(1)]` all int32 Q-format. The
app unpacks and plots them in real time.

---

## 10. Use Cases & Target Audience

### Primary Use Cases

1. **Salvage cell sorting.** An e-bike builder buys 200 used 18650 cells from
   a decommissioned laptop-pack lot. They use LithoCore to run a fast sweep
   (20 s) on each, sort by SoH, and build a 10S4P pack from the 40 best-matched
   cells. Cells showing lithium plating are rejected — preventing a future
   thermal runaway.

2. **Pack health monitoring.** An off-grid solar installer performs annual EIS
   checks on each cell in a 16S8P battery bank. Trending the Nyquist plot over
   months reveals which cells are degrading fastest, enabling targeted
   replacement before a cell failure triggers a BMS disconnect.

3. **EV module qualification.** A used-EV dealer tests salvaged battery modules
   before resale. LithoCore's degradation-mode classification distinguishes a
   healthy module (minor SEI growth) from a plated module (high Rct) — the
   latter is worth 40 % less and is a safety risk.

4. **Research & education.** University battery labs use LithoCore as an
   affordable EIS instrument for undergraduate lab courses and for
   characterizing experimental cells. The open firmware allows custom sweep
   profiles and raw data export.

5. **Quality control (small-scale cell manufacturing).** A small Li-ion cell
   manufacturer integrates LithoCore into their end-of-line test station,
   sweeping every cell in 20 s and logging the Nyquist fingerprint for
   traceability.

### Target Audience

| Audience | Need | LithoCore Advantage |
|---|---|---|
| DIY battery builders (e-bikes, powerwalls) | Sort & match salvaged cells | 100× cheaper than benchtop EIS |
| Off-grid solar installers | Pack health trending | Portable, BLE, no laptop needed |
| Used EV / battery dealers | Module valuation | Degradation-mode classification |
| Battery researchers | Affordable EIS | Open hardware, raw data access |
| Electronics hobbyists | Understand battery health | Educational Nyquist plots |
| Small cell manufacturers | End-of-line QC | Fast sweep, CSV logging |

---

## 11. Power Budget

| State | Current | Duration | Energy |
|---|---|---|---|
| STOP2 idle (analog off, BLE off) | 8 µA | continuous | 24 µWh/h |
| BLE connected, idle | 1.2 mA | continuous | 3.6 mWh/h |
| Fast sweep (20 s, 10 Hz–100 kHz) | 45 mA | 20 s | 0.75 mWh |
| Full sweep (12 min, 0.01 Hz–100 kHz) | 45 mA | 12 min | 27 mWh |
| DCIR pulse | 2 A (from supercap) | 100 ms | 0 (supercap) |

**Coin cell life:** CR2477 = 1000 mAh = 3000 mWh.
- Idle-only: 3000 / 0.024 = **125,000 hours (~14 years)** — limited by shelf life.
- 10 sweeps/day (fast): 3000 / (0.024 + 10×0.75/24) = **~3,600 days (~10 years)**.
- 10 full sweeps/day: 3000 / (0.024 + 10×27/24) = **~267 days** — full sweeps
  are power-hungry; USB-C power is recommended for full-sweep workflows.

The supercapacitor (2.5 F, 5.5 V) is charged from the coin cell via a
low-current boost converter (TPS61099, 90 % efficiency) at 5 mA, reaching full
charge in ~90 seconds. It delivers the 2 A DCIR pulse without stressing the
coin cell. The AC excitation (20 mA) is also drawn from the supercap during
sweeps.

---

## 12. Mechanical & Form Factor

LithoCore is housed in an aluminum tube (22 mm OD, 0.5 mm wall) with
3D-printed end caps (PETG). The PCB slides into the tube and is held by
internal rails. The tip has two spring-loaded pogo pins (positive and negative)
that contact the cell terminals when pressed against them. A sliding ring
contact allows the device to clip onto cylindrical cells hands-free.

```
  ┌─────────────────────────────────────────────────────┐
  │  ○ Button   ○○○○ Status LEDs    ○ Bi-color LED      │
  │═════════════════════════════════════════════════════│
  │  │                                                 │
  │  │  PCB (120 × 18 mm, 4-layer)                     │
  │  │  ┌─────┐ ┌───────┐ ┌──────┐ ┌──────┐ ┌───────┐ │
  │  │  │STM32│ │DDS    │ │ADS   │ │OPA   │ │BL654  │ │
  │  │  │G474 │ │AD9833 │ │1256  │ │2188×4│ │ BLE   │ │
  │  │  └─────┘ └───────┘ └──────┘ └──────┘ └───────┘ │
  │  │  ┌────────┐ ┌──────┐ ┌──────────┐               │
  │  │  │TCXO    │ │Superc│ │CR2477    │               │
  │  │  │16.384M │ │2.5F  │ │coin cell │               │
  │  │  └────────┘ └──────┘ └──────────┘               │
  │  │                                                 │
  │═════════════════════════════════════════════════════│
  │  ●●  Pogo pins (touch cell terminals)               │
  └─────────────────────────────────────────────────────┘
   ← 142 mm →
```

---

## 13. Bill of Materials

| Ref | Part | Qty | Unit Cost | Notes |
|---|---|---|---|---|
| U1 | STM32G474RET6 | 1 | $5.20 | Cortex-M4F, CORDIC, FMAC |
| U2 | AD9833BRMZ | 1 | $7.85 | DDS sine generator |
| U3 | ADS1256IDBT | 1 | $9.40 | 24-bit 30kSPS ADC |
| U4 | OPA2188UA | 4 | $1.90 ea | Zero-drift op-amp (V/I sense) |
| U5 | BL654-SA | 1 | $8.50 | BLE 5.2 module |
| U6 | TLV3201 | 1 | $0.95 | Over-voltage comparator |
| U7 | LM74700-Q1 | 1 | $1.60 | Ideal-diode reverse protection |
| U8 | TPS61099 | 1 | $1.10 | Boost converter (supercap charge) |
| U9 | TPL7407LA | 1 | $0.60 | Load switch (analog rail gating) |
| Y1 | TCXO 16.384 MHz | 1 | $1.30 | ±2 ppm |
| C1 | Supercap 2.5F 5.5V | 1 | $1.80 | DCIR pulse buffer |
| BT1 | CR2477 holder + cell | 1 | $1.20 | Coin cell |
| SW1 | Tactile button | 1 | $0.20 | Start/abort |
| LED1-4 | White 0603 | 4 | $0.08 ea | Status |
| LED5 | Bi-color R/G 0603 | 1 | $0.15 | Good/bad indicator |
| P1 | USB-C 16-pin | 1 | $0.70 | Data + power |
| J1 | Pogo pins (spring) | 2 | $0.50 ea | Cell contact |
| PCB | 4-layer 120×18mm ENIG | 1 | $2.50 | |
| Misc | R, C, passives | — | $2.00 | |
| **Total** | | | **~$56** | Volume pricing, 1k qty |

---

## 14. Calibration & Safety

### Calibration

LithoCore performs a **2-point calibration** using the included calibration
resistor (a precision 1.000 Ω 0.1 % SMD on the probe tip, activated by a
switch). The user presses the button while shorting the probes to the cal
resistor; the firmware measures the known impedance at 1 kHz and computes
gain/phase correction factors that are stored in flash. This should be done
monthly or after firmware updates. The app also supports a **full multi-frequency
calibration** using an external impedance standard (1 Ω + 10 µF RC network).

### Safety

- **Hardware over-voltage cutoff:** TLV3201 comparator monitors cell voltage
  on every analog sample. If V > 4.5 V or V < 1.5 V, the excitation is
  hard-disabled via a MOSFET gate — no firmware involvement required.
- **Reverse polarity:** LM74700 ideal-diode bridge prevents damage from
  reversed cell connection; firmware detects negative voltage and reports
  "REVERSE" error.
- **Thermal:** An NTC on the probe tip measures cell temperature; the sweep
  aborts if T > 60 °C.
- **Current limit:** The excitation current is limited to 20 mA AC by the PGA
  and sense-resistor design; the DCIR pulse is limited to 2 A × 100 ms by a
  hardware one-shot timer. No software can override these limits.
- **Cell isolation:** The cell is never connected to LithoCore's power supply.
  All excitation is AC-coupled through DC-blocking capacitors; the cell's own
  voltage is measured but cannot drive LithoCore's circuitry.

---

## 15. Repository Layout

```
lithocore/
├── README.md              — This file
├── firmware/
│   ├── main.c             — State machine, button, power management
│   ├── board.h            — Pin map, clock config
│   ├── registers.h        — STM32G474 register definitions
│   ├── Makefile           — Build system
│   ├── linker.ld          — Memory layout
│   ├── startup.s           — Reset handler + vector table
│   └── drivers/
│       ├── ads1256.c/h    — 24-bit ADC driver
│       ├── dds.c/h        — AD9833 DDS control
│       ├── lockin.c/h     — Digital lock-in detection
│       ├── eis_sweep.c/h  — Frequency sweep orchestrator
│       ├── randles.c/h    — Randles equivalent-circuit model
│       ├── cnls.c/h       — Levenberg-Marquardt CNLS fitter
│       ├── soh.c/h        — SoH scoring + degradation classification
│       ├── dcir.c/h       — DC internal resistance measurement
│       ├── coulomb.c/h    — Coulomb counter (capacity check)
│       ├── safety.c/h     — Hardware safety monitoring
│       ├── power.c/h      — Power management (STOP2, rail gating)
│       ├── ble.c/h        — BLE co-processor protocol
│       ├── usb.c/h        — USB-CDC + DFU
│       └── storage.c/h    — Flash result ring buffer
├── kicad/
│   ├── device.kicad_sch   — Schematic (7 sheets: MCU, AFE, DDS, ADC, Power, BLE, Safety)
│   ├── device.kicad_pcb   — 4-layer PCB layout (120×18 mm)
│   └── device.kicad_pro   — KiCad project file
└── app/
    ├── App.tsx            — Root navigator
    ├── package.json       — Dependencies
    ├── app.json           — Expo config
    ├── tsconfig.json      — TypeScript config
    └── src/
        ├── ble/
        │   ├── BleManager.ts  — BLE connection manager
        │   └── protocol.ts    — GATT characteristic definitions + unpack
        ├── components/
        │   ├── NyquistPlot.tsx  — Interactive Nyquist chart
        │   ├── BodePlot.tsx     — Magnitude + phase Bode chart
        │   └── SoHGauge.tsx     — Circular SoH gauge
        ├── db/
        │   └── database.ts     — SQLite local history store
        └── screens/
            ├── LiveSweepScreen.tsx
            ├── CellReportScreen.tsx
            ├── HistoryScreen.tsx
            ├── PackBuilderScreen.tsx
            └── SettingsScreen.tsx
```

---

## 16. Licensing & Credits

- **Hardware (KiCad schematics, PCB, mechanical):** CERN-OHL-S v2
- **Firmware (C source):** GPL-3.0
- **Companion app (TypeScript/React Native):** MIT

All designs, firmware, and documentation by **jayis1**. Copyright © 2026
jayis1. All rights reserved.

This project uses the following open-source components: KiCad (GPL-3.0),
arm-none-eabi-gcc (GPL-3.0), STM32 CMSIS headers (Apache-2.0), React Native
(MIT), react-native-ble-plx (MIT). Their licenses are respected; no
proprietary SDKs or closed-source toolchains are required to build or modify
LithoCore.

*Built with care for the battery-reuse community — every salvaged cell
deserves a proper impedance fingerprint.*