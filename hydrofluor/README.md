# HydroFluor — Open-Source Multi-Wavelength UV-Vis Fluorescence Water Quality Sonde

**Author: jayis1**
**License: MIT**

> A portable, submersible, battery-powered water quality probe that characterizes natural and engineered waters in-situ using pulsed-LED excited UV-Vis fluorescence and absorbance — quantifying CDOM/DOM, chlorophyll-a, phycocyanin (cyanobacteria), crude-oil hydrocarbons, and turbidity from a single immersion, with BLE 5 and LoRa telemetry and a React Native field-survey companion app.

![Form Factor](https://img.shields.io/badge/Form%20Factor-Ø48mm%20×%20240mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32L4R9-orange) ![Connectivity](https://img.shields.io/badge/Link-BLE%205%20+%20LoRa-green) ![Power](https://img.shields.io/badge/Power-3.6V%20Li-SOCl₂-yellow) ![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 1. Purpose & Overview

Water quality monitoring today is split between two unsatisfying extremes. On one end sit $15,000+ laboratory spectrofluorometers that require sample collection, transport, and trained operators. On the other sit single-parameter colorimetric test strips and cheap "TDS meters" that tell you almost nothing about what is actually dissolved in your water. The middle ground — a *field-deployable, multi-analyte fluorescence sonde* — has historically been locked behind proprietary turnkey instruments (Turner Designs, YSI, Sea-Bird) that cost thousands of dollars, use sealed optical modules, and expose no raw data or calibration models.

**HydroFluor** closes that gap. It is a fully open-hardware, open-firmware, submersible sonde that performs **pulsed-LED excited multi-wavelength fluorescence** and **transmittance/absorbance** measurements across six excitation bands and four detection channels, then converts raw photodiode integrals into calibrated concentrations of:

| Analyte                         | Excitation | Emission   | Range              | Use                                              |
|---------------------------------|-----------|-----------|--------------------|--------------------------------------------------|
| CDOM / Dissolved Organic Matter | 365 nm UV  | 430 nm     | 0–500 ppb QSU      | Drinking-water DBP precursors, wastewater        |
| Chlorophyll-a                   | 470 nm     | 685 nm     | 0–500 μg/L         | Algae / phytoplankton biomass, eutrophication    |
| Phycocyanin (cyanobacteria)     | 590 nm     | 650 nm     | 0–200 μg/L         | Harmful algal bloom (HAB) early warning           |
| Crude oil / petroleum hydrocarb. | 255 nm DUV | 360 nm     | 0–50 ppb           | Spill detection, intake protection, ballast water |
| Turbidity (90° side scatter)    | 660 nm     | 660 nm     | 0–1000 NTU         | Sediment, treatment filter breakthrough          |
| UV-Vis absorbance proxy (254 nm)| 255 nm     | trans      | A254 / SUVA proxy  | Aromaticity index for DOM characterization       |

The device is intended to be **dipped directly** into a stream, lake, reservoir intake, aquaculture tank, or a sample beaker. An optional integrated **peristaltic micro-pump** and flow cell allow continuous-flow monitoring from a fixed deployment or a moving boat survey.

### Why fluorescence, and why these channels?

Molecules with conjugated π-systems or aromatic rings absorb UV/visible light and re-emit at longer wavelengths with characteristic Stokes shifts. This makes fluorescence **orders of magnitude more sensitive** than absorbance for trace organics — CDOM is detectable below 1 ppb, and the technique is *inherently species-selective* because each fluorophore family has a distinct excitation/emission fingerprint. By covering six carefully-chosen excitation bands, HydroFluor resolves analytes whose fluorescence overlaps only partially, and a built-in **partial-least-squares (PLS) unmixing model** runs on-device to separate co-fluorescing species (e.g. distinguishing true chlorophyll-a from CDOM interference at the 685 nm band).

### Target users

- **Water utility operators** — raw intake monitoring, DBP-precursor (CDOM) tracking, filter-breakthrough turbidity alarms.
- **Aquaculture & fisheries managers** — chlorophyll and phycocyanin trending for feed optimization and HAB avoidance.
- **Environmental agencies & watershed groups** — citizen-science-compatible field surveys of lakes and rivers.
- **Research labs & universities** — teaching spectrofluorometry, limnology field campaigns, instrument development.
- **Industrial / spill-response** — hydrocarbon-in-water monitoring at refineries, ports, and ballast discharge points.

---

## 2. Hardware Specifications

### 2.1 Core architecture

| Subsystem              | Component                              | Notes                                                                 |
|-----------------------|----------------------------------------|-----------------------------------------------------------------------|
| MCU                   | **STM32L4R9VI** (Cortex-M4, 120 MHz)   | Ultra-low-power, 640 KB SRAM, Chrom-Art DMA for math, USB FS          |
| Excitation LEDs (×6)  | 255, 280, 365, 470, 590, 625/660 nm    | 255/280 nm: deep-UV AlGaN; 365/470: InGaN; 590: AlInGaP; 660: AlInGaP |
| Detectors (×4)        | Si photodiodes + bandpass filters      | 360, 430, 650, 685 nm, plus a 660 nm side-scatter channel            |
| Preamp                | **ADA4625-1** chopper-stabilized OPAs  | Ultra-low input bias, fA noise, programmable gain via PGA113          |
| ADC                   | **ADS1256** 24-bit delta-sigma         | 30 kSPS, SPI, programmable gain 1–128×                               |
| Pump                  | Brushless peristaltic micro-pump       | 6 V, 3–30 mL/min, Tygon formulation AWP food-grade tubing            |
| Flow cell             | Custom PTFE blackened flow cell        | 10 mm pathlength, fused-silica windows, 500 μL volume               |
| Wireless              | **BM71** BLE 5 module + **RFM95W** LoRa | BLE for phone; LoRa for fixed mesh deployments up to 15 km           |
| Storage               | microSD (UHS-I) + 128 Mb QSPI flash    | Field log buffering + calibration store                               |
| Power                 | 3.6 V Li-SOCl₂ (Saft LS 26500) + supercap | 19 Ah, 10+ year shelf; supercap handles LED/pump current peaks      |
| Real-time clock        | PCF85263A with CR2030 backup          | Timestamped logging in deep-sleep deployments                         |
| Temperature            | DS18B20 1-Wire + NTC on optical bench | Temperature compensation of fluorescence quantum yield               |
| Pressure / depth       | MS5837-02BA I²C                        | 0–30 bar, 2 mm resolution for depth-profile surveys                 |
| IMU / attitude         | LSM6DSO32                             | Detect orientation for boat-survey tagging                          |
| USB                   | USB-C (data + charging of supercap)   | CDC-ACM serial console & firmware update                             |
| Form factor            | Ø48 mm × 240 mm acetal/delrin tube     | IP68 to 100 m; anodized Al endcaps; M16 cable gland                   |

### 2.2 Optical bench layout

The optical bench is a CNC-machined black-anodized aluminum insert carrying six excitation LEDs arranged on a 60° arc around a central fused-silica sample cuvette / flow cell, with four detector photodiodes placed at 90° (fluorescence) and 180° (transmittance) geometries:

- **90° side-scatter** geometry for fluorescence channels (suppresses direct LED light leakage).
- **180° transmission** geometry for absorbance (A254, turbidity-attenuation) using a reference photodiode opposite the 255 nm and 660 nm LEDs.
- Each detector sits behind an interference bandpass filter (FWHM 10 nm) matched to the target emission wavelength.
- A mechanical light baffle and blackened PTFE flow cell eliminate internal reflections.

### 2.3 Power budget

| Mode                 | Current draw (typ.) | Runtime (19 Ah cell) |
|----------------------|--------------------|----------------------|
| Active sampling      | ~85 mA             | ~9 days continuous   |
| Pumping + sampling   | ~210 mA            | ~3.5 days continuous |
| BLE connected        | +6 mA              | —                    |
| LoRa TX (10 min)     | avg +0.2 mA         | —                    |
| Deep sleep (logging) | ~120 μA            | ~6.5 years           |

A 1 F supercapacitor buffers the ~1.5 A peak current of pulsed deep-UV LEDs so the Li-SOCl₂ cell never sees high instantaneous drain (which causes passivation sag).

---

## 3. Architecture & Block Diagram

```
 ┌─────────────┐    ┌──────────────────────────────────────────────────────────┐
 │  STM32L4R9  │    │                      OPTICAL BENCH                        │
 │  Cortex-M4  │    │   ┌────┐ 255 ┌────┐ 280 ┌────┐ 365 ┌────┐ 470 ┌────┐ 590 │
 │   120 MHz   │    │   │LED │────►│LED │────►│LED │────►│LED │────►│LED │ ... │
 │             │ PWM├──►│drv │     │drv │     │drv │     │drv │     │drv │     │
 │  Tim1 → LED │    │   └────┘    └────┘     └────┘     └────┘     └────┘     │
 │  ADC sync   │    │      ▲ 90° fluorescence geometry                     ▲ 180° │
 │             │    │   ┌────┐  ┌────┐  ┌────┐  ┌────┐                  ┌────┐   │
 │  DMA → SRAM │◄───┤   │PD  │  │PD  │  │PD  │  │PD  │ (660nm side-     │PD  │   │
 │             │    │   │360 │  │430 │  │650 │  │685 │  scatter)        │ref │   │
 │             │    │   └─┬──┘  └─┬──┘  └─┬──┘  └─┬──┘                  └─┬──┘   │
 │             │    └─────┼───────┼───────┼───────┼────────────────────────┼─────┘
 │             │          ▼       ▼       ▼       ▼                        ▼
 │             │   ┌──────────────────────────────────────────────────┐
 │             │   │  4× ADA4625-1 → 4× PGA113 gain → 8:1 mux → ADS1256│
 │             │   │           24-bit ΔΣ ADC, SPI                       │
 │             │   └──────────────────────────────────────────────────┘
 │             │
 │  SDIO ──► microSD   QSPI ──► 128 Mb W25Q128 (calibration + PLS model)
 │  I²C ──► MS5837 (depth), DS18B20 (T), PCF85263A (RTC)
 │  SPI ──► RFM95W LoRa
 │  USART► BM71 BLE 5
 │  USB-C CDC-ACM console / DFU
 └─────────────┘
```

### Firmware architecture

```
        ┌──────────── HydroFluor RT super-loop + interrupt-driven acquisition ───────────┐
        │                                                                                 │
        │   main()                                                                        │
        │   ├── board_init()  clocks, GPIO, DMA, NVIC                                      │
        │   ├── drv_* init  (led, pd, adc, pump, sd, ble, lora, rtc, depth, temp)         │
        │   ├── calib_load()  read PLS coefficients & per-channel gains from QSPI         │
        │   └── survey loop:                                                              │
        │         for each cycle:                                                         │
        │           1. pump_prime() if flow-cell mode                                     │
        │           2. for each excitation wavelength λ:                                  │
        │                a. led_pulse(λ, pulse_us, current_mA)   ← TIM1 PWM, gate via DMA  │
        │                b. adc_sync_acquire()  (dark + light integrals, N averages)      │
        │                c. compute net = light − dark, normalize by reference PD         │
        │           3. fluorometry_unmix()  apply PLS model → analyte concentrations     │
        │           4. temp_comp() / depth_tag()                                          │
        │           5. log_sd()  +  lora_uplink() if due                                   │
        │           6. low_power_wait(cycle_period_ms)                                     │
        │                                                                                  │
        │   BLE GATT server exposes: Control, Calibrate, DataStream, Status               │
        └──────────────────────────────────────────────────────────────────────────────────┘
```

### PLS unmixing model

Because fluorescence spectra overlap, HydroFluor stores an on-device **partial-least-squares regression** model (PLS-1 per analyte, 6 latent variables, ~30 bytes per model after float16 quantization) calibrated against a reference suite. For a sample vector **x** of net fluorescence at the 4 emission channels × 6 excitations (24 features), the predicted concentration is:

```
  ĉ_analyte = μ_analyte + Σ_i β_i · t_i,   t_i = wᵢᵀ (x − μ_x)
```

with coefficients loaded from QSPI flash. The model is small enough to fit in RAM and run in <2 ms per sample. Recalibration is supported over BLE by exposing a `calibrate` characteristic that accepts (reference_concentration, raw_vector) pairs.

---

## 4. Firmware Details & Design Decisions

### 4.1 Pulsed excitation & synchronous detection

The dominant noise source in field fluorometry is **ambient daylight** leaking into the flow cell. HydroFluor defeats this by *pulsing* each excitation LED (typ. 50–200 μs, 50–500 mA) and measuring the detector signal **only during the pulse**, then subtracting an immediately-adjacent dark sample. This *lock-in-like* approach (at ~1 kHz pulse rate, well below mains interference and above 1/f noise corner) rejects DC ambient light by >40 dB without requiring optical choppers or physical shutters.

The STM32L4's **TIM1 advanced-control timer** generates the LED pulse via a PWM channel feeding a MOSFET constant-current LED driver (AL8805 / discrete). The ADS1256 ADC is hardware-triggered by the same timer's second channel with a programmable delay so it samples the steady-state portion of the pulse. A DMA double-buffer reads the SPI ADC stream into SRAM with no CPU intervention.

### 4.2 Reference photodiode normalization

LED output drifts with temperature and aging. A fraction of each excitation LED's output is picked off by a **reference photodiode** (opposite the 180° transmission detector) and the fluorescence signal is divided by the reference reading — making the measurement ratiometric and immune to LED intensity variation. This is the single most important design decision for long-term stability in the field.

### 4.3 Temperature compensation

Fluorescence quantum yield of CDOM and chlorophyll varies ~1–3 %/°C. The bench-mounted DS18B20 reads the optical block temperature on every cycle, and a per-analyte quadratic temperature correction coefficient (stored in calibration) is applied. Without this, a sonde warming in the sun would report spurious CDOM increases.

### 4.4 Deep-sleep logging mode

For unattended deployments, HydroFluor sleeps in STOP2 mode (120 μA), waking on the RTC alarm every N minutes to take a sample, log to SD, optionally LoRa-uplink, and return to sleep. The BLE module is held in reset except when a phone is connected; connection attempts are detected by a passive NFC wake field (the BM71 supports NFC pairing).

### 4.5 Calibration philosophy

HydroFluor ships with a **blank + single-point** factory calibration for each analyte (using NIST-traceable reference standards where available — e.g. quinine sulfate for CDOM QSU). The companion app walks the user through a two-point field calibration using included ampouled standards. Raw 24-channel fluorescence vectors are always logged alongside derived concentrations, so post-hoc recalibration against new references is always possible without redeployment.

### 4.6 File layout

```
firmware/
├── Makefile              GNU ARM toolchain + STM32CubeL4 HAL link
├── board.h               Pin map, peripheral assignments, board constants
├── registers.h           Register-level bit definitions for STM32L4R9
├── main.c                Survey super-loop, state machine, BLE/Lora wiring
├── stm32l4r9.ld          Linker script (in skypilot-style; included here)
└── drivers/
    ├── led_excitation.c/.h   TIM1 PWM + constant-current LED driver, pulse sequencing
    ├── photodiode.c/.h       ADA4625/PGA113/ADS1256 SPI acquisition, dark/light subtraction
    ├── flowcell.c/.h         Peristaltic pump control, priming, air-bubble detect
    ├── fluorometry.c/.h      PLS unmixing model, temp comp, reference normalization
    ├── storage.c/.h          microSD FATFS logging, QSPI calibration store
    ├── ble_telemetry.c/.h    BM71 BLE GATT server: control, calibrate, data, status
    ├── lora_link.c/.h        RFM95W LoRa uplink for mesh deployments
    ├── depth.c/.h            MS5837 pressure/depth, I²C
    └── temp.c/.h             DS18B20 1-Wire bench temperature
```

---

## 5. Application / Software Interface

### 5.1 BLE GATT interface

| Service / UUID (short)        | Characteristic  | Access | Purpose                                  |
|-------------------------------|-----------------|--------|------------------------------------------|
| `0xFF01` Control             | `0xFF02` cmd    | W      | start/stop survey, set cycle period, pump on/off |
| `0xFF01` Control             | `0xFF03` status | N      | mode, battery, depth, temp, error flags  |
| `0xFF10` DataStream          | `0xFF11` sample | N      | notifications of completed sample records |
| `0xFF20` Calibrate           | `0xFF21` ref    | W      | push (ref_conc, raw_vector) calibration pair |
| `0xFF20` Calibrate           | `0xFF22` result | N      | calibration fit result notification       |
| `0xFF30` DeviceInfo          | `0xFF31` info   | R      | fw version, serial, calibration date     |

### 5.2 Sample record binary format (24 bytes)

```
offset  field           type    units
 0      seq             u16     —
 2      timestamp       u32     unix s
 6      depth           u16     0.01 m
 8      temp            i16     0.01 °C
10      battery_mv      u16     mV
12      cdom_ppb        u16     ppb QSU
14      chla_ugl        u16     μg/L
16      phyc_ugl        u16     μg/L
18      oil_ppb         u16     ppb
20      turb_ntu        u16     0.01 NTU
22      flags            u16    bit0=overrange, bit1=bubble, bit2=calwarn ...
```

### 5.3 Companion app

A React Native app provides:

- **Connect** — scan + connect to a HydroFluor over BLE, show RSSI, battery, firmware.
- **Live Monitor** — real-time analyte concentrations, raw 6×4 fluorescence heatmap, depth, temp.
- **Calibration** — guided blank + standard calibration workflow, stores results to device.
- **Survey Map** — GPS-tagged sample points overlaid on a map with color-coded analyte values; export to CSV/GeoJSON.
- **Deployments** — configure logging interval, LoRa parameters, deep-sleep schedule; download SD log.

---

## 6. Use Cases

1. **Drinking-water intake protection.** A utility mounts HydroFluor at the raw-water intake with the flow cell + pump. It trends CDOM (a disinfection-byproduct precursor) every 10 min and alarms on turbidity spikes (filter breakthrough / storm runoff). LoRa uplink sends daily summaries to SCADA.

2. **Lake cyanobacteria early warning.** A lake association deploys HydroFluor off a dock. Chlorophyll-a and phycocyanin are logged hourly; the app pushes an alert when phycocyanin exceeds the bloom threshold so beaches can be closed before microcystin levels spike.

3. **Spill response.** An environmental responder lowers HydroFluor on a cable into a suspected petroleum-contaminated water body. The 255→360 nm hydrocarbon channel gives a ppb-level reading in seconds without lab turnaround.

4. **Citizen-science watershed survey.** A volunteer dips HydroFluor at 20 points along a stream. The app geotags each reading and produces a colored watershed map of CDOM/turbidity to identify runoff hotspots.

5. **Aquaculture.** A shrimp farm runs HydroFluor in continuous-flow mode in the recirculation loop; phycocyanin trends trigger preemptive water exchange before a toxic bloom crashes the stock.

6. **University teaching.** A chemistry lab uses HydroFluor as a $200 stand-in for a $20,000 bench fluorometer — students write their own calibration models and explore the Stokes shift.

---

## 7. Design Decisions & Trade-offs

- **Pulsed-LED over xenon lamp.** A xenon flashlamp gives broadband excitation but adds complexity (high voltage, EMI, drift). Modern high-power UV LEDs (255/280 nm AlGaN, 365 nm InGaN) are stable, instant-on, and spectrally narrow enough to select analytes without a monochromator. The trade-off is that each LED samples a single excitation wavelength rather than a full spectrum — mitigated by choosing the six most diagnostic bands.

- **Discrete photodiodes over a spectrometer module.** A mini-spectrometer (e.g. Hamamatsu C12880MA) would give full emission spectra but costs ~$300, is temperature-sensitive, and has lower sensitivity per channel than a dedicated photodiode+filter. HydroFluor's four discrete channels cover the key emission peaks at far higher SNR and ~1/10 the cost.

- **ADS1256 over the MCU's internal ADC.** The STM32L4's 12-bit ADC lacks the dynamic range for ppb-level fluorescence against a bright LED background. The 24-bit ADS1256 provides 21 effective bits at the 30 SPS rate used for integration, with programmable gain to autorange across clear and turbid waters.

- **LoRa + BLE, not just BLE.** BLE alone limits range to a phone in hand. Many deployments (intake, dock) need telemetry to a gateway 1–10 km away; LoRa provides this with negligible power overhead at low duty cycle.

- **Open calibration.** The single biggest practical advantage over commercial sondes: every raw measurement is logged and the calibration model is editable. A lab that develops a better PLS model for their specific water can push it to the device over BLE and immediately benefit.

---

## 8. Target Audience & Cost

The bill of materials targets **~$180 in single quantities** (MCU $9, ADC $14, OPAs $8, LEDs $22, photodiodes $6, filters $18, pump $12, BLE $6, LoRa $9, depth sensor $12, SD/flash $4, battery $14, enclosure $20, PCB $15, passives $20). This is roughly **1/25th** the cost of a commercial 4-parameter fluorescence sonde, putting research-grade in-situ fluorometry within reach of citizen-science groups and teaching labs.

---

## 9. Bill of Materials (summary)

| Ref | Part                  | Qty | Approx $ |
|-----|----------------------|-----|----------|
| U1  | STM32L4R9VIT6         | 1   | 9.00     |
| U2  | ADS1256 (24-bit ADC)  | 1   | 14.00    |
| U3  | RFM95W LoRa           | 1   | 9.00     |
| U4  | BM71 BLE 5 module     | 1   | 6.00     |
| U5  | W25Q128 QSPI flash    | 1   | 2.50     |
| U6  | PCF85263A RTC         | 1   | 2.00     |
| U7  | MS5837-02BA depth     | 1   | 12.00    |
| U8  | LSM6DSO32 IMU         | 1   | 4.00     |
| U9  | DS18B20 temp          | 1   | 3.00     |
| U10-U13 | ADA4625-1 OPA      | 4   | 8.00     |
| U14 | PGA113 PGA (×4)       | 4   | 8.00     |
| LED1-LED6 | UV/Vis LEDs    | 6   | 22.00    |
| PD1-PD4 | Si photodiodes    | 4   | 6.00     |
| F1-F4 | Bandpass filters     | 4   | 18.00    |
| M1  | Peristaltic pump      | 1   | 12.00    |
| BT1 | Saft LS 26500         | 1   | 14.00    |
| Csup| 1 F supercapacitor    | 1   | 6.00     |
| —   | PCB, enclosure, misc. | —   | ~50      |

---

## 10. Repository Layout

```
hydrofluor/
├── README.md                   this file
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── stm32l4r9.ld             linker script
│   ├── main.c
│   └── drivers/
│       ├── led_excitation.c / .h
│       ├── photodiode.c / .h
│       ├── flowcell.c / .h
│       ├── fluorometry.c / .h
│       ├── storage.c / .h
│       ├── ble_telemetry.c / .h
│       ├── lora_link.c / .h
│       ├── depth.c / .h
│       └── temp.c / .h
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── package.json
    ├── App.js
    ├── screens/  (Connect, LiveMonitor, Calibration, SurveyMap, Deployments)
    ├── components/  (FluorescenceHeatmap, AnalyteGauge, DepthIndicator, CalibStep)
    └── utils/  (ble.js, protocol.js, theme.js, calibration.js)
```

---

## 11. License & Author

All hardware schematics, PCB layouts, firmware, and application code in this directory are © **jayis1**, released under the MIT License. See individual file headers. No part of this design is claimed to be a certified analytical instrument; users deploying for regulatory monitoring must validate against accepted reference methods per their jurisdiction.

*HydroFluor — fluorescence you can dip, afford, and actually own.*