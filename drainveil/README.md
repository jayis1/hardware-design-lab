# DrainVeil

**Author: jayis1**  
**Copyright (C) 2026 jayis1. All rights reserved.**

## Device purpose, overview, and why it should exist

DrainVeil is a **retrofit intelligent drain-line health monitor** for sinks, floor drains, condensate lines, branch waste pipes, and grease-interceptor inlets. It is designed to answer a practical question that most buildings cannot answer today: *what is happening inside a drain line before it becomes an emergency?* In homes, restaurants, apartment buildings, clinics, light industrial spaces, and schools, drain problems usually appear only after the failure is already expensive. A line backs up at dinner rush. A mop-room floor drain dries out and starts venting sewer gas. A cold-weather branch line partially freezes, cracks, and leaks inside a wall. A grease-heavy sink starts to gurgle and drain slowly for weeks, but nobody notices because the symptoms are subtle and intermittent. A condensate line looks fine externally yet biofilm growth is already producing odor and clog precursors.

DrainVeil is meant to close that gap. Instead of waiting for standing water, overflow, foul odor, or water damage, the device continuously characterizes the *behavior* of a pipe using non-invasive clamp-on sensing and local intelligence. It watches how fluid height changes, how quickly a slug clears, how the pipe wall vibrates under load, how humidity and gas composition evolve near the trap, how thermal exposure changes around cold-weather runs, and how all of those signals correlate over time. The result is not merely “drain okay” or “drain bad.” DrainVeil produces actionable assessments such as:

- grease buildup likely increasing at branch transition,
- trap oscillation suggests vent imbalance,
- odor generation rising due to biofilm growth,
- partial clog progression is measurable but not yet service-critical,
- freeze risk in an exposed utility chase is approaching rupture conditions,
- line was flushed successfully and risk dropped after maintenance.

That makes the concept original and useful. Existing smart leak sensors mostly tell you about water after it escapes a system. Existing commercial building systems may monitor pumps, lift stations, or sewer flow at large scale, but they do not give affordable, localized, retrofit visibility into a single branch drain or floor drain. Plumbers often diagnose recurring nuisance problems through experience and snapshots in time: camera inspection, customer description, or listening for a gurgle during a service visit. Those methods work, but they are episodic. DrainVeil adds **continuous condition awareness** without cutting into the pipe, introducing flow restriction, or requiring a full building automation deployment.

The device is especially relevant in settings where drainage failures are expensive but not mission-critical enough to justify industrial SCADA. Think restaurant prep sinks, apartment laundry drains, school kitchen floor drains, laboratory condensate lines, older home branch drains, refrigeration defrost drains, and grease-interceptor inlet piping. All of these systems have common pain points: clogs begin slowly, operators rarely know which branch is degrading, intermittent odor is difficult to attribute, and corrective work often happens reactively. DrainVeil makes those systems observable.

The name “DrainVeil” reflects the product goal: lifting the veil on what is usually hidden inside passive waste plumbing. It does not try to replace the plumber or building engineer. Instead, it gives them better timing and better evidence. Its firmware is intentionally local-first, so the unit still provides value in disconnected basements, utility rooms, kitchens, or mechanical spaces without requiring cloud analytics.

## What makes DrainVeil novel

DrainVeil is novel because it combines several sensing modes around a problem domain that is underserved by both consumer IoT and industrial instrumentation. Most products that touch plumbing intelligence focus on one of four narrow tasks:

1. leak detection after water escapes,
2. smart valve shutoff on supply lines,
3. septic or pump monitoring,
4. broad environmental monitoring in basements or mechanical rooms.

DrainVeil targets the **waste-side branch line itself** and builds a behavior model of drainage quality from the outside of the pipe. The novelty is not a single exotic sensor; it is the deliberate fusion of multiple modest sensors into an inference engine specific to drain failure modes.

The core innovation is a **pipe behavior fingerprint** built from:

- clamp-on ultrasonic transit and reflection sensing to estimate fill height and fluid clearing,
- MEMS pressure / vibration coupling to observe trap breathing, hammer, and backpressure signatures,
- gas and volatile sensing to infer sewer-gas emergence, anaerobic buildup, and biofilm activity,
- humidity and condensation context to identify wet stagnant conditions,
- thermal measurements that reveal freeze exposure or cold slug behavior,
- edge inference that distinguishes slow-clog growth from vent imbalance or freeze onset.

