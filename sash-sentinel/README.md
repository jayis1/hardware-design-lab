# Sash Sentinel

Author: jayis1  
Copyright (C) 2026 jayis1. All rights reserved.

## Device name, purpose, and overview

Sash Sentinel is a clamp-on window health instrument that turns ordinary residential and light-commercial windows into monitored building-envelope assets. It is designed for a problem most people feel every day but rarely measure correctly: windows fail long before they visibly break. A window can look closed while still leaking conditioned air, breeding condensation at the lower rail, soaking hidden wood, slowly feeding mold, and forcing HVAC systems to waste money. Existing smart home devices usually monitor whole-room temperature, humidity, or occupancy. They do not tell a homeowner whether one specific sash is misaligned, whether a latch is no longer compressing weatherstrip, whether wind-driven rain is entering through a corner joint, or whether a cold-edge bridge is forming the exact conditions that create rot.

Sash Sentinel exists for that gap. The device clips to the lower sash rail or casement edge and senses the physical and thermal state of the opening itself. It combines cavity humidity, cold-edge thermal mapping, latch compression sensing, differential pressure pulses, acoustic draft signatures, VOC and moisture persistence, and low-power trend analysis. Instead of simply reporting “room humidity is high,” it can conclude that the bottom-right weatherstrip is failing, the glass edge is dropping below dew point, and the owner should re-seat the sash before moisture damages paint, trim, and wall framing.

The device is intentionally more specific than a generic environmental sensor and less invasive than a full retrofitted smart window assembly. It is meant for renters, homeowners, property managers, restoration contractors, preservationists, schools, and small offices that need early warning on comfort loss and hidden envelope failure without replacing the window. Sash Sentinel can be installed seasonally, moved from room to room, or deployed across a fleet of buildings to build a ranked maintenance queue.

What makes the concept original is the idea of **window-state inference at the sash level**. Many products can tell you the air in a room is dry or humid. Very few can tell you *why that specific opening* is underperforming. Sash Sentinel instruments the failure boundary directly. It is not a thermostat. It is not just a leak detector. It is a structural microclimate monitor for a window opening.

## Why this device should exist

Window failures create a chain of practical costs that people accept as “normal” because the symptoms are diffuse. Rooms near bad windows feel colder, so occupants raise the thermostat. Condensation beads on the sill, but only on some mornings, so the issue is dismissed. Paint blisters on trim months later. A musty smell appears in winter. The utility bill climbs. The building owner does not know whether to replace weatherstrip, adjust hinges, tune latch hardware, repair drainage paths, or replace the entire unit.

That uncertainty is expensive. It causes over-repair in some buildings and under-repair in others. Sash Sentinel converts a vague comfort complaint into evidence. If the problem is infiltration, it shows repeat pressure-driven leakage and acoustic draft signatures. If the problem is condensation, it shows dew point crossing at the coldest seal edge. If the problem is a mechanical misfit, it shows reduced latch force and increased sash offset. If the problem is hidden moisture retention, it shows persistent sill moisture and elevated organic VOC trend that can precede visible mold.

This makes the device useful in four different ways:

1. **Comfort:** identify which openings are responsible for cold drafts or summer heat gain.
2. **Preservation:** catch hidden moisture before wood rot, drywall damage, or mold remediation is needed.
3. **Energy:** prioritize the small subset of windows that actually drive HVAC waste.
4. **Maintenance triage:** tell users whether the next step is adjustment, sealing, drainage cleaning, dehumidification, or replacement.

## Hardware specifications

### Core processing

- **Primary MCU:** Espressif ESP32-S3-WROOM-1-N8R8
- **CPU:** dual-core Xtensa LX7 up to 240 MHz
- **Memory:** 8 MB flash + 8 MB PSRAM for event history, OTA images, and local analytics
- **Security:** secure boot capable, signed OTA updates, BLE commissioning keys stored in NVS
- **Interfaces used:** I2C, SPI, I2S/PDM, ADC, USB, GPIO interrupts

