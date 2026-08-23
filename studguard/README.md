# StudGuard — Adhesive Hidden-Leak Localization Mesh for Walls, Risers, and Utility Chases

![StudGuard](https://img.shields.io/badge/Device-StudGuard-blue) ![Class](https://img.shields.io/badge/Class-Building%20leak%20localization-teal) ![MCU](https://img.shields.io/badge/MCU-nRF5340-green) ![Sensors](https://img.shields.io/badge/Sensors-Acoustic%20%2B%20humidity%20%2B%20capacitance%20%2B%20temperature-orange) ![Wireless](https://img.shields.io/badge/Comms-UWB%20%2B%20BLE%20Mesh-purple) ![Author](https://img.shields.io/badge/Author-jayis1-red)

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## 1. Purpose and Overview

**StudGuard** is a new category of building-health instrument: a **thin adhesive sensor tile that turns ordinary walls into a hidden-plumbing leak localization surface**. Instead of waiting for visible stains, soft drywall, mildew odor, or catastrophic flooring damage, StudGuard lets facilities teams, landlords, insurers, builders, and homeowners detect a developing leak **inside the wall cavity**, estimate its vertical band, and track whether the problem is active, intermittent, or already drying.

The practical problem is simple and expensive. Most residential and commercial water leaks do not begin as dramatic bursts. They begin as a slow drip at a compression fitting, a pinhole in copper, a microcrack in PEX, a bad shower valve seal, a sweating chilled-water riser, or a seasonal freeze-thaw defect. For days or weeks, the water remains hidden. It wets insulation, studs, vapor barriers, and gypsum from the back side. During that time, traditional leak detectors are usually blind. A puddle sensor only notices a leak after water escapes the wall. Thermal cameras can help, but only when temperature contrast happens to be favorable and the operator is trained. Moisture meters can find elevated readings, but they require a technician to know where to probe, and they are poor at continuously tracking a leak that may appear only during short fixture-use events.

StudGuard exists to solve that blind spot. The device is a low-profile tile that is temporarily or semi-permanently adhered to wallboard, tile backer, utility shaft covers, cabinet kick panels, or riser enclosures. Each tile contains:

- a **contactless wall-coupled acoustic exciter and contact microphone**,
- a **capacitive moisture gradient electrode ring**,
- a **temperature and humidity sensor for dew-point and drying analysis**,
- a **UWB timing transceiver** for synchronization between neighboring tiles,
- **BLE** for mobile setup and local service use,
- and a battery-backed, ultra-low-power processing core.

In deployment, several StudGuard tiles are placed vertically or horizontally around a likely leak zone: behind a bathroom vanity, on the wall behind a washing machine, near a mechanical riser, beside a shower valve wall, under a kitchenette sink, or in an apartment stack. The tiles periodically excite the wall with a tiny swept mechanical chirp, measure the returning signature, compare changes over time, and correlate those changes with local moisture and humidity drift. By combining **multi-point acoustic attenuation**, **phase shift**, **capacitive moisture rise**, and **microclimate persistence**, the firmware computes a **Leak Activity Index**, **Wetness Spread Index**, and **Probable Origin Band**.

That is the key novelty: StudGuard is not merely a water alarm. It is a **hidden-structure leak localization mesh** designed for normal walls and chases, where the operator cannot see the pipe. It does not require piercing the wall, adding a camera borescope, or relying on a thermal differential that may not exist. It creates a persistent, data-driven map of whether the wall cavity is getting wetter, staying wet, or drying back down.

### Why this device should exist

The economic case is enormous. Small hidden leaks generate some of the most frustrating repair workflows in buildings because by the time they become obvious, the cost is no longer the plumbing fix alone. The owner now also pays for drywall replacement, cabinet damage, flooring damage, mold remediation, tenant displacement, repainting, and repeated investigative labor. Insurance claims and multi-unit property management costs scale badly because maintenance teams often must open multiple walls just to identify the actual source.

StudGuard changes the sequence. Instead of “wait for damage, then cut open the wall,” the workflow becomes:

1. Suspect a hidden leak or high-risk area.
2. Attach a cluster of StudGuard tiles in minutes.
3. Let the system monitor over real fixture usage cycles.
4. Review the wall-cavity localization map in the companion app.
5. Open one targeted repair zone instead of exploratory demolition.

That matters in apartments, hotels, hospitals, schools, offices, and homes. It also matters in preventive maintenance: chilled-water lines, fan-coil walls, laboratory sinks, staff showers, janitorial closets, sprinkler risers, and utility shafts can all develop low-rate leakage where visible inspection is poor.

### What makes StudGuard original

StudGuard is original because it combines sensing modes that are individually known but rarely fused into a practical, low-cost wall diagnostic product:

1. **Wall-coupled active acoustics** rather than only airborne listening. The tile excites the wall itself and listens for changes in damping caused by hidden wet insulation, softened gypsum, or fluid contact pathways.
2. **Distributed time-synchronized tiles** using UWB timestamp exchange so neighboring devices can compare propagation changes rather than acting as isolated sensors.
3. **Capacitive surface-field moisture inference** that does not require penetrating pins. The electrode ring senses changing dielectric conditions near the wall surface and edge zones.
4. **Condensation vs. leak discrimination** by comparing humidity, surface temperature trend, and acoustic spread. A bathroom wall that briefly fogs from steam behaves differently than a wall cavity getting steadily wetter from behind.
5. **Activity pattern classification** that labels likely leak behavior: active pressure leak, intermittent fixture-linked leak, seasonal condensation issue, or residual drying after repair.
6. **App-guided placement and repair targeting** for non-expert users. The mobile interface explains where to place tiles and how many are needed for better localization confidence.

This makes StudGuard suitable both as a professional maintenance instrument and as a deploy-and-watch tool for property owners.

---

## 2. Device Concept

A StudGuard installation uses one to eight tiles. Each tile is roughly the footprint of a drink coaster, only a few millimeters thick, with a compliant adhesive ring and a slightly raised center pod for electronics. The tile is designed to couple gently into painted drywall, cement board, laminated cabinet toe-kicks, access panels, or utility chase covers without causing damage during temporary deployment.

Each tile runs a repeating measurement cycle:

1. Wake from low-power sleep.
2. Sample ambient temperature and humidity.
3. Measure capacitive coupling from the wall electrode ring.
4. Emit a short swept chirp through a piezo-bender bonded to the tile body.
5. Capture the returning wall vibration using a contact MEMS or piezo pickup.
6. Extract energy, decay, spectral centroid, phase, and damping features.
7. Exchange timing markers with peer tiles.
8. Compute local wetness trend and participate in a mesh-level origin estimate.
9. Broadcast summary data to the phone or gateway.
10. Return to sleep.

If the system detects a fast-rising anomaly, it can temporarily increase its sample rate and prompt the user to perform a guided “fixture event” test. For example, the app may say: **turn on the shower for 90 seconds** or **run the dishwasher fill cycle now**. During that interval, StudGuard looks for synchronized wetness/acoustic changes that correlate with the event, dramatically improving localization of intermittent leaks.

This workflow is especially valuable because many nuisance leaks are intermittent. A slow shower valve leak may only appear while pressure is applied. An upstairs drain leak may only occur during discharge. A chilled-water condensation issue may correlate with HVAC demand. Static moisture meters miss these patterns; StudGuard is built around them.

---

## 3. Hardware Specifications

### 3.1 Core Processing

| Parameter | Value |
|---|---|
| Primary MCU | Nordic nRF5340 |
| CPU | Dual-core Arm Cortex-M33 (128 MHz application + 64 MHz network) |
| RAM | 512 KB total |
| Flash | 1 MB internal |
| Auxiliary timing | DW3110 UWB transceiver timestamp engine |
| Storage | 64 Mbit QSPI NOR flash for local history |
| Why chosen | Strong low-power BLE support, enough DSP for acoustic feature extraction, modern secure OTA path |

The nRF5340 is a strong fit because StudGuard needs three things at once: aggressive battery life, reliable local wireless provisioning, and enough math throughput to run small-window DSP. The acoustic problem is not huge, but it is meaningful: windowing, spectral moment extraction, decay fitting, and peer comparison all need to happen on-device to minimize radio traffic and preserve privacy.

### 3.2 Sensing Subsystems

#### Wall acoustic subsystem

| Parameter | Value |
|---|---|
| Exciter | 18 mm piezo-bender disc with resonant backing mass |
| Receiver | Contact MEMS vibration sensor / piezo pickup front-end |
| Sweep band | 350 Hz to 8 kHz |
| Capture window | 96 ms nominal |
| Purpose | Detect damping, wet-material propagation shifts, and cavity condition changes |

The acoustic subsystem is the heart of the product. Dry gypsum, damp gypsum, wet insulation, and water-contacted wood each alter wall-coupled vibration differently. StudGuard intentionally uses **repeatable active excitation** rather than only passive noise because ambient building sound is too variable for dependable trend extraction.

#### Capacitive moisture ring

| Parameter | Value |
|---|---|
| Geometry | Segmented copper ring around tile perimeter |
| Method | Charge-transfer capacitance sensing with guard drive |
| Purpose | Detect dielectric changes from hidden moisture migration near wall surface |

The segmented ring also helps determine directional bias. If the top segment rises faster than the bottom, the likely source may be above the tile rather than below it.

#### Environmental sensing

| Parameter | Value |
|---|---|
| Sensor | Sensirion SHT41 or equivalent |
| Outputs | Temperature, relative humidity |
| Purpose | Condensation discrimination, dew-point trend, drying-rate calculations |

#### Motion / placement sensing

| Parameter | Value |
|---|---|
| Sensor | LIS2DW12 accelerometer |
| Outputs | Orientation, tap detect, vibration sanity checks |
| Purpose | Detect re-placement, confirm wall contact quality, reject user disturbance events |

### 3.3 Connectivity and Mesh

| Parameter | Value |
|---|---|
| Local setup | BLE 5.3 |
| Peer timing | UWB DW3110 |
| Optional gateway | BLE-to-Wi-Fi bridge or phone relay |
| Topology | Star or linear wall mesh |
| Range | Room-scale, unit-scale, and riser-stack multi-node deployment |

UWB is not used here for classic room positioning. Instead, it provides robust peer timing and marker exchange so tiles can align their active acoustic snapshots and compare propagation signatures without drift.

### 3.4 Power

| Parameter | Value |
|---|---|
| Battery | 1200 mAh LiPo pouch |
| Runtime | 4–6 months at 15-minute baseline interval; days to weeks at diagnostic high-rate mode |
| Charging | USB-C or magnetic pogo dock |
| PMIC | Low-Iq charger + fuel gauge |
| Energy strategy | Deep sleep, burst sensing, adaptive reporting |

### 3.5 Form Factor

| Parameter | Value |
|---|---|
| Tile footprint | 78 mm × 78 mm |
| Thickness | 8.5 mm center pod, 2.5 mm edge ring |
| Weight | ~72 g |
| Mounting | Repositionable adhesive ring + mechanical lanyard slot |
| Environmental | Indoor IP42, condensation tolerant but not immersion-rated |

---

## 4. Architecture and Block Diagram

### 4.1 Functional block diagram

```text
                    +----------------------------------+
                    |           USB-C / Dock           |
                    +----------------+-----------------+
                                     |
                          +----------v-----------+
                          | Charger / Fuel Gauge |
                          +----------+-----------+
                                     |
                     +---------------v----------------+
                     |            nRF5340             |
                     | DSP + leak model + BLE + OTA   |
                     +---+-----------+---------+------+
                         |           |         |
              +----------+--+   +----+---+  +-+----------------+
              | Piezo Driver |   | SHT41 |  | QSPI NOR History |
              +------+-------+   +--------+  +------------------+
                     |
             +-------v--------+    +---------------------+
             | Wall Piezo Tx  |    | Capacitive Ring AFE |
             +-------+--------+    +----------+----------+
                     |                        |
             +-------v--------+    +----------v----------+
             | Contact Vib Rx |    | Segment Moisture    |
             | + PGA + ADC    |    | Direction Estimator |
             +-------+--------+    +---------------------+
                     |
             +-------v---------+
             | Feature Engine  |
             | decay/spectrum  |
             +-------+---------+
                     |
             +-------v----------+
             | UWB DW3110 Peer  |
             | timing exchange  |
             +-------+----------+
                     |
               +-----v------+
               | BLE / App  |
               +------------+
```

### 4.2 Firmware architecture

The firmware is organized as a deterministic cooperative scheduler rather than a full RTOS. That decision reduces idle overhead, simplifies certification-style reasoning, and makes the active-acoustic cycle easy to test in simulation. The major modules are:

- **main.c** — scheduler, state machine, device modes, session orchestration.
- **drivers/acoustic.c** — chirp generation, sample synthesis/capture, feature extraction.
- **drivers/moisture.c** — segmented capacitive sensing and directional wetness estimation.
- **drivers/mesh.c** — peer status exchange, origin-band solving, network health.
- **drivers/display.c** — local status text and LED icon abstraction.
- **drivers/ble.c** — payload packing and companion-app telemetry schema.
- **drivers/power.c** — battery, interval policy, adaptive sample rate.
- **drivers/logger.c** — flash-backed ring buffer, session records, export formatting.

A key design choice is **late fusion rather than raw-stream dependence**. Instead of transmitting full waveforms to the app, the device computes compact features and only stores selected snapshots. That preserves battery, reduces privacy risk, and keeps the app responsive even with several deployed tiles.

### 4.3 Leak classification pipeline

StudGuard transforms raw measurements into decisions using a staged model:

1. **Acquire local features** — ambient temperature/RH, capacitive segments, acoustic envelope, decay time, spectral centroid, phase stability.
2. **Stabilize by baseline** — compare against each tile’s installation baseline and longer-term drift model.
3. **Peer compare** — compare simultaneous features across neighboring nodes.
4. **Infer spread direction** — use segment asymmetry and peer gradients.
5. **Classify event mode** — steady, intermittent, post-repair drying, or condensation-like.
6. **Compute risk outputs** — leak activity, wetness spread, probable origin band, confidence score.

This is deliberately interpretable. Users and technicians need to understand *why* the device believes a leak is present.

---

## 5. Firmware Details and Design Decisions

### 5.1 Active acoustic method

StudGuard emits a low-energy chirp that is mechanically coupled into the wall through the piezo bender and adhesive ring. The receiver captures the wall response, which is windowed and reduced to metrics such as:

- total energy,
- early/late decay ratio,
- exponential damping constant,
- spectral centroid,
- band energy skew,
- phase repeatability across pulses,
- peer-to-peer attenuation differential.

As water content increases behind a wall, the structure often exhibits more damping and altered frequency content. The exact response depends on construction, which is why StudGuard learns a site-specific baseline at install time and emphasizes change detection.

### 5.2 Moisture model

The capacitive ring is segmented into top, right, bottom, and left sectors. The firmware performs repeated charge-transfer measurements and converts them into normalized dielectric indices. A leak from above may first raise the top segment, while a sink-base leak may raise the bottom. The model combines segment deltas with ambient humidity and wall orientation to avoid false positives from room-wide humidity swings.

### 5.3 Intermittent leak capture

An intermittent leak is often the hardest to prove. StudGuard therefore includes a **fixture test mode**. When triggered by the app, all tiles near the area temporarily raise their sample cadence and mark the test window. The resulting graph can show, for example, that Tile 3 and Tile 4 spike only when the upstairs tub drains. That gives technicians much stronger evidence before opening a wall.

### 5.4 Privacy and safety

StudGuard does not record speech or room audio. The acoustic front-end is contact-coupled and only stores short feature vectors, not continuous microphone recordings. This is an important product decision for use in occupied apartments, healthcare environments, and hospitality spaces.

### 5.5 Fault tolerance

The firmware detects poor wall coupling, adhesive lift, battery aging, corrupted baseline data, and peer dropout. If confidence falls, the app explicitly warns the user rather than presenting a false sense of certainty.

---

## 6. Application / Software Interface

The StudGuard mobile app is designed around real maintenance work, not generic consumer IoT dashboards.

### 6.1 Primary screens

- **Dashboard** — live fleet summary, flagged zones, battery state, confidence status.
- **Survey Screen** — guided deployment wizard, placement spacing, wall photos, room tags.
- **Node Detail** — per-tile trend lines, segment bias, acoustic change chart, baseline age.
- **Leak Timeline** — fixture-event overlays, leak activity history, drying trend after repair.
- **Settings** — intervals, alert thresholds, export format, OTA controls.

### 6.2 Data model

Each node reports:

- node ID and room label,
- battery percent,
- temperature and RH,
- moisture ring segment values,
- acoustic energy and decay metrics,
- leak activity index,
- wetness spread index,
- probable origin band,
- confidence score,
- last event classification,
- peer connectivity status.

### 6.3 Companion workflow

A typical workflow is:

1. Create a new survey called “Unit 4B shower wall.”
2. Add four tiles using BLE provisioning.
3. Photograph the wall and drag tile positions onto the photo.
4. Start baseline capture.
5. Return later or run a fixture test.
6. Review the app’s recommended opening band and confidence heatmap.
7. Export a PDF or JSON service report.

This workflow makes the product useful to building managers and insurance adjusters who need evidence, repeatability, and documentation.

---

## 7. Use Cases and Target Audience

### 7.1 Multi-family housing maintenance

Apartment stacks frequently suffer from difficult-to-localize leaks involving tubs, shower valves, toilet seals, dishwasher feeds, condensate lines, and shared risers. StudGuard lets maintenance teams deploy tiles across units and floors, helping determine whether a problem is above, behind, or lateral to the visible damage.

### 7.2 Hospitality and healthcare

Hotels and hospitals have high room counts and expensive downtime. Hidden moisture events can disrupt occupancy and create hygiene concerns. StudGuard allows fast room-side investigation with minimal disruption.

### 7.3 Insurance field assessment

Adjusters and restoration contractors need a way to document whether a wall is actively getting wetter or simply drying after mitigation. StudGuard provides trend evidence rather than one-off spot readings.

### 7.4 Residential troubleshooting

Homeowners often know something is wrong but cannot justify invasive demolition yet. A kit of three to five StudGuard tiles could reduce guesswork and help determine whether a plumber, HVAC technician, or restoration contractor is needed.

### 7.5 Preventive mechanical-room and riser monitoring

Mechanical closets, fan-coil walls, chilled-water risers, janitorial sinks, and concealed service spaces are ideal targets for semi-permanent deployment.

---

## 8. Why StudGuard Could and Should Be Built

StudGuard sits at a practical intersection of affordability, usefulness, and technical feasibility. Every subsystem is based on mature building blocks: low-power MCUs, piezo exciters, capacitive sensing, humidity sensors, and BLE provisioning. The novelty comes from integrating them into a workflow-focused leak localization mesh rather than inventing exotic physics or requiring specialist infrastructure.

It could be built because the sensing problem is mainly one of repeatability, baseline management, and feature fusion—not impossible hardware. It should be built because buildings waste enormous money and material on late leak detection, while current tools are either reactive, invasive, or technician-heavy.

StudGuard also has a clear expansion path:

- wired gateway deployments for large portfolios,
- adhesive strips for cabinet toe-kicks,
- magnetic versions for metal riser panels,
- machine-learned wall-construction templates,
- integration with maintenance software and insurer workflows,
- “repair verified” mode showing that a wall is truly drying after intervention.

That last point is especially important. In practice, users do not just want to know *that* a leak exists. They want to know when the fix actually worked. A restoration contractor wants a graph that proves moisture conditions are trending back to baseline. StudGuard is explicitly designed to provide that operational confidence.

---

## 9. Development Notes for This Repository

This repository implementation includes:

- a substantial simulation-oriented firmware codebase in C,
- modular drivers for acoustic sensing, moisture estimation, power, BLE payloads, mesh correlation, display output, and logging,
- KiCad design files describing a manufacturable tile mainboard,
- a React Native companion app with multiple screens and realistic data flows.

The firmware is structured so it can be compiled on a host system for logic verification while still mirroring the architecture of an embedded product. That choice keeps the repository practical for review and extension.

---

## 10. Summary

StudGuard is a novel, practically useful hardware product because it solves a common and expensive problem with a workflow that existing leak alarms and spot meters do not address. By combining active wall acoustics, capacitive moisture inference, environmental sensing, peer timing, and a maintenance-focused app, it enables **hidden leak localization before visible wall failure**. It is original enough to be interesting, grounded enough to be buildable, and useful enough that building operators would actually want it.

In short: StudGuard is the leak detector you use **before** there is a puddle, **before** there is mold, and **before** you cut open the wrong wall.
