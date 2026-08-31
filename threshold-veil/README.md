# Threshold Veil

**Author:** jayis1  
**Copyright:** (C) 2026 jayis1. All rights reserved.

## Purpose and overview

Threshold Veil is a new class of apartment and small-building safety device: a smart, retrofit door-perimeter guardian that watches the air, pressure, and sound conditions on both sides of an entry door, then decides when to tighten the seal, warn occupants, or prepare the door edge as a temporary refuge barrier. It is designed for people who live in dense urban buildings where the front door is not just a security boundary, but also a leak point for smoke, hallway odors, wildfire particulates, neighbor noise, cold drafts, and dangerous pressure-driven airflow during emergencies.

Most homes have smoke alarms, weather stripping, and perhaps a draft guard. Those are important, but they work in isolation. A smoke alarm only reacts after contaminants are already inside. A draft stopper has no idea whether the corridor is filling with cooking smoke, e-bike battery fumes, renovation dust, or pressurized fire-smoke migration. Conventional smart-home devices usually measure indoor air quality in a room center, not the leakage behavior at the doorway itself. Threshold Veil focuses on the exact failure point where unwanted air, sound, and hazard cues enter the living space.

The device mounts as a slim, modular system around the inside perimeter of a door. A sensor bar sits on the jamb, a low-profile threshold pod sits near the sweep, and a small edge actuator module contains a microblower, inflatable gasket manifold, and latching seal control. Instead of simply telling the user that PM2.5 is elevated or noise is loud, Threshold Veil interprets what is happening at the boundary. It watches the pressure gradient, identifies whether the hallway is pushing air inward, compares volatile signatures and particulate trends, estimates leakage severity, and decides whether to stiffen the door seal, trigger a shelter mode, or recommend opening a window because the apartment itself is over-pressurized.

That makes it useful in situations that modern apartments routinely struggle with: wildfire smoke entering through corridors, hallway trash-room odors contaminating the unit, late-night door-edge sound leakage, stairwell pressure surges during fire events, cold-weather infiltration in older buildings, elevator shaft pressure pulses, and accidental indoor pollution migration from shared hallways. It is also valuable for families with asthma, people who work nights and need acoustic quieting, renters who cannot permanently modify the door, and building managers who need a retrofitable health-and-comfort solution.

Threshold Veil is original because it treats the door perimeter as an actively managed environmental membrane rather than passive hardware. It is not a doorbell, not a lock, not a room air purifier, and not only weather stripping. It is a boundary intelligence device that fuses dual-zone sensing with adaptive sealing and user-facing guidance.

## What makes the concept novel

The novelty is not any one sensor by itself. The new idea is **doorway-state inference**. Threshold Veil builds a model of what the threshold is doing in real time. Is hallway air currently intruding? Is the pressure pulse transient or sustained? Is the contaminant signature consistent with cooking aerosols, smoke, solvent fumes, or routine occupancy? Is the problem mainly acoustic leakage, particulate infiltration, or convective draft? Existing products do not answer those questions at the door edge.

The second novel element is the use of an **inflatable micro-gasket and variable exhaust path** as an active response. Traditional seals are fixed compromises. Tight seals reduce leaks but increase door-closing effort, can drag, and are not ideal in every condition. Threshold Veil keeps the system normally comfortable and low-drag, then inflates a soft perimeter bladder and biases a micro-louver when the model predicts that the corridor is pushing contaminants inward. In a quiet-hours mode it can prioritize acoustic attenuation; in shelter mode it prioritizes inward leak resistance. In purge mode it temporarily relaxes the seal after the user opens the door so the system can sample, evaluate, and re-seat intelligently.

The third novel element is **dual-side interpretation**. Threshold Veil uses an indoor sensor path and a near-gap corridor-facing sample path. That means it can reason about gradients rather than absolute values. A VOC spike indoors after cooking means one thing. A VOC spike at the corridor side combined with inward pressure and low apartment emissions means something else entirely. The device becomes more useful because it understands directionality.

Finally, Threshold Veil speaks in operational decisions instead of generic telemetry. It reports events such as "hallway smoke pushing inward," "odor event but low health risk," "pressure surge from stairwell," "quiet mode effective," or "seal wear detected." This turns raw measurements into action.

## Hardware specifications

### Compute and control