This allows workflows that currently require repeated manual inspection or experience-based guesswork:

- predicting a slow clog before overflow,
- quantifying whether a floor drain trap is evaporating or actively venting,
- separating grease accumulation from a venting problem,
- confirming whether maintenance actually improved hydraulic behavior,
- identifying which branch in a multi-sink prep area is becoming the chronic offender,
- recognizing cold-weather failure risk before freeze damage occurs.

## Hardware specifications

### Compute and wireless

- **Primary MCU:** Nordic nRF5340 dual-core wireless SoC
- **Companion secure element:** Microchip ATECC608B for device identity, signed provisioning, and local access tokens
- **Wireless:** Bluetooth Low Energy 5.4 for setup, diagnostics, event sync, and maintenance workflows
- **Facility uplink option:** Thread / Matter-ready mode for building or restaurant fleet dashboards
- **External storage:** 16 MB QSPI flash for event windows, maintenance history, trend compression, and OTA staging
- **Local processing:** event-feature extraction and risk scoring on-device; cloud is optional, not required

### Sensor suite

- **Clamp-on ultrasonic pair:** piezoelectric transducers with analog front-end for fluid presence, reflection strength, and drain-time estimation
- **High-resolution pressure / vibration sensor:** IIS3DWB or equivalent vibration MEMS bonded through compliant clamp interface to read pipe wall energy and branch pulsation
- **Differential humidity / temperature:** Sensirion SHT45 in vented cavity plus pipe-coupled thermal pad sensing
- **Gas sensing front-end:** low-power metal-oxide VOC sensor plus electrochemical H2S channel for sewer-gas and anaerobic activity trends
- **IR thermopile or pipe-contact RTD:** for pipe surface temperature and freeze exposure characterization
- **Battery gauge:** MAX17048
- **Power-path charger:** BQ25185 over USB-C
- **Optional energy harvester:** small indoor photovoltaic strip or thermoelectric scavenger for bright utility rooms / boiler chases

### Power system

- **Battery:** 2200 mAh LiFePO4 or 2000 mAh LiPo depending safety / enclosure choice
- **Input:** USB-C service power for charging, maintenance capture, and bench diagnostics
- **Battery life target:** 9–14 months in residential mode, 6–9 months in high-traffic commercial mode
- **Low-power strategy:** low-rate baseline sensing with burst acquisition during drain events, maintenance scans, or anomaly windows
- **Protection:** reverse polarity, charger thermal foldback, brownout logging, and battery aging estimation

### Mechanical and form factor

- **Enclosure:** two-piece clamp-on shell with flexible coupler pad to conform to PVC, ABS, cast iron, or copper drain runs
- **Nominal body size:** 92 mm × 38 mm × 24 mm
- **Pipe compatibility:** 1.25 in to 4 in OD with interchangeable strap and spacer kits
- **Ingress goal:** splash-resistant utility-space enclosure with vented chemistry chamber protected by replaceable membrane
- **Mounting:** quick band clamp, adhesive saddle, or magnetic maintenance bracket
- **Serviceability:** replaceable gas-sensor cartridge and accessible USB-C port under sealed flap

### Connectivity model

- BLE for commissioning, local dashboards, trend download, firmware updates, and technician diagnostics
- Thread / Matter bridge mode for facility-level monitoring where desired
- Local-first data retention with optional CSV / PDF export from companion app
- No cloud dependency for primary anomaly detection

## System architecture

DrainVeil is architected as a layered edge-diagnostic system.

1. **Mechanical interface layer** – the clamp assembly acoustically and mechanically couples the sensors to the outside of the drain line without piercing the pipe.
2. **Acquisition layer** – ultrasonic, vibration, gas, humidity, thermal, and power telemetry are sampled according to an event-aware schedule.
3. **Feature extraction layer** – firmware converts raw signals into fill-height estimates, drain-time constants, pulse variance, trap oscillation, gas-rise metrics, corrosion proxy, and freeze margin.
4. **Inference layer** – profile-specific weighted heuristics estimate clog risk, odor risk, freeze risk, service urgency, and data confidence.
5. **Communication layer** – compact BLE packets expose recent status, event summaries, and technician-ready logs.
6. **Application layer** – the mobile app presents drain health maps, maintenance actions, and site-wide comparison views.

### Block diagram

