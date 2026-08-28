# VentLattice

**Author: jayis1**  
**Copyright (C) 2026 jayis1. All rights reserved.**

## Overview

VentLattice is a **room-by-room HVAC register intelligence module** that snaps beneath a standard supply vent grille and turns an ordinary air register into a comfort, efficiency, and maintenance sensor. The device is designed for homes, apartments, classrooms, clinics, and small commercial spaces where the HVAC system may technically be functioning, yet the lived experience is poor: one room is stuffy, another is too cold, airflow slowly degrades as the filter loads, a partially closed damper starves a bedroom, a frozen evaporator coil causes abnormal supply temperature swings, or a tenant blocks the register with a rug or furniture and nobody notices except through wasted energy and discomfort.

What makes VentLattice original is **where it lives and what it measures together**. Existing smart thermostats reason about an entire zone. Existing room sensors usually measure only temperature, sometimes humidity, and occasionally occupancy. Air balancing tools exist for technicians, but they are handheld and used only during service visits. VentLattice creates a missing category: a persistent, low-profile, vent-level embedded node that can understand whether conditioned air is actually reaching a room, whether the room is benefiting from it, whether the register path is restricted, and whether the HVAC system upstream is showing signs of maintenance drift.

The device sits at the endpoint where occupants experience HVAC performance directly. That location allows it to fuse several useful signals: a thermal anemometer estimates airflow velocity, a differential pressure sensor measures vent pressure and dynamic pulses, a temperature and humidity sensor measures delivered air conditions and condensation risk, a VOC and CO2-equivalent sensor tracks perceived air freshness trends, and a low-power presence detector estimates whether the room is occupied when conditioned air is delivered. With those signals, VentLattice can answer practical questions that most building systems cannot answer cheaply today:

- Is this room receiving the airflow it is supposed to receive?
- Has airflow fallen because the filter is dirty, the duct is kinked, or the branch damper has shifted?
- Is cooled air reaching an empty room while occupied rooms are underserved?
- Is warm supply air short-cycling or overshooting comfort at the register?
- Is there a condensation or mold risk near a register in humid climates?
- Is the room stale because it is occupied but under-ventilated?

VentLattice should exist because buildings waste extraordinary amounts of energy through poor distribution rather than poor equipment alone. A heat pump, furnace, or rooftop unit can be perfectly healthy while comfort complaints persist because delivery to individual rooms is inconsistent. Property owners then respond by setting the thermostat more aggressively, which increases energy use without solving the root issue. Occupants close doors, block vents, add space heaters, or open windows, all of which further destabilize the system. By measuring distribution quality at the register, VentLattice provides a path to actionable, room-specific corrections.

## Device purpose

VentLattice is built to be the **last-foot delivery monitor** for forced-air HVAC systems. It is not a replacement thermostat and not a professional commissioning balometer. Instead, it is a deployable embedded device that stays in place and continuously learns how each room behaves. It identifies airflow health, comfort efficiency, occupancy alignment, and maintenance anomalies from a vantage point that conventional controls do not observe.

The device is intended to support:

1. **Room-level balancing** — find which rooms are over-served, under-served, or intermittently starved.
2. **Predictive maintenance** — identify probable filter loading, damper drift, coil icing signatures, or blocked registers.
3. **Comfort optimization** — align delivered air with occupancy and room freshness.
4. **Indoor air quality awareness** — detect stale air buildup, VOC spikes, or overnight stagnation.
5. **Automation handoff** — provide clean data to thermostat APIs, smart vents, and building dashboards.

Unlike generic room sensors, VentLattice understands the relationship between room state and delivered air state. If occupancy rises and CO2-equivalent trends up while airflow remains low during an active cooling call, that is not just “warm room” data — it is evidence of distribution failure. If supply air is present but temperature delta collapses, the device can suggest upstream equipment investigation. If airflow and pressure fall progressively over weeks across many VentLattice nodes, the system may be revealing filter loading or blower degradation before occupants complain.

## Why the concept is original

VentLattice is original because it is a **magnetic, under-grille, vent-native sensing platform** rather than a wall thermostat, duct insert actuator, or standalone room puck. The novelty comes from persistent register instrumentation with multimodal inference. Most current hardware falls into one of four categories:

- thermostats controlling whole zones,
- simple room sensors reporting temperature,
- professional commissioning tools used temporarily,
- and powered smart vents that actively modulate airflow.

VentLattice occupies a different role. It does not require duct cutting. It does not need to replace the vent grille. It does not need to actuate dampers to be useful, though it can coordinate with systems that do. Its value comes from **observability**: by watching pressure, velocity, delivered temperature, local humidity, freshness, and occupancy together, it can distinguish between “the system is running” and “the room is actually being served well.”

