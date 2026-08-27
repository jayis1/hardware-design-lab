# SealBeat

**Author: jayis1**  
**Copyright (C) 2026 jayis1. All rights reserved.**

## Overview

SealBeat is a **retrofit refrigerator and freezer seal intelligence module** designed to detect the subtle failure modes that quietly waste energy, degrade food quality, and create compliance risk long before the appliance is obviously broken. It mounts along the door frame or hinge-side liner of an existing refrigerator, freezer, vaccine cooler, lab cold cabinet, or convenience-store merchandiser and builds a live model of how the door actually closes in the real world. Instead of acting as a simple open/closed contact switch, SealBeat measures the quality of the closure event itself: gasket compression consistency, magnetic latch strength, warm-air intrusion after closure, hinge sag progression, shelf-overload side effects, nighttime door cycling, compressor recovery burden, and compartment zone drift.

That combination is what makes the concept new. Existing products typically do one of three things: they log temperature, alert if a door is left open, or provide full smart-fridge control as part of a new appliance purchase. Those are useful, but they miss a large practical gap. In homes, restaurants, pharmacies, and schools, many refrigeration failures start as **mechanical seal degradation**, not catastrophic compressor failure. A slightly warped door, a dirty gasket, a weak magnetic seal, overloaded bins that stop a proper close, or a hinge that has sagged by only a few millimeters can increase compressor runtime, create warm pockets, reduce food shelf life, and cause hard-to-explain temperature excursions. SealBeat is intended to catch that middle ground.

The device uses a **multi-sensor closure fingerprint**. A magnetometer observes the magnetic seal profile along the final centimeters of travel. A force-sensing compression strip or elastomer-coupled piezoresistive pad estimates how evenly the gasket lands. A time-of-flight distance sensor measures near-frame gap evolution in the final close arc. A thermopile watches the local interior edge temperature and recovers warm-air intrusion signatures after the door shuts. An ambient temperature and humidity sensor tracks room conditions and condensation risk. A low-power IMU tracks hinge dynamics, shock, and frame alignment changes. A contact acoustic path or MEMS microphone listens for compressor start behavior, fan harmonics, and latch impact character. On-device firmware turns those signals into actionable outputs: “top-right gasket leak growing,” “door bounce on overpacked shelf,” “freezer recovering too slowly after each close,” or “pharmacy cooler remained safe but closure quality degraded 22% over 14 days.”

SealBeat should exist because cold-chain integrity is often managed too late. Households throw away groceries without understanding why lettuce freezes on one shelf while milk warms on another. Small restaurants discover a bad reach-in only after prep ingredients exceed safe hold temperatures. Pharmacies and clinics invest in expensive coolers yet still rely on humans to notice when the door feels “a little off.” Property managers replace refrigerators only after energy bills rise or ice forms around the liner. The hidden problem is not lack of sensing; it is lack of **retrofit closure intelligence**. SealBeat provides that intelligence without replacing the appliance, penetrating refrigerant lines, or depending on cloud infrastructure.

## Device purpose

SealBeat is intended to answer practical questions that existing temperature loggers usually cannot answer by themselves:

1. Is the door physically sealing correctly, or is one edge leaking warm air?
2. Has hinge sag or shelf loading changed the close geometry over time?
3. Is the magnetic gasket still pulling uniformly along the frame?
4. Are repeated short door openings causing abnormal thermal recovery burden?
5. Is a unit still food-safe or medicine-safe even if closure quality is degrading?
6. Which corrective action matters most: clean the gasket, re-level the appliance, reduce shelf interference, replace the hinge, or replace the gasket?

The device is especially useful for households with aging refrigerators, pharmacies with vaccine and insulin cold storage, commercial kitchens, school cafeterias, lab sample cabinets, smart-building retrofit programs, energy auditors, and appliance service technicians who want a quick data-backed picture before replacing expensive components.

## Why the concept is original

SealBeat is original because it treats the refrigerator door as a **dynamic electromechanical system** instead of a binary state. Existing add-on products may detect “door open” or “temperature high,” but they generally do not characterize the *quality* of each closing event or infer *why* the appliance is drifting. SealBeat fuses closure magnetics, compression symmetry, thermal rebound, and recovery acoustics to build a door health signature unique to that specific appliance.

That makes several new workflows possible:

- Detecting a partial top-corner leak before food temperature alarms occur.
- Distinguishing “door was left ajar” from “door closed, but bounce-back created a recurring micro-gap.”
- Tracking hinge wear over months with quantified closure asymmetry.
- Comparing compressor recovery energy burden to closure quality score for predictive maintenance.
- Generating technician-ready evidence before a service visit.

