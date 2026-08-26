# PipeWhisper

**Author: jayis1**  
**Copyright (C) 2026 jayis1. All rights reserved.**

## Overview

PipeWhisper is a new class of household and light-commercial infrastructure device: a **clamp-on plumbing health sentinel** that can characterize water usage, detect hidden micro-leaks, identify destructive pressure transients, forecast freeze risk, and generate a behavioral signature of the plumbing branch it is attached to — **without cutting pipe, touching drinking water, or requiring a licensed plumbing retrofit**. The device clips around a metal or polymer supply line near a sink, washer, utility closet, or manifold and listens to the line using a fused set of contact-acoustic, strain, thermal, and ambient sensors. It creates a living digital twin of the pipe segment and the fixtures downstream.

That combination is what makes PipeWhisper novel. Existing leak sensors typically wait for water to reach the sensor, or they measure whole-home flow inline at the main shutoff. Those are useful, but they miss a large middle ground: the homeowner, property manager, facilities technician, or insurer often wants to know whether a single branch line is beginning to fail, whether a washing machine hose is starting to chatter, whether a faucet cartridge is leaking overnight, or whether freeze conditions are emerging behind a cabinet wall. PipeWhisper fills that gap with a non-invasive, install-in-minutes device that treats the pipe as an information channel.

The core invention is a **sensor fusion architecture for non-invasive plumbing inference**. A piezoelectric clamp ring measures pipe strain and hydraulic impulses. A contact microphone captures structure-borne acoustic signatures that distinguish steady draw, valve chatter, cavitation-like events, and drip patterns. A fast thermistor bonded to the clamp tracks thermal excursions caused by hot and cold water transients, while an ambient humidity and temperature sensor watches for condensation, enclosure stagnation, and freeze conditions. A 6-axis IMU verifies installation stability and helps reject false events caused by cabinet doors, appliance vibration, or accidental bumps. Firmware running on a low-power wireless MCU continuously computes feature windows, classifies event types, and scores risk conditions.

In practice, PipeWhisper does three important things that current consumer hardware rarely combines in one deployable product. First, it provides **branch-level leak intelligence**, not just “water is here now” alerting. Second, it produces **preventive maintenance insight**, such as water hammer severity, fixture signature drift, and abnormal duty cycle behavior. Third, it exposes a clean companion application for homeowners and operators, showing fixture fingerprints, overnight anomaly scores, install quality, battery health, and maintenance recommendations.

PipeWhisper should exist because plumbing failures are frequent, expensive, and often caught too late. Slow drips inside cabinetry, behind appliances, or in underused rooms can persist for weeks before visible damage appears. Freeze bursts are even worse, especially in rental properties and vacation homes. Inline smart shutoff systems are powerful but expensive, invasive, and usually overkill for apartments, condos, older homes, and targeted retrofits. PipeWhisper is designed as the smart smoke alarm for water lines: easy to install, affordable to scale, and smart enough to predict trouble before water hits the floor.

## Device purpose

PipeWhisper is intended to monitor a single branch supply line or appliance hose assembly and answer these practical questions:

1. Is there a likely **micro-leak or persistent drip** downstream?
2. Is this branch experiencing **pressure hammer** or repeated valve chatter that will shorten fixture life?
3. Is the line entering a **freeze-risk envelope** based on temperature slope, room conditions, and overnight activity?
4. Is a connected appliance or fixture behaving differently than usual?
5. Is the installation still mechanically sound, acoustically coupled, and battery healthy?

The device is especially useful for apartment owners and renters who cannot alter plumbing infrastructure, vacation homes and cabins with freeze exposure, laundry closets, vanity cabinets, utility sinks, property managers who need low-touch monitoring on selected high-risk fixtures, insurance pilots, and maintenance teams interested in branch-level risk scoring.

## Why the concept is original

PipeWhisper is original because it is not just another moisture puck, vibration sensor, or whole-home inline meter. The innovation lies in combining **external clamp mechanics**, **contact acoustics**, **pipe strain measurement**, **thermal event tracking**, and **cloud-optional edge inference** to generate a useful plumbing signature without touching the water path. It creates a branch-specific fingerprint for everyday water events and uses deviation from that fingerprint as the basis for alerts.

A faucet partially left open, a toilet fill valve starting to chatter, a washing machine solenoid aging, an icemaker feed line intermittently clicking, or a cabinet-hidden leak each produce different temporal and spectral patterns. PipeWhisper exploits that fact. The firmware uses low-power sampling most of the time and escalates sampling around events. This keeps battery life practical while still capturing rich diagnostic windows.

## Hardware specifications

### Compute and wireless

- **Primary MCU:** Nordic nRF5340, dual-core wireless SoC
- **Radio features:** Bluetooth Low Energy 5.4, Thread/Matter-ready networking
- **Nonvolatile storage:** 16 MB QSPI flash for event buffers, signatures, and OTA staging
- **Security:** secure boot support, signed firmware updates, device-unique provisioning secret

### Sensor suite