```text
+--------------------------------------------------------------------------------------+
|                                      DrainVeil                                       |
|                                   Author: jayis1                                     |
+--------------------------------------------------------------------------------------+
| Clamp-on enclosure                                                                    |
|   |                                                                                  |
|   +--> [Ultrasonic Tx/Rx pair] --> [AFE / ADC] -------------------------------+     |
|   +--> [Vibration / pressure MEMS] ------------------------------------------+ |     |
|   +--> [VOC sensor] --------------------------------------------------------+ | |     |
|   +--> [H2S electrochemical channel] --------------------------------------+ | | |     |
|   +--> [SHT45 ambient humidity / temperature] ----------------------------+ | | | |     |
|   +--> [Pipe RTD / thermopile] ------------------------------------------+ | | | | |     |
|                                                                            v v v v v     |
|                                                                        [nRF5340 MCU]     |
|                                                                            |             |
|                                                    +-----------------------+----------+  |
|                                                    |                                  |  |
|                                              [QSPI flash]                    [ATECC608B] |
|                                                    |                                  |  |
|                                                    +-----------------------+----------+  |
|                                                                            |             |
|                                                 +--------------------------+---------+   |
|                                                 |                                    |   |
|                                               [BLE]                         [Thread/Matter]|
|                                                 |                                    |   |
|                                         Companion app                    Facility gateway |
|                                                                            |             |
|                                              [BQ25185 charger] -- [Battery / energy harvester] |
+--------------------------------------------------------------------------------------+
```

## Firmware design and design decisions

DrainVeil firmware is intentionally designed as a **deployable simulation framework and architecture reference**, not a toy stub. The included C firmware models a real embedded application structure: board definitions, register map, multiple drivers, event logging, BLE packet generation, and a central inference loop. The code is portable C so it can be compiled on a host machine today while remaining easy to port onto an actual nRF5340-based BSP later.

### Event-driven sensing model

Drain events are intermittent, so always-on high-rate sampling would waste power. DrainVeil therefore uses several operating states:

- **Idle baseline:** low-rate humidity, gas, thermal, and health-beacon sampling.
- **Flow onset:** ultrasonic and vibration channels rise when water begins moving.
- **Active capture:** a short burst window records fill-height, turbulence, pulse variance, and slug-clearing behavior.
- **Recovery observation:** after flow stops, the system watches how fast the line clears and whether gas / humidity rebound suggests stagnation.
- **Anomaly extension:** if thresholds are exceeded, the burst window is prolonged and additional history is stored.

### Why these features were chosen

The firmware focuses on the kinds of features a plumber or facility operator can act on:

- **Fill height percent** approximates how much of the pipe cross-section remains occupied during / after a drain event.
- **Drain time** captures slow-clear behavior in a direct, intuitive way.
- **Blockage gradient** estimates whether resistance is increasing in a meaningful trend direction.
- **Trap oscillation** provides evidence of venting issues or branch pressure instability.
- **H2S / VOC growth** supports odor and hygiene diagnosis rather than leaving smell complaints subjective.
- **Freeze margin** identifies exposed lines headed toward rupture conditions.

### Local inference philosophy

DrainVeil does not need a giant ML model to be useful. Real deployments benefit from transparent edge heuristics first. The included firmware therefore uses interpretable weighted scoring. That has several advantages:

- easier validation during field trials,
- easier threshold tuning per installation class,
- lower computational cost,
- technician trust because the rationale can be surfaced clearly.

Later hardware revisions could add TinyML classification for richer pattern recognition, but the baseline product should already be explainable and dependable.

### Reliability decisions

- data is summarized on-device rather than constantly streaming raw traces,
- event logs are ring-buffered in local flash,
- the risk engine exposes a confidence metric, not just a score,
- power rail noise is included in confidence to avoid over-trusting dirty captures,
- BLE packets are small and inspection-friendly,
- maintenance recommendations are generated from the same core metrics used for alerting.

## Application and software interface

The companion application is implemented as a React Native-style JavaScript project with multiple real screens and shared utility code. It is structured around technician and operator workflows instead of generic smart-device controls.

### Primary app screens

- **Dashboard** – overall site health, current alert, top risk factors, and battery / connectivity summary.
- **Drain Map** – visual comparison of monitored branches, sinks, floor drains, or interceptor nodes.
- **Event Timeline** – history of clog growth, odor spikes, freeze windows, and maintenance markers.
- **Chemistry / Odor Screen** – H2S, VOC, humidity, and biofilm trend interpretation.
- **Maintenance Screen** – ranked actions with expected effect and parts / tools notes.
- **Device Screen** – firmware, signal confidence, battery status, and provisioning information.
- **Setup Screen** – installation profile, pipe material, diameter, expected traffic, and sync preferences.