The novelty is not one exotic sensor; it is the **architected fusion** of simple sensors around a problem that consumers and operators already have, but currently manage with guesswork.

## Hardware specifications

### Compute and wireless

- **Primary MCU:** Nordic nRF5340 dual-core wireless SoC
- **Wireless:** Bluetooth Low Energy 5.4 for provisioning, diagnostics, live closure capture, and historical sync
- **Mesh/automation option:** Thread/Matter-ready for facility dashboards and appliance fleet monitoring
- **External storage:** 16 MB QSPI flash for event windows, seal trend history, and OTA update staging
- **Security:** signed firmware updates, device-unique provisioning secret, optional local-only mode

### Sensor suite

- **3-axis magnetometer:** MMC5983MA for magnetic gasket pull profile and latch vector trend
- **Time-of-flight distance sensor:** VL53L4CX for final-gap measurement during closing arc
- **Infrared thermopile:** MLX90632 for non-contact edge-zone temperature recovery tracking
- **Ambient temperature / humidity:** SHT45 for room context and condensation analysis
- **6-axis IMU:** BMI270 for hinge motion, slam intensity, appliance vibration, and tilt drift
- **Compression sensor:** thin piezoresistive gasket landing strip through ADS122C04 24-bit ADC
- **Acoustic monitor:** analog MEMS microphone coupled to frame rail for latch and compressor signature capture
- **Battery gauge:** MAX17048
- **Power-path charger:** BQ25185 over USB-C

### Power

- **Battery:** 1800 mAh LiPo pouch or 2× AA lithium pack option
- **Charging/input:** USB-C at 5 V when using rechargeable SKU
- **Battery life target:** 8–12 months in residential mode, 5–7 months in high-traffic commercial mode
- **Low-power approach:** event-triggered high-rate sampling, baseline low-rate magnetics/temperature monitoring, adaptive sync windows

### Mechanical and form factor

- **Enclosure:** slim adhesive-backed spine plus optional hinge-side bracket
- **Primary body size:** 96 mm × 22 mm × 12 mm
- **Seal strip accessory:** 180 mm flexible compression strip with low-profile cable tail
- **Mounting:** removable VHB frame rail mount, hinge bracket, or pharmacy-cabinet clip
- **Ingress approach:** kitchen-safe wipeable housing with vented humidity chamber and splash-resistant ports

### Connectivity model

- BLE for setup, diagnostics, firmware updates, and on-demand live close capture
- Thread/Matter bridge mode for fleet dashboards, building automation, and cold-chain records
- Local-first operation with optional cloud export through the app; all primary diagnostics remain useful offline

## Architecture

SealBeat is architected as a layered edge diagnostic platform:

1. **Mechanical interface layer** – the frame rail housing and compression strip convert seal behavior into measurable signals without altering the appliance door.
2. **Acquisition layer** – magnetometer, ToF, thermopile, IMU, acoustic path, ambient sensor, and battery telemetry gather synchronized samples.
3. **Feature extraction layer** – firmware computes latch slope, compression symmetry, bounce count, warm-edge rebound, recovery time constant, hinge skew estimate, and door-cycle context.
4. **Inference layer** – weighted heuristics plus profile-specific scoring produce a seal score, thermal safety score, hinge wear score, and maintenance recommendation.
5. **Communication layer** – compact BLE packets expose recent closure events, daily summaries, zone trends, and technician diagnostics.
6. **Application layer** – the companion app turns raw closure signatures into practical action such as “clean lower gasket,” “reduce door-bin load,” or “service hinge before seal replacement.”

### Block diagram

```text
+--------------------------------------------------------------------------------+
|                                  SealBeat                                      |
|                               Author: jayis1                                   |
+--------------------------------------------------------------------------------+
| Door frame rail                                                                 |
|   |                                                                             |
|   +--> [Compression strip] --> [ADS122C04 ADC] -------------------------------+ |
|   +--> [MMC5983MA magnetometer] ---------------------------------------------+ | |
|   +--> [VL53L4CX ToF gap sensor] -------------------------------------------+ | |
|   +--> [MLX90632 thermopile] ----------------------------------------------+ | |
|   +--> [SHT45 ambient T/RH] -----------------------------------------------+ | |
|   +--> [BMI270 IMU] -------------------------------------------------------+ | |
|   +--> [MEMS microphone + analog AFE] ------------------------------------+ | |
|                                                                             v v v
|                                                                         [nRF5340]
|                                                                             |
|                                                  +--------------------------+-----------------------+
|                                                  |                                                  |
|                                             [QSPI flash]                                  [MAX17048 fuel gauge]
|                                                  |                                                  |
|                                                  +--------------------------+-----------------------+
|                                                                             |
|                                             +-------------------------------+----------------------+
|                                             |                                                      |
|                                           [BLE]                                           [Thread/Matter]
|                                             |                                                      |
|                                      Companion app                                   Building / fleet gateway
+--------------------------------------------------------------------------------+
```