- **Primary MCU:** Espressif ESP32-S3-WROOM-1-N8R8
- Dual-core Xtensa LX7 MCU with integrated Wi-Fi and Bluetooth Low Energy
- 8 MB PSRAM and 8 MB flash on module for local event history, OTA staging, and richer inference logic
- Native USB for provisioning, diagnostics, factory calibration, and data export
- Secondary ultra-low-power coprocessor behavior implemented through RTC domain sampling strategy

### Sensors

- **BME688** environmental gas sensor for temperature, humidity, pressure, and VOC trend on the indoor side
- **SEN55** particulate and air-quality module on the corridor sample path for PM1/PM2.5/PM4/PM10 and humidity/temperature cross-checking
- **DPS368** precision barometric sensor for micro-pressure gradient and door-pulse detection
- **ICS-43434 digital MEMS microphone** for acoustic leakage spectrum estimation and knock/noise classification
- **Hall sensors** on frame modules to detect door closed state, latch alignment, and seal engagement consistency
- **Current and pressure feedback** from the microblower and gasket manifold to estimate seal health and leaks
- Optional **NTC thermistors** near threshold and top jamb for condensation risk mapping in winter conditions

### Actuation and output

- **Microblower** for corridor-side sample draw and gasket inflation
- **Normally-relaxed silicone perimeter micro-gasket** with segmented inflatable chambers
- **Miniature louver shutter** to switch between sample, sealed, and equalize states
- **Status LED strip** facing inward for pairing, hazard, and quiet mode indicators
- **Piezo sounder** for severe hazard alerts or seal maintenance notices
- Capacitive touch pad on the jamb module for quick shelter-mode enable/disable

### Connectivity

- Bluetooth Low Energy for direct phone onboarding, local control, and offline apartment use
- Wi-Fi for event sync, firmware updates, multi-user notifications, and optional building dashboard integration
- USB-C service port for charging and wired provisioning

### Power

- 2-cell flat Li-ion pack or 1-cell high-capacity pouch variant depending module length
- BQ25895 charge controller with USB-C sink support
- Fuel gauge via MAX17055 or equivalent
- Normal operation from battery with optional trickle charging dock in frame adapter
- Target endurance: 10–14 days with routine sampling, several months in watch-only low-duty mode if actuators are rarely used

### Mechanical and form factor

- Jamb module: approximately 320 mm x 28 mm x 12 mm
- Threshold module: approximately 820 mm x 24 mm x 11 mm, field-cuttable end caps for common residential widths
- Removable adhesive and screw-assisted bracket system for renter or owner installation
- Soft antimicrobial silicone inflation tube integrated into replaceable seal strip
- Splash-resistant serviceable electronics compartment isolated from airflow path

## Architecture

Threshold Veil is organized into six cooperating subsystems:

1. **Boundary sensing** – measures indoor air, corridor sample air, particulate load, pressure pulses, and acoustic leakage.
2. **Door-state tracking** – determines open, closed, latched, recently opened, and misaligned conditions.
3. **Seal actuation** – controls the microblower, segmented gasket chambers, and louver positions.
4. **Inference engine** – classifies events and assigns seal strategies.
5. **Connectivity and logging** – stores history, exposes BLE and Wi-Fi messages, and synchronizes app state.
6. **Companion app** – presents conditions as apartment-relevant actions instead of engineering graphs alone.

### Block diagram

```text
+------------------------------------------------------------------+
|                         Threshold Veil                            |
|                         Author: jayis1                            |
+------------------------------------------------------------------+
|                                                                  |
|  Corridor sample inlet --> SEN55 --> louver --> microblower ----+ |
|                                                                  | |
|  Indoor side: BME688 -------------------------------+            | |
|                                                     |            | |
|  DPS368 pressure sensor ----------------------------+----> ESP32-S3|<-- BLE/Wi-Fi --> Mobile App
|                                                     |            | |
|  MEMS microphone -----------------------------------+            | |
|                                                     |            | |
|  Hall sensors / latch sense ------------------------+            | |
|                                                                  | |
|  Inference engine --> seal policy --> inflatable gasket driver --+ |
|                    --> alerts/logging --> LED/piezo               |
+------------------------------------------------------------------+
```

## Detailed operating concept

