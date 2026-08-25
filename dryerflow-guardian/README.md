# DryerFlow Guardian — Smart Dryer Exhaust Health Monitor

![Device](https://img.shields.io/badge/Device-DryerFlow%20Guardian-blue)
![MCU](https://img.shields.io/badge/MCU-ESP32--S3-orange)
![Sensors](https://img.shields.io/badge/Sensors-Airflow%20%2B%20Pressure%20%2B%20Thermal%20%2B%20Acoustic%20%2B%20CO-green)
![Connectivity](https://img.shields.io/badge/Connectivity-Wi--Fi%20%2B%20BLE-teal)
![Author](https://img.shields.io/badge/Author-jayis1-orange)

**Author:** jayis1  
**Copyright © 2026 jayis1. All rights reserved.**  
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)

---

## 1. Device Purpose and Overview

DryerFlow Guardian is a new class of residential safety instrument: a **smart dryer exhaust health monitor** that mounts directly on a clothes dryer vent run and continuously measures whether the exhaust path is staying clean, flowing freely, drying efficiently, and venting safely. It is designed for the part of the home nobody actively watches but that causes an outsized number of fires, moisture failures, and energy losses: the dryer duct.

Every modern household depends on a dryer, but nearly all dryers still operate blindly. Users can hear the drum spinning and feel warm air near the machine, yet they do not know whether lint is quietly narrowing the duct, whether a crushed hose is starving airflow, whether a long vent run is accumulating condensation, or whether a gas dryer is flirting with combustion backdraft. Most people only discover a problem after clothes start taking too long to dry, the laundry room gets humid, the machine overheats, or worse, lint ignites. Preventive maintenance is infrequent because the user has no practical signal that cleaning is needed.

DryerFlow Guardian solves that problem by treating the dryer vent like a monitored air-handling system rather than a passive tube. The device clamps around the duct with a service saddle, samples **differential pressure**, **air temperature**, **humidity decay**, **blower acoustics**, **volatile gas load**, and **carbon monoxide backup sensing** for gas dryers, then fuses those signals into clear maintenance and safety guidance. Instead of a crude “clean vent annually” rule, DryerFlow Guardian can tell the user:

- airflow is down 28% compared with the installation baseline,
- the vent resistance has climbed across the last 14 loads,
- lint buildup is likely in the elbow nearest the wall,
- the load is taking longer to enter the dry-down phase,
- a gas dryer may be spilling exhaust into the room, and
- approximately how many safe loads remain before service is strongly recommended.

That final metric is one of the device’s most novel ideas: **remaining safe loads**, a predictive maintenance estimate derived from load-to-load airflow degradation, thermal profile drift, and acoustic turbulence changes. Homeowners do not think in pascals and spectral peaks; they understand “you should clean the vent this weekend” and “you probably have three to five loads left before drying performance drops sharply.” DryerFlow Guardian converts engineering observability into that kind of actionable household language.

### Why This Device Should Exist

The appliance market contains lint traps, moisture sensors inside dryers, standalone carbon monoxide alarms, and a handful of vent thermometers or simple airflow alarms. What does not meaningfully exist is an integrated, retrofit, app-connected vent monitor that performs continuous **vent health analytics** without requiring dryer disassembly or custom HVAC tooling. DryerFlow Guardian fills that gap.

It is especially valuable because dryer vent issues create multiple categories of harm at once:

1. **Fire risk** — lint is combustible, and elevated backpressure raises exhaust temperature.
2. **Indoor air quality risk** — gas dryers can spill CO and combustion byproducts if venting is poor.
3. **Moisture risk** — restricted ducts trap humid exhaust, increasing mold potential in walls and laundry rooms.
4. **Energy waste** — poor airflow lengthens cycles and increases electrical or gas consumption.
5. **Appliance wear** — hot, slow-drying operation stresses heaters, blowers, and bearings.
6. **Tenant and property management risk** — landlords and facilities staff have little visibility into vent condition between inspections.

DryerFlow Guardian is therefore not just a gadget. It is a practical safety, maintenance, and efficiency tool for homes, multifamily housing, laundromats, RVs, tiny homes, and managed properties.

### The Core Product Idea

The system mounts externally in less than fifteen minutes. A split-body enclosure straps to a 4-inch duct segment behind the dryer or at an accessible service point. Two short silicone pitot tubes land upstream and downstream of a short constriction collar to create a repeatable differential pressure reference. A thermal-humidity intake samples exhaust air, a MEMS microphone listens to blower and turbulence signatures through the duct wall, and a low-current gas sensor bay samples room air for backdraft evidence during operation. The result is a non-invasive device that characterizes the dryer exhaust path with far greater insight than any single sensor could provide.

### What Makes DryerFlow Guardian Novel

DryerFlow Guardian is not simply a “dryer notification device.” Its novelty comes from **signal fusion around a ducted appliance**:

- **Differential pressure + acoustic turbulence** estimates developing lint accumulation and local turbulence caused by constrictions.
- **Thermal rise + humidity fall time** identifies drying efficiency and exhaustion of free moisture from the load.
- **VOC/NOx trend + optional CO sense** distinguishes normal operation from combustion backdraft suspicion in gas dryers.
- **Baseline learning** performed after installation makes the device adaptive to short, long, straight, or elbow-heavy vent runs.
- **Remaining safe loads forecasting** turns maintenance into a predictive workflow.
- **Retrofit-friendly clamp form factor** avoids internal dryer modification and avoids inline duct cutting beyond a small saddle collar where desired.

In short: it is a home appliance observability node, purpose-built for one of the most common yet least instrumented fire-risk systems in a house.

---

## 2. Hardware Specifications

### 2.1 Processing Core

- **MCU:** ESP32-S3-WROOM-1-N8R8
- Dual-core Xtensa LX7 up to 240 MHz
- Integrated Wi-Fi 2.4 GHz and BLE 5
- 8 MB flash, 8 MB PSRAM
- Native USB for provisioning, service, and factory test
- Chosen because it supports local analytics, BLE commissioning, OTA updates, and phone-first setup without an external radio coprocessor

### 2.2 Sensors

1. **Differential Pressure Sensor — Sensirion SDP31**  
   Measures pressure drop across the vent reference collar. This is the primary airflow restriction signal. Pressure alone cannot perfectly infer CFM across all installations, but it becomes highly predictive once normalized to the learned baseline.

2. **Exhaust Temperature and Humidity Sensor — Sensirion SHT41 with stainless probe shroud**  
   Tracks heater rise, moisture release, and dry-down timing. A healthy load usually shows a characteristic humidity peak and decay; prolonged high humidity implies poor airflow or overloaded loads.

3. **Room-Air VOC/NOx Sensor — Sensirion SGP41**  
   Used as a low-power combustion anomaly and laundry room air-quality channel. Not a legal replacement for a CO alarm, but useful as a trend signal for vent leakage and solvent-heavy loads.

4. **Electrochemical Carbon Monoxide Front End — SPEC 110-102 sensor via ADS1115 ADC**  
   Optional but strongly recommended for gas dryer configurations. The firmware monitors absolute ppm, rise rate, and correlation with active dryer cycles to detect likely backdraft conditions.

5. **MEMS Acoustic Sensor — INMP441 I²S microphone**  
   Captures blower broadband energy and duct turbulence patterns. Lint accumulation shifts the acoustic texture of the exhaust path; this becomes a powerful supporting feature when combined with pressure and thermal data.

6. **Ambient/Surface Temperature Pair — TMP117 plus duct-contact NTC**  
   Enables ambient compensation and enclosure thermal protection while estimating duct skin temperature for safety and diagnostics.

7. **Magnet/Reed Run-State Sensor**  
   Optional small magnetic tab on the dryer door or chassis helps anchor cycle start/stop detection. The device can also infer operation from airflow and acoustics alone.

### 2.3 Connectivity

- **BLE 5.0:** first-time phone commissioning, near-field service, direct live telemetry
- **Wi-Fi 2.4 GHz:** cloudless LAN sync, homeowner notifications, MQTT/Home Assistant option
- **USB-C:** provisioning, serial logs, firmware recovery, factory diagnostics

### 2.4 Power System

- **Primary input:** USB-C 5 V
- **Backup battery:** 600 mAh LiFePO4 cylindrical cell
- **Charger/PMIC:** BQ25798 or equivalent power-path charger topology
- **Protection:** resettable fuse, TVS, reverse input protection, battery temperature monitoring
- **Average consumption target:** < 140 mW idle, 620 mW active sensing, < 20 mW shipping mode

The backup battery is not intended for days of continuous operation. It exists so that if a user unplugs the dryer area power strip, the unit can still preserve event history, detect one additional cycle, and warn about maintenance gaps.

### 2.5 Form Factor

- Split clamp enclosure for 100 mm / 4-inch dryer ducts
- Approximate enclosure size: 128 mm × 66 mm × 31 mm
- Silicone hose taps to vent reference collar
- Magnetic plus strap mounting options
- Service hatch for filter screen and battery replacement
- IP42 indoor rating

### 2.6 Mechanical Notes

The enclosure is intentionally offset from the hottest region of the duct and uses a stainless thermal stand-off bracket for probe insertion. The pressure taps are designed to avoid lint ingestion via labyrinth inlet geometry and replaceable sintered PTFE membranes.

---

## 3. System Architecture and Block Diagram

### 3.1 Functional Architecture

DryerFlow Guardian is partitioned into five layers:

1. **Sensing Layer**  
   Pressure, exhaust thermal-humidity, ambient thermal, acoustic, VOC/NOx, and optional CO.

2. **Acquisition Layer**  
   Polls I²C sensors, streams microphone windows, debounces run-state, and applies calibration transforms.

3. **Analytics Layer**  
   Computes airflow estimates, vent resistance index, dry-down timing, blockage suspicion, backdraft suspicion, and service horizon.

4. **Communications Layer**  
   Exposes BLE GATT telemetry, LAN JSON packets, and local service logs over USB-C.

5. **Application Layer**  
   Mobile app renders live health, service coaching, historical loads, and installation guidance.

### 3.2 Text Block Diagram

```text
                +-----------------------------------------+
                |           DryerFlow Guardian            |
                |             Author: jayis1              |
                +-----------------------------------------+
                          |                |            |
                          |                |            |
                +---------v----+   +------v------+   +--v----------------+
                | Pressure Tap |   | Exhaust TH  |   | Acoustic / Gas    |
                | SDP31 +      |   | SHT41 + NTC |   | INMP441 + SGP41 + |
                | vent collar  |   | TMP117      |   | CO front-end      |
                +---------+----+   +------+------+   +---------+---------+
                          |                |                      |
                          +----------------+----------------------+
                                           |
                                   +-------v--------+
                                   |   ESP32-S3     |
                                   | acquisition +  |
                                   | sensor fusion  |
                                   +---+--------+---+
                                       |        |
                          +------------+        +----------------+
                          |                                     |
                  +-------v--------+                    +--------v--------+
                  | BLE / USB-C    |                    | Wi-Fi / MQTT /  |
                  | Setup + local  |                    | LAN telemetry   |
                  | service        |                    | and alerts      |
                  +-------+--------+                    +--------+--------+
                          |                                      |
                          +----------------+---------------------+
                                           |
                                  +--------v--------+
                                  | Companion App   |
                                  | live metrics,   |
                                  | history, guide  |
                                  +-----------------+
```

### 3.3 Data Products Generated by Firmware

The firmware does not merely forward raw sensor values. It generates domain-specific metrics:

- **Vent Resistance Index (VRI):** normalized pressure-to-flow obstruction score from 0 to 100.
- **Dryness Transition Time (DTT):** minutes until humidity decay indicates free-water removal.
- **Backdraft Suspicion Score (BSS):** fused CO, VOC, and pressure response risk estimate.
- **Lint Growth Rate (LGR):** slope of VRI across recent loads.
- **Service Horizon (SH):** forecast of remaining low-risk loads before cleaning is recommended.
- **Cycle Efficiency Score (CES):** normalized thermal and dry-down efficiency estimate.

---

## 4. Firmware Design and Engineering Decisions

### 4.1 Firmware Philosophy

The firmware is built around the idea that a household appliance monitor should remain understandable and serviceable. Rather than bury everything inside opaque machine learning, DryerFlow Guardian uses clear engineering models with tunable coefficients. Each risk score is decomposable into contributing terms so both the app and a future service technician can explain why the unit made a recommendation.

### 4.2 Main Tasks

- **Pressure Task:** samples differential pressure, converts to filtered pascals, estimates relative flow.
- **Thermal Task:** records exhaust and ambient temperature plus RH, derives dryness slope.
- **Acoustic Task:** computes band-energy style features from microphone windows.
- **Gas Task:** evaluates CO ppm and VOC drift during active cycles.
- **Analytics Task:** fuses all metrics into VRI, CES, BSS, and SH.
- **Logger Task:** stores recent cycles in ring-buffer flash records.
- **BLE/LAN Task:** publishes live telemetry and responds to setup requests.

### 4.3 Why the Firmware Uses Fusion Instead of a Single Threshold

A simple temperature alarm cannot distinguish a hot but healthy cycle from a genuinely restricted vent. A simple pressure sensor cannot distinguish a long but clean duct from a short but clogged one. Acoustic data alone varies with dryer model. By combining them, the firmware becomes robust across installations:

- Pressure explains restriction.
- Thermal behavior explains energy transfer.
- Humidity decay explains moisture removal performance.
- Acoustics explain turbulence and blower loading.
- Gas sensing explains leakage and combustion concerns.

That is the central systems insight behind the product.

### 4.4 Baseline Learning

During installation, the app asks the user to run one empty warm cycle and one normal laundry cycle. The device stores those traces as a baseline envelope. All future metrics are referenced against that envelope, which allows the same device to work on a compact apartment dryer with a short straight duct or a large home dryer with a long roof vent.

### 4.5 Safety Model

The firmware uses a three-stage response model:

1. **Observe** — note drift and log the issue.
2. **Advise** — recommend service soon with confidence score.
3. **Escalate** — trigger immediate local and app alert when backdraft or severe blockage is likely.

For gas dryers, escalation occurs faster when rising CO coincides with low measured flow and unusual acoustic loading.

### 4.6 Data Storage Strategy

Recent samples are kept in RAM for responsive graphs; summarized cycle records are stored in nonvolatile flash. Each cycle record contains timestamps, peak pressure, average flow estimate, dry-down timing, alert flags, and maintenance forecast values. This gives the user useful history without storing oversized raw waveforms.

### 4.7 Serviceability

A future installer or service technician can retrieve calibration offsets over USB-C, export recent history, run a pressure tap purge test, and compare current baseline drift to the original commissioning trace.

---

## 5. Companion Application and Software Interface

The companion app is a practical operations console rather than a novelty dashboard. It is intended to be useful for a homeowner standing in a laundry room, a property manager overseeing multiple units, or a service professional diagnosing a complaint.

### 5.1 Primary Screens

1. **Dashboard**  
   Current vent health, run state, estimated service horizon, and a one-line recommendation.

2. **Live Run**  
   Shows active cycle pressure, exhaust temperature, humidity, turbulence, and CO trend in near real time.

3. **Alerts**  
   Lists advisories such as “vent resistance rising,” “possible crushed hose,” “humidity not decaying normally,” or “gas backdraft suspicion.”

4. **Setup Wizard**  
   Guides the user through installation, baseline learning, and gas/electric dryer profile selection.

5. **Service Coach**  
   Provides step-by-step maintenance checklists, cleaning intervals, and before/after performance comparison.

### 5.2 BLE and LAN Protocol

The firmware emits compact JSON payloads containing:

- device ID
- timestamp
- run state
- pressure Pa
- estimated flow CFM
- exhaust temperature C
- relative humidity
- CO ppm
- VRI, CES, BSS, SH
- alert bitfield

The app can operate purely locally over BLE for privacy-conscious users, or on a home LAN over Wi-Fi for richer history and optional automation.

### 5.3 Home Automation Integration

A future extension can publish MQTT topics such as:

- `dryerflow/guardian/state`
- `dryerflow/guardian/alerts`
- `dryerflow/guardian/service_horizon`
- `dryerflow/guardian/backdraft_score`

This makes the device valuable to Home Assistant and rental property monitoring systems.

---

## 6. Use Cases and Target Audience

### 6.1 Homeowners

The core user is a homeowner who wants fewer fire risks, shorter dry times, and a simple reminder about when to clean the vent. DryerFlow Guardian makes invisible maintenance visible.

### 6.2 Multifamily and Property Managers

Apartment operators often have dozens or hundreds of dryers and no practical way to know which units are drifting into unsafe conditions between annual inspections. DryerFlow Guardian can flag the highest-risk units first.

### 6.3 Service Technicians

For appliance repair and vent cleaning businesses, the device creates a measurable before-and-after proof of service. A technician can show airflow resistance dropping after cleaning instead of relying on anecdote.

### 6.4 Laundromats and Shared Facilities

High-cycle environments benefit from cycle efficiency tracking, service horizon prediction, and quick identification of a blocked or partially detached vent.

### 6.5 RV and Tiny-Home Owners

Small spaces are more sensitive to heat, humidity, and combustion leakage. A compact, smart vent monitor is especially relevant there.

---

## 7. Example Operating Scenario

Imagine a household with a gas dryer in a second-floor laundry closet. The vent run includes two 90-degree elbows and a long route to an exterior wall hood. At installation the system learns a baseline: moderate pressure drop, healthy humidity decay after twelve minutes, and negligible room-air CO during normal operation.

Over the next two months, lint accumulates in the wall elbow. The user notices nothing. DryerFlow Guardian, however, begins seeing a repeatable pattern:

- pressure drop rises cycle over cycle,
- blower turbulence broadens in the 1.8 kHz to 3.2 kHz band,
- humidity stays elevated longer,
- the heater duty signature suggests longer active drying,
- one cycle shows a brief 5 ppm CO room-air bump near cycle start.

The app first marks the vent as “monitor.” After five more loads the service horizon falls to four loads remaining. The user gets a clear message: “Vent resistance is 31% above baseline. Clean vent within 1 week. Drying time and fire risk are increasing.” If the user ignores that and flow falls further while CO rises again, the device escalates to a high-priority backdraft alert.

That is the practical value proposition: catching a multi-factor household hazard before it becomes a damage event.

---

## 8. Manufacturability and Cost Targets

### 8.1 Estimated BOM Target

- ESP32-S3 module: $4.90
- Differential pressure sensor: $11.50
- Thermal-humidity sensor: $2.60
- VOC sensor: $5.10
- CO front-end and ADC: $12.20
- MEMS microphone: $1.10
- PMIC, passives, connectors, protection: $7.80
- PCB + assembly: $10.50
- Enclosure, tubes, straps, bracket: $8.40
- Battery: $2.30

**Target BOM:** approximately $66 to $72 depending on CO option and assembly volume.

### 8.2 Retail Positioning

A viable retail target would likely be $149 for the electric-dryer version and $179 for the gas-dryer-safe version with CO sensing. That is inexpensive compared with one vent cleaning visit, much less a moisture remediation or fire event.

---

## 9. Future Extensions

- paired exterior vent cap beacon for direct end-to-end airflow estimation,
- machine-learning classification of blockage location,
- maintenance marketplace integration for scheduling cleaning services,
- multi-unit fleet portal for building operators,
- optional mmWave occupancy sensor for appliance-room safety automation,
- insurance reporting mode with signed maintenance summaries.

---

## 10. Repository Contents

```text
dryerflow-guardian/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── acoustic.c
│       ├── acoustic.h
│       ├── airflow.c
│       ├── airflow.h
│       ├── analytics.c
│       ├── analytics.h
│       ├── ble.c
│       ├── ble.h
│       ├── gas.c
│       ├── gas.h
│       ├── logger.c
│       ├── logger.h
│       ├── power.c
│       ├── power.h
│       ├── pressure.c
│       ├── pressure.h
│       ├── thermal.c
│       └── thermal.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── App.js
    ├── package.json
    ├── components/
    │   ├── MetricCard.js
    │   └── TrendPanel.js
    ├── screens/
    │   ├── AlertsScreen.js
    │   ├── DashboardScreen.js
    │   ├── LiveRunScreen.js
    │   ├── ServiceScreen.js
    │   └── SetupScreen.js
    └── utils/
        └── protocol.js
```

---

## 11. Closing Summary

DryerFlow Guardian is a compelling new hardware product because it addresses a real household blind spot with a realistic combination of sensing, analytics, and user experience. It is original without being speculative. It does not require unrealistic laboratory instrumentation, yet it creates insights ordinary appliances have never exposed. It is the kind of product that could genuinely reduce fires, save energy, reduce nuisance service calls, and make homes safer.

Its uniqueness comes from applying systems engineering to a mundane but high-impact domestic problem. By turning the dryer vent into an observable subsystem, DryerFlow Guardian gives users something they have never meaningfully had before: **continuous, actionable awareness of vent health and drying safety**.