The ESP32-S3 is selected because Sash Sentinel benefits from local sensor fusion and practical connectivity. The device needs enough memory to keep multiple days of high-resolution trend data, enough compute to run risk models locally, and enough ecosystem support to make OTA updates, BLE onboarding, and low-power telemetry realistic. The part is also inexpensive enough to make fleet deployments practical.

### Sensor stack

- **SHT45** humidity and temperature sensor positioned inside the frame-facing cavity to measure trapped microclimate
- **BME280** secondary pressure and ambient temperature channel for room-side reference and pressure pulse estimation
- **MLX90632** infrared thermometer aimed at the lower glass edge and seal region
- **4-zone NTC ladder** bonded along the contact edge to map thermal gradient across latch side, center, and sill corners
- **Hall effect latch sensor** to verify closure state and repeatability of latch alignment
- **Foil strain bridge on clamp spine** to estimate compression force when the sash is pulled shut
- **MEMS microphone** coupled to a narrow acoustic port to capture high-frequency hiss signatures associated with air leakage
- **Capacitive moisture strip** along the lower contact lip to detect retained condensation or rain ingress
- **SGP40 VOC sensor** to track persistent organic moisture signatures associated with mildew-prone cavities
- **3-axis IMU** for tamper, slam, and vibration profiling on loose sashes

The novelty is not a single sensor. It is the combination. Pressure pulses alone are noisy. Humidity alone is ambiguous. Temperature alone does not distinguish a cold day from a seal fault. Sash Sentinel fuses these signals over time and produces a diagnosis with better specificity than any single channel could provide.

### Connectivity

- **Bluetooth LE 5.x** for setup, local dashboard access, and maintenance mode
- **Wi-Fi 2.4 GHz** for firmware updates, trend sync, and multi-window building view
- **USB-C** for charging, local logs, and manufacturing calibration
- **Optional magnetic dock** for seasonal storage and room-to-room relocation

### Power system

- **Battery:** 1200 mAh LiPo pouch cell
- **Charging:** BQ24075 power-path charger via USB-C
- **Regulation:** TPS63031 buck-boost for 3.3 V system rail
- **Estimated life:** 4 to 6 weeks in seasonal monitoring mode, 10 days in high-frequency draft watch mode
- **Power policy:** aggressive low-power sleep with burst sampling during gusts, latch events, and dew-point crossings

### Form factor

- **Mechanical type:** slim clip-on sash monitor with silicone compression pads
- **Body size:** 88 mm × 26 mm × 16 mm
- **Weight:** approximately 44 g including battery
- **Enclosure:** flame-retardant PC/ABS shell with replaceable elastomer contact pad
- **Ingress considerations:** indoor-side installation with conformal-coated PCB and hydrophobic acoustic mesh

## Architecture and block diagram

Sash Sentinel is organized around a split sensing architecture: one set of sensors references the room, one set references the sash edge and trapped cavity, and the inference engine compares them over time.

```text
                +-----------------------------------------------+
                |                 Sash Sentinel                 |
                |            Author/Creator: jayis1            |
                +-----------------------------------------------+
                               |                |
                               |                +--> USB-C / charger / service console
                               |
                     +---------v---------+
                     |   ESP32-S3 MCU    |
                     | BLE, Wi-Fi, OTA   |
                     +---------+---------+
                               |
      +-----------+------------+-------------+-------------+------------+
      |           |                          |             |            |
+-----v----+ +----v-----+             +------v-----+ +-----v----+ +-----v-----+
| Env I2C  | | Thermal  |             | Latch/IMU  | | Acoustic | | Power/BMS |
| SHT45    | | MLX90632 |             | Hall+Strain| | MEMS Mic | | Gauge     |
| BME280   | | NTC array|             | + IMU      | | Pressure | | USB detect |
| SGP40    | +----------+             +------------+ +----------+ +-----------+
| Moisture |
+-----+----+
      |
      +--> Local inference -> risk scoring -> alerts/logs -> app dashboard/OTA
```

### Data flow