Threshold Veil cycles through several operating states. In **watch mode**, the device samples indoor environmental conditions, pressure gradient, and near-gap acoustic leakage using very low power. In **sample mode**, the louver opens briefly and the microblower pulls a controlled quantity of corridor-side air past the particulate and gas path so the device can compare outside-the-door conditions to the indoor baseline. In **seal mode**, the louver closes, selected gasket chambers inflate, and the device shifts to high-confidence leak resistance. In **quiet mode**, the seal is biased toward acoustic damping and reduced resonant transmission rather than maximum airflow restriction. In **shelter mode**, Threshold Veil assumes an external hazard and tries to minimize inward leak paths while generating more frequent alerts.

This operating model matters because apartment doors experience competing requirements. A user does not always want the door maximally compressed; that creates wear, friction, and battery drain. By changing the seal geometry only when needed, Threshold Veil can be both responsive and practical.

## Firmware details and design decisions

The firmware included in this device folder is a compile-ready C simulation written for host GCC while following embedded-friendly structure. It contains `main.c`, `board.h`, `registers.h`, a `Makefile`, and dedicated drivers under `firmware/drivers/`. The code is not placeholder text: it implements a working model of sensor fusion, threshold inference, actuator policy, message framing, rolling history, and an event-driven operating loop.

### Firmware modules

- `environment.c` models the indoor and corridor air paths, including particulate burden, VOC index, humidity, and pressure trends.
- `acoustic.c` estimates leak-related noise energy in low, mid, and high bands and identifies events such as hallway conversation, rolling carts, and impact knocks.
- `seal.c` manages blower demand, louver state, gasket inflation target, seal wear score, and equalization timing.
- `power.c` tracks battery voltage, charge estimate, thermal derating, and mode-based power budgeting.
- `comms.c` generates concise telemetry frames suitable for BLE notifications or Wi-Fi payloads.
- `inference.c` turns raw data into user-facing states like `CALM`, `ODOR_PUSH`, `SMOKE_PUSH`, `QUIET_HOURS`, `PRESSURE_SURGE`, and `SHELTER`.
- `logger.c` keeps a ring buffer of events and metrics for both debugging and app history.

### Key firmware decisions

**1. Gradient-first inference**  
The code emphasizes indoor-vs-threshold differences instead of stand-alone numbers. This makes the system more robust across buildings with different absolute baselines.

**2. Interpretable scoring**  
Rather than burying every decision in an opaque machine-learning model, the firmware uses weighted evidence terms for smoke, odor intrusion, draft discomfort, and noise leakage. That keeps tuning inspectable for an embedded bring-up phase.

**3. Duty-cycled actuation**  
The blower and gasket are expensive in power and mechanical wear. The firmware inflates only when the risk score justifies it and otherwise relaxes the chambers to a maintenance pressure.

**4. Seal-health estimation**  
Because retrofit door geometry drifts over time, the firmware compares commanded inflation to achieved manifold pressure and estimates whether the gasket or alignment needs service.

**5. Event framing for the app**  
Instead of streaming only raw registers, the firmware publishes short semantic events so the mobile app can work well offline and on weak links.

### Real-time behavior

A typical cycle begins with door-state confirmation from the Hall sensors. If the door is closed, the system samples indoor temperature, humidity, and VOC baseline, then reads the pressure gradient. If the gradient suggests inward flow, the device runs a quick corridor sample. The inference engine computes smoke risk, odor intrusion score, draft severity, and acoustic leakage severity. The seal controller chooses a policy: remain relaxed, partly inflate top and latch-side chambers, fully inflate the perimeter, or equalize because the apartment itself is the source. The event logger stores the outcome. The communications layer then emits a compact frame that the app can render immediately.

## Electrical design summary

The electronics are split across a jamb logic board and a threshold actuator board connected through a keyed flex cable. The jamb board hosts the ESP32-S3, BME688, DPS368, microphone front end, Hall sensor conditioning, battery management, and LED driver. The threshold board hosts the SEN55, blower driver, manifold pressure sensing, louver actuator stage, and gasket valve transistor network.

The split-board design improves serviceability. If the replaceable threshold strip is damaged by footwear or cleaning, the more expensive logic section does not need replacement. It also shortens acoustic and Hall sensor routing around the latch area where signal integrity and mechanical stability matter most.

## KiCad design files

The `kicad/` directory includes a KiCad project, schematic, and PCB draft that represent the core topology:

- ESP32-S3 as the controller
- BME688, DPS368, and SEN55 sensor connections
- MEMS microphone digital interface
- Battery charging and regulation chain
- Motor driver path for the blower and louver
- GPIO/Hall/touch interface network
- Net names that reflect the intended embedded design

The PCB is a conceptual placement and connectivity draft rather than a production-routed board, but it contains real components, nets, footprints, and board outline information suitable for further refinement.

## Companion application

The `app/` directory contains a React Native companion application. It is structured around practical apartment workflows, not just dashboards.

### App screens

- **Dashboard** – current apartment protection state, ingress score, air quality delta, and battery level.
- **Seal Modes** – lets the user select Auto, Quiet, Shelter, or Open-Flow behavior.
- **Event History** – timeline of odor pushes, smoke pushes, pressure surges, and maintenance events.
- **Noise Map** – shows estimated leakage by time of day and suggests quiet-hours settings.
- **Health Check** – gasket wear estimate, blower runtime, battery status, and cleaning reminders.
- **Setup** – onboarding, calibration, Wi-Fi, and renter-friendly installation checks.

### Software interface

The app consumes semantic payloads from the firmware using a compact protocol object defined in `app/utils/protocol.js`. Each event includes state, confidence, gradient metrics, seal recommendation, and timestamp. This makes it possible to operate in BLE-only mode when the user does not want cloud dependence.

## Use cases

### 1. Wildfire smoke in apartment corridors
A user in a western U.S. apartment building notices a faint odor in the hallway during a smoke event. Threshold Veil sees elevated particulate density on the corridor path, a sustained inward pressure gradient, and a growing indoor delta. It inflates the perimeter gasket, closes the louver, and tells the user to keep the door closed and enable shelter mode.

### 2. Neighbor cooking odors at night
The hallway fills with strong food odors after midnight. The system detects high VOC change but low particulate hazard. Instead of alarming like a smoke detector, it labels the event as odor intrusion, inflates only the latch side and threshold chambers, and reports a comfort-protection action instead of a hazard event.

### 3. Drafty older building in winter
The user feels cold air near the door every morning. Threshold Veil logs recurring pressure and temperature drops near the threshold module and recommends a persistent winter quiet-seal profile, reducing both drafts and hallway noise.

### 4. Stairwell pressure surge during fire-door events
A sudden pressure pulse from the building HVAC or stairwell causes repeated inward puffs under the door. The system recognizes the event shape and seals preemptively before contaminants accumulate.

### 5. Renters with asthma
Because the device is removable and uses adhesive-backed modular mounts, renters can gain doorway protection without replacing the entire door or frame.

## Target audience

- Apartment dwellers in dense buildings
- Families managing asthma or smoke sensitivity
- Renters who need non-destructive retrofit hardware
- Condo owners seeking better comfort and odor isolation
- Small building managers testing health-oriented upgrades
- Night-shift workers who need quieter sleep environments

## Manufacturing and service considerations

Threshold Veil is intentionally designed around replaceable soft parts. The inflatable gasket strip is the most wear-prone item and should be sold as a maintenance consumable. The threshold pod should include a removable lint filter for the sample path. Calibration should occur in two phases: factory baseline for gas and pressure sensors, then in-home zeroing for the installed door geometry. The app’s setup workflow walks the user through a closed-door calibration, open-door sample, latch alignment confirmation, and quiet-hours preference capture.

## Safety and compliance notes

The device does not replace building fire safety systems, smoke alarms, or egress requirements. The seal force is deliberately limited so it does not meaningfully obstruct door opening from inside. Shelter mode is an exposure-reduction aid, not a life-safety guarantee. Production versions would require validation for battery safety, flammability of seal materials, actuator heating, and fail-safe behavior under low battery.

## Why this device should exist

Urban housing keeps getting denser, but doorway technology has barely evolved beyond passive strips and lock hardware. Indoor air quality devices are becoming common, yet they mostly ignore where contamination actually enters. Threshold Veil fills that gap by making the apartment threshold measurable, interpretable, and controllable. It gives people a way to understand the invisible flow at their front door and respond before discomfort becomes exposure.

That combination of retrofit practicality, dual-zone sensing, active sealing, and apartment-focused decision support is why Threshold Veil is both new and worth building.