- **Contact acoustic sensor:** ICS-40300 analog MEMS microphone mechanically coupled into the clamp body
- **Pipe strain / impulse sensor:** piezo film ring read through ADS122C04 24-bit ADC
- **Surface temperature:** NTC thermistor epoxied to thermally conductive clamp pad
- **Ambient microclimate:** SHT45 temperature/humidity sensor
- **Motion and installation validation:** BMI270 6-axis IMU
- **Battery gauging:** MAX17048 fuel gauge
- **Optional expansion header:** leak rope, cabinet door reed switch, external thermistor

### Power

- **Battery:** 1200 mAh LiPo pouch cell
- **Charging IC:** BQ25185 single-cell power path charger
- **Input power:** USB-C at 5 V
- **Battery life target:** 6 to 9 months typical on adaptive event-driven duty cycle
- **Low-power strategy:** always-on coarse event detector, burst sampling during hydraulic events, adaptive report intervals

### Mechanical and form factor

- **Enclosure:** two-piece clamp housing with silicone acoustic coupler insert
- **Pipe compatibility:** 3/8 in, 1/2 in, and 3/4 in copper, PEX, and CPVC using interchangeable pads
- **Overall size:** 78 mm × 42 mm × 24 mm
- **Ingress approach:** splash-resistant cabinet-safe enclosure with vent labyrinth for ambient sensor
- **Mounting:** hinged latch clamp plus anti-creep silicone band

### Connectivity model

- BLE for provisioning, diagnostics, and local live view
- Thread/Matter bridge mode for whole-home automation integration
- Optional cloud relay through companion app, but local operation is fully supported

## Architecture

PipeWhisper is architected as a layered edge diagnostic platform:

1. **Mechanical coupling layer** – clamp body couples vibration and strain from the pipe into the sensors while isolating airborne noise.
2. **Acquisition layer** – low-noise analog front end, ADC, environmental sensor bus, and IMU gather raw data.
3. **Feature extraction layer** – firmware computes RMS energy, impulse counts, decay time, thermal slope, humidity trend, and install confidence.
4. **Inference layer** – weighted heuristics and signature matching estimate event type, leak confidence, hammer score, freeze risk, and appliance fingerprint drift.
5. **Communication layer** – compact BLE packets and a higher-level companion app protocol expose telemetry and maintenance actions.
6. **User experience layer** – the app turns low-level signatures into practical advice: cold branch drip suspected, washer fill valve chatter increased 3×, freeze risk tonight, and re-seat clamp for stronger acoustic coupling.

### Block diagram

```text
                 +----------------------------------------------+
                 |                 PipeWhisper                  |
                 |              Author: jayis1                  |
                 +----------------------------------------------+
                                  |  Clamp body / coupler
                                  v
  Pipe wall ---> [Piezo strain ring] ----> [ADS122C04 ADC] -----+
            \                                               |
             \--> [Contact acoustic mic + AFE] -------------+----> [nRF5340]
              \                                              |        |
               \-> [Surface thermistor divider] ------------+        |
                \-> [BMI270 IMU] ---------------------------+        |
                 \-> [SHT45 ambient T/RH] ------------------+        |
                                                                  [QSPI Flash]
                                                                      |
                                                    +-----------------+----------------+
                                                    |                                  |
                                                  [BLE]                         [Thread/Matter]
                                                    |                                  |
                                              Companion App                    Home automation / gateway
```

## Firmware details and design decisions

The firmware is edge-centric. It does not depend on cloud inference to be useful. Instead, it keeps a short rolling history, computes features on-device, and only transmits compact summaries plus event records. This reduces radio energy, improves privacy, and supports disconnected homes.

Key subsystems include an acoustic driver, flow signature driver, pressure driver, environment driver, power manager, inference engine, BLE packet formatter, and event logger. The firmware in this repository is simulation-oriented but structured like production embedded code: board definitions, register maps, separate drivers, logging, status packet construction, and deterministic event evolution over time.

### Why the sensing stack matters

A non-invasive device must work harder than an inline flow sensor. It never sees direct volumetric flow. Instead, it infers state from the pipe as a mechanical system. That means the sensing stack was chosen for complementary failure modes. The microphone hears texture and periodicity. The piezo ring senses impulse strength and ringing. The thermistor reveals hot and cold usage and cold-soak behavior. Ambient humidity helps distinguish harmless quiet periods from growing condensation or enclosure stagnation. The IMU validates that the device has not been knocked loose. Together those signals support stronger conclusions than any one signal alone.

### Explainable heuristics over opaque AI

PipeWhisper intentionally prefers explainable scoring to a black-box machine-learning classifier. In residential maintenance, users want answers they can act on. “Leak confidence rose because periodic drips continued while branch draw stayed low and humidity climbed” is actionable. “Model confidence 0.87” is not enough. The simulated firmware therefore uses understandable weighted heuristics. That also makes the project easier to audit, extend, and port to real production hardware.

### Event-driven power model