1. The room-side pressure and temperature reference are sampled first.
2. The cavity humidity and sill moisture channels are read next to characterize trapped conditions.
3. The thermal edge stack measures glass, seal, and frame temperatures to estimate cold-bridge behavior.
4. Latch compression and sash offset features are calculated from Hall and strain inputs.
5. Acoustic and pressure burst samplers activate when the pressure reference suggests wind or stack-effect conditions.
6. The inference engine scores condensation risk, infiltration risk, mold risk, latch fault risk, rot risk, and comfort-loss risk.
7. Only compact telemetry and actionable recommendations are pushed to the companion app.

### Embedded architecture layers

- **HAL and board layer:** clock setup, register map, pin ownership, power rails, ADC scaling, and wake sources
- **Driver layer:** environmental, thermal, airflow, latch, logger, power, and communications drivers
- **Inference layer:** trend extraction, dew point comparison, anomaly scoring, and action selection
- **Application layer:** commissioning, alert policy, telemetry framing, and local state machine

## Firmware details and design decisions

The firmware is built around event-driven low-power sampling rather than continuous high-rate sensing. Most of the time, a window is static. The interesting moments are transitions: a gust, a dew-point crossing, a latch event, a storm, a condensation cycle, or a rapid thermal gradient shift. Sampling is therefore staged.

### Firmware behavior

- **Baseline mode:** periodic snapshots every few minutes for long-term trend and baseline drift
- **Draft burst mode:** higher-rate pressure and acoustic snapshots when a gust signature appears
- **Condensation watch mode:** elevated sampling near dawn, overnight cooldown, and high indoor humidity events
- **Maintenance mode:** interactive BLE diagnostics, raw sensor view, and calibration export
- **Fleet mode:** reduced telemetry payloads optimized for many windows in one building

### Key design decisions

1. **Local dew-point math instead of cloud dependence.** Users need alerts even if Wi-Fi is absent. The firmware computes dew point, condensation margin, and mold proxy locally.
2. **Inference over thresholds.** A single humidity threshold would create false positives. The firmware compares humidity, cold-edge temperature, and moisture persistence together.
3. **Mechanical state is first-class.** Many air leaks are really closure failures. The latch and strain channels let the firmware distinguish a climate problem from a hardware-fit problem.
4. **Ring-buffer history on device.** Service technicians need to inspect what happened overnight or during a storm without requiring the app to be open.
5. **Human-readable actions.** The app and firmware avoid vague language. The result is “clean weep path,” “adjust keeper,” or “replace lower weatherstrip,” not merely “warning.”

### Example inference cases

- **Case A: high humidity, low cold-edge temperature, rising sill moisture**  
  Output: likely condensation event with organic growth risk if repeated.
- **Case B: modest humidity, repeated pressure pulses, acoustic hiss, sash offset increase**  
  Output: mechanical infiltration from closure mismatch or worn seal.
- **Case C: storm conditions, moisture strip activity, pressure transients, one-corner thermal anomaly**  
  Output: probable rain ingress path near one lower corner.
- **Case D: normal humidity, strong thermal gradient, low infiltration**  
  Output: poor glazing or thermal bridge rather than simple draft.

### Firmware deliverables in this folder

The `firmware/` directory contains a compile-ready host-simulation build that models the device logic in portable C. It includes:

- `main.c` with system orchestration, sample history, register updates, console diagnostics, and telemetry generation
- `board.h` and `registers.h` for configuration, common types, and simulated MMIO definitions
- `drivers/env.*` for dew-point and microclimate sensing logic
- `drivers/thermal.*` for thermal edge modeling
- `drivers/latch.*` for closure state and compression estimation
- `drivers/airflow.*` for draft signature modeling
- `drivers/power.*` for battery and operating mode handling
- `drivers/logger.*` for ring-buffer event logging
- `drivers/comms.*` for command and telemetry framing
- `drivers/inference.*` for risk scoring and recommendation generation
- `Makefile` for building the simulation binary

This layout mirrors how a production embedded project would separate sensing, inference, transport, and system control.

## Companion application and software interface

The companion application is a lightweight web dashboard located in `app/`. It is intentionally portable: property managers can run it from a tablet, a phone browser, or a laptop without app-store friction. The application is focused on three jobs.

