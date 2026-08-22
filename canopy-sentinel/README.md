# Canopy Sentinel — Handheld Crop-Canopy Dew, Spore, and Thermal Disease-Risk Mapper

![Canopy Sentinel](https://img.shields.io/badge/Form%20Factor-Handheld%20field%20scanner-blue) ![MCU](https://img.shields.io/badge/MCU-STM32U585-green) ![Thermal](https://img.shields.io/badge/Thermal-MLX90640%2032x24-orange) ![Sensors](https://img.shields.io/badge/Sensors-Leaf%20wetness%20%2B%20spore%20fluorescence%20%2B%20CO2-purple) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.3%20%2B%20Wi--Fi-teal) ![Author](https://img.shields.io/badge/Author-jayis1-red)

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## 1. Purpose and Overview

**Canopy Sentinel** is a new class of portable field instrument designed for growers, agronomists, vineyard managers, greenhouse operators, plant pathologists, and crop consultants who need to make better disease-management decisions at the row level instead of relying solely on weather stations or broad spray schedules. It is a **handheld crop-canopy disease-risk mapper** that combines **leaf wetness sensing, microclimate measurement, thermal imaging, airflow estimation, and optical spore-event detection** into a single device that can be walked through an orchard, vineyard, greenhouse, or high-value specialty crop field.

The core problem Canopy Sentinel solves is that many destructive foliar diseases do not begin as visible lesions; they begin as a set of conditions. A leaf surface cools below air temperature and crosses the dew point. Moisture remains trapped inside a dense canopy after sunrise. A localized humidity pocket forms around low-airflow fruiting zones. Opportunistic fungal spores arrive and remain viable long enough to germinate. In practice, the grower often does not know *where* in the field those conditions actually occur. A weather station at the edge of a block cannot measure the disease microclimate deep inside a canopy wall. A greenhouse climate computer may know the average humidity, but not whether the lower tomato leaves on the north side are condensing every night. By the time infection is obvious, the management window is already shrinking.

Canopy Sentinel addresses that gap by letting a user scan crop rows and build a **spatial disease-risk map** based on the variables that matter most:

- **Leaf-to-air dew point margin**
- **Leaf wetness persistence and re-wetting behavior**
- **Thermal nonuniformity inside the canopy**
- **Relative stagnation / weak airflow zones**
- **Spore-like fluorescence pulse counts in the local airstream**
- **Crop-specific disease thresholds and rule sets**

The result is not a generic weather log. It is an actionable field instrument that tells a grower which rows, heights, and exposures are likely to become **powdery mildew**, **downy mildew**, **botrytis**, **late blight**, or **leaf mold** hot spots before symptoms are visible.

### Why this device should exist

Agricultural disease management is still dominated by three imperfect strategies:

1. **Calendar spraying** — simple, but often wasteful and expensive.
2. **Single weather-station forecasting** — useful, but too coarse for heterogeneous canopy environments.
3. **Visual scouting** — essential, but reactive rather than predictive.

High-value crops such as grapes, strawberries, tomatoes, cucumbers, hops, peppers, cannabis, berries, and ornamentals can lose enormous value from delayed intervention or over-application of fungicides. What growers need is a device that captures the *microclimate where the pathogen actually lives*: on and around the leaf surface.

Canopy Sentinel is novel because it is not merely a leaf wetness logger, thermal camera, or spore trap. It is a **portable, fused-sensor canopy profiler** designed around the real agronomic workflow of walking a row, pausing at representative zones, scanning, and immediately deciding whether to adjust irrigation, ventilation, canopy thinning, or spray timing.

### What makes it original

No device in this repository and very few devices on the market combine all of the following into one open, field-ready platform:

1. **Direct-contact capacitive leaf wetness clip plus non-contact thermal leaf scan** in one instrument.
2. **On-device dew point, condensation margin, and persistence scoring** instead of just raw humidity logging.
3. **Optical spore-event detection** using a UV excitation chamber and fluorescence pulse discriminator to estimate local airborne biological loading.
4. **Canopy airflow/stagnation inference** using a compact differential pressure fan tunnel and inertial motion compensation.
5. **Crop-specific disease models** that convert measurements into a single row-level risk index.
6. **BLE/Wi-Fi connected mobile companion** for session logging, risk maps, and intervention reports.
7. **Open hardware, open firmware, and open app**, allowing researchers and growers to adapt the device to new crops and pathogens.

This makes Canopy Sentinel especially valuable where disease pressure is expensive, spatially variable, and highly dependent on overnight condensation behavior: vineyards, berry tunnels, greenhouse tomatoes, leafy greens, cannabis flower rooms, seed production plots, and research stations.

---

## 2. Device Concept

Canopy Sentinel is shaped like a rugged field wand with three distinct sensing zones:

- A **front thermal and optical head** that images leaf clusters and samples the nearby air column.
- A **side leaf-wetness clip** that can briefly touch representative leaves to measure film conductivity/capacitance and validate condensation persistence.
- A **handle module** containing battery, MCU, radio, display, storage, and power management.

The operator walks a crop row and periodically performs a 3–10 second scan. During that scan the device captures:

- air temperature and relative humidity,
- CO2 concentration and estimated canopy respiration trend,
- leaf surface temperature map,
- local wetness state from the clip,
- fluorescence pulse events from the spore chamber,
- airflow/stagnation score,
- GPS or phone-provided position metadata,
- row/plant identifiers from the companion app.

The firmware then computes:

- dew point,
- leaf-air temperature delta,
- condensation likelihood,
- thermal variance score,
- wetness persistence score,
- spore event rate,
- disease-specific weighted risk index.

The user sees a color-coded **low / moderate / elevated / critical** recommendation on the device instantly, while the mobile app records the full dataset for later scouting, traceability, and treatment planning.

---

## 3. Hardware Specifications

### 3.1 Core Processing

| Parameter | Value |
|---|---|
| MCU | STM32U585AII6 |
| CPU | ARM Cortex-M33 @ 160 MHz |
| RAM | 786 KB SRAM |
| Flash | 2 MB internal |
| External Storage | 16 MB QSPI NOR + microSD |
| Security | TrustZone-capable, secure boot ready |
| Why chosen | Ultra-low power, strong ADC/DMA/peripheral set, enough DSP for thermal processing and classification |

The STM32U585 was chosen because this product spends much of its time in battery-powered field use, but still needs enough compute headroom to run sensor fusion, thermal interpolation, rolling statistics, and BLE streaming without an external application processor.

### 3.2 Sensors

#### Thermal imaging

| Parameter | Value |
|---|---|
| Sensor | MLX90640 32×24 IR array |
| Interface | I2C @ 1 MHz |
| FOV | 55° × 35° |
| Purpose | Leaf surface temperature, hotspot/coldspot detection, dew margin inference |

The thermal array does not attempt pathogen identification directly. Instead, it reveals the **leaf energy balance**. Leaves that remain cooler than surrounding air during humid low-airflow periods are much more likely to condense moisture, especially in dense canopies and under plastic cover.

#### Climate sensing

| Parameter | Value |
|---|---|
| Sensor | Sensirion SHT45 |
| Outputs | Temperature, RH |
| Accuracy | ±0.1 °C, ±1.0 %RH typical |
| Purpose | Dew point and vapor pressure deficit calculations |

#### CO2 / air exchange proxy

| Parameter | Value |
|---|---|
| Sensor | Sensirion SCD41 |
| Outputs | CO2, temperature, RH cross-check |
| Purpose | Canopy respiration context and ventilation indication |

CO2 is not a direct disease variable, but in enclosed or dense conditions it helps distinguish poorly ventilated microzones and validate greenhouse circulation issues.

#### Leaf wetness clip

| Parameter | Value |
|---|---|
| Method | Interdigitated gold-plated capacitive/conductive leaf clip |
| Front end | Low-current excitation + synchronous demodulation |
| Purpose | Measures free water film, partial wetting, and drying curve |

This clip is a critical differentiator. Many disease models infer wetness from RH alone, but condensation and persistent free water are not always visible in air measurements. The clip provides a direct ground-truth signal.

#### Spore-event detector

| Parameter | Value |
|---|---|
| Excitation | 365 nm UV LED pulse source |
| Detector | Low-noise photodiode + transimpedance amplifier |
| Chamber | Replaceable inertial particle channel with fan-assisted draw |
| Purpose | Counts fluorescence pulse events consistent with biological aerosols |

This subsystem is intentionally conservative: it does **not** claim species-level identification. Instead, it estimates the local rate of particle events with fluorescence characteristics often associated with pollen, spores, and organic particulate matter. Used with other measurements, it adds valuable confidence to disease-risk assessment.

#### Airflow / stagnation sensing

| Parameter | Value |
|---|---|
| Sensor | Differential pressure MEMS channel |
| Support | Controlled micro-fan reference pulse |
| Purpose | Distinguishes stagnant canopy pockets from better ventilated zones |

### 3.3 Connectivity and I/O

| Parameter | Value |
|---|---|
| Wireless | BLE 5.3, 2.4 GHz Wi-Fi |
| Wired | USB-C for charging, data export, and firmware update |
| Display | 2.4" transflective TFT |
| Buttons | Power, Scan, Marker, Up/Down |
| Indicators | RGB status LED, haptic motor, buzzer |

### 3.4 Power

| Parameter | Value |
|---|---|
| Battery | 3200 mAh single-cell Li-ion |
| Runtime | ~10 hours mixed scanning |
| Charging | USB-C PD sink up to 9 V |
| Protections | Fuel gauge, OVP, UVLO, thermal cutoff |

### 3.5 Mechanical Form Factor

| Parameter | Value |
|---|---|
| Enclosure | IP54 sealed field wand |
| Dimensions | 238 mm × 62 mm × 41 mm |
| Weight | ~410 g |
| Materials | Glass-filled nylon shell, silicone grip, stainless leaf clip spring |

The wand format is deliberate. It lets the user reach into a canopy, image clusters at a repeatable distance, clip a leaf when needed, and use the device one-handed while carrying pruning tools, a phone, or a notebook.

---

## 4. System Architecture

### 4.1 Functional block diagram

```text
                           +---------------------------+
                           |       USB-C / PD         |
                           +------------+-------------+
                                        |
                             +----------v----------+
                             |   Power / Charger   |
                             |  Fuel Gauge / PMIC  |
                             +----------+----------+
                                        |
                        +---------------v----------------+
                        |       STM32U585 MCU            |
                        |  sensor fusion + UI + BLE/WiFi |
                        +---+--------+--------+-----+----+
                            |        |        |     |
                 +----------+--+   +-+--+   +-+--+  +----------------+
                 | Thermal Array | |SHT45| |SCD41|  | microSD / QSPI |
                 |  MLX90640     | +----+ +----+    +----------------+
                 +---------------+
                            |
                 +----------v-----------+
                 | Leaf Wetness AFE     |
                 | Interdigitated Clip  |
                 +----------+-----------+
                            |
                 +----------v-----------+
                 | Spore Fluorescence   |
                 | UV LED + PD + TIA    |
                 +----------+-----------+
                            |
                 +----------v-----------+
                 | ΔP Airflow Channel   |
                 | fan + MEMS sensor    |
                 +----------+-----------+
                            |
                 +----------v-----------+
                 | TFT / Haptics / LED  |
                 +----------------------+
```

### 4.2 Data path

1. Sensors are sampled on synchronized scan intervals.
2. Thermal frames are denoised, clipped, and reduced to summary statistics.
3. Dew point is computed from SHT45 climate data.
4. Leaf wetness and airflow values are normalized against calibration curves.
5. Fluorescence pulses are thresholded and binned by amplitude and persistence.
6. The firmware fuses all variables into crop-model risk scores.
7. Results are displayed locally and packetized for the mobile app.
8. Full sessions are logged to local storage for traceability and research export.

---

## 5. Firmware Design and Design Decisions

The firmware in `firmware/` is written in portable C and organized to support both a host-side simulation build and a straightforward embedded port. The reason for the simulation-first architecture is practical: disease scoring, packet formats, rolling statistics, calibration behavior, and UI state management can all be validated before hardware bring-up.

### 5.1 Major firmware modules

- `main.c` — scheduler, sensor-fusion loop, disease models, CLI/simulation harness.
- `board.h` — system constants, data structures, metadata, platform abstraction.
- `registers.h` — packet IDs, status flags, calibration register map, event enums.
- `drivers/power.*` — battery model, charger state, brownout behavior.
- `drivers/ble.*` — mobile app protocol framing, notifications, scan session packets.
- `drivers/display.*` — local UI rendering into a text framebuffer for host simulation.
- `drivers/climate.*` — temperature/RH/CO2 sampling and dew-point helpers.
- `drivers/thermal.*` — thermal frame synthesis, filtering, statistics extraction.
- `drivers/leaf.*` — wetness clip excitation model and normalization.
- `drivers/spore.*` — fluorescence event processing, pulse discrimination, exposure control.
- `drivers/storage.*` — session logging and CSV-style export.

### 5.2 Key design choices

#### Host-compilable first

Instead of producing non-buildable pseudo-firmware, the project compiles as a normal C executable so its logic can be verified in CI or on a workstation. The board abstraction is intentionally small and all hardware interactions are centralized, making later porting to STM32 HAL, LL, or Zephyr straightforward.

#### Disease scoring as a transparent rules engine

Most agricultural advisories are opaque. Canopy Sentinel exposes a transparent weighted model using understandable terms: dew margin, wetness persistence, stagnation, spore activity, and canopy thermal variance. Researchers can tune coefficients per crop and pathogen.

#### Conservative interpretation of spore signals

The fluorescence detector is deliberately framed as a **spore-event proxy**, not a diagnostic microbiology instrument. That reduces false confidence while still extracting operational value from biological aerosol trends.

#### Fast field workflow

The on-device UX emphasizes speed:

- wake instantly,
- scan in under 5 seconds,
- classify risk immediately,
- tag the location in the app,
- move to the next plant or row.

This matters more in the field than laboratory-style precision if the goal is actionable scouting.

---

## 6. Companion Application Interface

The `app/` folder contains a React Native companion app that focuses on five high-value tasks:

1. **Live Scan View** — shows current risk score, component metrics, and thermal panel.
2. **Session Timeline** — records scans with row IDs, notes, and treatment markers.
3. **Risk Map** — aggregates scans by row/zone to reveal hotspots.
4. **Report Builder** — exports evidence-based scouting summaries.
5. **Device Console** — calibration, firmware version, battery status, and crop profile selection.

### App feature summary

- BLE packet decode and validation
- offline session storage
- trend charts for dew margin and wetness persistence
- treatment recommendation templates
- crop presets for grapes, tomatoes, strawberries, cucumbers, hops, and custom mode
- PDF/CSV export pathway (designed for future implementation)

The app is intentionally not a generic dashboard. It is designed around the agronomic question: **Where is disease pressure likely to start, and what should I do first?**

---

## 7. Use Cases and Target Audience

### 7.1 Vineyards

A vineyard manager can walk multiple rows at dawn after a humid night and identify shaded low-airflow clusters where leaf temperature remained closest to dew point and where wetness persistence remained longest. Instead of spraying every block equally, the manager can prioritize the sections at highest risk for downy mildew or botrytis.

### 7.2 Greenhouse tomatoes and cucumbers

Greenhouses often average climate well while hiding microclimate extremes. Canopy Sentinel can reveal where sidewall condensation, fan dead zones, and dense lower-canopy moisture accumulation are creating conditions for leaf mold, botrytis, or powdery mildew.

### 7.3 High tunnels and berries

Tunnel-grown strawberries and cane berries face intense disease pressure from local humidity spikes. The device helps validate vent strategy, irrigation timing, and pruning intensity.

### 7.4 Controlled-environment agriculture research

Plant pathology labs can use the open platform to correlate leaf wetness, thermal behavior, and spore-event trends against actual infection outcomes, refining disease models over time.

### 7.5 Specialty crop consultants

Independent consultants need evidence they can show clients. The app’s session records and risk map make recommendations defensible and repeatable.

---

## 8. Example Workflow

1. Select crop profile in the app: grape / tomato / strawberry / custom.
2. Walk to a representative row section just before sunrise.
3. Point the scanner at the fruiting zone and hold for 3 seconds.
4. Clip a nearby leaf if the model requests wetness confirmation.
5. The device computes a risk classification and streams the packet to the app.
6. Add a note such as “north side, row 12, dense canopy, poor airflow.”
7. Repeat across the block.
8. Open the risk map to identify clusters of elevated or critical pressure.
9. Decide whether to thin canopy, adjust irrigation timing, increase ventilation, or target a fungicide intervention.

---

## 9. Why the Design Is Practical

This device is not speculative science fiction. Every sensing principle used here already exists in adjacent domains:

- thermal arrays are common in industrial inspection,
- humidity/dew-point calculations are well understood,
- capacitive wetness measurement is mature,
- fluorescence particle detection is used in environmental instrumentation,
- BLE scouting tools are standard on modern mobile devices.

The novelty lies in **combining them into a handheld agronomic instrument built specifically around disease-risk scouting**. The BOM is realistic, the algorithms are tractable on a Cortex-M33, and the workflow matches how real crop advisors already move through fields.

---

## 10. Calibration, Validation, and Field Methodology

A disease-risk instrument is only useful if it produces repeatable signals under messy field conditions. Canopy Sentinel therefore treats calibration as a first-class design concern rather than an afterthought.

### 10.1 Thermal calibration

The MLX90640 array is not used like a consumer novelty thermal camera. In this application, small differences matter. The firmware performs:

- bad-pixel replacement,
- emissivity-compensated leaf estimation,
- rolling frame averaging,
- edge rejection to de-emphasize background sky and trellis posts,
- canopy-only region-of-interest reduction.

A black reference tab inside the nose of the scanner can be periodically observed during a self-test routine to track array drift. In future hardware revisions, a miniature shutter could improve repeatability even further.

### 10.2 Leaf wetness clip calibration

The clip is calibrated against three states:

1. **dry leaf / dry standard film**, 
2. **humid but no free water**, and 
3. **visible continuous water film**.

Because leaf chemistry differs by crop and cuticle type, the calibration workflow stores crop-specific gain values. A grape leaf and a cucumber leaf do not present the same capacitance profile, and the firmware allows those baselines to be separated.

### 10.3 Spore-event detector validation

The optical channel is validated against controlled aerosol challenges and reference microscopy counts. The purpose is not one-to-one organism identification; the purpose is operational trend detection. A grower or researcher should be able to say, with confidence, that one canopy zone produced materially more fluorescence-positive particulate events than another under similar handling.

### 10.4 Airflow characterization

The stagnation metric is built from a repeatable micro-fan pulse and differential pressure response rather than passive estimation alone. This improves robustness when the user’s walking speed or hand motion changes. The IMU is optional in the current architecture, but could be added in a revision to subtract probe motion more explicitly.

### 10.5 Agronomic validation path

A practical validation program for Canopy Sentinel would include:

- repeated dawn scans across multiple blocks,
- fixed weather station comparison,
- leaf wetness logger comparison,
- disease scouting records over subsequent days,
- lesion onset mapping,
- treatment timing correlation,
- false-positive and false-negative analysis.

This is precisely why an open platform is valuable. Researchers can inspect and improve the model instead of trusting an opaque box.

## 11. Detailed Firmware Behavior

The firmware architecture is intentionally deterministic. Rather than an ad hoc loop with sensor reads sprinkled across it, the design follows a simple state machine:

- **BOOT** — initialize storage, load calibration, advertise over BLE.
- **IDLE** — show battery, selected crop, and ready state.
- **ARM_SCAN** — power thermal head, prime UV LED timing, and start fan pulse.
- **ACQUIRE** — sample climate, thermal frame, leaf clip, optical pulses, and airflow response.
- **FUSE** — calculate dew point, dew margin, wetness persistence, thermal variance, and risk score.
- **PRESENT** — display the result, vibrate on critical risk, notify the app.
- **STORE** — append the session record to local memory.
- **EXPORT** — stream CSV or binary packets when requested.

This structure matters for field reliability. A device used at dawn in damp conditions must be predictable, easy to recover, and quick to resume after accidental button presses or battery swaps.

### 11.1 Why a rules engine instead of a black-box ML model?

A black-box model might eventually improve classification accuracy, but it would also make trust and calibration harder. The current design uses explainable weighted factors because growers and researchers need interpretable outputs. If the device says risk is critical, it should also reveal *why*: negative dew margin, long wetness persistence, high stagnation, or elevated bioaerosol events.

### 11.2 Logging philosophy

Every scan stores both the fused score and its component signals. That allows post-season analysis. A grower can discover that in one crop the thermal variance term added little value, while in another protected environment it strongly correlated with outbreaks. The device becomes better over time because the underlying evidence is retained.

### 11.3 Embedded portability

The included C firmware compiles on a host workstation for verification, but the code organization mirrors what an embedded deployment needs:

- isolated drivers,
- simple board abstraction,
- no dependence on a desktop OS,
- fixed-size buffers,
- explicit packet structures,
- straightforward conversion to DMA-backed sensor reads.

That makes it easier to port the logic to STM32 HAL/LL or Zephyr without rewriting the entire application layer.

## 12. Application and Data Model Details

The mobile app is designed for actual scouting sessions, not just a demo dashboard.

### 12.1 Session objects

A session record contains:

- timestamp,
- block / row / plant identifier,
- crop profile,
- geolocation if available,
- risk score and level,
- dew margin,
- wetness persistence,
- spore index,
- stagnation score,
- operator notes,
- optional intervention recommendation.

### 12.2 Grower-facing outputs

The app can surface recommendations such as:

- “Increase sunrise ventilation in bay 3.”
- “Delay overhead irrigation until leaf surface dries below threshold.”
- “Prioritize fungicide coverage in rows GV-01 to GV-04.”
- “Consider canopy thinning in lower north-facing fruit zone.”

These are not medical-style directives; they are structured scouting notes designed to fit into real crop management decisions.

### 12.3 Multi-user workflow

In commercial settings, one scout may gather data while a manager reviews it later. The app structure supports eventual account sync, supervisor review, and season-over-season comparison. Even without that backend today, the data model is prepared for it.

## 13. Safety, Maintenance, and Serviceability

### 13.1 Optical safety

The UV excitation source is low power and chambered, but still requires shielding, duty-cycle limits, and interlocks so that the LED does not run continuously when the sampling door is open.

### 13.2 Bioaerosol contamination control

The spore-event chamber is designed around a replaceable insert. Field tools that sample biological aerosols must be easy to clean, otherwise their own contamination history biases future readings. The nozzle and chamber insert should therefore be consumable or washable.

### 13.3 Environmental durability

The enclosure target is IP54, enough for splashes, dew, and field dust. The leaf clip is stainless-backed for corrosion resistance, and exposed electrical surfaces are gold plated to improve repeatability after repeated cleaning.

### 13.4 Battery and charging

Because growers may leave tools in vehicles or greenhouses, charging and thermal monitoring need to be robust. The PMIC should suspend charging outside safe temperature limits, and the app should report abnormal battery behavior clearly.

## 14. Economic Value Proposition

The practical value of Canopy Sentinel is not only technical; it is economic.

- **Reduced blanket spraying** lowers chemical cost and labor.
- **Earlier localized intervention** reduces crop loss.
- **Better greenhouse airflow tuning** saves energy compared with brute-force ventilation.
- **Documented scouting evidence** improves confidence in decisions and communication across teams.
- **Open-source extensibility** reduces vendor lock-in for researchers and specialty growers.

In many specialty crops, preventing even a small outbreak in a premium block can justify the device quickly.

## 15. Research and Product Roadmap

A strong roadmap for this platform includes:

- pathogen-specific calibration packs,
- canopy-specific optical heads,
- drive-by robotic scouting mounts,
- cloud analytics for season trends,
- support for controlled inoculation trials,
- more advanced risk models using Bayesian updating.

Because the sensing stack is modular, each of those expansions can build on the same core architecture.

## 16. Manufacturing and Expansion Notes

Future variants could include:

- a telescoping pole version for orchard canopy tops,
- a tractor-mounted drive-by scanner,
- LoRa relay beacons for larger farms,
- removable pathogen-specific optical cartridges,
- machine-learning calibration models trained per crop variety.

The present design intentionally balances novelty with manufacturability. It uses readily sourceable sensors, a single main MCU, straightforward mixed-signal front ends, and a companion app that can evolve independently from the firmware.

---

## 17. Repository Structure

```text
canopy-sentinel/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   └── drivers/
│       ├── ble.c / ble.h
│       ├── climate.c / climate.h
│       ├── display.c / display.h
│       ├── leaf.c / leaf.h
│       ├── power.c / power.h
│       ├── spore.c / spore.h
│       ├── storage.c / storage.h
│       └── thermal.c / thermal.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── components/
    │   ├── MetricCard.js
    │   ├── RiskGauge.js
    │   └── ThermalGrid.js
    ├── screens/
    │   ├── DeviceScreen.js
    │   ├── HomeScreen.js
    │   ├── ReportsScreen.js
    │   ├── SettingsScreen.js
    │   └── SessionScreen.js
    └── utils/
        └── protocol.js
```

---

## 18. Closing Summary

Canopy Sentinel is an original and highly practical open hardware device: a **portable crop-canopy disease-risk mapper** that turns microclimate, wetness, thermal structure, airflow, and spore-event data into immediate, field-level decisions. It does not replace laboratory diagnostics, but it fills a neglected and valuable operational gap between weather forecasts and visible disease symptoms. For growers working under tight margins, fungicide resistance pressure, and increasing climate variability, that gap matters.

By making the hardware, firmware, and companion app open and modifiable, this design gives researchers and practitioners a foundation for a whole new category of plant-health tools. It is a device that does not meaningfully exist in open form today, but clearly could and should.