Continuous high-rate acoustic sampling would waste energy. The production intent is a wake hierarchy: low-cost edge sensing at idle, burst acquisition when the branch becomes active, then compressed reporting. Even in simulation, the code reflects this philosophy by modeling short windows of heightened activity such as morning draws, valve chatter, and overnight drip persistence.

### Install quality as a first-class metric

A clamp-on monitor must know when its coupling is degraded. PipeWhisper keeps install quality visible because bad mechanical contact can masquerade as quiet plumbing. The app therefore includes installation guidance and the firmware continuously scores coupling confidence.

## Application and software interface

The included React Native companion application demonstrates how PipeWhisper would feel in practice. The application is not a single static view. It contains multiple screens and reusable components that present live branch health, fixture fingerprints, anomaly timeline entries, installation guidance, maintenance report output, settings, and device metadata.

### App workflow

1. The user pairs over BLE.
2. The app checks installation quality and suggests re-seating if needed.
3. The dashboard presents leak confidence, freeze risk, branch health, battery, and hammer severity.
4. The fingerprint view helps the user understand whether the branch behavior resembles faucet use, washer solenoid behavior, or a drip-like event.
5. The report screen turns technical findings into maintenance actions.
6. Settings control quiet hours, sensitivity, and local-first automation preferences.

### Interface strategy

The software avoids overloading the user with raw waveforms. Most users do not care about spectra or decay plots. They care about whether something changed, whether it matters, and what they should inspect. The interface therefore emphasizes clear language, confidence indicators, and a report format appropriate for homeowners, technicians, and property managers.

## Use cases and target audience

### Hidden vanity cabinet drip
A bathroom faucet cartridge begins leaking slowly overnight. Water never reaches the cabinet floor because it evaporates or follows a pipe path into the wall. PipeWhisper detects a low-level periodic branch signature recurring at unusual hours, with corresponding temperature and humidity drift. The app raises a medium-confidence persistent downstream drip alert days before visible damage.

### Laundry solenoid valve chatter
A washing machine inlet valve starts aging. Each fill cycle includes higher hammer energy and longer ringing decay than the learned baseline. PipeWhisper flags fixture signature drift and hammer severity elevation, helping the homeowner replace a part before hose damage or nuisance noise worsens.

### Freeze exposure in a vacation home
An exterior wall kitchen line cools quickly during a cold snap. PipeWhisper sees surface temperature dropping toward a freeze threshold, low overnight draws, and rising enclosure humidity. It escalates freeze risk and can trigger a Matter automation to increase local heat or energize a pipe heating cable through a smart plug.

### Insurance or property-management deployment
A property operator deploys PipeWhisper on the highest-risk branch lines across multiple units: washing machine feeds, under-sink hot lines, and remote bathroom lines. Because installation is clamp-on, rollout is fast and tenant disruption minimal. Data remains local-first, while periodic reports identify branches with rising maintenance priority.

### Temporary service instrumentation
A technician clips PipeWhisper onto a branch while diagnosing a complaint. The device captures branch signatures during faucet, icemaker, dishwasher, and washer operation, making it easier to distinguish supply-side issues from fixture-side faults.

## Safety, privacy, and deployment considerations

PipeWhisper is not a shutoff valve and does not claim billing-grade flow measurement. Its purpose is **risk detection and maintenance insight**. That constraint keeps installation simple and liability lower. A future paired actuator could exist, but the branch monitor stands on its own.

From a privacy standpoint, branch signatures reveal occupancy patterns, so the design keeps inference local by default. The device can summarize events rather than streaming raw acoustic data. The app can function without cloud dependence.

## Future roadmap

Potential future revisions include interchangeable clamp modules for radiant heating loops and irrigation zones, an optional e-paper cabinet-door status tag, an external leak rope for floor confirmation, fleet-level anonymized signature learning, and an optional paired smart valve accessory.

## Repository contents

- `firmware/` – compile-ready simulation firmware in C by jayis1
- `kicad/` – KiCad project files for the clamp monitor electronics
- `app/` – React Native companion application prototype by jayis1

## Implementation summary

The implementation in this repository is substantial rather than decorative. The firmware simulates realistic branch events and demonstrates a full internal architecture: drivers, registers, logging, event scoring, and BLE packet formatting. The KiCad files define real components, footprints, and nets appropriate to the concept. The app includes multiple screens, reusable components, and representative protocol data that demonstrate how users would interact with the product.

## Conclusion

PipeWhisper is the kind of hardware product that feels obvious once described: homeowners desperately need earlier plumbing fault detection, but the market mostly forces a choice between simplistic water-on-the-floor alarms and expensive inline systems. A clamp-on, branch-level, explainable plumbing health monitor is a missing category. It is practical, low-friction, and genuinely useful. The design in this repository shows how such a device could be built today using available silicon, disciplined firmware architecture, and a user-facing software model focused on preventive maintenance rather than gadget novelty.

Every part of this design — concept, firmware, app structure, and hardware files — credits **jayis1** as the author and creator.