### 1. Window health triage
The app shows a ranked list of risk categories with clear colors, concise summaries, and direct maintenance guidance. It is built for a user who needs to decide what to do in under a minute.

### 2. Calibration and install workflow
During setup, the app walks the user through installation orientation, reference capture, latch closure checks, and expected pressure-bias calibration. This matters because windows differ by frame geometry, weatherstrip type, and exposure.

### 3. Event review
The app can load a JSON telemetry frame, decode the risk report, and display a timeline summary showing which conditions worsened and which improved after a maintenance action.

### Software protocol
Sash Sentinel exposes a compact text-and-JSON interface over BLE UART style transport or Wi-Fi WebSocket bridge.

Example command set:

- `ping`
- `get:profile`
- `set:ssid=<value>`
- `set:location=<value>`
- `set:buzzer=on|off`

Example telemetry fields:

- `indoor_temp_c`
- `cavity_humidity_pct`
- `dew_point_c`
- `sill_moisture_pct`
- `leak_velocity_mps`
- `acoustic_leak_score`
- `force_n`
- `offset_mm`
- `condensation`
- `infiltration`
- `mold`
- `latch_fault`
- `summary`
- `action`

The web app in this design folder parses that schema and renders live cards, a command console, and an installation checklist.

## Use cases and target audience

### Homeowners and renters
A user in an older apartment can move one device from window to window during heating season and identify which opening causes the cold draft near a bed or desk. That is much more actionable than guessing based on feel.

### Property managers
A manager responsible for dozens of units can deploy Sash Sentinel during winter turnover and rank openings by leakage severity, condensation risk, and likely hardware adjustment need. This helps allocate maintenance budget instead of replacing windows blindly.

### Historic preservation and restoration
Old wood windows are repairable, but the failure modes are subtle. Sash Sentinel helps preservation teams distinguish between glazing, frame, paint, seal, and drainage issues while keeping original assemblies in service longer.

### Schools, clinics, and small offices
Buildings with occupant comfort complaints often struggle to identify whether the HVAC, occupancy load, or exterior envelope is the main cause. Sash Sentinel isolates the behavior of the opening itself.

### Restoration contractors
After storm events or tenant complaints, a contractor can place the device for a few days to gather evidence before opening walls or recommending replacement.

## Practical deployment scenarios

- **Winter bedroom comfort audit:** find the exact window responsible for overnight downdraft and cold pooling.
- **Bathroom mold prevention:** monitor a window that repeatedly fogs during showers and determine whether ventilation or seal repair is the better fix.
- **Historic double-hung tuning:** verify whether sash cord work and weatherstrip replacement actually improved closure and infiltration.
- **Coastal storm exposure:** detect wind-driven moisture ingress on windows facing prevailing rain.
- **Rental move-in check:** document envelope quality and latent moisture risk before a tenant experiences damage.

## Mechanical and electrical implementation notes

The PCB is a slim internal rigid board with a short flex tail for the edge temperature sensors and moisture strip. The acoustic port is isolated from direct splash but aligned with the lower seal gap where hiss signatures are strongest. The strain element is integrated into the clip spine so installation force and closure compression can be estimated separately. The Hall sensor sits near a small magnet placed in the opposing clamp half, giving repeatable closure-state measurement without needing to modify the window.

To keep the design practical, all sensing is non-destructive. No holes need to be drilled into the frame. No wires cross the opening. The user can remove the device without marks beyond normal temporary clip contact.

## Why the design is commercially plausible

The parts are mainstream, low-power, and available in moderate volume. The enclosure does not require exotic materials. The value proposition is easy to explain: identify draft, condensation, and hidden window damage before they cost more. The customer does not need to replace a window to benefit. Install time is under five minutes. Fleet users get ranked maintenance data. Consumers get comfort and energy savings. Contractors get evidence.

That combination makes Sash Sentinel the kind of product that does not broadly exist in this integrated form today but could and should. It occupies a practical middle ground between cheap room sensors and expensive building-envelope diagnostics, delivering localized insight where the failure actually occurs.