This is particularly useful in retrofit buildings, rentals, schools, and clinics where full HVAC redesign is unrealistic. A facilities team could deploy several VentLattice nodes in problem rooms, learn the system’s real distribution behavior over a month, and tune registers, dampers, schedules, or maintenance priorities accordingly. Homeowners could finally understand why a nursery is cold at night, why a home office feels stale by afternoon, or why an upstairs bedroom needs constant manual adjustment.

## Hardware specifications

### Compute and wireless

- **Primary MCU:** Nordic nRF52840 wireless MCU
- **Connectivity:** Bluetooth Low Energy 5.3 for onboarding and diagnostics; Thread-ready radio for mesh reporting in multi-room deployments
- **Storage:** 8 MB QSPI flash for learned airflow baselines, event history, OTA images, and room comfort models
- **Security:** signed firmware updates, per-device commissioning secret, local encryption for retained occupancy history

### Sensor suite

- **Airflow sensor:** Sensirion SDP810 differential pressure sensor across a vent nozzle geometry for airflow estimation
- **Thermal anemometry element:** dual NTC bridge used to refine low-flow estimates and identify stagnant but thermally active air
- **Supply / room climate:** Bosch BME688 for temperature, humidity, and VOC-derived air quality trend sensing
- **Occupancy:** low-power 60 GHz presence radar such as Infineon BGT60LTR11AIP for motion plus micro-presence persistence
- **Ambient light sensor:** Vishay VEML7700 to infer sunlight loading and shade-open daytime comfort patterns near perimeter rooms
- **Battery gauging:** MAX17048 fuel gauge

### Power

- **Primary power:** 2000 mAh flat Li-ion pack hidden in vent-side module
- **Charging:** USB-C service connector with BQ24074 charger/power path IC
- **Optional power harvesting:** thermoelectric add-on across hot/cool vent gradient for trickle extension in high-duty systems
- **Battery life target:** 4 to 6 months typical in BLE-only mode, 2 to 4 months in denser occupancy tracking mode
- **Power strategy:** radar duty cycling, adaptive sensing during HVAC calls, burst logging when anomalies appear

### Mechanical and form factor

- **Envelope:** 140 mm × 40 mm × 9 mm under-grille spine plus side pods
- **Mounting:** magnetic tabs and spring clips under standard metal or composite supply registers
- **Air path coupling:** low-obstruction venturi channel integrated into a thin removable insert
- **Materials:** UL94-V0 enclosure resin with washable intake lattice and conformal-coated PCB
- **Serviceability:** removable sensor spine and battery sled without uninstalling entire register

### Electrical interfaces

- BLE provisioning and diagnostics
- Thread mesh backhaul for whole-home or whole-building deployment
- USB-C maintenance port for deep logs and direct firmware recovery
- Expansion pads for external reed contact, relay handoff, or smart-vent interlock accessory

## System architecture

VentLattice is organized into six layers:

1. **Mechanical coupling layer** — a thin vent insert shapes airflow through a measurement channel while preserving most free area.
2. **Acquisition layer** — differential pressure, thermal, climate, occupancy, and light sensors sample at rates adapted to HVAC activity.
3. **Feature layer** — firmware computes velocity estimate, flow stability, supply delta-T, freshness decay, occupancy alignment, and condensation margin.
4. **Inference layer** — explainable heuristics estimate room service quality, blockage likelihood, maintenance drift, and comfort waste.
5. **Communication layer** — BLE and Thread expose snapshots, alerts, and learned baselines to apps and automation systems.
6. **Experience layer** — the companion app translates raw vent behavior into room-level actions such as “open this register 20% more,” “replace filter within a week,” or “guest room is over-conditioned while unoccupied.”

### Block diagram

```text
+------------------------------------------------------------------------------------------------+
|                                         VentLattice                                            |
|                                        Author: jayis1                                          |
+------------------------------------------------------------------------------------------------+
       Supply air
           |
           v
  [Vent insert / nozzle] --> [SDP810 differential pressure] ----+
           |                                                     |
           +--------------> [Dual NTC thermal bridge] -----------+
           |                                                     |
Room air --> [BME688 temp/RH/VOC] -------------------------------+----> [nRF52840 MCU] --> [QSPI Flash]
           |                                                     |             |
           +--------------> [BGT60 presence radar] --------------+             +--> [BLE]
           |                                                     |             +--> [Thread]
           +--------------> [VEML7700 light sensor] -------------+
                                                                   
Power path: [USB-C] -> [BQ24074 charger] -> [Li-ion battery] -> [3V3 rail] -> sensors + MCU
```