### App interaction model

The app treats each DrainVeil unit as both a local monitor and a member of a site fleet. A restaurant manager might compare prep sink A versus dishwasher branch B. A property manager might monitor several recurring trouble spots across a building. A plumber can attach to a device at a service visit, review trend evidence, perform a cleaning, and then confirm whether the hydraulic signature improved afterward.

### Data exchange

The included BLE protocol helpers demonstrate how the firmware can emit:

- compact real-time status packets,
- richer multiline report payloads,
- maintenance summaries,
- event history references.

That same interface can later be wrapped into a GATT profile, a Matter bridge characteristic map, or a technician USB diagnostic CLI.

## Use cases and target audience

### Restaurants and food service
Slow kitchen-drain failure creates downtime, sanitation problems, and rush-hour emergencies. DrainVeil helps kitchen managers detect grease accumulation and schedule service before the sink backs up mid-shift.

### Apartment and property maintenance
Drain complaints are often intermittent and resident descriptions are imprecise. DrainVeil gives property teams trend evidence for which branch is repeatedly degrading and whether the issue is clog, venting, or cold exposure.

### Schools, clinics, and facilities
Floor drains, mop sinks, and condensate lines are often ignored until odor or overflow appears. DrainVeil can alert to trap dry-out, sewer-gas emergence, or freeze risk in low-attention areas.

### Residential DIY and premium smart-home users
Homeowners already buy leak detectors and water shutoff systems. DrainVeil extends plumbing awareness to the waste side, where slow symptoms are common and damage is often hidden.

### Plumbing service companies
A technician can use DrainVeil as a pre-failure evidence tool, a temporary diagnostic clamp, or a permanent service add-on contract device.

## Example deployment scenarios

1. **Commercial prep sink:** grease proxy and drain time rise over three weeks; app recommends scheduled cleaning before Friday dinner service.
2. **Basement floor drain:** humidity is high, trap oscillation indicates vent interaction, and H2S begins appearing after dry weekends; app suggests trap-primer correction.
3. **Utility chase branch line in winter:** freeze margin collapses overnight while cold slug index increases; system escalates to critical freeze-risk alert.
4. **HVAC condensate line:** biofilm and VOC trend upward while flow bursts shrink; maintenance team clears line before ceiling leak develops.

## Electrical and PCB notes

The KiCad folder includes a real project skeleton with a schematic, board file, and project metadata. The intent is to provide a credible architectural starting point:

- nRF5340 as the central controller,
- QSPI flash and secure element near the MCU,
- analog front-end grouped with ultrasonic and gas sensing,
- charger, battery gauge, and USB-C service power on the edge of the board,
- sensor labels and named nets reflecting practical signal routing,
- a compact layered board suitable for clamp-on enclosure integration.

The PCB concept favors a narrow central board with connector or flex breakouts to acoustically-coupled sensor pads where mechanical isolation matters. In a production design, special attention would be given to analog guarding, gas-sensor airflow path, and vibration isolation so that the enclosure transfers the desired pipe signature without amplifying external noise.

## Future roadmap

DrainVeil can evolve into a family, not just one SKU.

- **DrainVeil Mini:** low-cost residential sink and condensate monitor.
- **DrainVeil Pro:** replaceable gas cartridge, Thread gateway features, and service analytics.
- **DrainVeil Fleet:** multi-node facility dashboard with ticket integration.
- **DrainVeil Temporary Clamp:** short-term diagnostic rental for plumbers and remediation firms.

Additional future features could include camera-inspection correlation, predictive maintenance scoring calibrated by service outcomes, and optional acoustic anomaly upload for centralized model training.

## Summary

DrainVeil is a practical but genuinely novel device concept: an intelligent, clamp-on, waste-side plumbing monitor that detects drain health issues before they become emergencies. It fills a clear gap between simple leak sensors and expensive industrial monitoring by making hidden branch drain behavior measurable, explainable, and actionable. The included design package provides a detailed concept, embedded firmware architecture, KiCad project skeleton, and companion app structure that together show how DrainVeil could move from idea to prototype.

Every part of this design is authored and credited to **jayis1**.