## Firmware details and design decisions

The firmware is designed to be useful even when no cloud service is present. SealBeat stores rolling event history locally, computes summary features on the MCU, and only transmits compact reports unless the user explicitly requests a high-detail diagnostic trace. This reduces energy consumption, improves privacy, and keeps the device viable in kitchens, clinics, and field deployments with poor connectivity.

### Event model

Instead of sampling every sensor at full rate all the time, SealBeat uses an event-driven state machine:

- **Idle baseline:** low-rate magnetometer, ambient, battery, and periodic thermopile sampling.
- **Approach detect:** accelerated sampling when door motion, field change, or IMU activity begins.
- **Closure capture:** high-rate capture during the final close arc to measure bounce, compression, and magnetic latch slope.
- **Recovery observe:** short thermal and acoustic window after close to estimate warm-air intrusion and compressor burden.
- **Digest and store:** extract features, update trends, compute scores, and append event summary to flash.

This approach is more efficient than continuous streaming while still preserving the parts of the event that matter most.

### Key firmware subsystems

- **Acoustic driver** – models latch sharpness, compressor onset, and frame vibration character.
- **Door kinematics driver** – tracks door angle, bounce-back, dwell-open duration, and cycle counts.
- **Seal fusion driver** – merges magnetic pull, distance-to-frame, and compression strip data into a unified closure quality model.
- **Thermal driver** – estimates warm-edge rebound, recovery half-time, compartment drift, and frosting tendency.
- **Power manager** – battery state, charger state, and adaptive duty cycle policy.
- **Inference engine** – produces seal integrity, safety confidence, hinge wear, maintenance priority, and recommended action.
- **BLE formatter** – constructs compact JSON-like packets for local apps and commissioning tools.
- **Logger** – retains event summaries and trend snapshots for human-readable service reports.

### Why the sensing stack matters

A single sensor cannot reliably explain refrigeration problems. If only temperature is measured, the system knows *that* things are warm but not *why*. If only a hall switch is used, it knows the door opened but not whether the gasket landed evenly. If only acoustic monitoring is used, it may detect compressor strain but not the edge leak causing it. SealBeat deliberately combines orthogonal signals so false positives can be rejected and maintenance can be specific.

For example, a dirty gasket and an overloaded door bin may both lead to warm recovery, but they produce different closure fingerprints. A dirty gasket may show weaker, delayed compression and a soft magnetic finish along one edge. An overloaded door may produce asymmetric bounce and a higher hinge skew estimate. That distinction matters because the fix is different.

### Edge inference philosophy

SealBeat uses explainable scoring rather than opaque cloud-only AI. The output is meant to be trusted by homeowners and technicians. The firmware therefore preserves intermediate values such as:

- latch slope
- compression uniformity
- rebound count
- warm-edge rise after close
- recovery time constant
- hinge skew estimate
- cycle intensity index

These numbers feed a set of weighted rules and trend deltas. The user can still understand why a score changed.

### Operating profiles

The firmware supports multiple appliance profiles:

- **Residential refrigerator** – moderate traffic, broad temperature tolerance, focus on energy and food quality.
- **Upright freezer** – stronger emphasis on frost risk, long recovery windows, and seal stiffness changes.
- **Pharmacy / vaccine cooler** – tighter temperature excursion thresholds, more aggressive logging, and audit-oriented summaries.

Profiles alter acceptable gap limits, magnetic expectations, recovery timing, and alert severity.

## Application / software interface

The companion app is meant to be practical rather than flashy. It should answer, within a few seconds, whether the appliance is safe, efficient, and mechanically healthy.

Core app views include:

- **Dashboard** – seal score, safety score, battery, cycle count, and current alert level.
- **Seal Map** – top, latch-side, hinge-side, and bottom edge health visualization.
- **Door Cycles** – timing and intensity chart for daily usage patterns.
- **Thermal Recovery** – after-close rebound and recovery curves with safe-hold interpretation.
- **Maintenance Coach** – ranked actions with explanations, not just alarms.
- **Technician Mode** – raw closure packets, calibration values, and profile changes.