## Detailed hardware design rationale

The design goal is to obtain useful HVAC endpoint observability without creating a noisy, power-hungry object that occupants dislike. That leads to several decisions.

### Why vent-native airflow sensing matters

Measuring room temperature alone is an incomplete proxy for HVAC distribution. A room can have acceptable air temperature but still suffer poor ventilation, delayed recovery, stratification, or occupant discomfort. By measuring pressure drop across a defined mini-nozzle and pairing it with thermal cues, VentLattice estimates actual airflow behavior at the register. This enables the device to distinguish a normal low-flow maintenance mode from an abnormal restriction event.

### Why occupancy is included

Comfort and energy efficiency are inseparable from occupancy. If a room is empty for long periods yet receives strong conditioned airflow during peak runtime, that indicates potential balancing waste. Conversely, if occupancy persists while air freshness worsens and airflow is weak, that signals comfort under-delivery. Integrating presence sensing at the register lets VentLattice reason about whether delivered air is aligned with room use rather than just system runtime.

### Why a VOC/freshness trend sensor belongs in a vent monitor

Many comfort complaints are really mixed comfort-plus-air-quality complaints. A room can feel stuffy, stale, and “off” before it drifts far from the thermostat setpoint. The BME688-class sensor is not meant to produce laboratory IAQ certification; it provides trend information. Paired with occupancy and airflow, it helps determine when a room is served inadequately relative to actual use.

### Why explainable inference is preferred

VentLattice is aimed at homeowners, property operators, and technicians. These users need recommendations that make sense. “Filter loading likelihood increased because pressure ripple rose while multi-room airflow baselines fell 18% over 10 days” is actionable. “Model probability 0.83” is not enough. The firmware therefore favors weighted, transparent calculations over opaque cloud-only ML. A future fleet analytics service could layer learned models on top, but the base product should remain understandable and locally useful.

## Firmware details and design decisions

The firmware included in this repository is a compile-ready simulation of VentLattice behavior and is structured like embedded production code. It separates board-level definitions, register maps, sensor drivers, inference logic, BLE packet formatting, and logging into discrete modules. The simulation models a typical day in a perimeter room with morning occupancy, midday cooling demand, late-afternoon solar load, and a gradually worsening blockage event caused by furniture drifting over the register.

### Core firmware modules

- **airflow driver** — estimates face velocity, nozzle pressure delta, blockage index, and delivery stability.
- **pressure driver** — models dynamic pulses, blower signature strength, and filter-loading indicators.
- **environment driver** — tracks supply temperature, room temperature, humidity, VOC index, and condensation margin.
- **occupancy driver** — models presence confidence, dwell time, and sunlight influence.
- **power driver** — tracks battery voltage, charge state, radar duty cycle, and estimated runtime remaining.
- **inference engine** — produces service score, comfort waste, maintenance priority, stale-air risk, and alert level.
- **BLE formatter** — serializes concise status and recommendation packets.
- **logger** — stores time snapshots and noteworthy events.

### Event-driven sensing strategy

VentLattice should not sample every sensor at maximum rate continuously. HVAC calls are intermittent, occupancy changes gradually, and most rooms spend large portions of the day in stable states. The firmware therefore uses a hierarchy: low-power background sensing establishes whether airflow and occupancy conditions are changing; once supply delivery is detected, the device increases pressure and thermal sampling to capture the delivery episode accurately; if an anomaly is suspected, it expands logging granularity and retains a denser event history.

### Comfort model

The device’s comfort model is intentionally simple enough to audit. It compares room temperature and humidity against a learned preference band, adjusts expected airflow need based on occupancy and light load, and then evaluates whether delivered air is aligned with that need. A vacant room receiving high airflow during peak operation increments the comfort-waste score. An occupied room receiving inadequate cooling or heating increments the under-service score. Combined with freshness and pressure trends, this becomes a service-quality assessment rather than a mere climate reading.

### Maintenance model

VentLattice includes a maintenance perspective because many endpoint problems are caused upstream. If pressure ripple signatures shift across HVAC calls, if supply delta-T becomes erratic, and if several rooms show concurrent airflow decline, the likely issue is not the single room — it is the system. The repository’s simulation models a more local blockage case, but the architecture is ready for multi-node fleet reasoning.

## Application and software interface

The companion app in this repository is a React Native prototype that presents VentLattice as a practical building tool rather than a gadget dashboard. The app includes multiple screens and reusable components so that installers, homeowners, and facilities teams can all understand what the device is saying.