The protocol intentionally uses small self-describing packets so the app can remain responsive over BLE. Typical packet fields include:

- device name and profile
- recent seal score
- top/right/bottom/left edge integrity
- warm-edge rebound magnitude
- recovery time in seconds
- hinge skew estimate
- cycle counts and night openings
- battery, firmware version, and last sync time

## Use cases

### 1. Household refrigerator aging gracefully
A family with a 9-year-old refrigerator notices occasional condensation and warmer drinks in the top door shelf. SealBeat reveals that the top-right closure edge is consistently weak and recovery time has increased 18% over two weeks. The app recommends cleaning the gasket and reducing load in the upper door bin. If the trend continues after cleaning, it suggests a hinge service before full gasket replacement.

### 2. Freezer door bounce in a garage
A garage freezer in a warm climate often appears closed but bounces slightly when slammed. SealBeat detects repeated micro-reopen events under one second and correlates them with frost buildup and higher compressor burden. The owner learns the issue is not a failing compressor; it is closure bounce plus overstuffed baskets.

### 3. Pharmacy vaccine cooler compliance
A clinic needs a better explanation for intermittent recovery warnings. SealBeat shows that staff frequently perform clustered short openings during the morning rush, but one cooler also has a degrading lower seal segment. The clinic can separate workflow problems from maintenance problems and document both.

### 4. Small restaurant prep line
A reach-in undercounter refrigerator runs longer during service hours. SealBeat identifies heavy night-cleaning door cycles combined with latch-side compression loss due to a worn gasket. The restaurant can replace the right component at the right time rather than purchasing a new unit.

### 5. Appliance service technician triage
A technician installs SealBeat temporarily for 24 hours and returns with quantitative evidence: closure asymmetry, thermal recovery drift, and hinge sag trend. The service visit becomes shorter and more accurate.

## Target audience

- Homeowners with aging refrigerators or freezers
- Renters who need non-invasive evidence for appliance issues
- Appliance repair and maintenance technicians
- Pharmacies, clinics, and labs managing sensitive cold storage
- Restaurants, cafés, convenience stores, and school kitchens
- Smart-building retrofitting teams and energy auditors
- Insurers evaluating preventable spoilage and equipment loss risk

## Industrial design rationale

SealBeat is intentionally slim and flexible because appliance doors vary widely. A thick box stuck to the front of a fridge would be ugly and easy to remove accidentally. Instead, the main module hides on the inner frame rail or hinge side, while the compression strip disappears under the gasket edge or adheres just adjacent to the landing surface. The device must survive door slams, wipe-downs, humidity swings, and the visual expectations of a kitchen or clinic.

The enclosure uses a dual-material strategy: a rigid electronics spine for structural stability and a softer sensor interface section where repeatable coupling matters. The removable bracket option supports high-value use cases such as pharmacy coolers where adhesives may be undesirable.

## Safety and privacy

SealBeat does not need to monitor room audio continuously. The acoustic subsystem is event-gated and optimized for structural signatures such as latch impact and compressor harmonics. The device can operate in a local-only mode without cloud sync. Cold-chain and maintenance records can be exported intentionally rather than silently uploaded.

## Manufacturing and serviceability

The bill of materials is chosen around parts that are broadly manufacturable in low- to medium-volume runs. The sensor set is intentionally richer than a commodity door alarm, but still well within the practical range for a premium retrofit product. The board can be assembled as a rigid-flex pair or as a main rigid board with a cabled compression strip daughtercard. Battery replacement is optional depending on SKU strategy; the rechargeable variant emphasizes convenience, while the primary-cell version simplifies field deployment.

## Development notes

This repository contains a simulation-oriented but compile-ready embedded firmware architecture, a detailed design-oriented KiCad project stub with real component references and interconnect intent, and a React Native-style companion application prototype. The firmware models closure events, seal asymmetry, thermal rebound, hinge dynamics, and maintenance scoring so that the system behavior can be demonstrated and iterated before moving into hardware bring-up.

## Summary

SealBeat is a new class of retrofit hardware: a **door-seal intelligence layer** for cold appliances. It does not compete with full appliance controllers, and it is more capable than a simple door sensor or temperature logger. By analyzing how the door closes, how the seal behaves, and how the compartment recovers, SealBeat can save food, reduce energy waste, improve service decisions, and strengthen cold-chain confidence. It is original, useful, technically achievable, and especially valuable because it addresses a real failure mode that current consumer and light-commercial monitoring products mostly ignore.