### App workflow

1. Pair the VentLattice node over BLE.
2. Confirm install quality by checking airflow coupling and grille coverage.
3. View the room dashboard with service score, airflow, delivered temperature, freshness, and occupancy alignment.
4. Open the airflow map to see whether the room is under-served, balanced, or over-served across the day.
5. Review the alerts screen for blockage, maintenance, and stale-air findings.
6. Use the automation screen to export recommendations to a smart thermostat, smart vent, or building integration service.
7. Inspect the device screen for battery, radio, firmware, and learned baseline health.

### User interface strategy

The app does not drown users in raw sensor traces. Instead it surfaces outcomes:

- “Home office airflow dropped 22% this week.”
- “Guest room is receiving cooling while unoccupied.”
- “Likely partial vent obstruction detected after 15:00.”
- “Filter loading trend emerging across three rooms.”
- “Freshness degradation suggests closed-door occupancy with low supply.”

Technicians can drill into more detail, but the first layer remains plain language. This is important if the product is to be genuinely useful in homes and small facilities.

## Use cases and target audience

### Home office comfort stabilization
A homeowner working in a south-facing office notices afternoon stuffiness and overcooling cycles. VentLattice reveals that solar gain rises sharply while actual register airflow falls whenever a nearby chair drifts over part of the vent. The app recommends clearing obstruction and modestly opening a branch damper. Comfort improves without lowering the whole-home thermostat.

### Nursery airflow validation
Parents want confidence that a nursery receives stable nighttime airflow without over-conditioning. VentLattice learns the room’s overnight pattern, confirms occupancy-aligned delivery, and alerts if airflow suddenly drops due to a closed register vane or clogged filter trend.

### Classroom and clinic deployment
A school or clinic can deploy VentLattice in problem rooms to identify stale-air buildup, occupancy mismatch, and poor distribution without invasive HVAC work. Facility staff gain evidence to prioritize balancing or maintenance resources.

### Rental property diagnostics
Property managers often receive vague “one room is always uncomfortable” complaints. Instead of repeated trial-and-error thermostat changes, a temporary or permanent VentLattice deployment can show whether the issue is blocked airflow, room over-occupancy, branch under-delivery, or an upstream maintenance problem.

### High-humidity climate condensation watch
In humid regions, cold supply air at certain registers can create condensation risk around metal grilles and adjacent drywall. VentLattice tracks dew margin and warns before chronic wetting encourages mold growth.

## Safety, privacy, and deployment considerations

VentLattice does not claim certified air-balancing measurements for code compliance. It is an observational and optimization device intended to guide action, not replace a commissioning instrument. Occupancy history remains local-first and summarized unless the user explicitly exports data. Raw radar or fine-grained occupancy traces need not leave the device to provide useful outcomes. This makes the system more privacy-respectful than many room analytics platforms.

The device is also designed to be non-destructive. It does not require opening walls, cutting ducts, or replacing HVAC controls. That makes it especially attractive for rentals, historic buildings, schools during short maintenance windows, and phased rollouts in commercial spaces.

## Future roadmap

Potential future expansions include:

- mesh coordination across multiple rooms for whole-building balancing recommendations,
- optional smart-vent actuation accessory,
- thermostat API integration for occupancy-aware airflow scheduling,
- learning-based seasonal comfort baselines,
- passive e-paper room tags showing current service quality,
- and a plug-in mains-powered version for large commercial registers.

## Repository contents

- `firmware/` — compile-ready C firmware simulation for the VentLattice endpoint
- `kicad/` — KiCad project files describing the electronics and board layout
- `app/` — React Native companion application prototype

## Implementation summary

This repository includes a substantial implementation rather than placeholders. The firmware exceeds the level of simple demonstration code: it models room behavior over time, separates drivers cleanly, exposes a virtual register map, and generates human-readable status and report packets. The KiCad files include real components and named nets suitable for turning into a true board project. The app includes multiple screens, reusable UI components, and protocol helpers that show how a real product experience would be organized.

## Conclusion

VentLattice is the kind of device that should exist because it addresses an expensive, everyday blind spot in buildings. HVAC systems are judged at the thermostat, but experienced at the register. By instrumenting that endpoint intelligently, VentLattice can make rooms more comfortable, reduce wasted conditioning, and identify maintenance issues earlier. It is novel without being speculative, feasible with current silicon, and valuable across homes, rentals, schools, clinics, and light commercial spaces.

Every part of this design — concept, firmware, hardware files, and app prototype — credits **jayis1** as the sole author and creator.
